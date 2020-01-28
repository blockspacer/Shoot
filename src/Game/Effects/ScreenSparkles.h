
#pragma once

#include "Zone/EntityBase.h"
#include "Particle/ParticleFwd.h"

#include "Math/Quad.h"

namespace game
{
    class ScreenSparkles : public mono::EntityBase
    {
    public:

        ScreenSparkles(const math::Quad& viewport);
        virtual ~ScreenSparkles();

        void Draw(mono::IRenderer& renderer) const override;
        void Update(const mono::UpdateContext& update_context) override;

        const math::Quad m_viewport;

        //std::unique_ptr<mono::ParticlePool> m_pool;
        //std::unique_ptr<mono::ParticleEmitter> m_emitter;
        //std::unique_ptr<mono::ParticleDrawer> m_drawer;
    };
}
