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
    tess::Painter* pt = (tess::Painter*)vessel_set_slot_new_foreign(0, 0, sizeof(tess::Painter));
    *pt = tess::Painter();
}

static void add_rect_filled()
{
    tess::Painter* pt = (tess::Painter*)vessel_get_slot_foreign(0);

    float x, y, w, h;
    assert(vessel_get_list_count(1) == 4);
    vessel_get_list_element(1, 0, 0);
    x = vessel_get_slot_double(0);
    vessel_get_list_element(1, 1, 0);
    y = vessel_get_slot_double(0);
    vessel_get_list_element(1, 2, 0);
    w = vessel_get_slot_double(0);
    vessel_get_list_element(1, 3, 0);
    h = vessel_get_slot_double(0);

    int r, g, b, a;
    const int col_n = vessel_get_list_count(2);
    assert(col_n == 3 || col_n == 4);
    vessel_get_list_element(2, 0, 0);
    r = static_cast<int>(vessel_get_slot_double(0));
    vessel_get_list_element(2, 1, 0);
    g = static_cast<int>(vessel_get_slot_double(0));
    vessel_get_list_element(2, 2, 0);
    b = static_cast<int>(vessel_get_slot_double(0));
    if (col_n == 4) {
        vessel_get_list_element(2, 3, 0);
        a = static_cast<int>(vessel_get_slot_double(0));
    } else {
        a = 255;
    }

    const uint32_t col = r << 24 | g << 16 | b << 8 | a;
    pt->AddRectFilled(sm::vec2(x, y), sm::vec2(x + w, y + h), col);
}

static void draw_painter()
{
    if (!SPR_RD) {
        SPR_RD = std::make_shared<tt::SpriteRenderer>();
    }

    tess::Painter* pt = (tess::Painter*)vessel_get_slot_foreign(1);
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