#include "modules/graphics/wrap_Graphics.h"
#include "modules/graphics/Graphics.h"
#include "modules/graphics/GTxt.h"
#include "modules/graphics/DTex.h"
#include "modules/graphics/SpriteRenderer.h"
#include "modules/graphics/Viewport.h"
#include "modules/script/TransHelper.h"
#include "modules/script/Proxy.h"
#include "modules/render/Render.h"

#include <tessellation/Painter.h>
#include <geoshape/Bezier.h>
#include <unirender/Texture.h>
#include <unirender/Device.h>
#include <unirender/Factory.h>
#include <guard/check.h>
#include <SM_Calc.h>

namespace
{

void w_Viewport_allocate()
{
    auto proxy = (tt::Proxy<tt::Viewport>*)ves_set_newforeign(0, 0, sizeof(tt::Proxy<tt::Viewport>));
    proxy->obj = std::make_shared<tt::Viewport>();
}

int w_Viewport_finalize(void* data)
{
    auto proxy = (tt::Proxy<tt::Viewport>*)(data);
    proxy->~Proxy();
    return sizeof(tt::Proxy<tt::Viewport>);
}

void w_Viewport_set_size()
{
    auto vp = ((tt::Proxy<tt::Viewport>*)ves_toforeign(0))->obj;

    float w = (float)ves_tonumber(1);
    float h = (float)ves_tonumber(2);

    vp->SetSize(w, h);
}

void w_Painter_allocate()
{
    auto pt = std::make_shared<tess::Painter>();
    pt->SetPalette(tt::Graphics::Instance()->GetSpriteRenderer()->GetPalette());

    auto proxy = (tt::Proxy<tess::Painter>*)ves_set_newforeign(0, 0, sizeof(tt::Proxy<tess::Painter>));
    proxy->obj = pt;
}

int w_Painter_finalize(void* data)
{
    auto proxy = (tt::Proxy<tess::Painter>*)(data);
    proxy->~Proxy();
    return sizeof(tt::Proxy<tess::Painter>);
}

void w_Painter_add_line()
{
    auto pt = ((tt::Proxy<tess::Painter>*)ves_toforeign(0))->obj;

    float x0, y0, x1, y1;
    GD_ASSERT(ves_len(1) == 4, "error number");
    ves_geti(1, 0);
    x0 = (float)ves_tonumber(-1);
    ves_pop(1);
    ves_geti(1, 1);
    y0 = (float)ves_tonumber(-1);
    ves_pop(1);
    ves_geti(1, 2);
    x1 = (float)ves_tonumber(-1);
    ves_pop(1);
    ves_geti(1, 3);
    y1 = (float)ves_tonumber(-1);
    ves_pop(1);

    const uint32_t col = tt::list_to_abgr(2);
    const float width = (float)ves_tonumber(3);

    pt->AddLine(sm::vec2(x0, y0), sm::vec2(x1, y1), col, width);
}

void w_Painter_add_rect()
{
    auto pt = ((tt::Proxy<tess::Painter>*)ves_toforeign(0))->obj;

    float x, y, w, h;
    GD_ASSERT(ves_len(1) == 4, "error number");
    ves_geti(1, 0);
    x = (float)ves_tonumber(-1);
    ves_pop(1);
    ves_geti(1, 1);
    y = (float)ves_tonumber(-1);
    ves_pop(1);
    ves_geti(1, 2);
    w = (float)ves_tonumber(-1);
    ves_pop(1);
    ves_geti(1, 3);
    h = (float)ves_tonumber(-1);
    ves_pop(1);

    if (w <= 0 || h <= 0) {
        return;
    }

    const uint32_t col = tt::list_to_abgr(2);
    const float width = (float)ves_tonumber(3);

    pt->AddRect(sm::vec2(x, y), sm::vec2(x + w, y + h), col, width);
}

void w_Painter_add_rect_filled()
{
    auto pt = ((tt::Proxy<tess::Painter>*)ves_toforeign(0))->obj;

    float x, y, w, h;
    GD_ASSERT(ves_len(1) == 4, "error number");
    ves_geti(1, 0);
    x = (float)ves_tonumber(-1);
    ves_pop(1);
    ves_geti(1, 1);
    y = (float)ves_tonumber(-1);
    ves_pop(1);
    ves_geti(1, 2);
    w = (float)ves_tonumber(-1);
    ves_pop(1);
    ves_geti(1, 3);
    h = (float)ves_tonumber(-1);
    ves_pop(1);

    if (w <= 0 || h <= 0) {
        return;
    }

    const uint32_t col = tt::list_to_abgr(2);

    pt->AddRectFilled(sm::vec2(x, y), sm::vec2(x + w, y + h), col);
}

void w_Painter_add_capsule_filled()
{
    auto pt = ((tt::Proxy<tess::Painter>*)ves_toforeign(0))->obj;

    auto rect = tt::list_to_float_array(1);
    GD_ASSERT(rect.size() == 4, "error number");
    if (rect[2] <= 0 || rect[3] <= 0) {
        return;
    }

    const uint32_t col = tt::list_to_abgr(2);

    auto hori = ves_toboolean(3);

    float rounding = (hori ? rect[3] : rect[2]) * 0.49f;
    uint32_t rounding_corners_flags;
    if (hori) {
        rounding_corners_flags = tess::CORNER_FLAGS_LEFT | tess::CORNER_FLAGS_RIGHT;
    } else {
        rounding_corners_flags = tess::CORNER_FLAGS_TOP | tess::CORNER_FLAGS_BOT;
    }

    pt->AddRectFilled(sm::vec2(rect[0], rect[1]), sm::vec2(rect[0] + rect[2], rect[1] + rect[3]), col, rounding, rounding_corners_flags);
}

void w_Painter_add_polygon()
{
    auto pt = ((tt::Proxy<tess::Painter>*)ves_toforeign(0))->obj;

    auto vertices = tt::list_to_vec2_array(1);
    uint32_t col = tt::list_to_abgr(2);
    const float width = (float)ves_tonumber(3);

    pt->AddPolygon(vertices.data(), vertices.size(), col, width);
}

void w_Painter_add_polygon_filled()
{
    auto pt = ((tt::Proxy<tess::Painter>*)ves_toforeign(0))->obj;

    auto vertices = tt::list_to_vec2_array(1);
    uint32_t col = tt::list_to_abgr(2);

    pt->AddPolygonFilled(vertices.data(), vertices.size(), col);
}

void w_Painter_add_polyline()
{
    auto pt = ((tt::Proxy<tess::Painter>*)ves_toforeign(0))->obj;

    auto vertices = tt::list_to_vec2_array(1);
    uint32_t col = tt::list_to_abgr(2);
    const float width = (float)ves_tonumber(3);

    pt->AddPolyline(vertices.data(), vertices.size(), col, width);
}

void w_Painter_add_circle()
{
    auto pt = ((tt::Proxy<tess::Painter>*)ves_toforeign(0))->obj;

    const float x = (float)ves_tonumber(1);
    const float y = (float)ves_tonumber(2);
    const float r = (float)ves_tonumber(3);
    uint32_t col = tt::list_to_abgr(4);
    const float width = (float)ves_tonumber(5);
    const uint32_t seg = (uint32_t)ves_tonumber(6);

    pt->AddCircle(sm::vec2(x, y), r, col, width, seg);
}

void w_Painter_add_circle_filled()
{
    auto pt = ((tt::Proxy<tess::Painter>*)ves_toforeign(0))->obj;

    const float x = (float)ves_tonumber(1);
    const float y = (float)ves_tonumber(2);
    const float r = (float)ves_tonumber(3);
    uint32_t col = tt::list_to_abgr(4);
    const uint32_t seg = (uint32_t)ves_tonumber(5);

    pt->AddCircleFilled(sm::vec2(x, y), r, col, seg);
}

void w_Painter_add_bezier()
{
    auto pt = ((tt::Proxy<tess::Painter>*)ves_toforeign(0))->obj;
    auto bezier = ((tt::Proxy<gs::Bezier>*)ves_toforeign(1))->obj;
    uint32_t col = tt::list_to_abgr(2);
    const float width = (float)ves_tonumber(3);

    auto& vertices = bezier->GetVertices();
    pt->AddPolyline(vertices.data(), vertices.size(), col, width);
}

sm::mat4* CAM_MAT = nullptr;

auto trans3d = [&](const sm::vec3& pos3)->sm::vec2 
{
    auto vp = tt::Graphics::Instance()->GetViewport();
    if (CAM_MAT) {
        return vp->TransPosProj3ToProj2(pos3, *CAM_MAT);
    } else {
        return vp->TransPosProj3ToProj2(pos3, sm::mat4());
    }
};

void w_Painter_add_point3d()
{
    auto pt = ((tt::Proxy<tess::Painter>*)ves_toforeign(0))->obj;
    CAM_MAT = (sm::mat4*)ves_toforeign(1);

    auto p = tt::list_to_vec3(2);

    uint32_t col = tt::list_to_abgr(3);

    auto size = ves_tonumber(4);

    pt->AddPoint3D(p, trans3d, col, size);
}

void w_Painter_add_line3d()
{
    auto pt = ((tt::Proxy<tess::Painter>*)ves_toforeign(0))->obj;
    CAM_MAT = (sm::mat4*)ves_toforeign(1);

    auto p0 = tt::list_to_vec3(2);
    auto p1 = tt::list_to_vec3(3);

    uint32_t col = tt::list_to_abgr(4);
    const float width = (float)ves_tonumber(5);

    pt->AddLine3D(p0, p1, trans3d, col, width);
}

void w_Painter_add_cube()
{
    auto pt = ((tt::Proxy<tess::Painter>*)ves_toforeign(0))->obj;
    CAM_MAT = (sm::mat4*)ves_toforeign(1);

    auto min = tt::list_to_vec3(2);
    auto max = tt::list_to_vec3(3);

    uint32_t col = tt::list_to_abgr(4);
    const float width = (float)ves_tonumber(5);

    pt->AddCube(sm::cube(min, max), trans3d, col, width);
}

void w_Painter_add_polyline3d()
{
    auto pt = ((tt::Proxy<tess::Painter>*)ves_toforeign(0))->obj;
    CAM_MAT = (sm::mat4*)ves_toforeign(1);

    auto vertices = tt::list_to_vec3_array(2);
    uint32_t col = tt::list_to_abgr(3);
    const float width = (float)ves_tonumber(4);

    pt->AddPolyline3D(vertices.data(), vertices.size(), trans3d, col, width);
}

void w_Painter_add_polygon3d_filled()
{
    auto pt = ((tt::Proxy<tess::Painter>*)ves_toforeign(0))->obj;
    CAM_MAT = (sm::mat4*)ves_toforeign(1);

    auto vertices = tt::list_to_vec3_array(2);
    uint32_t col = tt::list_to_abgr(3);

    pt->AddPolygonFilled3D(vertices.data(), vertices.size(), trans3d, col);
}

void w_Graphics_on_size()
{
    float w = (float)ves_tonumber(1);
    float h = (float)ves_tonumber(2);
    tt::Graphics::Instance()->OnSize(w, h);
}

void w_Graphics_on_cam_update()
{
    float dx = (float)ves_tonumber(1);
    float dy = (float)ves_tonumber(2);
    float scale = (float)ves_tonumber(3);
    tt::Graphics::Instance()->OnCameraUpdate(sm::vec2(-dx, -dy), 1.0f / scale);
}

void w_Graphics_draw_painter()
{
    auto pt = ((tt::Proxy<tess::Painter>*)ves_toforeign(1))->obj;
    tt::Graphics::Instance()->DrawPainter(*pt);
}

void w_Graphics_draw_text()
{
    const char* text = ves_tostring(1);

    const float x = (float)ves_tonumber(2);
    const float y = (float)ves_tonumber(3);
    const float scale = (float)ves_tonumber(4);

    sm::Matrix2D mt;
    mt.Scale(scale, scale);
    mt.Translate(x, y);

    gtxt_label_style st;
    tt::GTxt::InitLabelStype(st);

    if (ves_getfield(5, "width") == VES_TYPE_NUM) {
        st.width = (int)ves_tonumber(-1);
    }
    ves_pop(1);
    if (ves_getfield(5, "height") == VES_TYPE_NUM) {
        st.height = (int)ves_tonumber(-1);
    }
    ves_pop(1);
    if (ves_getfield(5, "align_hori") == VES_TYPE_NUM) {
        st.align_h = (int)ves_tonumber(-1);
    }
    ves_pop(1);
    if (ves_getfield(5, "align_vert") == VES_TYPE_NUM) {
        st.align_v = (int)ves_tonumber(-1);
    }
    ves_pop(1);
    if (ves_getfield(5, "font_type") == VES_TYPE_NUM) {
        st.gs.font = (int)ves_tonumber(-1);
    }
    ves_pop(1);
    if (ves_getfield(5, "font_color") == VES_TYPE_LIST) {
        st.gs.font_color.mode_type = 0;
        st.gs.font_color.mode.ONE.color.integer = tt::list_to_rgba(-1);
    }
    ves_pop(1);
    if (ves_getfield(5, "font_size") == VES_TYPE_NUM) {
        st.gs.font_size = (int)ves_tonumber(-1);
    }
    ves_pop(1);

    tt::Graphics::Instance()->DrawText(text, mt, st);
}

bool calc_vertices(const sm::rect& pos, const sm::Matrix2D& mat, float* vertices)
{
    float xmin = FLT_MAX, ymin = FLT_MAX,
        xmax = -FLT_MAX, ymax = -FLT_MAX;

    const float* mt = mat.x;

    // Vertices are ordered for use with triangle strips:
    // 3----2
    // |  / |
    // | /  |
    // 0----1

    float x, y;

    float* ptr_dst = &vertices[0];

    x = (pos.xmin * mt[0] + pos.ymin * mt[2]) + mt[4];
    y = (pos.xmin * mt[1] + pos.ymin * mt[3]) + mt[5];
    if (x < xmin) xmin = x;
    if (x > xmax) xmax = x;
    if (y < ymin) ymin = y;
    if (y > ymax) ymax = y;
    *ptr_dst++ = x;
    *ptr_dst++ = y;

    x = (pos.xmax * mt[0] + pos.ymin * mt[2]) + mt[4];
    y = (pos.xmax * mt[1] + pos.ymin * mt[3]) + mt[5];
    if (x < xmin) xmin = x;
    if (x > xmax) xmax = x;
    if (y < ymin) ymin = y;
    if (y > ymax) ymax = y;
    *ptr_dst++ = x;
    *ptr_dst++ = y;

    x = (pos.xmax * mt[0] + pos.ymax * mt[2]) + mt[4];
    y = (pos.xmax * mt[1] + pos.ymax * mt[3]) + mt[5];
    if (x < xmin) xmin = x;
    if (x > xmax) xmax = x;
    if (y < ymin) ymin = y;
    if (y > ymax) ymax = y;
    *ptr_dst++ = x;
    *ptr_dst++ = y;

    x = (pos.xmin * mt[0] + pos.ymax * mt[2]) + mt[4];
    y = (pos.xmin * mt[1] + pos.ymax * mt[3]) + mt[5];
    if (x < xmin) xmin = x;
    if (x > xmax) xmax = x;
    if (y < ymin) ymin = y;
    if (y > ymax) ymax = y;
    *ptr_dst++ = x;
    *ptr_dst++ = y;

    //if (rp.IsViewRegionValid()) {
    //	const sm::rect& vr = rp.GetViewRegion();
    //	if (xmax <= vr.xmin || xmin >= vr.xmax ||
    //		ymax <= vr.ymin || ymin >= vr.ymax) {
    //		return false;
    //	}
    //}

    return true;
}

void draw_texture(const ur::TexturePtr& tex, const sm::Matrix2D& mat)
{
    float w = static_cast<float>(tex->GetWidth());
    float h = static_cast<float>(tex->GetHeight());
    float vertices[8];
    calc_vertices(sm::rect(w, h), mat, vertices);

	auto draw_without_dtex = [&](std::shared_ptr<tt::SpriteRenderer>& rd, const float* vertices, const ur::TexturePtr& tex)
	{
		float txmin, txmax, tymin, tymax;
		txmin = tymin = 0;
		txmax = tymax = 1;
		float texcoords[8] = {
			txmin, tymin,
			txmax, tymin,
			txmax, tymax,
			txmin, tymax,
		};
        auto ctx = tt::Render::Instance()->Context();
        auto rs = ur::DefaultRenderState2D();
		rd->DrawQuad(*ctx, rs, vertices, texcoords, tex, 0xffffffff);
	};

    auto rd = tt::Graphics::Instance()->GetSpriteRenderer();
	//if (use_dtex)
	//{
	//	sm::irect qr(0, 0, tex_w, tex_h);
 //       ur::TexturePtr cached_tex = nullptr;
	//	auto cached_texcoords = Callback::QueryCachedTexQuad(tex->GetTexID(), qr, cached_tex);
	//	if (cached_texcoords) {
	//		std::static_pointer_cast<rp::SpriteRenderer>(rd)->DrawQuad(ctx, rs, vertices, cached_texcoords, cached_tex, 0xffffffff);
	//	} else {
	//		draw_without_dtex(rd, vertices, tex);
	//		Callback::AddCacheSymbol(tex, qr);
	//	}
	//}
	//else
	{
		draw_without_dtex(rd, vertices, tex);
	}
}

void w_Graphics_draw_texture()
{
    auto tex = ((tt::Proxy<ur::Texture>*)ves_toforeign(1))->obj;

    auto  pos    = tt::list_to_vec2(2);
    float angle  = (float)ves_tonumber(3);
    auto  scale  = tt::list_to_vec2(4);
    auto  offset = tt::list_to_vec2(5);

    sm::Matrix2D mat;
    auto center = pos + sm::rotate_vector(-offset, angle) + offset;
    mat.SetTransformation(center.x, center.y, angle, scale.x, scale.y, 0, 0, 0, 0);

    draw_texture(tex, mat);
}

void w_Graphics_draw_texture2()
{
    auto tex = ((tt::Proxy<ur::Texture>*)ves_toforeign(1))->obj;
    sm::Matrix2D* mat = (sm::Matrix2D*)ves_toforeign(2);
    if (mat) {
        draw_texture(tex, *mat);
    } else {
        draw_texture(tex, sm::Matrix2D());
    }
}

void w_Graphics_flush()
{
    tt::Graphics::Instance()->Flush();
}

void w_Graphics_dtex_debug_draw()
{
    tt::DTex::Instance()->DebugDraw(*tt::Render::Instance()->Context());
}

void w_Graphics_get_width()
{
    ves_set_number(0, (double)tt::Graphics::Instance()->GetWidth());
}

void w_Graphics_get_height()
{
    ves_set_number(0, (double)tt::Graphics::Instance()->GetHeight());
}

}

namespace tt
{

VesselForeignMethodFn GraphicsBindMethod(const char* signature)
{
    if (strcmp(signature, "Viewport.set_size(_,_)") == 0) return w_Viewport_set_size;

    // 2D
    if (strcmp(signature, "Painter.add_line(_,_,_)") == 0) return w_Painter_add_line;
    if (strcmp(signature, "Painter.add_rect(_,_,_)") == 0) return w_Painter_add_rect;
    if (strcmp(signature, "Painter.add_rect_filled(_,_)") == 0) return w_Painter_add_rect_filled;
    if (strcmp(signature, "Painter.add_capsule_filled(_,_,_)") == 0) return w_Painter_add_capsule_filled;
    if (strcmp(signature, "Painter.add_polygon(_,_,_)") == 0) return w_Painter_add_polygon;
    if (strcmp(signature, "Painter.add_polygon_filled(_,_)") == 0) return w_Painter_add_polygon_filled;
    if (strcmp(signature, "Painter.add_polyline(_,_,_)") == 0) return w_Painter_add_polyline;
    if (strcmp(signature, "Painter.add_circle(_,_,_,_,_,_)") == 0) return w_Painter_add_circle;
    if (strcmp(signature, "Painter.add_circle_filled(_,_,_,_,_)") == 0) return w_Painter_add_circle_filled;
    if (strcmp(signature, "Painter.add_bezier(_,_,_)") == 0) return w_Painter_add_bezier;
    // 3D
    if (strcmp(signature, "Painter.add_point3d(_,_,_,_)") == 0) return w_Painter_add_point3d;
    if (strcmp(signature, "Painter.add_line3d(_,_,_,_,_)") == 0) return w_Painter_add_line3d;
    if (strcmp(signature, "Painter.add_cube(_,_,_,_,_)") == 0) return w_Painter_add_cube;
    if (strcmp(signature, "Painter.add_polyline3d(_,_,_,_)") == 0) return w_Painter_add_polyline3d;
    if (strcmp(signature, "Painter.add_polygon3d_filled(_,_,_)") == 0) return w_Painter_add_polygon3d_filled;

    if (strcmp(signature, "static Graphics.on_size(_,_)") == 0) return w_Graphics_on_size;
    if (strcmp(signature, "static Graphics.on_cam_update(_,_,_)") == 0) return w_Graphics_on_cam_update;
    if (strcmp(signature, "static Graphics.draw_painter(_)") == 0) return w_Graphics_draw_painter;
    if (strcmp(signature, "static Graphics.draw_text(_,_,_,_,_)") == 0) return w_Graphics_draw_text;
    if (strcmp(signature, "static Graphics.draw_texture(_,_,_,_,_)") == 0) return w_Graphics_draw_texture;
    if (strcmp(signature, "static Graphics.draw_texture2(_,_)") == 0) return w_Graphics_draw_texture2;
    if (strcmp(signature, "static Graphics.flush()") == 0) return w_Graphics_flush;
    if (strcmp(signature, "static Graphics.dtex_debug_draw()") == 0) return w_Graphics_dtex_debug_draw;
    if (strcmp(signature, "static Graphics.get_width()") == 0) return w_Graphics_get_width;
    if (strcmp(signature, "static Graphics.get_height()") == 0) return w_Graphics_get_height;

    return nullptr;
}

void GraphicsBindClass(const char* class_name, VesselForeignClassMethods* methods)
{
    if (strcmp(class_name, "Viewport") == 0)
    {
        methods->allocate = w_Viewport_allocate;
        methods->finalize = w_Viewport_finalize;
        return;
    }

    if (strcmp(class_name, "Painter") == 0)
    {
        methods->allocate = w_Painter_allocate;
        methods->finalize = w_Painter_finalize;
        return;
    }
}

}