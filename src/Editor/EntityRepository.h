
#pragma once

#include "Math/Vector.h"
#include "SnapPoint.h"
#include "Prefabs.h"

#include <string>
#include <vector>

namespace editor
{
    struct EntityDefinition
    {
        std::string name;
        std::string sprite_file;
        math::Vector scale;
        std::vector<unsigned int> attribute_types;
    };

    class EntityRepository
    {
    public:

        bool LoadDefinitions();

        const EntityDefinition* GetDefinitionFromName(const std::string& name) const;
        const PrefabDefinition* GetPrefabFromName(const std::string& name) const;

        std::vector<EntityDefinition> m_entities;
        std::vector<PrefabDefinition> m_prefabs;
    };
}
