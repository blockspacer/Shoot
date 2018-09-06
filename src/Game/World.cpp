
#include "World.h"
#include "CollisionConfiguration.h"
#include "Enemies/Enemy.h"
#include "Pickups/Ammo.h"

#include "Physics/IBody.h"
#include "Physics/IShape.h"
#include "Physics/CMFactory.h"
#include "Physics/CMPhysicsData.h"

#include "Zone/IPhysicsZone.h"
#include "RenderLayers.h"

#include "Rendering/IDrawable.h"
#include "Rendering/IRenderer.h"
#include "Rendering/IRenderBuffer.h"
#include "Rendering/BufferFactory.h"
#include "Rendering/Texture/TextureFactory.h"

#include "Rendering/Sprite/Sprite.h"
#include "Rendering/Sprite/SpriteFactory.h"

#include "Math/Matrix.h"
#include "Math/Quad.h"
#include "Math/MathFunctions.h"
#include "StringFunctions.h"

#include "ObjectAttribute.h"
#include "DefinedAttributes.h"
#include "Prefabs.h"

#include <algorithm>
#include <string>

namespace
{
    struct TerrainDrawData
    {
        mono::ITexturePtr texture;
        size_t offset;
        size_t count;
    };

    class StaticTerrainBlock : public mono::IDrawable
    {
    public:

        StaticTerrainBlock(size_t vertex_count, size_t polygon_count)
            : m_index(0)
        {
            m_static_physics.body = mono::PhysicsFactory::CreateStaticBody();

            m_draw_data.reserve(polygon_count);
            m_vertex_buffer = mono::CreateRenderBuffer(mono::BufferType::STATIC, mono::BufferData::FLOAT, vertex_count * 2);
            m_texture_buffer = mono::CreateRenderBuffer(mono::BufferType::STATIC, mono::BufferData::FLOAT, vertex_count * 2);
 
            m_color_buffer = mono::CreateRenderBuffer(mono::BufferType::STATIC, mono::BufferData::FLOAT, vertex_count * 4);

            const std::vector<float> temp_buffer(vertex_count * 4, 0.0f);
            m_color_buffer->UpdateData(temp_buffer.data(), 0, vertex_count);
        }

        void AddPolygon(const world::PolygonData& polygon)
        {
            mono::IShapePtr shape = mono::PhysicsFactory::CreateShape(m_static_physics.body, polygon.vertices, math::ZeroVec);
            shape->SetCollisionFilter(game::CollisionCategory::STATIC, game::STATIC_MASK);
            m_static_physics.shapes.push_back(shape);

            TerrainDrawData draw_data;
            draw_data.offset = m_index;
            draw_data.count = polygon.vertices.size();
            draw_data.texture = mono::CreateTexture(polygon.texture);

            m_draw_data.emplace_back(draw_data);

            std::vector<math::Vector> texture_coordinates;
            texture_coordinates.reserve(polygon.vertices.size());

            math::Quad bounding_box = math::Quad(math::INF, math::INF, -math::INF, -math::INF);
            for(const math::Vector& vertex : polygon.vertices)
                bounding_box |= vertex;

            const math::Vector& repeate = (bounding_box.mB - bounding_box.mA) / 2.0f;
                
            for(const math::Vector& vertex : polygon.vertices)
                texture_coordinates.push_back(math::MapVectorInQuad(vertex, bounding_box) * repeate);

            m_vertex_buffer->UpdateData(polygon.vertices.data(), draw_data.offset * 2, draw_data.count * 2);
            m_texture_buffer->UpdateData(texture_coordinates.data(), draw_data.offset * 2, draw_data.count * 2);

            m_index += draw_data.count;
        }

        virtual void doDraw(mono::IRenderer& renderer) const
        {
            for(const TerrainDrawData& draw_data : m_draw_data)
            {
                renderer.DrawGeometry(m_vertex_buffer.get(),
                                      m_texture_buffer.get(),
                                      draw_data.offset,
                                      draw_data.count,
                                      draw_data.texture);
                renderer.DrawPolyline(
                    m_vertex_buffer.get(), m_color_buffer.get(), draw_data.offset, draw_data.count);
            }
        }

        virtual math::Quad BoundingBox() const
        {
            return math::InfQuad;
        }

        unsigned int m_index;
        mono::PhysicsData m_static_physics;

        std::unique_ptr<mono::IRenderBuffer> m_vertex_buffer;
        std::unique_ptr<mono::IRenderBuffer> m_texture_buffer;
        std::unique_ptr<mono::IRenderBuffer> m_color_buffer;

        std::vector<TerrainDrawData> m_draw_data;
    };

    class StaticPrefab : public mono::PhysicsEntityBase
    {
    public:

        StaticPrefab(const math::Vector& position, const PrefabDefinition* prefab_definition)
        {
            m_position = position;
            m_scale = prefab_definition->scale;

            std::vector<math::Vector> collision_polygon;
            collision_polygon.reserve(prefab_definition->collision_shape.size());

            const math::Matrix& transform = Transformation();

            for(const math::Vector& collision_vertex : prefab_definition->collision_shape)
                collision_polygon.push_back(math::Transform(transform, collision_vertex));

            m_physics.body = mono::PhysicsFactory::CreateStaticBody();

            mono::IShapePtr shape = mono::PhysicsFactory::CreateShape(m_physics.body, collision_polygon, math::ZeroVec);
            shape->SetCollisionFilter(game::CollisionCategory::STATIC, game::STATIC_MASK);
            m_physics.shapes.push_back(shape);

            m_sprite = mono::CreateSprite(prefab_definition->sprite_file.c_str());
        }

        void Draw(mono::IRenderer& renderer) const override
        {
            renderer.DrawSprite(*m_sprite);
        }

        // Overriden so that we dont get any updates that screws up the position
        void doUpdate(unsigned int delta) override
        { }

        void Update(unsigned int delta) override
        { }

        mono::ISpritePtr m_sprite;
    };
}

void game::LoadWorld(
    mono::IPhysicsZone* zone, const std::vector<world::PolygonData>& polygons, const std::vector<world::PrefabData>& prefabs)
{
    size_t count = 0;

    for(const world::PolygonData& polygon : polygons)
        count += polygon.vertices.size();

    auto static_terrain = std::make_shared<StaticTerrainBlock>(count, polygons.size());

    for(const world::PolygonData& polygon : polygons)
        static_terrain->AddPolygon(polygon);

    zone->AddDrawable(static_terrain, BACKGROUND);
    zone->AddPhysicsData(static_terrain->m_static_physics);


    const std::vector<PrefabDefinition>& prefab_definitions = LoadPrefabDefinitions();

    for(const world::PrefabData& prefab : prefabs)
    {
        const PrefabDefinition* prefab_definition = FindPrefabFromName(prefab.name, prefab_definitions);
        zone->AddPhysicsEntity(std::make_shared<StaticPrefab>(prefab.position, prefab_definition), LayerId::PREFABS);
    }
}

namespace
{
    void LoadAttributes(game::SpawnPoint& spawn_point, const std::vector<Attribute>& attributes)
    {
        world::FindAttribute(world::POSITION_ATTRIBUTE, attributes, spawn_point.position);
        world::FindAttribute(world::RADIUS_ATTRIBUTE, attributes, spawn_point.radius);
    }

    void LoadAttributes(game::EnemyPtr& enemy, const std::vector<Attribute>& attributes)
    {
        math::Vector position;
        float rotation = 0.0f;

        world::FindAttribute(world::POSITION_ATTRIBUTE, attributes, position);
        world::FindAttribute(world::ROTATION_ATTRIBUTE, attributes, rotation);

        enemy->SetPosition(position);
        enemy->SetRotation(rotation);
    }

    void LoadAttributes(game::Ammo& ammo_pickup, const std::vector<Attribute>& attributes)
    {
        world::FindAttribute(world::POSITION_ATTRIBUTE, attributes, ammo_pickup.position);
        world::FindAttribute(world::PICKUP_TYPE_ATTRIBUTE, attributes, ammo_pickup.type);
        world::FindAttribute(world::AMOUNT_ATTRIBUTE, attributes, ammo_pickup.value);
    }
}

void game::LoadWorldObjects(
    const std::vector<world::WorldObject>& objects,
    IEnemyFactory* enemy_factory,
    IGameObjectFactory* gameobject_factory,
    std::vector<game::EnemyPtr>& enemies,
    std::vector<mono::IPhysicsEntityPtr>& gameobjects,
    std::vector<SpawnPoint>& spawn_points,
    std::vector<math::Vector>& player_points,
    std::vector<game::Ammo>& ammo_pickups)
{
    for(const world::WorldObject& object : objects)
    {
        const std::string name = object.name;
        if(name == "spawnpoint")
        {
            SpawnPoint spawn_point;
            LoadAttributes(spawn_point, object.attributes);
            spawn_points.push_back(spawn_point);
        }
        else if(name == "playerpoint")
        {
            math::Vector position;
            world::FindAttribute(world::POSITION_ATTRIBUTE, object.attributes, position);
            player_points.push_back(position);
        }
        else if(name == "ammo_pickup")
        {
            game::Ammo ammo_pickup;
            LoadAttributes(ammo_pickup, object.attributes);
            ammo_pickups.push_back(ammo_pickup);
        }
        else
        {
            math::Vector position;
            world::FindAttribute(world::POSITION_ATTRIBUTE, object.attributes, position);

            game::EnemyPtr enemy = enemy_factory->CreateFromName(name.c_str(), position, object.attributes);
            if(enemy)
            {
                LoadAttributes(enemy, object.attributes);
                enemies.push_back(enemy);
            }
            else
            {
                mono::IPhysicsEntityPtr gameobject = gameobject_factory->CreateGameObject(name.c_str(), object.attributes);
                if(gameobject)
                {
                    gameobjects.push_back(gameobject);
                }
            }
        }
    }
}
