#pragma once

#include "core/macro.h"

#include <SM_Vector.h>
#include <SM_Matrix2D.h>
#include <SM_Rect.h>
#include <unirender/typedef.h>
#include <gtxt_label.h>

#include <memory>

namespace tess { class Painter; }

namespace tt
{

class Viewport;
class SpriteRenderer;

class Graphics
{
public:
	void OnSize(float width, float height);
	void OnCameraUpdate(const sm::vec2& offset, float scale);

	void DrawPainter(const tess::Painter& pt, const sm::rect& region = sm::rect()) const;
	void DrawText(const char* text, const sm::Matrix2D& mt, const gtxt_label_style& style) const;
	void DrawTexQuad(const float* positions, const float* texcoords,
		const ur::TexturePtr& tex, uint32_t color);

	void Flush();

	auto GetViewport() const { return m_viewport; }

	auto GetSpriteRenderer() { return m_spr_rd; }

	auto GetWidth() const { return m_width; }
	auto GetHeight() const { return m_height; }

	void EnableDTex(bool enable);

private:
	void RelocatePaletteUV();

private:
	float m_width = 0, m_height = 0;

	std::shared_ptr<Viewport> m_viewport = nullptr;

	std::shared_ptr<SpriteRenderer> m_spr_rd = nullptr;

	bool m_use_dtex = false;

	TT_SINGLETON_DECLARATION(Graphics)

}; // Graphics

}