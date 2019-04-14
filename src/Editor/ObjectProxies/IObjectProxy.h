
#pragma once

#include "MonoPtrFwd.h"
#include "Math/MathFwd.h"
#include <vector>

struct Attribute;
struct Component;

namespace editor
{
    struct SnapPoint;
    
    class IObjectProxy
    {
    public:

        virtual ~IObjectProxy()
        { }

        virtual const char* Name() const = 0;
        virtual uint32_t Id() const = 0;
        virtual mono::IEntityPtr Entity() = 0;

        virtual void SetSelected(bool selected) = 0;
        virtual bool Intersects(const math::Vector& position) const = 0;
        virtual std::vector<struct Grabber> GetGrabbers() const = 0;
        virtual std::vector<SnapPoint> GetSnappers() const = 0;

        virtual void UpdateUIContext(struct UIContext& context) = 0;

        virtual const std::vector<Component>& GetComponents() const = 0;
        virtual std::vector<Component>& GetComponents() = 0;

        virtual std::unique_ptr<IObjectProxy> Clone() const = 0;
        virtual void Visit(class IObjectVisitor& visitor) = 0;
    };
}

using IObjectProxyPtr = std::unique_ptr<editor::IObjectProxy>;
