#pragma once

#include "core/macro.h"
#include "modules/filesystem/ResourceUID.h"

#include <SM_Rect.h>
#include <gtxt_glyph.h>
#include <unirender/typedef.h>
#include <unirender/noncopyable.h>

#include <map>
#include <set>

namespace ur { class Context; }

namespace tt
{

class LoadingList : ur::noncopyable
{
public:
	void AddGlyph(UID uid, int unicode, float line_x, const gtxt_glyph_style& gs);
	void AddSymbol(UID uid, const ur::TexturePtr& tex, const sm::irect& region);

	bool Flush(ur::Context& ctx);

private:
	bool FlushGlyphs(ur::Context& ctx);
	bool FlushSymbols(ur::Context& ctx);

private:
	struct Glyph
	{
		int unicode;
		float line_x;
		gtxt_glyph_style gs;
	};

	struct Symbol
	{
        ur::TexturePtr tex = nullptr;
		sm::irect region;
	};

private:
	std::map<UID, Glyph> m_glyphs;
	std::map<UID, Symbol> m_symbols;

	// Glyphs that produced no bitmap (permanently missing). Without this they were
	// re-queued and re-rasterized by freetype every single frame, forever.
	std::set<UID> m_failed_glyphs;

	TT_SINGLETON_DECLARATION(LoadingList)

}; // LoadingList

}