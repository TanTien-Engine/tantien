#include "modules/graphics/wrap_Graphics.h"
#include "modules/graphics/Graphics.h"
#include "modules/graphics/GTxt.h"
#include "modules/graphics/DTex.h"
#include "modules/graphics/SpriteRenderer.h"
#include "modules/script/TransHelper.h"
#include "modules/render/Render.h"
#include "modules/image/ImageData.h"

#include <tessellation/Painter.h>
#include <geoshape/Bezier.h>
#include <unirender/Texture.h>
#include <unirender/Device.h>
#include <unirender/Factory.h>
#include <gimg_typedef.h>

namespace
{

void w_Painter_allocate()
{
    auto pt = new tess::Painter();
    pt->SetPalette(tt::Graphics::Instance()->GetSpriteRenderer()->GetPalette());
    tess::Painter** ptr = (tess::Painter**)ves_set_newforeign(0, 0, sizeof(pt));
    *ptr = pt;
}

static int w_Painter_finalize(void* data)
{
    tess::Painter** ptr = static_cast<tess::Painter**>(data);
    int ret = sizeof(*ptr);
    delete *ptr;
    return ret;
}

void w_Painter_addRect()
{
    tess::Painter* pt = *(tess::Painter**)ves_toforeign(0);

    float x, y, w, h;
    assert(ves_len(1) == 4);
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

void w_Painter_addRectFilled()
{
    tess::Painter* pt = *(tess::Painter**)ves_toforeign(0);

    float x, y, w, h;
    assert(ves_len(1) == 4);
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

void w_Painter_addPolygon()
{
    tess::Painter* pt = *(tess::Painter**)ves_toforeign(0);

    auto vertices = tt::list_to_vec2_array(1);
    uint32_t col = tt::list_to_abgr(2);
    const float width = (float)ves_tonumber(3);

    pt->AddPolygon(vertices.data(), vertices.size(), col, width);
}

void w_Painter_addPolyline()
{
    tess::Painter* pt = *(tess::Painter**)ves_toforeign(0);

    auto vertices = tt::list_to_vec2_array(1);
    uint32_t col = tt::list_to_abgr(2);
    const float width = (float)ves_tonumber(3);

    pt->AddPolyline(vertices.data(), vertices.size(), col, width);
}

void w_Painter_addCircle()
{
    tess::Painter* pt = *(tess::Painter**)ves_toforeign(0);

    const float x = (float)ves_tonumber(1);
    const float y = (float)ves_tonumber(2);
    const float r = (float)ves_tonumber(3);
    uint32_t col = tt::list_to_abgr(4);

    pt->AddCircle(sm::vec2(x, y), r, col);
}

void w_Painter_addCircleFilled()
{
    tess::Painter* pt = *(tess::Painter**)ves_toforeign(0);

    const float x = (float)ves_tonumber(1);
    const float y = (float)ves_tonumber(2);
    const float r = (float)ves_tonumber(3);
    uint32_t col = tt::list_to_abgr(4);

    pt->AddCircleFilled(sm::vec2(x, y), r, col);
}

void w_Painter_addBezier()
{
    tess::Painter* pt = *(tess::Painter**)ves_toforeign(0);

    gs::Bezier* bezier = *(gs::Bezier**)ves_toforeign(1);
    uint32_t col = tt::list_to_abgr(2);
    const float width = (float)ves_tonumber(3);

    auto& vertices = bezier->GetVertices();
    pt->AddPolyline(vertices.data(), vertices.size(), col, width);
}

void w_Texture_allocate()
{
    if (ves_type(1) == VES_TYPE_FOREIGN)
    {
        tt::ImageData* img = (tt::ImageData*)ves_toforeign(1);

        ur::TextureFormat tf;
        size_t bpp = 0; // bytes per pixel
	    switch (img->format)
	    {
	    case GPF_ALPHA: case GPF_LUMINANCE: case GPF_LUMINANCE_ALPHA:
		    tf =  ur::TextureFormat::A8;
            bpp = 1;
		    break;
        case GPF_RED:
            tf =  ur::TextureFormat::RED;
            bpp = 1;
            break;
	    case GPF_RGB:
		    tf =  ur::TextureFormat::RGB;
            bpp = 3;
		    break;
        case GPF_RGB565:
            tf = ur::TextureFormat::RGB565;
            bpp = 2;
            break;
	    case GPF_RGBA8:
		    tf =  ur::TextureFormat::RGBA8;
            bpp = 4;
		    break;
	    case GPF_BGRA_EXT:
		    tf =  ur::TextureFormat::BGRA_EXT;
            bpp = 4;
		    break;
	    case GPF_BGR_EXT:
		    tf =  ur::TextureFormat::BGR_EXT;
            bpp = 3;
		    break;
        case GPF_RGBA16F:
            tf =  ur::TextureFormat::RGBA16F;
            bpp = 4 * 4;
            break;
        case GPF_RGB16F:
            tf =  ur::TextureFormat::RGB16F;
            bpp = 3 * 4;
            break;
        case GPF_RGB32F:
            tf =  ur::TextureFormat::RGB32F;
            bpp = 3 * 4;
            break;
	    case GPF_COMPRESSED_RGBA_S3TC_DXT1_EXT:
		    tf =  ur::TextureFormat::COMPRESSED_RGBA_S3TC_DXT1_EXT;
            bpp = 4;
		    break;
	    case GPF_COMPRESSED_RGBA_S3TC_DXT3_EXT:
		    tf =  ur::TextureFormat::COMPRESSED_RGBA_S3TC_DXT3_EXT;
            bpp = 4;
		    break;
	    case GPF_COMPRESSED_RGBA_S3TC_DXT5_EXT:
		    tf =  ur::TextureFormat::COMPRESSED_RGBA_S3TC_DXT5_EXT;
            bpp = 4;
		    break;
	    default:
		    assert(0);
	    }

        size_t buf_sz = img->width * img->height * bpp;
        ur::TexturePtr* tex = (ur::TexturePtr*)ves_set_newforeign(0, 0, sizeof(ur::TexturePtr));
        *tex = tt::Render::Instance()->Device()->CreateTexture(img->width, img->height, tf, img->pixels, buf_sz);
    }
    else
    {
        int width  = (int)ves_tonumber(1);
        int height = (int)ves_tonumber(2);
        const char* format = ves_tostring(3);
        ur::TextureFormat tf;
        size_t bpp = 0; // bytes per pixel
        if (strcmp(format, "rgb") == 0) {
            tf = ur::TextureFormat::RGB;
            bpp = 3;
        } else if (strcmp(format, "rgba") == 0) {
            tf = ur::TextureFormat::RGBA8;
            bpp = 4;
        } else {
            assert(0);
        }

        size_t buf_sz = width * height * bpp;
        ur::TexturePtr* tex = (ur::TexturePtr*)ves_set_newforeign(0, 0, sizeof(ur::TexturePtr));
        *tex = tt::Render::Instance()->Device()->CreateTexture(width, height, tf, nullptr, buf_sz);
    }
}

static int w_Texture_finalize(void* data)
{
    ur::TexturePtr* tex = static_cast<ur::TexturePtr*>(data);
    (*tex)->~Texture();
    return sizeof(ur::TexturePtr);
}

void w_Texture_getWidth()
{
    ur::TexturePtr* tex = static_cast<ur::TexturePtr*>(ves_toforeign(0));
    ves_set_number(0, (double)(*tex)->GetWidth());
}

void w_Texture_getHeight()
{
    ur::TexturePtr* tex = static_cast<ur::TexturePtr*>(ves_toforeign(0));
    ves_set_number(0, (double)(*tex)->GetHeight());
}

void w_Graphics_onSize()
{
    float w = (float)ves_tonumber(1);
    float h = (float)ves_tonumber(2);
    tt::Graphics::Instance()->OnSize(w, h);
}

void w_Graphics_onCamUpdate()
{
    float dx = (float)ves_tonumber(1);
    float dy = (float)ves_tonumber(2);
    float scale = (float)ves_tonumber(3);
    tt::Graphics::Instance()->OnCameraUpdate(sm::vec2(-dx, -dy), 1.0f / scale);
}

void w_Graphics_drawPainter()
{
    tess::Painter* pt = *(tess::Painter**)ves_toforeign(1);
    tt::Graphics::Instance()->DrawPainter(*pt);
}

void w_Graphics_drawText()
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

void w_Graphics_drawTexture()
{
    ur::TexturePtr* tex = static_cast<ur::TexturePtr*>(ves_toforeign(1));

    const float x = (float)ves_tonumber(2);
    const float y = (float)ves_tonumber(3);
    const float scale = (float)ves_tonumber(4);

    sm::Matrix2D mt;
    mt.Scale(scale, scale);
    mt.Translate(x, y);

	float vertices[8];
    float w = static_cast<float>((*tex)->GetWidth());
    float h = static_cast<float>((*tex)->GetHeight());
    calc_vertices(sm::rect(w, h), mt, vertices);

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
		draw_without_dtex(rd, vertices, *tex);
	}
}

void w_Graphics_flush()
{
    tt::Graphics::Instance()->Flush();
}

void w_Graphics_dtexDebugDraw()
{
    tt::DTex::Instance()->DebugDraw(*tt::Render::Instance()->Context());
}

}

namespace tt
{

VesselForeignMethodFn GraphicsBindMethod(const char* signature)
{
    if (strcmp(signature, "Painter.addRect(_,_,_)") == 0) return w_Painter_addRect;
    if (strcmp(signature, "Painter.addRectFilled(_,_)") == 0) return w_Painter_addRectFilled;
    if (strcmp(signature, "Painter.addPolygon(_,_,_)") == 0) return w_Painter_addPolygon;
    if (strcmp(signature, "Painter.addPolyline(_,_,_)") == 0) return w_Painter_addPolyline;
    if (strcmp(signature, "Painter.addCircle(_,_,_,_)") == 0) return w_Painter_addCircle;
    if (strcmp(signature, "Painter.addCircleFilled(_,_,_,_)") == 0) return w_Painter_addCircleFilled;
    if (strcmp(signature, "Painter.addBezier(_,_,_)") == 0) return w_Painter_addBezier;

    if (strcmp(signature, "Texture.getWidth()") == 0) return w_Texture_getWidth;
    if (strcmp(signature, "Texture.getHeight()") == 0) return w_Texture_getHeight;

    if (strcmp(signature, "static Graphics.onSize(_,_)") == 0) return w_Graphics_onSize;
    if (strcmp(signature, "static Graphics.onCamUpdate(_,_,_)") == 0) return w_Graphics_onCamUpdate;
    if (strcmp(signature, "static Graphics.drawPainter(_)") == 0) return w_Graphics_drawPainter;
    if (strcmp(signature, "static Graphics.drawText(_,_,_,_,_)") == 0) return w_Graphics_drawText;
    if (strcmp(signature, "static Graphics.drawTexture(_,_,_,_)") == 0) return w_Graphics_drawTexture;
    if (strcmp(signature, "static Graphics.flush()") == 0) return w_Graphics_flush;
    if (strcmp(signature, "static Graphics.dtexDebugDraw()") == 0) return w_Graphics_dtexDebugDraw;

    return nullptr;
}

void GraphicsBindClass(const char* className, VesselForeignClassMethods* methods)
{
    if (strcmp(className, "Painter") == 0)
    {
        methods->allocate = w_Painter_allocate;
        methods->finalize = w_Painter_finalize;
        return;
    }

    if (strcmp(className, "Texture") == 0)
    {
        methods->allocate = w_Texture_allocate;
        methods->finalize = w_Texture_finalize;
        return;
    }
}

}