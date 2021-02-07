#include "modules/image/wrap_Image.h"
#include "modules/image/ImageData.h"

#include <gimg_import.h>

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

void image_data_trans()
{
}

}

namespace tt
{

VesselForeignMethodFn ImageBindMethod(const char* signature)
{
    if (strcmp(signature, "ImageData.trans()") == 0) return image_data_trans;

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