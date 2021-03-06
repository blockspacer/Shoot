
#pragma once

#include "Rendering/IDrawable.h"
#include "Rendering/RenderBuffer/IRenderBuffer.h"
#include "Rendering/RenderFwd.h"
#include "Rendering/Texture/ITextureFactory.h"

class StaticBackground : public mono::IDrawable
{
public:

    StaticBackground(const char* background_texture);
    void Draw(mono::IRenderer& renderer) const override;
    math::Quad BoundingBox() const override;

    std::unique_ptr<mono::IRenderBuffer> m_vertex_buffer;
    std::unique_ptr<mono::IRenderBuffer> m_texture_buffer;
    std::unique_ptr<mono::IElementBuffer> m_index_buffer;

    mono::ITexturePtr m_texture;
};
