#include "modules/maths/Float16.h"

#include <string.h>

//#include <d3dx9.h>

/*+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+*/

short FloatToFloat16(float value)
{
    short   fltInt16;
    int     fltInt32;
    memcpy(&fltInt32, &value, sizeof(float));
    fltInt16 = ((fltInt32 & 0x7fffffff) >> 13) - (0x38000000 >> 13);
    fltInt16 |= ((fltInt32 & 0x80000000) >> 16);

    return fltInt16;
}

float Float16ToFloat(short fltInt16)
{
    int fltInt32 = ((fltInt16 & 0x8000) << 16);
    fltInt32 |= ((fltInt16 & 0x7fff) << 13) + 0x38000000;

    float fRet;
    memcpy(&fRet, &fltInt32, sizeof(float));
    return fRet;
}