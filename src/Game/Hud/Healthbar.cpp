
#include "Healthbar.h"
#include "Rendering/IRenderer.h"
#include "Rendering/Color.h"
#include "Math/Quad.h"

#include "DamageSystem.h"
#include "TransformSystem/TransformSystem.h"
#include "EntitySystem/EntitySystem.h"

#include <limits>

using namespace game;

namespace
{
    struct Healthbar
    {
        math::Vector position;
        float health_percentage;
        uint32_t last_damaged_timestamp;
        std::string name;
    };

    std::vector<math::Vector> GenerateHealthbarVertices(const std::vector<Healthbar>& healthbars, bool full_width)
    {
        std::vector<math::Vector> vertices;
        vertices.reserve(healthbars.size() * 2);

        for(const Healthbar& bar : healthbars)
        {
            const math::Vector shift_vector(1.0f / 2.0f, 0.0f);

            const math::Vector& left = bar.position - shift_vector;
            const math::Vector& right = bar.position + shift_vector;
            const math::Vector& diff = right - left;

            const float percentage = full_width ? 1.0f : bar.health_percentage;

            vertices.emplace_back(left);
            vertices.emplace_back(left + (diff * percentage));
        }

        return vertices;
    }
}

HealthbarDrawer::HealthbarDrawer(game::DamageSystem* damage_system, mono::TransformSystem* transform_system, mono::EntitySystem* entity_system)
    : m_damage_system(damage_system)
    , m_transform_system(transform_system)
    , m_entity_system(entity_system)
{ }

void HealthbarDrawer::Draw(mono::IRenderer& renderer) const
{
    constexpr uint32_t max_uint = std::numeric_limits<uint32_t>::max();
    const uint32_t timestamp = renderer.GetTimestamp();
    const math::Quad viewport = renderer.GetViewport();

    std::vector<Healthbar> healthbars;
    std::vector<Healthbar> boss_healthbars;

    const auto collect_func = [&](uint32_t entity_id, DamageRecord& record) {
        const bool ignore_record = (record.last_damaged_timestamp == max_uint);
        if(ignore_record)
            return;

        const uint32_t delta = timestamp - record.last_damaged_timestamp;
        if(delta > 5000)
            return;

        Healthbar bar;
        bar.last_damaged_timestamp = record.last_damaged_timestamp;
        bar.health_percentage = float(record.health) / float(record.full_health);
        bar.name = m_entity_system->GetName(entity_id);
        
        if(record.is_boss)
        {
            bar.position = math::TopCenter(viewport);
            boss_healthbars.push_back(bar);
        }
        else
        {
            const math::Matrix& transform = m_transform_system->GetTransform(entity_id);
            const math::Vector& position = math::GetPosition(transform);

            bar.position = position - math::Vector(0.0f, 1.0f / 2.0f + 0.5f);
            healthbars.push_back(bar);
        }
    };

    m_damage_system->ForEeach(collect_func);

    constexpr float line_width = 4.0f;
    constexpr mono::Color::RGBA background_color(0.5f, 0.5f, 0.5f, 1.0f);
    constexpr mono::Color::RGBA healthbar_color(1.0f, 0.3f, 0.3f, 1.0f);

    const std::vector<math::Vector>& background_lines = GenerateHealthbarVertices(healthbars, true);
    const std::vector<math::Vector>& healthbar_lines = GenerateHealthbarVertices(healthbars, false);

    renderer.DrawLines(background_lines, background_color, line_width);
    renderer.DrawLines(healthbar_lines, healthbar_color, line_width);


    // Boss health bars

    if(boss_healthbars.empty())
        return;

    const auto sort_on_timestamp = [](const Healthbar& first, const Healthbar& second) {
        return first.last_damaged_timestamp < second.last_damaged_timestamp;
    };

    std::sort(boss_healthbars.begin(), boss_healthbars.end(), sort_on_timestamp);

    const std::vector<math::Vector>& boss_background_lines = GenerateHealthbarVertices(boss_healthbars, true);
    const std::vector<math::Vector>& boss_healthbar_lines = GenerateHealthbarVertices(boss_healthbars, false);

    {
        const mono::ScopedTransform scope = mono::MakeTransformScope(math::Matrix(), &renderer);
        renderer.DrawLines(boss_background_lines, background_color, line_width);
        renderer.DrawLines(boss_healthbar_lines, healthbar_color, line_width);
    }
}

math::Quad HealthbarDrawer::BoundingBox() const
{
    return math::InfQuad;
}
