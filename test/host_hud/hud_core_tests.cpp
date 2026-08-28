#include "modules/graphics/GtxtCore.h"
#include "voxel_game/ui/GtxtHud.h"
#include "voxel_game/ui/HudBatch.h"
#include "voxel_game/ui/HudGlyphCache.h"
#include "voxel_game/ui/HudLayout.h"
#include "voxel_game/ui/HudTypes.h"
#include "voxel_game/ui/HudUtf8.h"

#include <gtxt_freetype.h>
#include <gtxt_glyph.h>
#include <gtxt_label.h>
#include <gtxt_richtext.h>

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <exception>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <string_view>
#include <thread>
#include <unordered_set>
#include <vector>

namespace
{

void require(bool condition, const char* message)
{
    if (!condition)
    {
        throw std::runtime_error(message);
    }
}

void require_status(tt::voxel_game::HudStatus actual, tt::voxel_game::HudStatus expected,
                    const char* message)
{
    if (actual != expected)
    {
        throw std::runtime_error(std::string(message) + " got " +
                                 tt::voxel_game::hud_status_name(actual));
    }
}

struct FakeGlyphCache : tt::voxel_game::HudGlyphCache
{
    struct Key
    {
        char32_t codepoint;
        int font_size;
        std::uint32_t color;

        bool operator==(const Key& other) const
        {
            return codepoint == other.codepoint && font_size == other.font_size &&
                   color == other.color;
        }
    };

    struct KeyHash
    {
        std::size_t operator()(const Key& key) const
        {
            return static_cast<std::size_t>(key.codepoint) ^
                   (static_cast<std::size_t>(key.font_size) << 8) ^
                   static_cast<std::size_t>(key.color);
        }
    };

    bool exists(char32_t codepoint, int font_size, std::uint32_t packed_rgba) const override
    {
        return m_loaded.find(Key{codepoint, font_size, packed_rgba}) != m_loaded.end();
    }

    void request_load(char32_t codepoint, int font_size, std::uint32_t packed_rgba) override
    {
        m_pending.insert(Key{codepoint, font_size, packed_rgba});
    }

    bool flush_loads() override
    {
        if (m_fail_flush)
        {
            m_pending.clear();
            return false;
        }
        for (const Key& key : m_pending)
        {
            if (m_never_load.find(key.codepoint) == m_never_load.end())
            {
                m_loaded.insert(key);
            }
        }
        m_pending.clear();
        return true;
    }

    bool query(char32_t codepoint, int font_size, std::uint32_t packed_rgba,
               tt::voxel_game::HudGlyphUv& uv) const override
    {
        if (!exists(codepoint, font_size, packed_rgba))
        {
            return false;
        }
        uv.texcoords[0] = 0.0f;
        uv.texcoords[1] = 0.0f;
        uv.texcoords[2] = 1.0f;
        uv.texcoords[3] = 0.0f;
        uv.texcoords[4] = 1.0f;
        uv.texcoords[5] = 1.0f;
        uv.texcoords[6] = 0.0f;
        uv.texcoords[7] = 1.0f;
        uv.texture_token = reinterpret_cast<const void*>(1);
        return true;
    }

    bool m_fail_flush = false;
    std::unordered_set<char32_t> m_never_load;
    std::unordered_set<Key, KeyHash> m_pending;
    std::unordered_set<Key, KeyHash> m_loaded;
};

void test_utf8_accepts_well_formed()
{
    std::vector<char32_t> codepoints;
    require_status(tt::voxel_game::HudUtf8::decode(std::string_view(), codepoints),
                   tt::voxel_game::HudStatus::Ok, "empty");
    require(codepoints.empty(), "empty has no codepoints");

    require_status(tt::voxel_game::HudUtf8::decode(std::string_view("ASCII", 5), codepoints),
                   tt::voxel_game::HudStatus::Ok, "ascii");
    require(codepoints.size() == 5, "ascii count");

    require_status(tt::voxel_game::HudUtf8::decode(std::string_view("\xC2\xA2", 2), codepoints),
                   tt::voxel_game::HudStatus::Ok, "two-byte");
    require(codepoints.size() == 1 && codepoints[0] == 0xA2, "cent");

    require_status(tt::voxel_game::HudUtf8::decode(std::string_view("\xE6\x96\xB9", 3), codepoints),
                   tt::voxel_game::HudStatus::Ok, "three-byte");
    require(codepoints.size() == 1 && codepoints[0] == 0x65B9, "han");

    require_status(tt::voxel_game::HudUtf8::decode(std::string_view("\xF0\x9F\x98\x80", 4), codepoints),
                   tt::voxel_game::HudStatus::Ok, "four-byte");
    require(codepoints.size() == 1 && codepoints[0] == 0x1F600, "emoji");

    const char embedded[] = {'A', '\0', 'B'};
    require_status(tt::voxel_game::HudUtf8::decode(std::string_view(embedded, 3), codepoints),
                   tt::voxel_game::HudStatus::Ok, "embedded NUL");
    require(codepoints.size() == 3 && codepoints[0] == U'A' && codepoints[1] == 0 &&
                codepoints[2] == U'B',
            "A\\0B is three codepoints");
}

void test_utf8_rejects_illegal()
{
    std::vector<char32_t> codepoints;
    require_status(tt::voxel_game::HudUtf8::decode(std::string_view("\xC2", 1), codepoints),
                   tt::voxel_game::HudStatus::InvalidUtf8, "truncated two-byte");
    require_status(tt::voxel_game::HudUtf8::decode(std::string_view("\xE6\x96", 2), codepoints),
                   tt::voxel_game::HudStatus::InvalidUtf8, "truncated three-byte");
    require_status(tt::voxel_game::HudUtf8::decode(std::string_view("\xF0\x9F\x98", 3), codepoints),
                   tt::voxel_game::HudStatus::InvalidUtf8, "truncated four-byte");

    require_status(tt::voxel_game::HudUtf8::decode(std::string_view("\xC2\x20", 2), codepoints),
                   tt::voxel_game::HudStatus::InvalidUtf8, "illegal continuation");
    require_status(tt::voxel_game::HudUtf8::decode(std::string_view("\x80", 1), codepoints),
                   tt::voxel_game::HudStatus::InvalidUtf8, "lone continuation");

    require_status(tt::voxel_game::HudUtf8::decode(std::string_view("\xC0\x80", 2), codepoints),
                   tt::voxel_game::HudStatus::InvalidUtf8, "overlong two-byte");
    require_status(tt::voxel_game::HudUtf8::decode(std::string_view("\xE0\x80\x80", 3), codepoints),
                   tt::voxel_game::HudStatus::InvalidUtf8, "overlong three-byte");
    require_status(tt::voxel_game::HudUtf8::decode(std::string_view("\xF0\x80\x80\x80", 4), codepoints),
                   tt::voxel_game::HudStatus::InvalidUtf8, "overlong four-byte");

    require_status(tt::voxel_game::HudUtf8::decode(std::string_view("\xED\xA0\x80", 3), codepoints),
                   tt::voxel_game::HudStatus::InvalidUtf8, "surrogate");
    require_status(tt::voxel_game::HudUtf8::decode(std::string_view("\xF4\x90\x80\x80", 4), codepoints),
                   tt::voxel_game::HudStatus::InvalidUtf8, "above U+10FFFF");
    require_status(tt::voxel_game::HudUtf8::decode(std::string_view("\xF8\x80\x80\x80\x80", 5), codepoints),
                   tt::voxel_game::HudStatus::InvalidUtf8, "five-byte overlong encoding");
    require_status(tt::voxel_game::HudUtf8::decode(std::string_view("\xFF", 1), codepoints),
                   tt::voxel_game::HudStatus::InvalidUtf8, "illegal lead");
}

void test_glyph_key_keeps_alpha()
{
    tt::voxel_game::HudGlyphKey opaque;
    opaque.font_id = 1;
    opaque.codepoint = U'A';
    opaque.font_size_px = 16;
    opaque.packed_rgba = 0xffffffFFu;

    tt::voxel_game::HudGlyphKey transparent = opaque;
    transparent.packed_rgba = 0xffffff00u;

    require(tt::voxel_game::hud_glyph_key_bits(opaque) !=
                tt::voxel_game::hud_glyph_key_bits(transparent),
            "key must keep alpha");
    require(!(opaque == transparent), "keys with different alpha are distinct");

    tt::voxel_game::HudColor color;
    color.r = 1.0f;
    color.g = 0.0f;
    color.b = 0.0f;
    color.a = 0.5f;
    const std::uint32_t packed = tt::voxel_game::hud_pack_rgba(color);
    require((packed & 0xffu) == 128 || (packed & 0xffu) == 127, "alpha packed");
}

void test_layout_letter_spacing()
{
    tt::voxel_game::HudStyle style;
    style.font_size_px = 20;
    style.letter_spacing = 10.0f;
    std::vector<char32_t> text = {U'A', U'B'};
    std::vector<tt::voxel_game::HudLaidGlyph> glyphs;
    require_status(tt::voxel_game::HudLayout::layout(text, style, {0.0f, 0.0f}, glyphs),
                   tt::voxel_game::HudStatus::Ok, "layout spacing");
    require(glyphs.size() == 2, "two glyphs");
    require(glyphs[1].x - glyphs[0].x == 20.0f, "advance is cell width plus letter_spacing");
}

void test_layout_and_origin()
{
    tt::voxel_game::HudStyle style;
    style.font_size_px = 20;
    std::vector<char32_t> text = {U'A', U'B'};
    std::vector<tt::voxel_game::HudLaidGlyph> glyphs;
    require_status(tt::voxel_game::HudLayout::layout(text, style, {10.0f, 30.0f}, glyphs),
                   tt::voxel_game::HudStatus::Ok, "layout AB");
    require(glyphs.size() == 2, "two glyphs");
    require(glyphs[0].x == 10.0f && glyphs[0].y == 30.0f, "origin is first cell bottom-left");
    require(glyphs[1].x > glyphs[0].x, "left to right");
    require(glyphs[1].y == glyphs[0].y, "same baseline");

    text = {U'A', U'\n', U'B'};
    require_status(tt::voxel_game::HudLayout::layout(text, style, {0.0f, 40.0f}, glyphs),
                   tt::voxel_game::HudStatus::Ok, "layout newline");
    require(glyphs.size() == 3, "newline keeps a whitespace glyph");
    require(glyphs[2].y < glyphs[0].y, "y-up framebuffer: next line is lower y");

    style.font_size_px = 0;
    require_status(tt::voxel_game::HudLayout::layout(text, style, {}, glyphs),
                   tt::voxel_game::HudStatus::InvalidArgument, "zero font size");
}

void test_batch_capacity()
{
    tt::voxel_game::HudBatch batch;
    tt::voxel_game::HudQuad quad;
    for (std::size_t i = 0; i < tt::voxel_game::HudBatch::kMaxQuads; ++i)
    {
        require_status(batch.try_add(quad), tt::voxel_game::HudStatus::Ok, "fill batch");
    }
    require_status(batch.try_add(quad), tt::voxel_game::HudStatus::BatchFull, "overflow");
    require(batch.size() == tt::voxel_game::HudBatch::kMaxQuads, "cap holds");
    batch.clear();
    require(batch.empty(), "clear");
}

void test_compose_capacity_is_atomic()
{
    tt::voxel_game::HudStyle style;
    style.font_size_px = 16;
    std::vector<char32_t> text = {U'A', U'B'};
    std::vector<tt::voxel_game::HudLaidGlyph> glyphs;
    require_status(tt::voxel_game::HudLayout::layout(text, style, {}, glyphs),
                   tt::voxel_game::HudStatus::Ok, "layout");

    tt::voxel_game::HudBatch batch;
    tt::voxel_game::HudQuad filler;
    for (std::size_t i = 0; i < tt::voxel_game::HudBatch::kMaxQuads - 1; ++i)
    {
        batch.try_add(filler);
    }
    const std::size_t before = batch.size();
    FakeGlyphCache cache;
    require_status(tt::voxel_game::compose_hud_batch(glyphs, style, cache, batch),
                   tt::voxel_game::HudStatus::BatchFull, "two glyphs exceed remaining cap");
    require(batch.size() == before, "failed compose must not write a partial batch");
}

void test_same_frame_upload_failure_and_missing()
{
    tt::voxel_game::HudStyle style;
    style.font_size_px = 16;
    std::vector<char32_t> text = {U'H', U'i'};
    std::vector<tt::voxel_game::HudLaidGlyph> glyphs;
    require_status(tt::voxel_game::HudLayout::layout(text, style, {}, glyphs),
                   tt::voxel_game::HudStatus::Ok, "layout Hi");

    FakeGlyphCache cache;
    tt::voxel_game::HudBatch batch;
    require_status(tt::voxel_game::compose_hud_batch(glyphs, style, cache, batch),
                   tt::voxel_game::HudStatus::Ok, "same-frame collect/flush/emit");
    require(batch.size() == 2, "both glyphs emitted after flush in the same call");

    FakeGlyphCache failing;
    failing.m_fail_flush = true;
    tt::voxel_game::HudBatch empty_batch;
    require_status(tt::voxel_game::compose_hud_batch(glyphs, style, failing, empty_batch),
                   tt::voxel_game::HudStatus::AtlasUploadFailed,
                   "upload failure is AtlasUploadFailed");
    require(empty_batch.empty(), "no quads on upload failure");

    FakeGlyphCache missing;
    missing.m_never_load.insert(0x5757);
    std::vector<char32_t> cjk = {0x5757};
    std::vector<tt::voxel_game::HudLaidGlyph> cjk_glyphs;
    require_status(tt::voxel_game::HudLayout::layout(cjk, style, {}, cjk_glyphs),
                   tt::voxel_game::HudStatus::Ok, "layout CJK");
    tt::voxel_game::HudBatch cjk_batch;
    require_status(tt::voxel_game::compose_hud_batch(cjk_glyphs, style, missing, cjk_batch),
                   tt::voxel_game::HudStatus::GlyphMissing,
                   "missing CJK codepoint is GlyphMissing, not a fallback box");
    require(cjk_batch.empty(), "no fallback quads");
}

int g_process_callback_hits = 0;
int g_hud_callback_hits = 0;

void process_label_callback(int, float, float, float, float, float,
                            const gtxt_glyph_style*, const gtxt_draw_style*, void*)
{
    ++g_process_callback_hits;
}

void hud_label_callback(int, float, float, float, float, float,
                        const gtxt_glyph_style*, const gtxt_draw_style*, void*)
{
    ++g_hud_callback_hits;
}

const char* pick_core_font()
{
    static const char* kCandidates[] = {
        "assets/fonts/hkljhw8.ttf",
        "assets/fonts/default2.ttf",
    };
    for (const char* path : kCandidates)
    {
        std::ifstream stream(path, std::ios::binary);
        if (stream)
        {
            return path;
        }
    }
    return nullptr;
}

void stub_user_font_layout(int, int, gtxt_glyph_layout* layout)
{
    if (layout)
    {
        std::memset(layout, 0, sizeof(*layout));
    }
}

int g_user_font_layout_index = -1;
int g_user_font_layout_unicode = -1;

void capture_user_font_layout(int unicode, int font, gtxt_glyph_layout* layout)
{
    g_user_font_layout_unicode = unicode;
    g_user_font_layout_index = font;
    if (layout)
    {
        std::memset(layout, 0, sizeof(*layout));
        layout->sizer.width = 7.0f;
        layout->sizer.height = 9.0f;
        layout->advance = 8.0f;
    }
}

bool is_zero_layout(const gtxt_glyph_layout& layout)
{
    return layout.sizer.width == 0.0f && layout.sizer.height == 0.0f &&
           layout.bearing_x == 0.0f && layout.bearing_y == 0.0f &&
           layout.advance == 0.0f && layout.metrics_height == 0.0f;
}

float g_synthetic_bitmap_width = 1.0f;
float g_synthetic_bitmap_height = 1.0f;

std::uint32_t* synthetic_user_font_bitmap(const char*, const gtxt_glyph_style*,
                                          gtxt_glyph_layout* layout)
{
    static std::uint32_t pixel = 0xffffffffu;
    std::memset(layout, 0, sizeof(*layout));
    layout->sizer.width = g_synthetic_bitmap_width;
    layout->sizer.height = g_synthetic_bitmap_height;
    return &pixel;
}

struct RichtextProbe
{
    int glyphs = 0;
    int external_glyphs = 0;
};

int richtext_probe_callback(const char*, float, gtxt_richtext_style* style, void* userdata)
{
    auto* probe = static_cast<RichtextProbe*>(userdata);
    ++probe->glyphs;
    if (style && style->ext_sym_ud)
    {
        ++probe->external_glyphs;
    }
    return 1;
}

int g_ext_create_hits = 0;
int g_ext_release_hits = 0;
int g_ext_query_hits = 0;
int g_ext_token = 0;

void* test_ext_create(const char* name)
{
    if (!name || name[0] == '\0')
    {
        return nullptr;
    }
    ++g_ext_create_hits;
    return &g_ext_token;
}

void* test_ext_create_null(const char*)
{
    ++g_ext_create_hits;
    return nullptr;
}

void test_ext_release(void* ext_sym)
{
    if (ext_sym)
    {
        ++g_ext_release_hits;
    }
}

void test_ext_size(void*, int* width, int* height)
{
    *width = 16;
    *height = 16;
}

bool test_ext_query(void*, float, float, float, float, int, int, void*)
{
    ++g_ext_query_hits;
    return true;
}

void test_gtxt_transactional_lifecycle()
{
    gtxt_label_cb_init(nullptr);
    gtxt_glyph_release();
    gtxt_glyph_release();
    gtxt_ft_release();
    gtxt_ft_release();
    gtxt_richtext_release();
    gtxt_richtext_release();

    gtxt_glyph_create(0, 0, nullptr, &stub_user_font_layout);
    require(gtxt_glyph_ready() == 0, "zero-capacity glyph cache is rejected without partial state");
    gtxt_glyph_release();
    gtxt_glyph_release();

    gtxt_glyph_create(4, 16, nullptr, &stub_user_font_layout);
    require(gtxt_glyph_ready() != 0, "valid glyph cache is ready");
    gtxt_glyph_release();
    gtxt_glyph_release();

    gtxt_glyph_style missing_style;
    std::memset(&missing_style, 0, sizeof(missing_style));
    missing_style.font_size = 16;

    gtxt_glyph_create(4, 16, nullptr, nullptr);
    require(gtxt_glyph_ready() != 0,
            "glyph cache may omit the user-font layout callback safely");
    const gtxt_glyph_layout* missing_layout =
        gtxt_glyph_get_layout('A', 0.0f, &missing_style);
    require(missing_layout != nullptr && is_zero_layout(*missing_layout),
            "missing user-font layout callback returns a zero layout");
    gtxt_glyph_layout missing_bitmap_layout;
    std::memset(&missing_bitmap_layout, 0x7f, sizeof(missing_bitmap_layout));
    require(gtxt_glyph_get_bitmap('A', 0.0f, &missing_style,
                                  &missing_bitmap_layout) == nullptr,
            "missing user-font callbacks fail bitmap lookup safely");
    require(is_zero_layout(missing_bitmap_layout),
            "failed user-font bitmap lookup returns a zero layout");
    require(gtxt_glyph_get_layout('A', 0.0f, nullptr) == nullptr,
            "null glyph style is rejected");
    std::memset(&missing_bitmap_layout, 0x7f, sizeof(missing_bitmap_layout));
    require(gtxt_glyph_get_bitmap('A', 0.0f, nullptr,
                                  &missing_bitmap_layout) == nullptr,
            "null bitmap style is rejected");
    require(is_zero_layout(missing_bitmap_layout),
            "null bitmap style still clears the output layout");
    gtxt_glyph_release();

    gtxt_glyph_create(4, 16, &synthetic_user_font_bitmap, nullptr);
    require(gtxt_glyph_ready() != 0, "oversize bitmap test cache is ready");
    g_synthetic_bitmap_width = 1000000000.0f;
    g_synthetic_bitmap_height = 1000000000.0f;
    for (int attempt = 0; attempt < 2; ++attempt)
    {
        std::memset(&missing_bitmap_layout, 0x7f, sizeof(missing_bitmap_layout));
        require(gtxt_glyph_get_bitmap('B', 0.0f, &missing_style,
                                      &missing_bitmap_layout) == nullptr,
                "oversize bitmap allocation fails without dereference");
        require(is_zero_layout(missing_bitmap_layout),
                "oversize bitmap failure returns a zero layout");
    }
    g_synthetic_bitmap_width = 1.5f;
    g_synthetic_bitmap_height = 2.0f;
    require(gtxt_glyph_get_bitmap('B', 0.0f, &missing_style,
                                  &missing_bitmap_layout) == nullptr,
            "fractional bitmap dimensions are rejected");
    require(is_zero_layout(missing_bitmap_layout),
            "fractional bitmap rejection returns a zero layout");
    g_synthetic_bitmap_width = std::numeric_limits<float>::quiet_NaN();
    g_synthetic_bitmap_height = 2.0f;
    require(gtxt_glyph_get_bitmap('B', 0.0f, &missing_style,
                                  &missing_bitmap_layout) == nullptr,
            "NaN bitmap dimensions are rejected");
    require(is_zero_layout(missing_bitmap_layout),
            "NaN bitmap rejection returns a zero layout");
    g_synthetic_bitmap_width = std::numeric_limits<float>::infinity();
    require(gtxt_glyph_get_bitmap('B', 0.0f, &missing_style,
                                  &missing_bitmap_layout) == nullptr,
            "infinite bitmap dimensions are rejected");
    require(is_zero_layout(missing_bitmap_layout),
            "infinite bitmap rejection returns a zero layout");
    g_synthetic_bitmap_width = 1.0f;
    g_synthetic_bitmap_height = 1.0f;
    require(gtxt_glyph_get_bitmap('B', 0.0f, &missing_style,
                                  &missing_bitmap_layout) != nullptr,
            "valid bitmap dimensions recover after strict rejection");
    require(missing_bitmap_layout.sizer.width == 1.0f &&
            missing_bitmap_layout.sizer.height == 1.0f,
            "recovered bitmap publishes its exact integral dimensions");
    gtxt_glyph_release();

    g_hud_callback_hits = 0;
    gtxt_label_draw_n("A", 1, nullptr, &hud_label_callback, nullptr);
    require(g_hud_callback_hits == 0, "draw_n rejects a null style before callback");

    gtxt_label_style parser_style;
    std::memset(&parser_style, 0, sizeof(parser_style));
    parser_style.gs.font_size = 16;
    parser_style.width = 256;
    parser_style.height = 64;

    int ext_width = 7;
    int ext_height = 9;
    gtxt_richtext_ext_sym_cb_init(nullptr, nullptr, nullptr, nullptr, nullptr);
    gtxt_ext_sym_get_size(&g_ext_token, &ext_width, &ext_height);
    require(ext_width == 0 && ext_height == 0,
            "missing ext-symbol size callback returns zero dimensions");
    gtxt_ext_sym_get_size(&g_ext_token, nullptr, &ext_height);
    require(ext_height == 0, "ext-symbol size rejects a null output safely");
    gtxt_ext_sym_render(&g_ext_token, 0.0f, 0.0f, nullptr);
    require(!gtxt_ext_sym_query(&g_ext_token, 0.0f, 0.0f, 1.0f, 1.0f,
                                0, 0, nullptr),
            "missing ext-symbol query callback returns false");

    RichtextProbe probe;
    gtxt_richtext_parser("<file=missing>A</file>", &parser_style,
                         &richtext_probe_callback, &probe);
    require(probe.external_glyphs == 0,
            "file token without create callback never publishes external state");

    g_ext_create_hits = 0;
    g_ext_release_hits = 0;
    probe = {};
    gtxt_richtext_ext_sym_cb_init(&test_ext_create_null, &test_ext_release,
                                  nullptr, nullptr, nullptr);
    gtxt_richtext_parser("<file=null>A</file>", &parser_style,
                         &richtext_probe_callback, &probe);
    require(g_ext_create_hits == 1 && g_ext_release_hits == 0 &&
            probe.external_glyphs == 0,
            "null create result is not treated as a valid external symbol");

    g_ext_create_hits = 0;
    probe = {};
    gtxt_richtext_ext_sym_cb_init(&test_ext_create, nullptr,
                                  nullptr, nullptr, nullptr);
    gtxt_richtext_parser("<file=partial>A</file>", &parser_style,
                         &richtext_probe_callback, &probe);
    require(g_ext_create_hits == 0 && probe.external_glyphs == 0,
            "file token without release callback is rejected before create");

    g_ext_create_hits = 0;
    g_ext_release_hits = 0;
    probe = {};
    gtxt_richtext_ext_sym_cb_init(&test_ext_create, &test_ext_release,
                                  nullptr, nullptr, nullptr);
    gtxt_richtext_parser("<file=unclosed>A", &parser_style,
                         &richtext_probe_callback, &probe);
    require(g_ext_create_hits == 1 && g_ext_release_hits == 1 &&
            probe.external_glyphs == 1,
            "create/release pair works when optional ext callbacks are absent");

    g_ext_create_hits = 0;
    g_ext_release_hits = 0;
    g_ext_query_hits = 0;
    gtxt_richtext_ext_sym_cb_init(&test_ext_create, &test_ext_release,
                                  &test_ext_size, nullptr, &test_ext_query);
    void* query_hit = gtxt_label_point_query("<file=hit>X</file>",
                                             &parser_style, 0, 0, nullptr);
    require(query_hit == &g_ext_token && g_ext_query_hits == 1,
            "true external-symbol query terminates once and returns the object");
    require(g_ext_create_hits == 2 && g_ext_release_hits == 1,
            "query result transfers one live external-symbol reference");
    gtxt_ext_sym_release(query_hit);
    require(g_ext_release_hits == 2,
            "transferred query result has an explicit safe release path");

    probe = {};
    gtxt_richtext_parser("</font></size></color></edge>X", &parser_style,
                         &richtext_probe_callback, &probe);
    require(probe.external_glyphs == 0,
            "unmatched closing tags are rejected without corrupting parser state");
    gtxt_richtext_parser(nullptr, &parser_style, &richtext_probe_callback, &probe);
    gtxt_richtext_parser("A", nullptr, &richtext_probe_callback, &probe);
    gtxt_richtext_parser("A", &parser_style, nullptr, &probe);

    float label_width = 7.0f;
    float label_height = 9.0f;
    gtxt_get_label_size(nullptr, &parser_style, &label_width, &label_height);
    require(label_width == 0.0f && label_height == 0.0f,
            "label size rejects a null string and zeros outputs");
    label_width = 7.0f;
    label_height = 9.0f;
    gtxt_get_label_size("A", nullptr, &label_width, &label_height);
    require(label_width == 0.0f && label_height == 0.0f,
            "label size rejects a null style and zeros outputs");
    label_height = 9.0f;
    gtxt_get_label_size("A", &parser_style, nullptr, &label_height);
    require(label_height == 0.0f,
            "label size rejects a null width output without touching layout");
    gtxt_richtext_ext_sym_cb_init(nullptr, nullptr, nullptr, nullptr, nullptr);

    gtxt_ft_create();
    require(gtxt_ft_ready() != 0, "FreeType core is ready");
    gtxt_ft_create();
    require(gtxt_ft_ready() != 0, "repeated FreeType create keeps the ready instance");

    const char* font_path = pick_core_font();
    require(font_path != nullptr, "transaction tests need a font");
    require(gtxt_ft_get_font_cout() == 0, "font table starts empty");
    require(gtxt_richtext_get_font_slot_count() == 0, "richtext slot table starts empty");

    require(gtxt_ft_add_font("failed_first", "no_such_font.ttf") < 0,
            "failed first font load is observable");
    require(gtxt_ft_get_font_cout() == 0, "failed first font does not reserve FT slots");
    require(gtxt_richtext_get_font_slot_count() == 0,
            "failed first font does not reserve richtext slots");
    require(gtxt_richtext_find_font("default") < 0,
            "failed first font does not publish default");
    require(gtxt_richtext_find_font("failed_first") < 0,
            "failed first font does not publish its name");

    const int font = gtxt_ft_add_font("transaction_font", font_path);
    require(font == 1, "first non-default font atomically reserves default slot 0");
    require(gtxt_ft_get_font_cout() == 2, "FT count matches default plus loaded font");
    require(gtxt_richtext_get_font_slot_count() == 2,
            "richtext count matches default plus loaded font");
    require(gtxt_richtext_find_font("default") == 0, "default name maps to slot 0");
    require(gtxt_richtext_find_font("transaction_font") == font,
            "loaded name maps to returned FT slot");

    gtxt_glyph_create(4, 16, nullptr, &capture_user_font_layout);
    require(gtxt_glyph_ready() != 0, "user-font index test cache is ready");
    gtxt_glyph_style user_font_style;
    std::memset(&user_font_style, 0, sizeof(user_font_style));
    user_font_style.font_size = 16;
    const int ft_count = gtxt_ft_get_font_cout();
    user_font_style.font = ft_count;
    g_user_font_layout_index = -1;
    g_user_font_layout_unicode = -1;
    require(gtxt_glyph_get_layout('U', 0.0f, &user_font_style) != nullptr,
            "first user-font layout is cached");
    require(g_user_font_layout_index == 0 && g_user_font_layout_unicode == 'U',
            "first user-font slot is relative index zero");
    user_font_style.font = ft_count + 2;
    require(gtxt_glyph_get_layout('V', 0.0f, &user_font_style) != nullptr,
            "later user-font layout is cached");
    require(g_user_font_layout_index == 2 && g_user_font_layout_unicode == 'V',
            "later user-font slot uses style.font minus FreeType count");
    gtxt_glyph_release();

    gtxt_glyph_create(4, 16, nullptr, nullptr);
    require(gtxt_glyph_ready() != 0, "reserved-default test cache is ready");
    gtxt_glyph_style reserved_default_style;
    std::memset(&reserved_default_style, 0, sizeof(reserved_default_style));
    reserved_default_style.font = 0;
    reserved_default_style.font_size = 16;
    const gtxt_glyph_layout* reserved_layout =
        gtxt_glyph_get_layout('A', 0.0f, &reserved_default_style);
    require(reserved_layout != nullptr && is_zero_layout(*reserved_layout),
            "unbound reserved default returns a zero cached layout");
    gtxt_glyph_layout reserved_bitmap_layout;
    std::memset(&reserved_bitmap_layout, 0x7f, sizeof(reserved_bitmap_layout));
    require(gtxt_glyph_get_bitmap('A', 0.0f, &reserved_default_style,
                                  &reserved_bitmap_layout) == nullptr,
            "unbound reserved default bitmap lookup fails safely");
    require(is_zero_layout(reserved_bitmap_layout),
            "unbound reserved default bitmap failure returns zero layout");
    std::memset(&reserved_bitmap_layout, 0x7f, sizeof(reserved_bitmap_layout));
    gtxt_ft_get_layout('A', 0.0f, &reserved_default_style, &reserved_bitmap_layout);
    require(is_zero_layout(reserved_bitmap_layout),
            "direct FreeType layout on unbound default is zero");
    gtxt_glyph_release();

    require(gtxt_ft_add_font("default", "no_such_font.ttf") < 0,
            "failed reserved-default bind is observable");
    require(gtxt_ft_get_font_cout() == 2,
            "failed reserved-default bind does not change FT slot count");
    require(gtxt_richtext_find_font("default") == 0,
            "failed reserved-default bind keeps its existing mapping");
    require(gtxt_ft_add_font("default", font_path) == 0,
            "reserved default slot can be bound after a failed attempt");
    require(gtxt_ft_add_font("default", font_path) == 0,
            "same default name and path is idempotent");
    require(gtxt_ft_add_font("default", "no_such_font.ttf") < 0,
            "bound default cannot be replaced with another path");

    gtxt_glyph_style edge_style;
    std::memset(&edge_style, 0, sizeof(edge_style));
    edge_style.font = 0;
    edge_style.font_size = 64;
    edge_style.edge = true;
    edge_style.edge_size = 2.0f;
    gtxt_glyph_layout edge_layout;
    require(gtxt_ft_gen_char('W', 0.0f, &edge_style, &edge_layout) != nullptr,
            "normal outlined glyph generation succeeds");
    edge_style.edge_size = std::numeric_limits<float>::infinity();
    std::memset(&edge_layout, 0x7f, sizeof(edge_layout));
    require(gtxt_ft_gen_char('W', 0.0f, &edge_style, &edge_layout) == nullptr,
            "non-finite outline size is strictly rejected");
    require(is_zero_layout(edge_layout),
            "non-finite outline rejection publishes no partial layout");
    edge_style.edge_size = 5000.0f;
    for (int attempt = 0; attempt < 3; ++attempt)
    {
        std::memset(&edge_layout, 0x7f, sizeof(edge_layout));
        require(gtxt_ft_gen_char('W', 0.0f, &edge_style, &edge_layout) == nullptr,
                "oversized outline span batch is strictly rejected");
        require(is_zero_layout(edge_layout),
                "oversized outline rejection publishes no partial layout");
    }
    edge_style.edge_size = 2.0f;
    require(gtxt_ft_gen_char('W', 0.0f, &edge_style, &edge_layout) != nullptr,
            "normal outlined glyph recovers after repeated oversized failures");

    require(gtxt_ft_add_font("transaction_font", font_path) == font,
            "same font name and path is idempotent");
    require(gtxt_ft_get_font_cout() == 2, "idempotent add does not grow FT slots");
    require(gtxt_richtext_get_font_slot_count() == 2,
            "idempotent add does not grow richtext slots");
    require(gtxt_ft_add_font("transaction_font", "no_such_font.ttf") < 0,
            "same font name with a different path is rejected");
    require(gtxt_richtext_find_font("transaction_font") == font,
            "rejected duplicate keeps the original mapping");

    const char* duplicate_batch[] = {"batch_duplicate", "batch_duplicate"};
    require(gtxt_richtext_add_fonts(duplicate_batch, 2) < 0,
            "duplicate batch is rejected atomically");
    require(gtxt_richtext_find_font("batch_duplicate") < 0,
            "rejected batch publishes no partial name");
    require(gtxt_richtext_get_font_slot_count() == 2,
            "rejected batch publishes no partial slot");

    require(gtxt_richtext_add_font_alias("transaction_alias", font) == 0,
            "alias to a valid slot succeeds");
    require(gtxt_richtext_add_font_alias("transaction_alias", font) == 0,
            "same alias registration is idempotent");
    require(gtxt_richtext_add_font_alias("transaction_alias", 0) < 0,
            "same alias cannot be rebound");
    require(gtxt_richtext_add_font_alias("invalid_negative", -1) < 0,
            "negative alias slot is rejected");
    require(gtxt_richtext_add_font_alias("invalid_past_end",
                                         gtxt_richtext_get_font_slot_count()) < 0,
            "past-end alias slot is rejected");
    require(gtxt_richtext_find_font("invalid_negative") < 0,
            "invalid negative alias is not published");
    require(gtxt_richtext_find_font("invalid_past_end") < 0,
            "invalid past-end alias is not published");

    bool alias_table_full = false;
    for (int i = 0; i < 64; ++i)
    {
        char alias[32];
        std::snprintf(alias, sizeof(alias), "transaction_fill_%d", i);
        if (gtxt_richtext_add_font_alias(alias, font) != 0)
        {
            require(gtxt_richtext_find_font(alias) < 0,
                    "failed capacity alias is not published");
            alias_table_full = true;
            break;
        }
    }
    require(alias_table_full, "transaction test fills the richtext name table");
    require(gtxt_ft_add_font("registration_should_fail", font_path) < 0,
            "font registration failure is observable after file load");
    require(gtxt_ft_get_font_cout() == 2,
            "font registration failure does not commit an FT slot");
    require(gtxt_richtext_get_font_slot_count() == 2,
            "font registration failure does not commit a richtext slot");
    require(gtxt_richtext_find_font("registration_should_fail") < 0,
            "font registration failure publishes no name");

    gtxt_glyph_create(4, 16, nullptr, &stub_user_font_layout);
    require(gtxt_glyph_ready() != 0, "glyph cache is ready for release-path coverage");

    gtxt_label_style style;
    std::memset(&style, 0, sizeof(style));
    style.width = 256;
    style.height = 64;
    style.space_h = 1.0f;
    style.space_v = 1.0f;
    style.gs.font = font;
    style.gs.font_size = 16;

    gtxt_glyph_layout bitmap_layout;
    std::memset(&bitmap_layout, 0, sizeof(bitmap_layout));
    require(gtxt_glyph_get_bitmap('A', 0.0f, &style.gs, &bitmap_layout) != nullptr,
            "used bitmap exists before cache release");
    gtxt_label_cb_init(nullptr);
    gtxt_label_draw_richtext("A", &style, 0, nullptr);

    gtxt_glyph_release();
    gtxt_glyph_release();
    gtxt_ft_release();
    gtxt_ft_release();
    gtxt_richtext_release();
    gtxt_richtext_release();
}

void test_callback_coexistence_and_embedded_nul()
{
    gtxt_label_style style;
    std::memset(&style, 0, sizeof(style));
    style.width = 256;
    style.height = 64;
    style.space_h = 1.0f;
    style.space_v = 1.0f;
    style.gs.font_size = 16;

    gtxt_label_cb_init(&process_label_callback);
    require(gtxt_label_cb_get() == &process_label_callback, "process callback installed");

    const char* font_path = pick_core_font();
    require(font_path != nullptr, "core tests need a font to count callbacks");
    require(tt::GtxtCore::Ensure(), "GtxtCore::Ensure");
    const int font = tt::GtxtCore::AddFont("voxel_hud", font_path);
    require(font > 0, "HUD-first font must not occupy slot 0");
    require(gtxt_ft_get_font_cout() >= 2, "slot 0 reserved for default");
    require(gtxt_richtext_find_font("default") == 0, "default richtext name is slot 0");
    require(gtxt_richtext_find_font("voxel_hud") == font, "voxel_hud richtext index");
    require(tt::GtxtCore::AddFont("voxel_hud", font_path) == font, "font name is deduped");
    require(tt::GtxtCore::AddFont("voxel_hud", "no_such_font.ttf") < 0,
            "same name different path is rejected");
    require(tt::GtxtCore::AddFont("voxel_hud_alias", font_path) == font, "font path is aliased");
    require(gtxt_richtext_find_font("voxel_hud_alias") == font, "alias is registered for <font=>");

    const char* second_font = nullptr;
    static const char* kFonts[] = { "assets/fonts/hkljhw8.ttf", "assets/fonts/default2.ttf" };
    for (const char* path : kFonts)
    {
        if (std::strcmp(path, font_path) == 0)
        {
            continue;
        }
        std::ifstream stream(path, std::ios::binary);
        if (stream)
        {
            second_font = path;
            break;
        }
    }
    if (second_font)
    {
        require(tt::GtxtCore::AddFont("path_conflict", font_path) >= 0, "first real font under shared name");
        require(tt::GtxtCore::AddFont("path_conflict", second_font) < 0,
                "GtxtCore rejects same name with a second real font file");
    }

    std::string too_long(128, 'z');
    require(tt::GtxtCore::AddFont(too_long.c_str(), font_path) < 0, "overlong font name is rejected");
    require(gtxt_richtext_find_font(too_long.c_str()) < 0, "overlong name is not parsed");

    bool alias_table_full = false;
    for (int i = 0; i < 64; ++i)
    {
        char alias[32];
        std::snprintf(alias, sizeof(alias), "cap_alias_%d", i);
        const int idx = tt::GtxtCore::AddFont(alias, font_path);
        if (idx < 0)
        {
            require(gtxt_richtext_find_font(alias) < 0, "failed alias is not parsed");
            alias_table_full = true;
            break;
        }
        require(idx == font, "alias reuses HUD font index");
        require(gtxt_richtext_find_font(alias) == font, "accepted alias is parsed");
    }
    require(alias_table_full, "alias table fails observably when full");

    style.gs.font = font;

    const char embedded[] = {'A', '\0', 'B'};
    g_process_callback_hits = 0;
    g_hud_callback_hits = 0;
    gtxt_label_draw_n(embedded, 3, &style, &hud_label_callback, nullptr);
    require(gtxt_label_cb_get() == &process_label_callback,
            "per-call HUD callback must not replace the process callback");
    require(g_process_callback_hits == 0, "process callback stays idle during HUD draw_n");
    require(g_hud_callback_hits == 3, "A\\0B must invoke the HUD callback three times");
}

void test_wrong_thread_shutdown_keeps_hud_usable()
{
    tt::voxel_game::GtxtHud hud;
    require_status(hud.init_cpu_for_tests(), tt::voxel_game::HudStatus::Ok, "cpu init");
    require(hud.ready(), "ready after cpu init");

    tt::voxel_game::HudStatus remote = tt::voxel_game::HudStatus::Ok;
    std::thread worker([&]()
    {
        remote = hud.shutdown();
    });
    worker.join();
    require_status(remote, tt::voxel_game::HudStatus::WrongThread, "wrong-thread shutdown");
    require(hud.ready(), "HUD remains usable after wrong-thread shutdown");
    require_status(hud.shutdown(), tt::voxel_game::HudStatus::Ok, "owner shutdown");
    require(!hud.ready(), "not ready after owner shutdown");
}

void test_whitespace_does_not_require_atlas()
{
    tt::voxel_game::HudStyle style;
    style.font_size_px = 16;
    std::vector<char32_t> text = {U' ', U'\n', U'\t'};
    std::vector<tt::voxel_game::HudLaidGlyph> glyphs;
    require_status(tt::voxel_game::HudLayout::layout(text, style, {}, glyphs),
                   tt::voxel_game::HudStatus::Ok, "layout whitespace");
    FakeGlyphCache cache;
    tt::voxel_game::HudBatch batch;
    require_status(tt::voxel_game::compose_hud_batch(glyphs, style, cache, batch),
                   tt::voxel_game::HudStatus::Ok, "whitespace without atlas");
    require(batch.empty(), "whitespace emits no quads");
    require(cache.m_loaded.empty(), "whitespace does not request glyphs");
}

} // namespace

int main()
{
    try
    {
        test_utf8_accepts_well_formed();
        test_utf8_rejects_illegal();
        test_glyph_key_keeps_alpha();
        test_layout_letter_spacing();
        test_layout_and_origin();
        test_batch_capacity();
        test_compose_capacity_is_atomic();
        test_same_frame_upload_failure_and_missing();
        test_gtxt_transactional_lifecycle();
        test_callback_coexistence_and_embedded_nul();
        test_wrong_thread_shutdown_keeps_hud_usable();
        test_whitespace_does_not_require_atlas();
        std::cout << "hud_core_tests: PASS" << std::endl;
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << "hud_core_tests: FAIL: " << error.what() << std::endl;
        return 1;
    }
}
