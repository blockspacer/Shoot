
#include "StaticBackground.h"

#include "Math/MathFunctions.h"
#include "Math/Quad.h"
#include "Rendering/RenderSystem.h"
#include "Rendering/RenderBuffer/BufferFactory.h"
#include "Rendering/IRenderer.h"
#include "Rendering/Texture/ITextureFactory.h"

StaticBackground::StaticBackground(const char* background_texture)
{
    constexpr math::Quad background_bb = math::Quad(-100.0f, -100.0f, 100.0f, 100.0f);

    const math::Vector vertices[] = {
        math::BottomLeft(background_bb),
        math::TopLeft(background_bb),
        math::TopRight(background_bb),
        math::BottomRight(background_bb)
    };

    const uint16_t indices[] = {
        0, 1, 2,
        0, 2, 3,
    };

    math::Vector uv_coordinates[] = {
        math::ZeroVec,
        math::ZeroVec,
        math::ZeroVec,
        math::ZeroVec,
    };

    //const math::Vector& repeate = (background_bb.mB - background_bb.mA) / 8.0f;
    const math::Vector repeate(12.0f, 12.0f);
        
    for(size_t index = 0; index < std::size(vertices); ++index)
        uv_coordinates[index] = math::MapVectorInQuad(vertices[index], background_bb) * repeate;

    m_texture = mono::GetTextureFactory()->CreateTexture(background_texture);

    m_vertex_buffer = mono::CreateRenderBuffer(mono::BufferType::STATIC, mono::BufferData::FLOAT, 2, std::size(vertices), vertices);
    m_texture_buffer = mono::CreateRenderBuffer(mono::BufferType::STATIC, mono::BufferData::FLOAT, 2, std::size(uv_coordinates), uv_coordinates);
    m_index_buffer = mono::CreateElementBuffer(mono::BufferType::STATIC, std::size(indices), indices);
}

void StaticBackground::Draw(mono::IRenderer& renderer) const
{
    renderer.DrawGeometry(m_vertex_buffer.get(), m_texture_buffer.get(), m_index_buffer.get(), m_texture.get(), 6);
}

math::Quad StaticBackground::BoundingBox() const
{
    return math::InfQuad;
}
