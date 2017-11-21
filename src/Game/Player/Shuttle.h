
#pragma once

#include "MonoPtrFwd.h"
#include "Entity/PhysicsEntityBase.h"
#include "Rendering/RenderPtrFwd.h"
#include "ShuttleGamepadController.h"
#include "Physics/IBody.h"
#include "Particle/ParticleFwd.h"

#include "Weapons/IWeaponSystem.h"

#include "System/System.h"

class SpriteEntity;

namespace game
{
    struct PlayerInfo;
    enum class WeaponType;

    enum class BoosterPosition
    {
        LEFT,
        RIGHT,
        MAIN,
        ALL
    };

    class Shuttle : public mono::PhysicsEntityBase, public mono::ICollisionHandler
    {
    public:
        
        Shuttle(
            const math::Vector& postiion, mono::EventHandler& eventHandler, const System::ControllerState& controller);
        ~Shuttle();

        void SelectWeapon(WeaponType weapon);
        void ApplyImpulse(const math::Vector& force);

        void Fire();
        void StopFire();
        void Reload();

        void SetBoosterThrusting(BoosterPosition position, bool enabled);
        void SetShading(const mono::Color::RGBA& shade);
        void SetPlayerInfo(PlayerInfo* info);

    private:
        
        void ApplyRotationForce(float force);
        void ApplyThrustForce(float force);

        virtual void Draw(mono::IRenderer& renderer) const;
        virtual void Update(unsigned int delta);
        
        virtual void OnCollideWith(const mono::IBodyPtr& body);
        virtual void OnPostStep();
        
        ShuttleGamepadController m_controller;
        bool m_fire;
        
        mono::ISpritePtr m_sprite;
        std::unique_ptr<IWeaponSystem> m_weapon;
        std::shared_ptr<SpriteEntity> m_left_booster;
        std::shared_ptr<SpriteEntity> m_right_booster;

        std::unique_ptr<mono::ParticlePool> m_pool;
        std::unique_ptr<mono::ParticleDrawer> m_particle_drawer;

        PlayerInfo* m_player_info;
    };
}

