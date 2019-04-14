
#include "ComponentProxy.h"
#include "SnapPoint.h"
#include "Grabber.h"

#include "ObjectAttribute.h"
#include "Component.h"
#include "UI/UIProperties.h"
#include "IObjectVisitor.h"
#include "Math/MathFunctions.h"

#include "Entity/IEntityManager.h"
#include "TransformSystem.h"

using namespace editor;

ComponentProxy::ComponentProxy(uint32_t entity_id, const std::string& name, IEntityManager* entity_manager, mono::TransformSystem* transform_system)
    : m_entity_id(entity_id)
    , m_name(name)
    , m_entity_manager(entity_manager)
    , m_transform_system(transform_system)
{
    m_components = {
        DefaultComponentFromHash(TRANSFORM_COMPONENT),
    };

    for(const Component& component : m_components)
    {
        entity_manager->AddComponent(entity_id, component.hash);
        entity_manager->SetComponentData(entity_id, component.hash, component.properties);
    }
}

ComponentProxy::ComponentProxy(uint32_t entity_id, const std::string& name, const std::vector<Component>& components, IEntityManager* entity_manager, mono::TransformSystem* transform_system)
    : m_entity_id(entity_id)
    , m_name(name)
    , m_components(components)
    , m_entity_manager(entity_manager)
    , m_transform_system(transform_system)
{ }

ComponentProxy::~ComponentProxy()
{
    m_entity_manager->ReleaseEntity(m_entity_id);
}

const char* ComponentProxy::Name() const
{
    return m_name.c_str();
}

uint32_t ComponentProxy::Id() const
{
    return m_entity_id;
}

mono::IEntityPtr ComponentProxy::Entity()
{
    return nullptr;
}

void ComponentProxy::SetSelected(bool selected)
{

}

bool ComponentProxy::Intersects(const math::Vector& position) const
{
    const math::Quad& world_bb = m_transform_system->GetWorldBoundingBox(m_entity_id);
    return math::PointInsideQuad(position, world_bb);
}

std::vector<Grabber> ComponentProxy::GetGrabbers() const
{
    return { };
}

std::vector<SnapPoint> ComponentProxy::GetSnappers() const
{
    return { };
}

void ComponentProxy::UpdateUIContext(UIContext& context)
{
    DrawName(m_name);
    
    const int modified_index = DrawComponents(context, m_components);
    if(modified_index != -1)
    {
        Component& modified_component = m_components[modified_index];
        m_entity_manager->SetComponentData(m_entity_id, modified_component.hash, modified_component.properties);
    }
}

const std::vector<Component>& ComponentProxy::GetComponents() const
{
    return m_components;
}

std::vector<Component>& ComponentProxy::GetComponents()
{
    return m_components;
}

float ComponentProxy::GetRotation() const
{
    const math::Matrix& transform = m_transform_system->GetTransform(m_entity_id);
    return math::GetZRotation(transform);
}

void ComponentProxy::SetRotation(float rotation)
{
    math::Matrix& transform = m_transform_system->GetTransform(m_entity_id);
    const math::Vector position = math::GetPosition(transform);
    transform = math::CreateMatrixFromZRotation(rotation);
    math::Position(transform, position);
}

math::Vector ComponentProxy::GetPosition() const
{
    const math::Matrix& transform = m_transform_system->GetTransform(m_entity_id);
    return math::GetPosition(transform);
}

void ComponentProxy::SetPosition(const math::Vector& position)
{
    math::Matrix& transform = m_transform_system->GetTransform(m_entity_id);
    math::Position(transform, position);
}

std::unique_ptr<editor::IObjectProxy> ComponentProxy::Clone() const
{
    const std::vector<uint32_t> empty_components;
    mono::Entity new_entity = m_entity_manager->CreateEntity("", empty_components);

    for(const Component& component : m_components)
    {
        m_entity_manager->AddComponent(new_entity.id, component.hash);
        m_entity_manager->SetComponentData(new_entity.id, component.hash, component.properties);
    }

    return std::make_unique<ComponentProxy>(new_entity.id, "unnamed", m_components, m_entity_manager, m_transform_system);
}

void ComponentProxy::Visit(IObjectVisitor& visitor)
{
    visitor.Accept(this);
}