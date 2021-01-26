#include "modules/graphics/Graphics.h"
#include "modules/graphics/SpriteRenderer.h"
#include "modules/render/Render.h"

#include <unirender/RenderState.h>
#include <unirender/Factory.h>
#include <tessellation/Painter.h>

#include <string.h>

namespace
{

std::shared_ptr<tt::SpriteRenderer> SPR_RD = nullptr;

static void painter_allocate()
{
    tess::Painter* pt = (tess::Painter*)ves_set_newforeign(0, 0, sizeof(tess::Painter));
    *pt = tess::Painter();
}

static void add_rect_filled()
{
    tess::Painter* pt = (tess::Painter*)ves_toforeign(0);

    float x, y, w, h;
    assert(ves_len(1) == 4);
    ves_geti(1, 0);
    x = ves_tonumber(-1);
    ves_geti(1, 1);
    y = ves_tonumber(-1);
    ves_geti(1, 2);
    w = ves_tonumber(-1);
    ves_geti(1, 3);
    h = ves_tonumber(-1);
    ves_pop(4);

    int r, g, b, a;
    const int col_n = ves_len(2);
    assert(col_n == 3 || col_n == 4);
    ves_geti(2, 0);
    r = ves_tonumber(-1);
    ves_geti(2, 1);
    g = ves_tonumber(-1);
    ves_geti(2, 2);
    b = ves_tonumber(-1);
    if (col_n == 4) {
        ves_geti(2, 3);
        a = ves_tonumber(-1);
    } else {
        a = 255;
    }
    ves_pop(col_n);

    const uint32_t col = r << 24 | g << 16 | b << 8 | a;
    pt->AddRectFilled(sm::vec2(x, y), sm::vec2(x + w, y + h), col);
}

static void draw_painter()
{
    if (!SPR_RD) {
        SPR_RD = std::make_shared<tt::SpriteRenderer>();
    }

    tess::Painter* pt = (tess::Painter*)ves_toforeign(1);
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
    if (strcmp(signature, "Painter.addRectFilled(_,_)") == 0) return add_rect_filled;
    if (strcmp(signature, "static Graphics.drawPainter(_)") == 0) return draw_painter;
    if (strcmp(signature, "static Graphics.flush()") == 0) { return flush; }

    return NULL;
}

void GraphicsBindClass(const char* className, VesselForeignClassMethods* methods)
{
    if (strcmp(className, "Painter") == 0)
    {
        methods->allocate = painter_allocate;
        return;
    }
}

}