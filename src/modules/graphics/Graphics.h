#pragma once

#include "core/macro.h"

#include <SM_Vector.h>
#include <unirender/typedef.h>

#include <memory>

namespace tess { class Painter; }

namespace tt
{

class SpriteRenderer;

class Graphics
{
public:
	void OnSize(float width, float height);
	void OnCameraUpdate(const sm::vec2& offset, float scale);

	void DrawPainter(const tess::Painter& pt) const;
	void DrawText(const char* text) const;
	void DrawTexQuad(const float* positions, const float* texcoords, 
		const ur::TexturePtr& tex, uint32_t color);

	void Flush();

private:
	std::shared_ptr<SpriteRenderer> m_spr_rd = nullptr;

	TT_SINGLETON_DECLARATION(Graphics)

}; // Graphics

}