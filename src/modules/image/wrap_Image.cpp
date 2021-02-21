#include "modules/image/wrap_Image.h"
#include "modules/image/ImageData.h"

#include <gimg_import.h>
#include <gimg_typedef.h>
#include <gimg_export.h>

#include <string>
#include <algorithm>

#include <assert.h>

namespace
{

int get_format_channels(int format)
{
    int channels = 0;
    switch (format)
    {
    case GPF_RGBA8:
        channels = 4;
        break;
    case GPF_RGB:
        channels = 3;
        break;
    default:
        // unsupported format
        break;
    }
    return channels;
}

void w_ImageData_allocate()
{
    tt::ImageData* img = (tt::ImageData*)ves_set_newforeign(0, 0, sizeof(tt::ImageData));
    if (ves_type(1) == VES_TYPE_STRING)
    {
        const char* filepath = ves_tostring(1);

        int width = 0, height = 0;
        int format = 0;
        uint8_t* pixels = gimg_import(filepath, &width, &height, &format);

        img->pixels = pixels;
        img->width  = width;
        img->height = height;
        img->format = format;
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
        } else {
            assert(0);
            return;
        }

        const int channels = get_format_channels(img->format);
        const int size = img->width * img->height * channels;
        img->pixels = new uint8_t[size];
        memset(img->pixels, 0, size);
    }
}

static int w_ImageData_finalize(void* data)
{
    tt::ImageData* img = static_cast<tt::ImageData*>(data);
    delete[] img->pixels;
    return sizeof(tt::ImageData);
}

void w_ImageData_getWidth()
{
    tt::ImageData* img = (tt::ImageData*)ves_toforeign(0);
    ves_set_number(0, (double)img->width);
}

void w_ImageData_getHeight()
{
    tt::ImageData* img = (tt::ImageData*)ves_toforeign(0);
    ves_set_number(0, (double)img->height);
}

void w_ImageData_toRGB565()
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

void w_ImageData_storeToFile()
{
    tt::ImageData* img = (tt::ImageData*)ves_toforeign(0);
    const char* filepath = ves_tostring(1);
    gimg_export(filepath, img->pixels, img->width, img->height, img->format, 1);
}

void w_ImageData_getPixel()
{
    tt::ImageData* img = (tt::ImageData*)ves_toforeign(0);
    const int x = (int)ves_tonumber(1);
    const int y = (int)ves_tonumber(2);
    if (x < 0 || x >= img->width ||
        y < 0 || y >= img->height) {
        return;
    }

    const int channels = get_format_channels(img->format);
    int rgba[4];
    for (int i = 0; i < channels; ++i) {
        rgba[i] = img->pixels[(y * img->width + x) * channels + i];
    }
    for (int i = channels; i < 4; ++i) {
        rgba[i] = 0;
    }
    uint32_t col = rgba[0] << 24 | rgba[1] << 16 | rgba[2] << 8 | rgba[3];
    ves_set_number(0, (double)col);
}

void w_ImageData_setPixel()
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

void w_ImageData_copyFrom()
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

void w_ImageData_isEmpty()
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
    if (strcmp(signature, "ImageData.getWidth()") == 0) return w_ImageData_getWidth;
    if (strcmp(signature, "ImageData.getHeight()") == 0) return w_ImageData_getHeight;
    if (strcmp(signature, "ImageData.toRGB565()") == 0) return w_ImageData_toRGB565;
    if (strcmp(signature, "ImageData.storeToFile(_)") == 0) return w_ImageData_storeToFile;
    if (strcmp(signature, "ImageData.getPixel(_,_)") == 0) return w_ImageData_getPixel;
    if (strcmp(signature, "ImageData.setPixel(_,_,_,_,_)") == 0) return w_ImageData_setPixel;
    if (strcmp(signature, "ImageData.cropping(_,_,_,_)") == 0) return w_ImageData_cropping;
    if (strcmp(signature, "ImageData.copyFrom(_,_,_,_)") == 0) return w_ImageData_copyFrom;
    if (strcmp(signature, "ImageData.isEmpty()") == 0) return w_ImageData_isEmpty;

    return nullptr;
}

void ImageBindClass(const char* className, VesselForeignClassMethods* methods)
{
    if (strcmp(className, "ImageData") == 0)
    {
        methods->allocate = w_ImageData_allocate;
        methods->finalize = w_ImageData_finalize;
        return;
    }
}

}