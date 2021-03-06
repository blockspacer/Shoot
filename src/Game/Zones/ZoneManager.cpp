
#include "ZoneManager.h"
#include "ZoneFlow.h"
#include "TitleScreen.h"
#include "RemoteZone.h"
#include "SystemTestZone.h"
#include "EmptyZone.h"
#include "EndScreen.h"

using namespace game;

namespace
{
    template <typename T>
    game::IZonePtr LoadZone(const ZoneCreationContext& zone_context)
    {
        return std::make_unique<T>(zone_context);
    }
}

ZoneManager::ZoneManager(
    System::IWindow* window, mono::ICamera* camera, const ZoneCreationContext& zone_context, int initial_zone)
    : m_engine(window, camera, zone_context.system_context, zone_context.event_handler)
    , m_zone_context(zone_context)
    , m_active_zone(initial_zone)
{
    m_zones[TITLE_SCREEN] = LoadZone<game::TitleScreen>;
    m_zones[TEST_ZONE] = LoadZone<game::SystemTestZone>;
    m_zones[REMOTE_ZONE] = LoadZone<game::RemoteZone>;
    m_zones[EMPTY_ZONE] = LoadZone<game::EmptyZone>;
    m_zones[END_SCREEN] = LoadZone<game::EndScreen>;
}

void ZoneManager::Run()
{
    while(true)
    {
        if(m_active_zone == QUIT)
            break;

        LoadFunction load_func = m_zones[m_active_zone];
        game::IZonePtr zone = load_func(m_zone_context);
        m_active_zone = m_engine.Run(zone.get());
    }
}
