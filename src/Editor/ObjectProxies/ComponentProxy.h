
#pragma once

#include "IObjectProxy.h"
#include <vector>
#include <string>

struct Component;

namespace mono
{
    class IEntityManager;
    class TransformSystem;
}

namespace editor
{
    class Editor;

    class ComponentProxy : public IObjectProxy
    {
    public:

        ComponentProxy(
            uint32_t entity_id,
            const std::string& name,
            const std::string& folder,
            const std::vector<Component>& components,
            mono::IEntityManager* entity_manager,
            mono::TransformSystem* transform_system,
            Editor* editor);
        
        ~ComponentProxy();

        const char* Name() const override;
        uint32_t Id() const override;

        void SetSelected(bool selected) override;
        bool Intersects(const math::Vector& position) const override;
        std::vector<Grabber> GetGrabbers() override;
        std::vector<SnapPoint> GetSnappers() const override;

        void UpdateUIContext(struct UIContext& context) override;

        void SetFolder(const std::string& folder);
        std::string GetFolder() const override;

        const std::vector<Component>& GetComponents() const override;
        std::vector<Component>& GetComponents() override;

        uint32_t GetEntityProperties() const;
        void SetEntityProperties(uint32_t properties);

        math::Vector GetPosition() const override;
        void SetPosition(const math::Vector& position) override;
        float GetRotation() const override;
        void SetRotation(float rotation) override;
        math::Quad GetBoundingBox() const override;

        std::unique_ptr<IObjectProxy> Clone() const override;
        void Visit(class IObjectVisitor& visitor) override;

    private:

        const uint32_t m_entity_id;
        std::string m_name;
        std::string m_folder;
        uint32_t m_entity_properties;
        std::vector<Component> m_components;
        mono::IEntityManager* m_entity_manager;
        mono::TransformSystem* m_transform_system;
        Editor* m_editor;
    };
}
