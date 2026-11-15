#pragma once

#include <gtxt_label.h>

#include <thread>

namespace tt
{

class GtxtCore
{
public:
    static bool Ensure();
    static bool ready();
    static bool OnOwnerThread();
    static std::thread::id OwnerThread();
    static int AddFont(const char* name, const char* filepath);
    static bool HasGlyph(int font, int unicode);
    static void SetProcessLabelCallback(gtxt_draw_glyph_fn draw_glyph);
};

} // namespace tt
