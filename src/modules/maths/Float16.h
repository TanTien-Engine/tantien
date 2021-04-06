// https://stackoverflow.com/questions/22210684/16-bit-floats-and-gl-half-float

#ifndef THE__FLOAT_16_H_
#define THE__FLOAT_16_H_

/*+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+*/

extern short FloatToFloat16( float value );
extern float Float16ToFloat( short value );

/*+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+*/

class Float16
{
protected:
    short mValue;
public:
    Float16();
    Float16( float value );
    Float16( const Float16& value );

    operator float();
    operator float() const;

    friend Float16 operator + ( const Float16& val1, const Float16& val2 );
    friend Float16 operator - ( const Float16& val1, const Float16& val2 );
    friend Float16 operator * ( const Float16& val1, const Float16& val2 );
    friend Float16 operator / ( const Float16& val1, const Float16& val2 );

    Float16& operator =( const Float16& val );
    Float16& operator +=( const Float16& val );
    Float16& operator -=( const Float16& val );
    Float16& operator *=( const Float16& val );
    Float16& operator /=( const Float16& val );
    Float16& operator -();
};

/*+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+*/

inline Float16::Float16()
{
}

/*+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+*/

inline Float16::Float16( float value )
{
    mValue  = FloatToFloat16( value );
}

/*+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+*/

inline Float16::Float16( const Float16 &value )
{
    mValue  = value.mValue;
}

/*+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+*/

inline Float16::operator float()
{
    return Float16ToFloat( mValue );
}

/*+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+*/

inline Float16::operator float() const
{
    return Float16ToFloat( mValue );
}

/*+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+*/

inline Float16& Float16::operator =( const Float16& val )
{
    mValue  = val.mValue;
}

/*+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+*/

inline Float16& Float16::operator +=( const Float16& val )
{
    *this   = *this + val;
    return *this;
}

/*+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+*/

inline Float16& Float16::operator -=( const Float16& val )
{
    *this   = *this - val;
    return *this;

}

/*+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+*/

inline Float16& Float16::operator *=( const Float16& val )
{
    *this   = *this * val;
    return *this;
}

/*+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+*/

inline Float16& Float16::operator /=( const Float16& val )
{
    *this   = *this / val;
    return *this;
}

/*+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+*/

inline Float16& Float16::operator -()
{
    *this   = Float16( -(float)*this );
    return *this;
}

/*+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+*/
/*+----+                                 Friends                                       +----+*/
/*+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+*/

inline Float16 operator + ( const Float16& val1, const Float16& val2 )
{
    return Float16( (float)val1 + (float)val2 );
}

/*+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+*/

inline Float16 operator - ( const Float16& val1, const Float16& val2 )
{
    return Float16( (float)val1 - (float)val2 );
}

/*+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+*/

inline Float16 operator * ( const Float16& val1, const Float16& val2 )
{
    return Float16( (float)val1 * (float)val2 );
}

/*+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+*/

inline Float16 operator / ( const Float16& val1, const Float16& val2 )
{
    return Float16( (float)val1 / (float)val2 );
}

/*+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+*/


#endif