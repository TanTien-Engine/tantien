#include "modules/graphics/LoadingList.h"
#include "modules/graphics/DTex.h"

namespace
{

const int SYM_EXTRUDE = 1;

}

namespace tt
{

TT_SINGLETON_DEFINITION(LoadingList)

LoadingList::LoadingList()
{
}

void LoadingList::AddGlyph(UID uid, int unicode, float line_x, const gtxt_glyph_style& gs)
{
	auto itr = m_glyphs.find(uid);
	if (itr == m_glyphs.end()) {
		m_glyphs.insert({ uid,{ unicode, line_x, gs } });
	}
}

void LoadingList::AddSymbol(UID uid, const ur::TexturePtr& tex, const sm::irect& region)
{
	auto itr = m_symbols.find(uid);
	if (itr == m_symbols.end()) {
		m_symbols.insert({ uid,{ tex, region } });
	}
}

bool LoadingList::Flush(ur::Context& ctx)
{
	bool dirty = false;
	if (FlushGlyphs(ctx)) {
		dirty = true;
	}
	if (FlushSymbols(ctx)) {
		dirty = true;
	}
	return dirty;
}

bool LoadingList::FlushGlyphs(ur::Context& ctx)
{
	bool dirty = false;
	if (m_glyphs.empty()) {
		return dirty;
	}

	auto dtex = DTex::Instance();
	for (auto& itr : m_glyphs)
	{
		auto& g = itr.second;

		struct gtxt_glyph_layout layout;
		uint32_t* bmp = gtxt_glyph_get_bitmap(g.unicode, g.line_x, &g.gs, &layout);
		if (!bmp) {
			continue;
		}
		int w = static_cast<int>(layout.sizer.width);
		int h = static_cast<int>(layout.sizer.height);
		dtex->LoadGlyph(ctx, bmp, w, h, itr.first);
		dirty = true;
	}
	m_glyphs.clear();

	return dirty;
}

bool LoadingList::FlushSymbols(ur::Context& ctx)
{
	if (m_symbols.empty()) {
		return false;
	}

	auto dtex = DTex::Instance();
	dtex->LoadSymStart();
	for (auto& itr : m_symbols) {
		auto& s = itr.second;
		dtex->LoadSymbol(itr.first, s.tex, s.region, 0, SYM_EXTRUDE);
	}
	dtex->LoadSymFinish(ctx);
	m_symbols.clear();

	return true;
}

}