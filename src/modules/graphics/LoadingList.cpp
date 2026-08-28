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
	// Already known to have no bitmap -> don't re-queue it for rasterization.
	if (m_failed_glyphs.find(uid) != m_failed_glyphs.end()) {
		return;
	}
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

DTexFlushResult LoadingList::Flush(ur::Context& ctx)
{
	DTexFlushResult result;
	if (FlushGlyphs(ctx)) {
		result.had_work = true;
	}
	const DTexFlushResult symbols = FlushSymbols(ctx);
	result.success = symbols.success;
	result.had_work = result.had_work || symbols.had_work;
	return result;
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
			// No bitmap for this glyph: remember it so AddGlyph stops re-queuing it
			// every frame (freetype rasterize attempt per frame, forever).
			m_failed_glyphs.insert(itr.first);
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

DTexFlushResult LoadingList::FlushSymbols(ur::Context& ctx)
{
	DTexFlushResult result;
	if (m_symbols.empty()) {
		return result;
	}

	auto dtex = DTex::Instance();
	dtex->LoadSymStart();
	for (auto& itr : m_symbols) {
		auto& s = itr.second;
		if (!dtex->LoadSymbol(itr.first, s.tex, s.region, 0, SYM_EXTRUDE)) {
			result.success = false;
		}
	}
	if (!dtex->LoadSymFinish(ctx)) {
		result.success = false;
		result.had_work = true;
		return result;
	}
	result.had_work = true;
	for (auto itr = m_symbols.begin(); itr != m_symbols.end(); ) {
		ur::TexturePtr texture;
		int block_id = 0;
		if (dtex->QuerySymbol(itr->first, texture, block_id)) {
			itr = m_symbols.erase(itr);
		} else {
			result.success = false;
			++itr;
		}
	}
	return result;
}

}