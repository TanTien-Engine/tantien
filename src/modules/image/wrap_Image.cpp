#include "modules/image/wrap_Image.h"
#include "modules/image/ImageData.h"
#include "modules/filesystem/Filesystem.h"
#include "modules/script/Proxy.h"
#include "modules/render/Render.h"
#include "modules/maths/float16.h"

#include <gimg_import.h>
#include <gimg_typedef.h>
#include <gimg_export.h>
#include <unirender/typedef.h>
#include <unirender/Texture.h>
#include <unirender/TextureUtility.h>
#include <unirender/Device.h>
#include <guard/check.h>

#include <string>
#include <algorithm>

namespace
{

int get_format_channels(int format)
{
    int channels = 0;
    switch (format)
    {
    case GPF_RED:
        channels = 1;
        break;
    case GPF_R16:
        channels = 1;
        break;
    case GPF_RGB:
        channels = 3;
        break;
    case GPF_RGBA8:
        channels = 4;
        break;
    default:
        GD_REPORT_ASSERT("unknown type.");
        break;
    }
    return channels;
}

void w_ImageData_allocate()
{
    tt::ImageData* img = (tt::ImageData*)ves_set_newforeign(0, 0, sizeof(tt::ImageData));
    if (ves_type(1) == VES_TYPE_STRING)
    {
        int width = 0, height = 0;
        int format = 0;
        uint8_t* pixels = nullptr;

        const char* filepath = ves_tostring(1);
        if (strcmp(filepath, "SCREEN_CAPTURE") == 0)
        {
            static const int TEX_SZ = 512;
            pixels = new uint8_t[TEX_SZ * TEX_SZ * 3];
            tt::Render::Instance()->Device()->ReadPixels(pixels, ur::TextureFormat::RGB, 0, 0, TEX_SZ, TEX_SZ);
            width = TEX_SZ;
            height = TEX_SZ;
            format = GPF_RGB;
        }
        else
        {
            if (tt::Filesystem::IsExists(filepath)) {
                pixels = gimg_import(filepath, &width, &height, &format);
            } else {
                std::string path = tt::Filesystem::Instance()->GetAssetBaseDir() + "/" + filepath;
                pixels = gimg_import(path.c_str(), &width, &height, &format);
            }
        }

        img->pixels = pixels;
        img->width  = width;
        img->height = height;
        img->format = format;
    }
    else if (ves_type(1) == VES_TYPE_FOREIGN)
    {
        auto tex = ((tt::Proxy<ur::Texture>*)ves_toforeign(1))->obj;
        const auto w = tex->GetWidth();
        const auto h = tex->GetHeight();
        const auto fmt = tex->GetFormat();
        img->width = w;
        img->height = h;
        if (fmt == ur::TextureFormat::RGB) {
            img->format = GPF_RGB;
        } else if (fmt == ur::TextureFormat::RGBA8) {
            img->format = GPF_RGBA8;
        } else {
            GD_REPORT_ASSERT("unknown type.");
            return;
        }
        size_t sz = ur::TextureUtility::RequiredSizeInBytes(w, h, fmt, 4);
        img->pixels = (uint8_t*)tex->WriteToMemory(sz);
    }
    else
    {
        img->width  = (int)ves_tonumber(1);
        img->height = (int)ves_tonumber(2);
        const char* format = ves_tostring(3);
        if (strcmp(format, "rgb") == 0) {
            img->format = GPF_RGB;
        } else if (strcmp(format, "rgba") == 0) {
            img->format = GPF_RGBA8;
        } else if (strcmp(format, "r16f") == 0) {
            img->format = GPF_R16;
        } else {
            GD_REPORT_ASSERT("unknown type.");
            return;
        }

        const int channels = get_format_channels(img->format);
        const int size = img->width * img->height * channels;
        img->pixels = new uint8_t[size];
        memset(img->pixels, 0, size);
    }
}

int w_ImageData_finalize(void* data)
{
    tt::ImageData* img = static_cast<tt::ImageData*>(data);
    delete[] img->pixels;
    return sizeof(tt::ImageData);
}

void w_ImageData_get_width()
{
    tt::ImageData* img = (tt::ImageData*)ves_toforeign(0);
    ves_set_number(0, (double)img->width);
}

void w_ImageData_get_height()
{
    tt::ImageData* img = (tt::ImageData*)ves_toforeign(0);
    ves_set_number(0, (double)img->height);
}

void w_ImageData_to_rgb565()
{
    tt::ImageData* img = (tt::ImageData*)ves_toforeign(0);

    const int channels = get_format_channels(img->format);
    uint8_t* pixels = new uint8_t[img->width * img->height * 2];
    for (int i = 0, n = img->width * img->height; i < n; ++i)
    {
        uint8_t r = img->pixels[i * channels];
        uint8_t g = img->pixels[i * channels + 1];
        uint8_t b = img->pixels[i * channels + 2];
        uint16_t _r = uint16_t(r / 255.0f * 31) << 11;
        uint16_t _g = uint16_t(g / 255.0f * 63) << 5;
        uint16_t _b = uint16_t(b / 255.0f * 31);
        uint16_t rgb = _r | _g | _b;
        memcpy(&pixels[i * 2], &rgb, 2);
    }

    delete[] img->pixels;
    img->pixels = pixels;
    img->format = GPF_RGB565;
}

void w_ImageData_store_to_file()
{
    tt::ImageData* img = (tt::ImageData*)ves_toforeign(0);
    const char* filepath = ves_tostring(1);
    gimg_export(filepath, img->pixels, img->width, img->height, img->format, 1);
}

void w_ImageData_get_pixel()
{
    tt::ImageData* img = (tt::ImageData*)ves_toforeign(0);
    const int x = (int)ves_tonumber(1);
    const int y = (int)ves_tonumber(2);
    if (x < 0 || x >= img->width ||
        y < 0 || y >= img->height) {
        return;
    }

    switch (img->format)
    {
    case GPF_R16:
    {
        unsigned short* pixels = (unsigned short*)img->pixels;
        ves_set_number(0, Float16ToFloat(pixels[y * img->width + x]));
    }
        break;
    case GPF_RGB:
    {
        int rgb[3];
        for (int i = 0; i < 3; ++i) {
            rgb[i] = img->pixels[(y * img->width + x) * 3 + i];
        }
        uint32_t col = rgb[0] << 24 | rgb[1] << 16 | rgb[2] << 8 | 0xff;
        ves_set_number(0, (double)col);
    }
        break;
    case GPF_RGBA8:
    {
        int rgba[4];
        for (int i = 0; i < 4; ++i) {
            rgba[i] = img->pixels[(y * img->width + x) * 4 + i];
        }
        uint32_t col = rgba[0] << 24 | rgba[1] << 16 | rgba[2] << 8 | rgba[3];
        ves_set_number(0, (double)col);
    }
        break;
    default:
        GD_REPORT_ASSERT("unknown type.");
        break;
    }
}

void w_ImageData_set_pixel()
{
    tt::ImageData* img = (tt::ImageData*)ves_toforeign(0);
    const int x = (int)ves_tonumber(1);
    const int y = (int)ves_tonumber(2);
    //const uint32_t col = (uint32_t)ves_tonumber(3);

    uint8_t rgba[4];
    //rgba[0] = (col >> 24) & 0xff;
    //rgba[1] = (col >> 16) & 0xff;
    //rgba[2] = (col >> 8) & 0xff;
    //rgba[3] = col & 0xff;

    rgba[0] = (uint32_t)ves_tonumber(3);
    rgba[1] = (uint32_t)ves_tonumber(4);
    rgba[2] = (uint32_t)ves_tonumber(5);
    rgba[3] = 0;

    const int channels = get_format_channels(img->format);
    for (int i = 0; i < channels; ++i) {
        img->pixels[(y * img->width + x) * channels + i] = rgba[i];
    }
}

void w_ImageData_cropping()
{
    tt::ImageData* img = (tt::ImageData*)ves_toforeign(0);
    const int x = (int)ves_tonumber(1);
    const int y = (int)ves_tonumber(2);
    const int w = (int)ves_tonumber(3);
    const int h = (int)ves_tonumber(4);

    const int channels = get_format_channels(img->format);
    const int new_w = std::max(0, std::min(w, img->width - 1 - x));
    const int new_h = std::max(0, std::min(h, img->height - 1 - y));
    if (new_w == 0 || new_h == 0) {
        return;
    }

    const int size = new_w * new_h * channels;
    uint8_t* sub_pixels = new uint8_t[size];
    memset(sub_pixels, 0, size);
    for (int _x = 0; _x < new_w; ++_x) {
        for (int _y = 0; _y < new_h; ++_y) {
            const int x_f = x + _x;
            const int y_f = y + _y;
            if (x_f < 0 || x_f >= img->width || y_f < 0 || y_f >= img->height) {
                continue;
            }
            for (int i = 0; i < channels; ++i) {
                sub_pixels[(_y * new_w + _x) * channels + i] = img->pixels[(y_f * img->width + x_f) * channels + i];
            }
        }
    }

    tt::ImageData* sub_img = (tt::ImageData*)ves_set_newforeign(0, 0, sizeof(tt::ImageData));
    sub_img->pixels = sub_pixels;
    sub_img->width = new_w;
    sub_img->height = new_h;
    sub_img->format = img->format;
}

void w_ImageData_copy_from()
{
    tt::ImageData* img = (tt::ImageData*)ves_toforeign(0);
    tt::ImageData* sub_img = (tt::ImageData*)ves_toforeign(1);
    if (img->format != sub_img->format) {
        return;
    }

    const int x = (int)ves_tonumber(2);
    const int y = (int)ves_tonumber(3);

    const bool debug_border = ves_toboolean(4);

    const int new_w = std::max(0, std::min(sub_img->width,  img->width  - 1 - x));
    const int new_h = std::max(0, std::min(sub_img->height, img->height - 1 - y));
    if (new_w == 0 || new_h == 0) {
        return;
    }

    const int channels = get_format_channels(img->format);
    for (int _x = 0; _x < new_w; ++_x) {
        for (int _y = 0; _y < new_h; ++_y) {
            if (debug_border && (_x == 0 || _x == new_w - 1 || _y == 0 || _y == new_h - 1)) {
                for (int i = 0; i < channels; ++i) {
                    img->pixels[((y + _y) * img->width + (x + _x)) * channels + i] = 0;
                }
                img->pixels[((y + _y) * img->width + (x + _x)) * channels] = 255;
                if (channels == 4) {
                    img->pixels[((y + _y) * img->width + (x + _x)) * channels + 3] = 255;
                }
            } else {
                for (int i = 0; i < channels; ++i) {
                    img->pixels[((y + _y) * img->width + (x + _x)) * channels + i] = sub_img->pixels[(_y * sub_img->width + _x) * channels + i];
                }
            }
        }
    }
}

void w_ImageData_is_empty()
{
    tt::ImageData* img = (tt::ImageData*)ves_toforeign(0);
    const int channels = get_format_channels(img->format);
    for (int x = 0; x < img->width; ++x) {
        for (int y = 0; y < img->height; ++y) {
            for (int i = 0; i < channels; ++i) {
                if (img->pixels[(y * img->width + x) * channels + i] != 0) {
                    ves_set_boolean(0, false);
                    return;
                }
            }
        }
    }

    ves_set_boolean(0, true);
}

}

namespace tt
{

VesselForeignMethodFn ImageBindMethod(const char* signature)
{
    if (strcmp(signature, "ImageData.get_width()") == 0) return w_ImageData_get_width;
    if (strcmp(signature, "ImageData.get_height()") == 0) return w_ImageData_get_height;
    if (strcmp(signature, "ImageData.to_rgb565()") == 0) return w_ImageData_to_rgb565;
    if (strcmp(signature, "ImageData.store_to_file(_)") == 0) return w_ImageData_store_to_file;
    if (strcmp(signature, "ImageData.get_pixel(_,_)") == 0) return w_ImageData_get_pixel;
    if (strcmp(signature, "ImageData.set_pixel(_,_,_,_,_)") == 0) return w_ImageData_set_pixel;
    if (strcmp(signature, "ImageData.cropping(_,_,_,_)") == 0) return w_ImageData_cropping;
    if (strcmp(signature, "ImageData.copy_from(_,_,_,_)") == 0) return w_ImageData_copy_from;
    if (strcmp(signature, "ImageData.is_empty()") == 0) return w_ImageData_is_empty;

    return nullptr;
}

void ImageBindClass(const char* class_name, VesselForeignClassMethods* methods)
{
    if (strcmp(class_name, "ImageData") == 0)
    {
        methods->allocate = w_ImageData_allocate;
        methods->finalize = w_ImageData_finalize;
        return;
    }
}

}