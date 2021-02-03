#pragma once

#include "core/macro.h"

#include <SM_Vector.h>
#include <SM_Matrix2D.h>
#include <gtxt_label.h>	// gtxt_label_style

#include <vector>
#include <string>
#include <set>

namespace ur { class Device; class Context; }
namespace tess { class Painter; }

namespace tt
{

class GTxt
{
public:
    void Init(const ur::Device& dev);

	void LoadFonts(const std::vector<std::pair<std::string, std::string>>& fonts,
		const std::vector<std::pair<std::string, std::string>>& user_fonts);

	static void Draw(ur::Context& ctx, const std::string& text, const gtxt_label_style& style, const sm::Matrix2D& mat,
		uint32_t mul, uint32_t add, int time, bool richtext, tess::Painter* pt = nullptr, bool texcoords_relocate = true);

	//static sm::vec2 CalcLabelSize(const std::string& text, const gtxt_label_style& style);

private:
	void LoadFont(const std::string& name, const std::string& filepath);
	void LoadUserFont(const std::string& name, const std::string& filepath);
	void LoadUserFontChar(const std::string& str, const std::string& pkg, const std::string& node);

private:
	static int m_cap_bitmap, m_cap_layout;

	std::set<std::string> m_fonts;

private:
	TT_SINGLETON_DECLARATION(GTxt)

}; // GTxt

}