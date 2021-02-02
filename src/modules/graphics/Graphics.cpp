#include "modules/graphics/Graphics.h"
#include "modules/graphics/SpriteRenderer.h"
#include "modules/render/Render.h"

#include <unirender/Context.h>
#include <unirender/ShaderProgram.h>
#include <unirender/RenderState.h>
#include <unirender/Factory.h>
#include <tessellation/Painter.h>

#include <string.h>

namespace
{

std::shared_ptr<tt::SpriteRenderer> SPR_RD = nullptr;

static void painter_allocate()
{
    auto pt = new tess::Painter();
    tess::Painter** ptr = (tess::Painter**)ves_set_newforeign(0, 0, sizeof(pt));
    *ptr = pt;
}

static int painter_finalize(void* data)
{
    tess::Painter** ptr = static_cast<tess::Painter**>(data);
    int ret = sizeof(*ptr);
    delete *ptr;
    return ret;
}

static void add_rect()
{
    tess::Painter* pt = *(tess::Painter**)ves_toforeign(0);

    float x, y, w, h;
    assert(ves_len(1) == 4);
    ves_geti(1, 0);
    x = (float)ves_tonumber(-1);
    ves_geti(1, 1);
    y = (float)ves_tonumber(-1);
    ves_geti(1, 2);
    w = (float)ves_tonumber(-1);
    ves_geti(1, 3);
    h = (float)ves_tonumber(-1);
    ves_pop(4);

    if (w <= 0 || h <= 0) {
        return;
    }

    int r, g, b, a;
    const int col_n = ves_len(2);
    assert(col_n == 3 || col_n == 4);
    ves_geti(2, 0);
    r = (int)ves_tonumber(-1);
    ves_geti(2, 1);
    g = (int)ves_tonumber(-1);
    ves_geti(2, 2);
    b = (int)ves_tonumber(-1);
    if (col_n == 4) {
        ves_geti(2, 3);
        a = (int)ves_tonumber(-1);
    } else {
        a = 255;
    }
    ves_pop(col_n);

    const float width = (float)ves_tonumber(3);

    const uint32_t col = a << 24 | b << 16 | g << 8 | r;
    pt->AddRect(sm::vec2(x, y), sm::vec2(x + w, y + h), col, width);
}

static void add_rect_filled()
{
    tess::Painter* pt = *(tess::Painter**)ves_toforeign(0);

    float x, y, w, h;
    assert(ves_len(1) == 4);
    ves_geti(1, 0);
    x = (float)ves_tonumber(-1);
    ves_geti(1, 1);
    y = (float)ves_tonumber(-1);
    ves_geti(1, 2);
    w = (float)ves_tonumber(-1);
    ves_geti(1, 3);
    h = (float)ves_tonumber(-1);
    ves_pop(4);

    if (w <= 0 || h <= 0) {
        return;
    }

    int r, g, b, a;
    const int col_n = ves_len(2);
    assert(col_n == 3 || col_n == 4);
    ves_geti(2, 0);
    r = (int)ves_tonumber(-1);
    ves_geti(2, 1);
    g = (int)ves_tonumber(-1);
    ves_geti(2, 2);
    b = (int)ves_tonumber(-1);
    if (col_n == 4) {
        ves_geti(2, 3);
        a = (int)ves_tonumber(-1);
    } else {
        a = 255;
    }
    ves_pop(col_n);

    const uint32_t col = a << 24 | b << 16 | g << 8 | r;
    pt->AddRectFilled(sm::vec2(x, y), sm::vec2(x + w, y + h), col);
}

static void add_polygon()
{
    tess::Painter* pt = *(tess::Painter**)ves_toforeign(0);

    const int num = ves_len(1);
    std::vector<sm::vec2> vertices;
    vertices.reserve(num / 2);
    for (int i = 0; i < num;)
    {
        ves_geti(1, i++);
        float x = (float)ves_tonumber(-1);
        ves_geti(1, i++);
        float y = (float)ves_tonumber(-1);
        vertices.emplace_back(x, y);
    }
    ves_pop(num);

    int r, g, b, a;
    const int col_n = ves_len(2);
    assert(col_n == 3 || col_n == 4);
    ves_geti(2, 0);
    r = (int)ves_tonumber(-1);
    ves_geti(2, 1);
    g = (int)ves_tonumber(-1);
    ves_geti(2, 2);
    b = (int)ves_tonumber(-1);
    if (col_n == 4) {
        ves_geti(2, 3);
        a = (int)ves_tonumber(-1);
    } else {
        a = 255;
    }
    ves_pop(col_n);

    const float width = (float)ves_tonumber(3);

    const uint32_t col = a << 24 | b << 16 | g << 8 | r;
    pt->AddPolygon(vertices.data(), vertices.size(), col, width);
}

static void on_size()
{
    float w = (float)ves_tonumber(1);
    float h = (float)ves_tonumber(2);

    auto ctx = tt::Render::Instance()->Context();
    ctx->SetViewport(0, 0, (int)w, (int)h);

    if (!SPR_RD) {
        SPR_RD = std::make_shared<tt::SpriteRenderer>();
    }

    SPR_RD->OnSize(w, h);
}

static void on_cam_update()
{
    float dx = (float)ves_tonumber(1);
    float dy = (float)ves_tonumber(2);
    float scale = (float)ves_tonumber(3);

    if (!SPR_RD) {
        SPR_RD = std::make_shared<tt::SpriteRenderer>();
    }

    SPR_RD->OnCameraUpdate(sm::vec2(-dx, -dy), 1.0f / scale);
}

static void draw_painter()
{
    if (!SPR_RD) {
        SPR_RD = std::make_shared<tt::SpriteRenderer>();
    }

    tess::Painter* pt = *(tess::Painter**)ves_toforeign(1);
    SPR_RD->DrawPainter(*tt::Render::Instance()->Context(), ur::DefaultRenderState2D(), *pt);
}

static void flush()
{
    if (!SPR_RD) {
        SPR_RD = std::make_shared<tt::SpriteRenderer>();
    }

    SPR_RD->Flush(*tt::Render::Instance()->Context());
}

}

namespace tt
{

VesselForeignMethodFn GraphicsBindMethod(const char* signature)
{
    if (strcmp(signature, "Painter.addRect(_,_,_)") == 0) return add_rect;
    if (strcmp(signature, "Painter.addRectFilled(_,_)") == 0) return add_rect_filled;
    if (strcmp(signature, "Painter.addPolygon(_,_,_)") == 0) return add_polygon;
    if (strcmp(signature, "static Graphics.onSize(_,_)") == 0) return on_size;
    if (strcmp(signature, "static Graphics.onCamUpdate(_,_,_)") == 0) return on_cam_update;
    if (strcmp(signature, "static Graphics.drawPainter(_)") == 0) return draw_painter;
    if (strcmp(signature, "static Graphics.flush()") == 0) { return flush; }

    return nullptr;
}

void GraphicsBindClass(const char* className, VesselForeignClassMethods* methods)
{
    if (strcmp(className, "Painter") == 0)
    {
        methods->allocate = painter_allocate;
        methods->finalize = painter_finalize;
        return;
    }
}

}