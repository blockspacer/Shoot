
#pragma once

#include "Entity/EntityBase.h"
#include "Rendering/RenderPtrFwd.h"
#include <string>
#include <vector>

struct SnapPoint;

namespace editor
{
    class Prefab : public mono::EntityBase
    {
    public:

        Prefab(const std::string& name, const std::string& sprite_file, const std::vector<SnapPoint>& snap_points);
        ~Prefab();

        void SetSelected(bool selected);
        const std::string& Name() const;
        const std::vector<SnapPoint>& SnapPoints() const;
        virtual math::Quad BoundingBox() const;
        
    private:

        virtual void Draw(mono::IRenderer& renderer) const;
        virtual void Update(unsigned int delta);
        
        const std::string m_name;
        std::vector<SnapPoint> m_snap_points;
        bool m_selected;
        mono::ISpritePtr m_sprite;
    };
}
