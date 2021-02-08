#include "modules/image/wrap_Image.h"
#include "modules/image/ImageData.h"

#include <gimg_import.h>
#include <gimg_typedef.h>
#include <gimg_export.h>

#include <string>

namespace
{

void image_data_allocate()
{
    const char* filepath = ves_tostring(1);

    int width = 0, height = 0;
    int format = 0;
    uint8_t* pixels = gimg_import(filepath, &width, &height, &format);

    tt::ImageData* img = (tt::ImageData*)ves_set_newforeign(0, 0, sizeof(tt::ImageData));
    img->pixels = pixels;
    img->width = width;
    img->height = height;
    img->format = format;
}

static int image_data_finalize(void* data)
{
    tt::ImageData* img = static_cast<tt::ImageData*>(data);
    delete[] img->pixels;
    return sizeof(tt::ImageData);
}

void w_ImageData_toRGB565()
{
    tt::ImageData* img = (tt::ImageData*)ves_toforeign(0);

    int channels = 0;
    switch (img->format)
    {
    case GPF_RGBA8:
        channels = 4;
        break;
    case GPF_RGB:
        channels = 3;
        break;
    default:
        // unsupported format
        return;
    }

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
    gimg_export(filepath, img->pixels, img->width, img->height, img->format, 0);
}

}

namespace tt
{

VesselForeignMethodFn ImageBindMethod(const char* signature)
{
    if (strcmp(signature, "ImageData.toRGB565()") == 0) return w_ImageData_toRGB565;
    if (strcmp(signature, "ImageData.storeToFile(_)") == 0) return w_ImageData_storeToFile;

    return nullptr;
}

void ImageBindClass(const char* className, VesselForeignClassMethods* methods)
{
    if (strcmp(className, "ImageData") == 0)
    {
        methods->allocate = image_data_allocate;
        methods->finalize = image_data_finalize;
        return;
    }
}

}