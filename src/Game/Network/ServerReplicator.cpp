
#include "ServerReplicator.h"
#include "ServerManager.h"
#include "INetworkPipe.h"
#include "NetworkMessage.h"
#include "BatchedMessageSender.h"

#include "EntitySystem.h"
#include "TransformSystem.h"
#include "Rendering/Sprite/ISprite.h"
#include "Rendering/Sprite/SpriteSystem.h"
#include "Rendering/Sprite/Sprite.h"

#include "Entity/IEntityManager.h"
#include "Entity/EntityProperties.h"

#include "ScopedTimer.h"
#include "AIKnowledge.h"

#include "Math/MathFunctions.h"
#include "Camera/ICamera.h"

#include <unordered_set>

using namespace game;

static constexpr uint32_t KEYFRAME_INTERVAL = 10;

ServerReplicator::ServerReplicator(
    mono::EntitySystem* entity_system,
    mono::TransformSystem* transform_system,
    mono::SpriteSystem* sprite_system,
    IEntityManager* entity_manager,
    ServerManager* server_manager,
    uint32_t replication_interval)
    : m_entity_system(entity_system)
    , m_transform_system(transform_system)
    , m_sprite_system(sprite_system)
    , m_entity_manager(entity_manager)
    , m_server_manager(server_manager)
    , m_replication_interval(replication_interval)
{
    std::memset(&m_transform_data, 0, std::size(m_transform_data) * sizeof(TransformData));
    std::memset(&m_sprite_data, 0, std::size(m_sprite_data) * sizeof(SpriteData));

    m_keyframe_low = 0;
    m_keyframe_high = KEYFRAME_INTERVAL;
}

void ServerReplicator::doUpdate(const mono::UpdateContext& update_context)
{
    //SCOPED_TIMER_AUTO();

    std::vector<uint32_t> entity_ids;

    const auto collect_entities = [&entity_ids](const mono::Entity& entity) {
        entity_ids.push_back(entity.id);
    };
    m_entity_system->ForEachEntity(collect_entities);

    const std::unordered_map<network::Address, ClientData>& clients = m_server_manager->GetConnectedClients();
    for(const auto& client : clients)
    {
        BatchedMessageSender batch_sender(client.first, m_message_queue);

        ReplicateSpawns(batch_sender);
        ReplicateTransforms(entity_ids, batch_sender, client.second.viewport, update_context);
        ReplicateSprites(entity_ids, batch_sender, update_context);
    }

    for(int index = 0; index < 5 && !m_message_queue.empty(); ++index)
    {
        m_server_manager->SendMessage(m_message_queue.front());
        m_message_queue.pop();
    }

    m_keyframe_low += KEYFRAME_INTERVAL;
    m_keyframe_high += KEYFRAME_INTERVAL;

    m_keyframe_high = std::min(m_keyframe_high, 500u);

    if(m_keyframe_low >= 500)
    {
        m_keyframe_low = 0;
        m_keyframe_high = KEYFRAME_INTERVAL;
    }
}

void ServerReplicator::ReplicateSpawns(BatchedMessageSender& batched_sender)
{
    for(const IEntityManager::SpawnEvent& spawn_event : m_entity_manager->GetSpawnEvents())
    {
        SpawnMessage spawn_message;
        spawn_message.entity_id = spawn_event.entity_id;
        spawn_message.spawn = spawn_event.spawned;

        batched_sender.SendMessage(spawn_message);
    }
}

void ServerReplicator::ReplicateTransforms(
    const std::vector<uint32_t>& entities, BatchedMessageSender& batched_sender, const math::Quad& client_viewport, const mono::UpdateContext& update_context)
{
    int total_transforms = 0;
    int replicated_transforms = 0;

    const auto transform_func = [&, this](const math::Matrix& transform, uint32_t id) {

        total_transforms++;

        TransformData& last_transform_message = m_transform_data[id];
        last_transform_message.time_to_replicate -= update_context.delta_ms;

        const bool keyframe = false; //(id >= m_keyframe_low && id < m_keyframe_high);
        const bool time_to_replicate = (last_transform_message.time_to_replicate < 0);

        if(!time_to_replicate && !keyframe)
            return;

        if(!keyframe)
        {
            const math::Quad& client_bb = math::ResizeQuad(client_viewport, 5.0f); // Expand by 5 meter to send positions before in sight
            const math::Quad& world_bb = m_transform_system->GetWorldBoundingBox(id);

            const bool overlaps = math::QuadOverlaps(client_bb, world_bb);
            if(!overlaps)
                return;
        }

        TransformMessage transform_message;
        transform_message.timestamp = update_context.total_time;
        transform_message.entity_id = id;
        transform_message.position = math::GetPosition(transform);
        transform_message.rotation = math::GetZRotation(transform);
        transform_message.settled = 
            math::IsPrettyMuchEquals(last_transform_message.position, transform_message.position, 0.001f) &&
            math::IsPrettyMuchEquals(last_transform_message.rotation, transform_message.rotation, 0.001f);

        const bool same_as_last_time = false;
            //transform_message.settled && (last_transform_message.settled == transform_message.settled);

        if(!same_as_last_time || keyframe)
        {
            batched_sender.SendMessage(transform_message);

            last_transform_message.position = transform_message.position;
            last_transform_message.rotation = transform_message.rotation;
            last_transform_message.settled = transform_message.settled;
            last_transform_message.time_to_replicate = m_replication_interval;

            replicated_transforms++;
       }
    };

    //m_transform_system->ForEachTransform(transform_func);

    for(uint32_t entity_id : entities)
        transform_func(m_transform_system->GetTransform(entity_id), entity_id);

    //std::printf(
    //    "keyframe %u - %u, transforms %d/%d\n", m_keyframe_low, m_keyframe_high, replicated_transforms, total_transforms);
}

void ServerReplicator::ReplicateSprites(
    const std::vector<uint32_t>& entities, BatchedMessageSender& batched_sender, const mono::UpdateContext& update_context)
{
    int total_sprites = 0;
    int replicated_sprites = 0;

    const auto sprite_func = [&, this](mono::ISprite* sprite, uint32_t id) {

        SpriteMessage sprite_message;
        sprite_message.entity_id = id;
        sprite_message.filename_hash = sprite->GetSpriteHash();
        sprite_message.hex_color = mono::Color::ToHex(sprite->GetShade());
        sprite_message.vertical_direction = (short)sprite->GetVerticalDirection();
        sprite_message.horizontal_direction = (short)sprite->GetHorizontalDirection();
        sprite_message.animation_id = sprite->GetActiveAnimation();

        total_sprites++;

        SpriteData& last_sprite_data = m_sprite_data[id];
        last_sprite_data.time_to_replicate -= update_context.delta_ms;

        const bool time_to_replicate = (last_sprite_data.time_to_replicate < 0);
        const bool same =
            last_sprite_data.animation_id == sprite_message.animation_id &&
            last_sprite_data.hex_color == sprite_message.hex_color &&
            last_sprite_data.vertical_direction == sprite_message.vertical_direction &&
            last_sprite_data.horizontal_direction == sprite_message.horizontal_direction;
        const bool keyframe = (id >= m_keyframe_low && id < m_keyframe_high);

        if((time_to_replicate && !same) || keyframe)
        {
            batched_sender.SendMessage(sprite_message);
            
            last_sprite_data.animation_id = sprite_message.animation_id;
            last_sprite_data.hex_color = sprite_message.hex_color;
            last_sprite_data.vertical_direction = sprite_message.vertical_direction;
            last_sprite_data.horizontal_direction = sprite_message.horizontal_direction;
            last_sprite_data.time_to_replicate = m_replication_interval;

            replicated_sprites++;
        }
    };

    //m_sprite_system->ForEachSprite(sprite_func);

    for(uint32_t entity_id : entities)
        sprite_func(m_sprite_system->GetSprite(entity_id), entity_id);

//    std::printf(
//        "keyframe %u - %u, transforms %d/%d, sprites %d/%d\n", m_keyframe_low, m_keyframe_high, replicated_transforms, total_transforms, replicated_sprites, total_sprites);
}