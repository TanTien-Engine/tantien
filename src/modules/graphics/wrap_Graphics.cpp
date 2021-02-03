#include "modules/graphics/wrap_Graphics.h"
#include "modules/graphics/Graphics.h"
#include "modules/script/TransHelper.h"

#include <tessellation/Painter.h>

namespace
{

void painter_allocate()
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

void painter_add_rect()
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

    const uint32_t col = tt::list_to_color(2);
    const float width = (float)ves_tonumber(3);

    pt->AddRect(sm::vec2(x, y), sm::vec2(x + w, y + h), col, width);
}

void painter_add_rect_filled()
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

    const uint32_t col = tt::list_to_color(2);

    pt->AddRectFilled(sm::vec2(x, y), sm::vec2(x + w, y + h), col);
}

void painter_add_polygon()
{
    tess::Painter* pt = *(tess::Painter**)ves_toforeign(0);

    auto vertices = tt::list_to_vec2_array(1);
    uint32_t col = tt::list_to_color(2);
    const float width = (float)ves_tonumber(3);

    pt->AddPolygon(vertices.data(), vertices.size(), col, width);
}

void graphics_on_size()
{
    float w = (float)ves_tonumber(1);
    float h = (float)ves_tonumber(2);
    tt::Graphics::Instance()->OnSize(w, h);
}

void graphics_on_cam_update()
{
    float dx = (float)ves_tonumber(1);
    float dy = (float)ves_tonumber(2);
    float scale = (float)ves_tonumber(3);
    tt::Graphics::Instance()->OnCameraUpdate(sm::vec2(-dx, -dy), 1.0f / scale);
}

void graphics_draw_painter()
{
    tess::Painter* pt = *(tess::Painter**)ves_toforeign(1);
    tt::Graphics::Instance()->DrawPainter(*pt);
}

void graphics_draw_text()
{
    const char* text = ves_tostring(1);
    tt::Graphics::Instance()->DrawText(text);
}

void graphics_flush()
{
    tt::Graphics::Instance()->Flush();
}

}

namespace tt
{

VesselForeignMethodFn GraphicsBindMethod(const char* signature)
{
    if (strcmp(signature, "Painter.addRect(_,_,_)") == 0) return painter_add_rect;
    if (strcmp(signature, "Painter.addRectFilled(_,_)") == 0) return painter_add_rect_filled;
    if (strcmp(signature, "Painter.addPolygon(_,_,_)") == 0) return painter_add_polygon;

    if (strcmp(signature, "static Graphics.onSize(_,_)") == 0) return graphics_on_size;
    if (strcmp(signature, "static Graphics.onCamUpdate(_,_,_)") == 0) return graphics_on_cam_update;
    if (strcmp(signature, "static Graphics.drawPainter(_)") == 0) return graphics_draw_painter;
    if (strcmp(signature, "static Graphics.drawText(_)") == 0) return graphics_draw_text;
    if (strcmp(signature, "static Graphics.flush()") == 0) { return graphics_flush; }

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