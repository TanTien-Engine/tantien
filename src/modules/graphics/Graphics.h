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

	// Incremented every time the palette texture is re-pointed into the DTex
	// atlas (RelocatePaletteUV). Painters bake the palette white UV at build
	// time, so a relocation silently invalidates every cached painter; consumers
	// (e.g. the blueprint canvas render cache) watch this epoch and rebuild.
	auto GetPaletteEpoch() const { return m_palette_epoch; }

	void EnableDTex(bool enable);

private:
	void RelocatePaletteUV();

private:
	float m_width = 0, m_height = 0;

	std::shared_ptr<Viewport> m_viewport = nullptr;

	std::shared_ptr<SpriteRenderer> m_spr_rd = nullptr;

	bool m_use_dtex = false;

	int m_palette_epoch = 0;

	TT_SINGLETON_DECLARATION(Graphics)

}; // Graphics

}