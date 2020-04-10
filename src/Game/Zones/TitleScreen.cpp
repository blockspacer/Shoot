
#include "TitleScreen.h"
#include "ZoneFlow.h"
#include "Effects/ScreenSparkles.h"

#include "UI/TextEntity.h"
#include "UI/IconEntity.h"
#include "UI/Background.h"
#include "RenderLayers.h"
#include "Actions/MoveAction.h"

#include "Math/Quad.h"
#include "Camera/ICamera.h"
#include "Rendering/Color.h"

#include "Events/KeyEvent.h"
#include "Events/QuitEvent.h"
#include "Events/EventFuncFwd.h"
#include "EventHandler/EventHandler.h"

#include "SystemContext.h"
#include "Particle/ParticleSystem.h"
#include "Particle/ParticleSystemDrawer.h"
#include "System/System.h"

using namespace game;

#define IS_TRIGGERED(variable) (!m_last_state.variable && state.variable)
#define HAS_CHANGED(variable) (m_last_state.variable != state.variable)

namespace
{
    class MoveContextUpdater : public mono::IUpdatable
    {
    public:

        MoveContextUpdater(std::vector<MoveActionContext>& contexts)
            : m_contexts(contexts)
        { }

        void doUpdate(const mono::UpdateContext& update_context)
        {
            game::UpdateMoveContexts(update_context.delta_ms, m_contexts);
        }

        std::vector<MoveActionContext>& m_contexts;
    };

    class CheckControllerInput : public mono::IUpdatable
    {
    public:

        CheckControllerInput(TitleScreen* title_screen)
            : m_title_screen(title_screen)
        { }

        void doUpdate(const mono::UpdateContext& update_context)
        {
            const System::ControllerState& state = System::GetController(System::ControllerId::Primary);

            const bool a_pressed = IS_TRIGGERED(a) && HAS_CHANGED(a);
            const bool y_pressed = IS_TRIGGERED(y) && HAS_CHANGED(y);
            const bool x_pressed = IS_TRIGGERED(x) && HAS_CHANGED(x);

            if(a_pressed)
                m_title_screen->Continue();
            else if(y_pressed)
                m_title_screen->Quit();
            else if(x_pressed)
                m_title_screen->Remote();

            m_last_state = state;
        }

        TitleScreen* m_title_screen;
        System::ControllerState m_last_state;
    };
}

TitleScreen::TitleScreen(const ZoneCreationContext& context)
    : m_event_handler(*context.event_handler)
    , m_system_context(context.system_context)
{
    using namespace std::placeholders;
    const event::KeyUpEventFunc& key_callback = std::bind(&TitleScreen::OnKeyUp, this, _1);
    m_key_token = m_event_handler.AddListener(key_callback);
}

TitleScreen::~TitleScreen()
{
    m_event_handler.RemoveListener(m_key_token);
}

mono::EventResult TitleScreen::OnKeyUp(const event::KeyUpEvent& event)
{
    if(event.key == Keycode::ENTER)
        Continue();
    else if(event.key == Keycode::R)
        Remote();

    return mono::EventResult::PASS_ON;
}

void TitleScreen::OnLoad(mono::ICamera* camera)
{
    camera->SetViewport(math::Quad(0, 0, 22, 14));

    mono::ParticleSystem* particle_system = m_system_context->GetSystem<mono::ParticleSystem>();

    const math::Quad& viewport = camera->GetViewport();
    const math::Vector new_position(viewport.mB.x + (viewport.mB.x / 2.0f), viewport.mB.y / 3.0f);

    AddUpdatable(std::make_shared<MoveContextUpdater>(m_move_contexts));
    AddUpdatable(std::make_shared<CheckControllerInput>(this));

    auto background1 = std::make_shared<Background>(viewport, mono::Color::HSL(0.6f, 0.6f, 0.5f));
    auto background2 = std::make_shared<Background>(viewport, mono::Color::HSL(0.6f, 0.3f, 0.5f));

    auto title_text = std::make_shared<TextEntity>("Shoot, Survive!", FontId::PIXELETTE_MEDIUM, false);
    title_text->m_shadow_color = mono::Color::RGBA(0.8, 0.0, 0.8, 1.0);
    title_text->m_text_color = mono::Color::RGBA(0.9, 0.8, 0.8, 1.0f);
    title_text->SetPosition(new_position);

    const math::Vector dont_die_position(viewport.mB.x * 2.0f, viewport.mB.y / 4.0f);

    auto dont_die_text = std::make_shared<TextEntity>("...dont die.", FontId::PIXELETTE_SMALL, false);
    dont_die_text->m_shadow_color = mono::Color::RGBA(0.8, 0.0, 0.8, 1.0);
    dont_die_text->m_text_color = mono::Color::RGBA(0.9, 0.9, 0.0, 1.0f);
    dont_die_text->SetPosition(dont_die_position);

    auto hit_enter_text = std::make_shared<TextEntity>("Hit enter, or   /", FontId::PIXELETTE_SMALL, true);
    hit_enter_text->m_shadow_color = mono::Color::RGBA(0.3, 0.3, 0.3, 1);
    hit_enter_text->SetPosition(math::Vector(viewport.mB.x / 2.0f, 2.0f));

    auto ps_cross = std::make_shared<IconEntity>("res/sprites/ps_cross.sprite");
    ps_cross->SetPosition(math::Vector(3.0f, 0.25f));
    ps_cross->SetScale(math::Vector(0.4f, 0.4f));

    auto xbox_a = std::make_shared<IconEntity>("res/sprites/xbox_one_a.sprite");
    xbox_a->SetPosition(math::Vector(4.65f, 0.25f));
    xbox_a->SetScale(math::Vector(0.4f, 0.4f));

    hit_enter_text->AddChild(ps_cross);
    hit_enter_text->AddChild(xbox_a);

    game::MoveActionContext title_text_animation;
    title_text_animation.entity = title_text;
    title_text_animation.duration = 500;
    title_text_animation.start_position = new_position;
    title_text_animation.end_position = math::Vector(1.0f, new_position.y);
    title_text_animation.ease_func = EaseOutCubic;

    game::MoveActionContext dont_die_animation;
    dont_die_animation.entity = dont_die_text;
    dont_die_animation.duration = 1000;
    dont_die_animation.start_position = dont_die_position;
    dont_die_animation.end_position = math::Vector(5.0f, dont_die_position.y);
    dont_die_animation.ease_func = EaseOutCubic;

    game::MoveActionContext background_animation;
    background_animation.entity = background1;
    background_animation.duration = 1000;
    background_animation.start_position = math::Vector(new_position.x, 0.0f);
    background_animation.end_position = math::Vector(0.0f, 0.0f);
    background_animation.ease_func = EaseInOutCubic;

    game::MoveActionContext hit_enter_animation;
    hit_enter_animation.entity = hit_enter_text;
    hit_enter_animation.duration = 800;
    hit_enter_animation.start_position = hit_enter_text->Position();
    hit_enter_animation.end_position = hit_enter_text->Position() + math::Vector(0.0f, 0.4f);
    hit_enter_animation.ease_func = EaseInOutCubic;
    hit_enter_animation.ping_pong = true;

    m_move_contexts.push_back(title_text_animation);
    m_move_contexts.push_back(dont_die_animation);
    m_move_contexts.push_back(background_animation);
    m_move_contexts.push_back(hit_enter_animation);

    AddEntity(background2, LayerId::BACKGROUND);
    AddEntity(background1, LayerId::BACKGROUND);

    AddDrawable(std::make_shared<mono::ParticleSystemDrawer>(particle_system), LayerId::GAMEOBJECTS);

    AddEntity(title_text, LayerId::GAMEOBJECTS);
    AddEntity(dont_die_text, LayerId::GAMEOBJECTS);
    AddEntity(hit_enter_text, LayerId::GAMEOBJECTS);

    m_sparkles = std::make_unique<ScreenSparkles>(particle_system, viewport);
}

int TitleScreen::OnUnload()
{
    return m_exit_zone;
}

void TitleScreen::Continue()
{
    m_exit_zone = TEST_ZONE;
    m_event_handler.DispatchEvent(event::QuitEvent());
}

void TitleScreen::Remote()
{
    m_exit_zone = REMOTE_ZONE;
    m_event_handler.DispatchEvent(event::QuitEvent());
}

void TitleScreen::Quit()
{
    m_exit_zone = QUIT;
    m_event_handler.DispatchEvent(event::QuitEvent());
}
