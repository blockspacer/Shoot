
#pragma once

#include "Entity.h"

#include <cstdint>
#include <vector>

struct Attribute;

class IEntityManager
{
public:

    virtual ~IEntityManager() = default;
    virtual mono::Entity CreateEntity(const char* name, const std::vector<uint32_t>& components) = 0;
    virtual mono::Entity CreateEntity(const char* entity_file) = 0;
    virtual bool AddComponent(uint32_t entity_id, uint32_t component_hash) = 0;
    virtual bool RemoveComponent(uint32_t entity_id, uint32_t component_hash) = 0;
    virtual bool SetComponentData(uint32_t entity_id, uint32_t component_hash, const std::vector<Attribute>& properties) = 0;
    virtual std::vector<Attribute> GetComponentData(uint32_t entity_id, uint32_t component_hash) const = 0;
    virtual void ReleaseEntity(uint32_t entity_id) = 0;
    virtual void Sync() = 0;
};