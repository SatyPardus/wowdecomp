#ifndef _FMOD_3D_H
#define _FMOD_3D_H

#include "fmod_settings.h"

#include "fmod.h"
#include "fmod_types.h"

#include <stdlib.h>

/*
    CONSTANTS
*/

#define FMOD_3D_MAXLISTENERS  4

/*
    MACROS
*/

#define FMOD_Vector_CrossProduct(_a, _b, _r)                \
	 (_r)->x = ((_a)->y * (_b)->z) - ((_a)->z * (_b)->y);	\
	 (_r)->y = ((_a)->z * (_b)->x) - ((_a)->x * (_b)->z);	\
	 (_r)->z = ((_a)->x * (_b)->y) - ((_a)->y * (_b)->x);

#define FMOD_Vector_Subtract(_a, _b, _r) \
	 (_r)->x = (_a)->x - (_b)->x;	       \
	 (_r)->y = (_a)->y - (_b)->y;	       \
	 (_r)->z = (_a)->z - (_b)->z;

#define FMOD_Vector_Add(_a, _b, _r) \
	 (_r)->x = (_a)->x + (_b)->x;	  \
	 (_r)->y = (_a)->y + (_b)->y;     \
	 (_r)->z = (_a)->z + (_b)->z;

#define FMOD_Vector_Set(_r, _x, _y, _z) \
	 (_r)->x = (_x);	                  \
	 (_r)->y = (_y);	                  \
	 (_r)->z = (_z);

#define FMOD_Vector_Copy(_a, _r) \
	 (_r)->x = (_a)->x;	           \
	 (_r)->y = (_a)->y;	           \
	 (_r)->z = (_a)->z;

#define FMOD_Vector_DotProduct(_a, _b)								\
    (((_a)->x * (_b)->x) + ((_a)->y * (_b)->y) + ((_a)->z * (_b)->z))

#define FMOD_Vector_DotProduct64(_a, _b)								\
    FMOD_SCALEDOWN64( ((FMOD_SINT64)((_a)->x) * (FMOD_SINT64)((_b)->x)) + ((FMOD_SINT64)((_a)->y) * (FMOD_SINT64)((_b)->y)) + ((FMOD_SINT64)((_a)->z) * (FMOD_SINT64)((_b)->z)) )

#define FMOD_Vector_Scale(_a, _s, _r) \
	 (_r)->x = ((_a)->x * (_s));	\
	 (_r)->y = ((_a)->y * (_s));	\
	 (_r)->z = ((_a)->z * (_s));

static FMOD_INLINE void FMOD_Vector_AxisRotate(FMOD_VECTOR *in,FMOD_VECTOR *at,FMOD_VECTOR *up,float theta)
{
	float a,b,c,u,v,w,x,y,z;
	float cost,sint;

	a = at->x;
	b = at->y;
	c = at->z;

	u = up->x;
	v = up->y;
	w = up->z;

	x = in->x;
	y = in->y;
	z = in->z;

	cost = FMOD_COS(theta);
	sint = FMOD_SIN(theta);

	in->x=(a*(v*v+w*w)+u*(-b*v-c*w+u*x+v*y+w*z)+((x-a)*(v*v+w*w)+u*(b*v+c*w-v*y-w*z))*cost+FMOD_SQRT(u*u+v*v+w*w)*(b*w-c*v-w*y+v*z)*sint)/(u*u+v*v+w*w);
	in->y=(b*(u*u+w*w)+v*(-a*u-c*w+u*x+v*y+w*z)+((y-b)*(u*u+w*w)+v*(a*u+c*w-u*x-w*z))*cost+FMOD_SQRT(u*u+v*v+w*w)*(-a*w+c*u+w*x-u*z)*sint)/(u*u+v*v+w*w);
	in->z=(c*(u*u+v*v)+w*(-a*u-b*v+u*x+v*y+w*z)+((z-c)*(u*u+v*v)+w*(a*u+b*v-u*x-v*y))*cost+FMOD_SQRT(u*u+v*v+w*w)*(a*v-b*u-v*x+u*y)*sint)/(u*u+v*v+w*w);
}

static FMOD_INLINE int FMOD_Cart2Angle(int y, int x)
{
    int coeff_1, coeff_2, abs_y;
    int r, angle;

    #define SHIFT 10
    #define M_PII (int)(3.1415926535897932384626433832795f * (float)(1<<SHIFT))

    if (!x && !y)
    {
        return 0;
    }

    x = static_cast<int>(static_cast<unsigned int>(x) << SHIFT);
    y = static_cast<int>(static_cast<unsigned int>(y) << SHIFT);

    coeff_1 = M_PII/4;
    coeff_2 = 3*coeff_1;

    abs_y = abs(y) + 1 ;      /* kludge to prevent 0/0 condition */

    if (x >= 0)
    {
        r = (x - abs_y) / ((x + abs_y)   >> SHIFT);
        angle = coeff_1 - ((coeff_1 * r) >> SHIFT);
    }
    else
    {
        r = (x + abs_y) / ((abs_y - x)   >> SHIFT);
        angle = coeff_2 - ((coeff_1 * r) >> SHIFT);
    }

    if (y < 0)
    {
        angle = -angle;     /* negate if in quad III or IV */
    }
    
    angle *= 180;
    angle /= M_PII;
    angle = (angle < 0) ? angle + 360 : (angle >= 360) ? angle - 360 : angle;

    return angle;
}


/*
    Angle Sort Functions
    ====================
    Functions for quick sorting and comparison of 2D vector angles.
    Values are in the range [0,8) and correspond to the angles shown below:

          y
     0.0  |  2.0
        .-+-. 
     ---+ + +--- x
        .-+-.
     6.0  |  4.0
*/

#define FMOD_ANGLESORT_REVOLUTION 8.0f
#define FMOD_ANGLESORT_STRAIGHT_TOLERANCE 0.002f /* ~0.1 degrees */

/*
    Return an angle in the range [0,8) from cartesian coordinates. (vector doesn't need to be normalised)
*/
static FMOD_INLINE float FMOD_AngleSort_GetValue(float x, float y)
{
    if(x == 0.0f && y == 0.0f)
    {
        return 0.0f;
    }

    float abs_y = FMOD_ABS(y);
    float abs_x = FMOD_ABS(x);
    float value;

    if(abs_x <= abs_y)
    {
        /*
            Top and bottom sides of square.
            The angle is calculated for positive 'y' and then mirrored if y is negative.
            (-1, 1) => 0; (1, 1) => 2
        */
        value = x / abs_y + 1.0f;

        if(y < 0.0f)
        {
            /* values 0 -> 2 (front) map to values 6 -> 4 (back) */
            return 6.0f - value;
        }
        else
        {
            return value;
        }
    }
    else
    {
        /*
            Left and right sides of square.
            The angle is calculated for positive 'x' and then mirrored if x is negative.
            (1, -1) => 4; (1, 1) => 2
        */
        value = 3.0f - y / abs_x;

        if(x < 0.0f)
        {
            /* values 2 -> 4 (front) map to values 8 -> 6 (back) */
            return 10.0f - value;
        }
        else
        {
            return value;
        }
    }
}

/*
    Check that values in the range [0,8) represent angles sorted in a clockwise order. 
    (or anti-clockwise for left-handed coordinate systems)
*/
static FMOD_INLINE bool FMOD_AngleSort_IsClockwise(float a, float b, float c)
{
    float c_unwrapped = (c >= a) ? c : c + FMOD_ANGLESORT_REVOLUTION;

    return (a <= b && b < c_unwrapped) || (a <= b + FMOD_ANGLESORT_REVOLUTION && b + FMOD_ANGLESORT_REVOLUTION < c_unwrapped);
}

/*
    Checks if the clockwise (or anti-clockwise for left-handed coordinate systems) difference
    between two angles is 180 degrees (FMOD_AngleSort_IsStraight) or more (FMOD_AngleSort_IsReflex). 
*/
#define FMOD_AngleSort_IsStraight(_a, _b) (FMOD_ABS(FMOD_ABS((_b) - (_a)) - FMOD_ANGLESORT_REVOLUTION * 0.5f) <= FMOD_ANGLESORT_STRAIGHT_TOLERANCE)
#define FMOD_AngleSort_IsReflex(_a, _b) (((_b) - (_a) > FMOD_ANGLESORT_REVOLUTION * 0.5f) || (((_a) > (_b)) && ((_a) - (_b) < FMOD_ANGLESORT_REVOLUTION * 0.5f)))


#include <math.h>

#define FMOD_Vector_GetLengthFast(_x)  FMOD_SQRT(FMOD_Vector_DotProduct((_x), (_x)))
#define FMOD_Vector_GetLength(_x)      FMOD_SQRT(FMOD_Vector_DotProduct((_x), (_x)))


static FMOD_INLINE void FMOD_Vector_Normalize(FMOD_VECTOR *v)
{
    float distance = FMOD_Vector_GetLength(v);

    if (distance <= 0)
    {
        v->x = 0;
        v->y = 0;
        v->z = 0;
    }
    else
    {
        /*
            Normalize
        */
        v->x = v->x / distance;
        v->y = v->y / distance;
        v->z = v->z / distance;
    }
}

static FMOD_INLINE FMOD_RESULT FMOD_CHECKFLOAT(float value)
{
    int s, e;
    unsigned long src;
    long f; 
    union {
        float fval;
        unsigned int ival;
    } u;
    u.fval = value;

    src = u.ival;

    s = (src & 0x80000000UL) >> 31;
    e = (src & 0x7F800000UL) >> 23;
    f = (src & 0x007FFFFFUL);

    if (e == 255 && f != 0) 
    {
        return FMOD_ERR_INVALID_FLOAT;
    }
    else if (e == 255 && f == 0 && s == 1) 
    {
        return FMOD_ERR_INVALID_FLOAT;
    }
    else if (e == 255 && f == 0 && s == 0) 
    {
        return FMOD_ERR_INVALID_FLOAT;
    }
    else if (e == 0 && f != 0) 
    {
        return FMOD_ERR_INVALID_FLOAT;
    }

    return FMOD_OK;
}

#endif




