
#include "EntityProxy.h"
#include "IObjectVisitor.h"
#include "Grabber.h"
#include "SnapPoint.h"
#include "Objects/SpriteEntity.h"
#include "UI/UIContext.h"
#include "UI/UIProperties.h"

#include "Math/Quad.h"
#include "Math/MathFunctions.h"
#include "Math/Matrix.h"

#include "ImGuiImpl/ImGuiImpl.h"

#include "DefinedAttributes.h"
#include "ObjectAttribute.h"

using namespace editor;

EntityProxy::EntityProxy(const std::shared_ptr<SpriteEntity>& entity, const std::vector<Attribute>& attributes)
    : m_entity(entity),
      m_attributes(attributes)
{ }

const char* EntityProxy::Name() const
{
    return m_entity->Name().c_str();
}

unsigned int EntityProxy::Id() const
{
    return m_entity->Id();
}

mono::IEntityPtr EntityProxy::Entity()
{
    return m_entity;
}

void EntityProxy::SetSelected(bool selected)
{
    m_entity->SetSelected(selected);
}

bool EntityProxy::Intersects(const math::Vector& world_position) const
{
    const math::Quad bb = m_entity->BoundingBox();
    return math::PointInsideQuad(world_position, bb);
}

std::vector<Grabber> EntityProxy::GetGrabbers() const
{
    return std::vector<Grabber>();
}

std::vector<SnapPoint> EntityProxy::GetSnappers() const
{
    return std::vector<SnapPoint>();
}

void EntityProxy::UpdateUIContext(UIContext& context)
{
    world::SetAttribute(world::POSITION_ATTRIBUTE, m_attributes, m_entity->Position());
    world::SetAttribute(world::ROTATION_ATTRIBUTE, m_attributes, m_entity->Rotation());

    const std::string& name = m_entity->Name();
    ImGui::Text("%s", name.c_str());
    
    for(Attribute& id_attribute : m_attributes)
        DrawProperty(id_attribute);

    math::Vector position;
    float rotation = m_entity->Rotation();

    world::FindAttribute(world::POSITION_ATTRIBUTE, m_attributes, position);
    world::FindAttribute(world::ROTATION_ATTRIBUTE, m_attributes, rotation);

    m_entity->SetPosition(position);
    m_entity->SetRotation(rotation);
}

std::vector<Attribute> EntityProxy::GetAttributes() const
{
    return m_attributes;
}

void EntityProxy::SetAttributes(const std::vector<Attribute>& attributes)
{
    math::Vector position;
    float rotation = 0.0f;

    world::FindAttribute(world::POSITION_ATTRIBUTE, attributes, position);
    world::FindAttribute(world::ROTATION_ATTRIBUTE, attributes, rotation);

    m_entity->SetPosition(position);
    m_entity->SetRotation(rotation);

    world::MergeAttributes(m_attributes, attributes);
}

void EntityProxy::Visit(IObjectVisitor& visitor)
{
    visitor.Accept(this);
}
