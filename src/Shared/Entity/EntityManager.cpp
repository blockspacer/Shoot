
#include "EntityManager.h"

#include "Hash.h"
#include "Serialize.h"
#include "SystemContext.h"
#include "System/File.h"
#include "Util/Algorithm.h"

#include "nlohmann/json.hpp"

#include <algorithm>
#include <vector>

namespace
{
    std::vector<ComponentData> LoadEntityFile(const char* entity_file)
    {
        file::FilePtr file = file::OpenAsciiFile(entity_file);
        if(!file)
            return { };

        std::vector<byte> file_data;
        file::FileRead(file, file_data);
        file_data.push_back('\0');

        const nlohmann::json& json = nlohmann::json::parse((const char*)file_data.data());
        //const nlohmann::json& entities = json["entities"];
        const nlohmann::json& first_entity = json["entities"][0];

        std::vector<ComponentData> entity_components;

        for(const nlohmann::json& component : first_entity["components"])
        {
            ComponentData component_data;
            component_data.name = component["name"];

            for(const nlohmann::json& property : component["properties"])
                component_data.properties.push_back(property);

            entity_components.push_back(std::move(component_data));
        }

        return entity_components;
    }
}

EntityManager::EntityManager(mono::SystemContext* system_context)
    : m_system_context(system_context)
{ }

EntityManager::~EntityManager()
{
    Sync();
}

mono::Entity EntityManager::CreateEntity(const char* name, const std::vector<uint32_t>& components)
{
    mono::EntitySystem* entity_system = m_system_context->GetSystem<mono::EntitySystem>();
    mono::Entity& new_entity = entity_system->AllocateEntity();
    entity_system->SetName(new_entity.id, name);
    for(uint32_t component : components)
        AddComponent(new_entity.id, component);

    return new_entity;
}

mono::Entity EntityManager::CreateEntity(const char* entity_file)
{
    const uint32_t entity_hash = mono::Hash(entity_file);

    const auto it = m_cached_entities.find(entity_hash);
    if(it == m_cached_entities.end())
        m_cached_entities[entity_hash] = LoadEntityFile(entity_file);

    mono::EntitySystem* entity_system = m_system_context->GetSystem<mono::EntitySystem>();
    mono::Entity& new_entity = entity_system->AllocateEntity();

    const std::vector<ComponentData>& entity_components = m_cached_entities[entity_hash];
    for(const ComponentData& component : entity_components)
    {
        const uint32_t component_hash = mono::Hash(component.name.c_str());
        if(AddComponent(new_entity.id, component_hash))
            SetComponentData(new_entity.id, component_hash, component.properties);
    }

    return new_entity;
}

bool EntityManager::AddComponent(uint32_t entity_id, uint32_t component_hash)
{
    mono::EntitySystem* entity_system = m_system_context->GetSystem<mono::EntitySystem>();
    mono::Entity& entity = entity_system->GetEntity(entity_id);

    const auto factory_it = m_component_factories.find(component_hash);
    if(factory_it != m_component_factories.end())
    {
        const bool success = factory_it->second.create(entity, m_system_context);
        if(success)
            entity.components.push_back(component_hash);
        return success;
    }

    std::printf("There is no component registered with hash '%ul'\n", component_hash);
    return false;
}

bool EntityManager::RemoveComponent(uint32_t entity_id, uint32_t component_hash)
{
    mono::EntitySystem* entity_system = m_system_context->GetSystem<mono::EntitySystem>();
    mono::Entity& entity = entity_system->GetEntity(entity_id);

    const auto factory_it = m_component_factories.find(component_hash);
    if(factory_it != m_component_factories.end())
    {
        const bool success = factory_it->second.release(entity, m_system_context);
        if(success)
            mono::remove(entity.components, component_hash);
        return success;
    }

    std::printf("Unable to remove component with hash: %ul\n", component_hash);
    return false;
}

bool EntityManager::SetComponentData(uint32_t entity_id, uint32_t component_hash, const std::vector<Attribute>& properties)
{
    mono::EntitySystem* entity_system = m_system_context->GetSystem<mono::EntitySystem>();
    mono::Entity& entity = entity_system->GetEntity(entity_id);

    const auto factory_it = m_component_factories.find(component_hash);
    if(factory_it != m_component_factories.end())
        return factory_it->second.update(entity, properties, m_system_context);

    std::printf("Unable to update component with hash: %ul\n", component_hash);
    return false;
}

std::vector<Attribute> EntityManager::GetComponentData(uint32_t entity_id, uint32_t component_hash) const
{
    mono::EntitySystem* entity_system = m_system_context->GetSystem<mono::EntitySystem>();
    mono::Entity& entity = entity_system->GetEntity(entity_id);

    const auto factory_it = m_component_factories.find(component_hash);
    if(factory_it != m_component_factories.end())
    {
        ComponentGetFunc get_func = factory_it->second.get;
        if(get_func)
            return factory_it->second.get(entity, m_system_context);
    }

    std::printf("Unable to get component with hash: %ul\n", component_hash);
    return { };
}

void EntityManager::RegisterComponent(
    uint32_t component_hash,
    ComponentCreateFunc create_component,
    ComponentReleaseFunc release_component,
    ComponentUpdateFunc update_component,
    ComponentGetFunc get_component)
{
    m_component_factories[component_hash] = {
        create_component,
        release_component,
        update_component,
        get_component
    };
}

void EntityManager::ReleaseEntity(uint32_t entity_id)
{
    m_entities_to_release.insert(entity_id);
}

void EntityManager::DefferedRelease()
{
    mono::EntitySystem* entity_system = m_system_context->GetSystem<mono::EntitySystem>();

    for(uint32_t entity_id : m_entities_to_release)
    {
        mono::Entity& entity = entity_system->GetEntity(entity_id);
        for(uint32_t component_hash : entity.components)
        {
            const auto factory_it = m_component_factories.find(component_hash);
            if(factory_it != m_component_factories.end())
                factory_it->second.release(entity, m_system_context);
        }

        entity_system->ReleaseEntity(entity_id);
    }

    m_entities_to_release.clear();
}