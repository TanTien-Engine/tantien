#include "modules/graphics/GtxtCore.h"

#include <gtxt_freetype.h>
#include <gtxt_glyph.h>
#include <gtxt_label.h>
#include <gtxt_richtext.h>

#include <cstring>
#include <mutex>
#include <string>
#include <unordered_map>

namespace
{

std::recursive_mutex g_mutex;
bool g_ready = false;
bool g_failed = false;
bool g_owner_set = false;
std::thread::id g_owner;
std::unordered_map<std::string, int> g_name_to_font;
std::unordered_map<std::string, int> g_path_to_font;
std::unordered_map<int, std::string> g_font_to_path;

void stub_user_font_layout(int, int, struct gtxt_glyph_layout* layout)
{
    if (layout)
    {
        std::memset(layout, 0, sizeof(*layout));
    }
}

bool lock_owner()
{
    const auto id = std::this_thread::get_id();
    if (!g_owner_set)
    {
        g_owner = id;
        g_owner_set = true;
        return true;
    }
    return g_owner == id;
}

} // namespace

namespace tt
{

bool GtxtCore::Ensure()
{
    std::lock_guard<std::recursive_mutex> lock(g_mutex);
    if (g_failed)
    {
        return false;
    }
    if (!lock_owner())
    {
        return false;
    }
    if (g_ready)
    {
        return gtxt_ft_ready() != 0 && gtxt_glyph_ready() != 0;
    }

    gtxt_ft_create();
    if (!gtxt_ft_ready())
    {
        g_failed = true;
        return false;
    }
    gtxt_glyph_create(50, 500, nullptr, stub_user_font_layout);
    if (!gtxt_glyph_ready())
    {
        g_failed = true;
        return false;
    }
    g_ready = true;
    return true;
}

bool GtxtCore::ready()
{
    std::lock_guard<std::recursive_mutex> lock(g_mutex);
    return g_ready && !g_failed && gtxt_ft_ready() != 0 && gtxt_glyph_ready() != 0;
}

bool GtxtCore::OnOwnerThread()
{
    std::lock_guard<std::recursive_mutex> lock(g_mutex);
    return g_owner_set && g_owner == std::this_thread::get_id();
}

std::thread::id GtxtCore::OwnerThread()
{
    std::lock_guard<std::recursive_mutex> lock(g_mutex);
    return g_owner;
}

int GtxtCore::AddFont(const char* name, const char* filepath)
{
    std::lock_guard<std::recursive_mutex> lock(g_mutex);
    if (!Ensure())
    {
        return -1;
    }
    if (!name || !filepath || name[0] == '\0' || filepath[0] == '\0')
    {
        return -1;
    }

    const std::string key_name(name);
    const std::string key_path(filepath);
    const auto by_name = g_name_to_font.find(key_name);
    if (by_name != g_name_to_font.end())
    {
        const auto path_it = g_font_to_path.find(by_name->second);
        if (path_it != g_font_to_path.end() && path_it->second == key_path)
        {
            return by_name->second;
        }
        return -1;
    }
    if (key_name != "default")
    {
        const auto by_path = g_path_to_font.find(key_path);
        if (by_path != g_path_to_font.end())
        {
            if (gtxt_richtext_add_font_alias(name, by_path->second) != 0)
            {
                return -1;
            }
            g_name_to_font[key_name] = by_path->second;
            return by_path->second;
        }
    }

    const int index = gtxt_ft_add_font(name, filepath);
    if (index < 0)
    {
        return -1;
    }
    g_name_to_font[key_name] = index;
    if (g_path_to_font.find(key_path) == g_path_to_font.end())
    {
        g_path_to_font[key_path] = index;
    }
    g_font_to_path[index] = key_path;
    return index;
}

bool GtxtCore::HasGlyph(int font, int unicode)
{
    std::lock_guard<std::recursive_mutex> lock(g_mutex);
    if (!Ensure())
    {
        return false;
    }
    return gtxt_ft_has_glyph(font, unicode) != 0;
}

void GtxtCore::SetProcessLabelCallback(gtxt_draw_glyph_fn draw_glyph)
{
    std::lock_guard<std::recursive_mutex> lock(g_mutex);
    if (!Ensure())
    {
        return;
    }
    gtxt_label_cb_init(draw_glyph);
}

} // namespace tt
