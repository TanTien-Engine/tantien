//#include "tantien.h"
#include "modules/graphics/Graphics.h"

void tt_on_size(float width, float height)
{
	tt::Graphics::Instance()->OnSize(width, height);
}

void tt_enable_dtex()
{
	tt::Graphics::Instance()->EnableDTex(true);
}