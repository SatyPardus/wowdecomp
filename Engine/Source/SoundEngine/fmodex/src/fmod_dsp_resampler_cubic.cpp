#include "fmod_settings.h"
#include <stddef.h>

#include "fmod_dsp_resampler_cubic.h"

#define OO4GIG (1.0f / 4294967296.0f)

/*
[
	[DESCRIPTION]

	[PARAMETERS]
 
	[RETURN_VALUE]
    FMOD_OK

	[REMARKS]

    [PLATFORMS]
    Win32, Win64, Linux, Macintosh, XBox, PlayStation 2, GameCube

	[SEE_ALSO]
]
*/
// VERIFY: 326ae538 manual
// OFFSET: 0x009411A0
void FMOD_Resampler_Cubic(float *out, int outlength, void *src, FMOD_SOUND_FORMAT srcformat, FMOD_UINT64P *position, FMOD_SINT64P *speed, int channels)
{
    float scale = 1.0f;
    
    switch (srcformat)
    {
        /*
            8 BIT
        */
        case FMOD_SOUND_FORMAT_PCM8:
        {
            signed char *inptr = (signed char *)src;

            scale /= (float)(1<<7);

            if (channels == 1)
            {
                float f;
                float p0,p1,p2,p3;
                float r1,r2,r3,r4;
                float a,b,c;
                int len;

                len = outlength >> 2;
                while (len)
                {
                    f = (float)position->mLo * OO4GIG;
                    p0 = (float)inptr[static_cast<ptrdiff_t>(position->mHi) - 1] * scale;
                    p1 = (float)inptr[static_cast<ptrdiff_t>(position->mHi) + 0] * scale;
                    p2 = (float)inptr[static_cast<ptrdiff_t>(position->mHi) + 1] * scale;
                    p3 = (float)inptr[static_cast<ptrdiff_t>(position->mHi) + 2] * scale;
                    a = (3.0f * (p1-p2) - p0 + p3) / 2.0f;
                    b = 2.0f * p2 + p0 - (5.0f * p1 + p3) / 2.0f;
                    c = (p2 - p0) / 2.0f;
                    r1 = (((a * f) + b) * f + c) * f + p1;
                    position->mValue += speed->mValue;

                    f = (float)position->mLo * OO4GIG;
                    p0 = (float)inptr[static_cast<ptrdiff_t>(position->mHi) - 1] * scale;
                    p1 = (float)inptr[static_cast<ptrdiff_t>(position->mHi) + 0] * scale;
                    p2 = (float)inptr[static_cast<ptrdiff_t>(position->mHi) + 1] * scale;
                    p3 = (float)inptr[static_cast<ptrdiff_t>(position->mHi) + 2] * scale;
                    a = (3.0f * (p1-p2) - p0 + p3) / 2.0f;
                    b = 2.0f * p2 + p0 - (5.0f * p1 + p3) / 2.0f;
                    c = (p2 - p0) / 2.0f;
                    r2 = (((a * f) + b) * f + c) * f + p1;
                    position->mValue += speed->mValue;

                    f = (float)position->mLo * OO4GIG;
                    p0 = (float)inptr[static_cast<ptrdiff_t>(position->mHi) - 1] * scale;
                    p1 = (float)inptr[static_cast<ptrdiff_t>(position->mHi) + 0] * scale;
                    p2 = (float)inptr[static_cast<ptrdiff_t>(position->mHi) + 1] * scale;
                    p3 = (float)inptr[static_cast<ptrdiff_t>(position->mHi) + 2] * scale;
                    a = (3.0f * (p1-p2) - p0 + p3) / 2.0f;
                    b = 2.0f * p2 + p0 - (5.0f * p1 + p3) / 2.0f;
                    c = (p2 - p0) / 2.0f;
                    r3 = (((a * f) + b) * f + c) * f + p1;
                    position->mValue += speed->mValue;

                    f = (float)position->mLo * OO4GIG;
                    p0 = (float)inptr[static_cast<ptrdiff_t>(position->mHi) - 1] * scale;
                    p1 = (float)inptr[static_cast<ptrdiff_t>(position->mHi) + 0] * scale;
                    p2 = (float)inptr[static_cast<ptrdiff_t>(position->mHi) + 1] * scale;
                    p3 = (float)inptr[static_cast<ptrdiff_t>(position->mHi) + 2] * scale;
                    a = (3.0f * (p1-p2) - p0 + p3) / 2.0f;
                    b = 2.0f * p2 + p0 - (5.0f * p1 + p3) / 2.0f;
                    c = (p2 - p0) / 2.0f;
                    r4 = (((a * f) + b) * f + c) * f + p1;
                    position->mValue += speed->mValue;

                    out[0] = r1;
                    out[1] = r2;
                    out[2] = r3;
                    out[3] = r4;
                    len--;
                    out+=4;
                } 

                len = outlength & 3;
                while (len)
                {
                    f = position->mLo * OO4GIG;
                    p0 = (float)inptr[static_cast<ptrdiff_t>(position->mHi) - 1] * scale;
                    p1 = (float)inptr[static_cast<ptrdiff_t>(position->mHi) + 0] * scale;
                    p2 = (float)inptr[static_cast<ptrdiff_t>(position->mHi) + 1] * scale;
                    p3 = (float)inptr[static_cast<ptrdiff_t>(position->mHi) + 2] * scale;
                    a = (3.0f * (p1-p2) - p0 + p3) / 2.0f;
                    b = 2.0f * p2 + p0 - (5.0f * p1 + p3) / 2.0f;
                    c = (p2 - p0) / 2.0f;
                    r1 = (((a * f) + b) * f + c) * f + p1;

                    *out++ = r1;

                    position->mValue += speed->mValue;
                    len--;
                } 
            }
            else
            {
                while (outlength)
                {
                    int count;
                    float p0,p1,p2,p3;
                    float r;
                    float a,b,c;
                    float f = position->mLo * OO4GIG;

                    for (count = 0; count < channels; count++)
                    {
                        p0 = (float)inptr[((static_cast<ptrdiff_t>(position->mHi) - 1) * channels) + count] * scale;
                        p1 = (float)inptr[((static_cast<ptrdiff_t>(position->mHi) + 0) * channels) + count] * scale;
                        p2 = (float)inptr[((static_cast<ptrdiff_t>(position->mHi) + 1) * channels) + count] * scale;
                        p3 = (float)inptr[((static_cast<ptrdiff_t>(position->mHi) + 2) * channels) + count] * scale;
                        a = (3.0f * (p1-p2) - p0 + p3) / 2.0f;
                        b = 2.0f * p2 + p0 - (5.0f * p1 + p3) / 2.0f;
                        c = (p2 - p0) / 2.0f;
                        r = (((a * f) + b) * f + c) * f + p1;
                        *out++ = r;
                    }
                    position->mValue += speed->mValue;
                    outlength--;
                } 
            }
            break;
        }

        /*
            16 BIT
        */
        case FMOD_SOUND_FORMAT_PCM16:
        {
            signed short *inptr = (signed short *)src;

            scale /= (float)(1<<15);

            if (channels == 1)
            {
                float f;
                float p0,p1,p2,p3;
                float r1,r2,r3,r4;
                float a,b,c;
                int len;

                len = outlength >> 2;
                while (len)
                {
                    f = (float)position->mLo * OO4GIG;
                    p0 = (float)inptr[static_cast<ptrdiff_t>(position->mHi) - 1] * scale;
                    p1 = (float)inptr[static_cast<ptrdiff_t>(position->mHi) + 0] * scale;
                    p2 = (float)inptr[static_cast<ptrdiff_t>(position->mHi) + 1] * scale;
                    p3 = (float)inptr[static_cast<ptrdiff_t>(position->mHi) + 2] * scale;
                    a = (3.0f * (p1-p2) - p0 + p3) / 2.0f;
                    b = 2.0f * p2 + p0 - (5.0f * p1 + p3) / 2.0f;
                    c = (p2 - p0) / 2.0f;
                    r1 = (((a * f) + b) * f + c) * f + p1;
                    position->mValue += speed->mValue;

                    f = (float)position->mLo * OO4GIG;
                    p0 = (float)inptr[static_cast<ptrdiff_t>(position->mHi) - 1] * scale;
                    p1 = (float)inptr[static_cast<ptrdiff_t>(position->mHi) + 0] * scale;
                    p2 = (float)inptr[static_cast<ptrdiff_t>(position->mHi) + 1] * scale;
                    p3 = (float)inptr[static_cast<ptrdiff_t>(position->mHi) + 2] * scale;
                    a = (3.0f * (p1-p2) - p0 + p3) / 2.0f;
                    b = 2.0f * p2 + p0 - (5.0f * p1 + p3) / 2.0f;
                    c = (p2 - p0) / 2.0f;
                    r2 = (((a * f) + b) * f + c) * f + p1;
                    position->mValue += speed->mValue;

                    f = (float)position->mLo * OO4GIG;
                    p0 = (float)inptr[static_cast<ptrdiff_t>(position->mHi) - 1] * scale;
                    p1 = (float)inptr[static_cast<ptrdiff_t>(position->mHi) + 0] * scale;
                    p2 = (float)inptr[static_cast<ptrdiff_t>(position->mHi) + 1] * scale;
                    p3 = (float)inptr[static_cast<ptrdiff_t>(position->mHi) + 2] * scale;
                    a = (3.0f * (p1-p2) - p0 + p3) / 2.0f;
                    b = 2.0f * p2 + p0 - (5.0f * p1 + p3) / 2.0f;
                    c = (p2 - p0) / 2.0f;
                    r3 = (((a * f) + b) * f + c) * f + p1;
                    position->mValue += speed->mValue;

                    f = (float)position->mLo * OO4GIG;
                    p0 = (float)inptr[static_cast<ptrdiff_t>(position->mHi) - 1] * scale;
                    p1 = (float)inptr[static_cast<ptrdiff_t>(position->mHi) + 0] * scale;
                    p2 = (float)inptr[static_cast<ptrdiff_t>(position->mHi) + 1] * scale;
                    p3 = (float)inptr[static_cast<ptrdiff_t>(position->mHi) + 2] * scale;
                    a = (3.0f * (p1-p2) - p0 + p3) / 2.0f;
                    b = 2.0f * p2 + p0 - (5.0f * p1 + p3) / 2.0f;
                    c = (p2 - p0) / 2.0f;
                    r4 = (((a * f) + b) * f + c) * f + p1;
                    position->mValue += speed->mValue;

                    out[0] = r1;
                    out[1] = r2;
                    out[2] = r3;
                    out[3] = r4;
                    len--;
                    out+=4;
                } 

                len = outlength & 3;
                while (len)
                {
                    f = position->mLo * OO4GIG;
                    p0 = (float)inptr[static_cast<ptrdiff_t>(position->mHi) - 1] * scale;
                    p1 = (float)inptr[static_cast<ptrdiff_t>(position->mHi) + 0] * scale;
                    p2 = (float)inptr[static_cast<ptrdiff_t>(position->mHi) + 1] * scale;
                    p3 = (float)inptr[static_cast<ptrdiff_t>(position->mHi) + 2] * scale;
                    a = (3.0f * (p1-p2) - p0 + p3) / 2.0f;
                    b = 2.0f * p2 + p0 - (5.0f * p1 + p3) / 2.0f;
                    c = (p2 - p0) / 2.0f;
                    r1 = (((a * f) + b) * f + c) * f + p1;

                    *out++ = r1;

                    position->mValue += speed->mValue;
                    len--;
                } 
            }
            else
            {
                while (outlength)
                {
                    int count;
                    float p0,p1,p2,p3;
                    float r;
                    float a,b,c;
                    float f = position->mLo * OO4GIG;

                    for (count = 0; count < channels; count++)
                    {
                        p0 = (float)inptr[((static_cast<ptrdiff_t>(position->mHi) - 1) * channels) + count] * scale;
                        p1 = (float)inptr[((static_cast<ptrdiff_t>(position->mHi) + 0) * channels) + count] * scale;
                        p2 = (float)inptr[((static_cast<ptrdiff_t>(position->mHi) + 1) * channels) + count] * scale;
                        p3 = (float)inptr[((static_cast<ptrdiff_t>(position->mHi) + 2) * channels) + count] * scale;
                        a = (3.0f * (p1-p2) - p0 + p3) / 2.0f;
                        b = 2.0f * p2 + p0 - (5.0f * p1 + p3) / 2.0f;
                        c = (p2 - p0) / 2.0f;
                        r = (((a * f) + b) * f + c) * f + p1;
                        *out++ = r;
                    }
                    position->mValue += speed->mValue;
                    outlength--;
                } 
            }
            break;
        }

        /*
            24 BIT
        */
        case FMOD_SOUND_FORMAT_PCM24:
        {
            FMOD_INT24 *inptr = (FMOD_INT24 *)src;

            scale /= (float)(1<<23);

            if (channels == 1)
            {
                while (outlength)
                {
                    float r;
                    float a,b,c;
                    float f = position->mLo * OO4GIG;
                    FMOD_INT24 *s0 = &inptr[static_cast<ptrdiff_t>(position->mHi) - 1];
                    FMOD_INT24 *s1 = &inptr[static_cast<ptrdiff_t>(position->mHi) + 0];
                    FMOD_INT24 *s2 = &inptr[static_cast<ptrdiff_t>(position->mHi) + 1];
                    FMOD_INT24 *s3 = &inptr[static_cast<ptrdiff_t>(position->mHi) + 2];
                    float  p0 = (float)((int)(((unsigned int)s0->val[0] <<  8) | ((unsigned int)s0->val[1] << 16) | ((unsigned int)s0->val[2] << 24)) >> 8) * scale;
                    float  p1 = (float)((int)(((unsigned int)s1->val[0] <<  8) | ((unsigned int)s1->val[1] << 16) | ((unsigned int)s1->val[2] << 24)) >> 8) * scale;
                    float  p2 = (float)((int)(((unsigned int)s2->val[0] <<  8) | ((unsigned int)s2->val[1] << 16) | ((unsigned int)s2->val[2] << 24)) >> 8) * scale;
                    float  p3 = (float)((int)(((unsigned int)s3->val[0] <<  8) | ((unsigned int)s3->val[1] << 16) | ((unsigned int)s3->val[2] << 24)) >> 8) * scale;

                    a = (3.0f * (p1-p2) - p0 + p3) / 2.0f;
                    b = 2.0f * p2 + p0 - (5.0f * p1 + p3) / 2.0f;
                    c = (p2 - p0) / 2.0f;
                    r = (((a * f) + b) * f + c) * f + p1;
                    *out++ = r;

                    position->mValue += speed->mValue;
                    outlength--;
                } 
            }
            else
            {
                while (outlength)
                {
                    int count;
                    float r;
                    float a,b,c;
                    float f = position->mLo * OO4GIG;

                    for (count = 0; count < channels; count++)
                    {
                        FMOD_INT24 *s0 = &inptr[((static_cast<ptrdiff_t>(position->mHi) - 1) * channels) + count];
                        FMOD_INT24 *s1 = &inptr[((static_cast<ptrdiff_t>(position->mHi) + 0) * channels) + count];
                        FMOD_INT24 *s2 = &inptr[((static_cast<ptrdiff_t>(position->mHi) + 1) * channels) + count];
                        FMOD_INT24 *s3 = &inptr[((static_cast<ptrdiff_t>(position->mHi) + 2) * channels) + count];
                        float  p0 = (float)((int)(((unsigned int)s0->val[0] <<  8) | ((unsigned int)s0->val[1] << 16) | ((unsigned int)s0->val[2] << 24)) >> 8) * scale;
                        float  p1 = (float)((int)(((unsigned int)s1->val[0] <<  8) | ((unsigned int)s1->val[1] << 16) | ((unsigned int)s1->val[2] << 24)) >> 8) * scale;
                        float  p2 = (float)((int)(((unsigned int)s2->val[0] <<  8) | ((unsigned int)s2->val[1] << 16) | ((unsigned int)s2->val[2] << 24)) >> 8) * scale;
                        float  p3 = (float)((int)(((unsigned int)s3->val[0] <<  8) | ((unsigned int)s3->val[1] << 16) | ((unsigned int)s3->val[2] << 24)) >> 8) * scale;

                        a = (3.0f * (p1-p2) - p0 + p3) / 2.0f;
                        b = 2.0f * p2 + p0 - (5.0f * p1 + p3) / 2.0f;
                        c = (p2 - p0) / 2.0f;
                        r = (((a * f) + b) * f + c) * f + p1;
                        *out++ = r;
                    }
                    position->mValue += speed->mValue;
                    outlength--;
                } 
            }
            break;
        }

        /*
            32 BIT
        */
        case FMOD_SOUND_FORMAT_PCM32:
        {
            signed int *inptr = (signed int *)src;

            scale /= (float)(1U<<31);

            if (channels == 1)
            {
                float f;
                float p0,p1,p2,p3;
                float r1,r2,r3,r4;
                float a,b,c;
                int len;

                len = outlength >> 2;
                while (len)
                {
                    f = (float)position->mLo * OO4GIG;
                    p0 = (float)inptr[static_cast<ptrdiff_t>(position->mHi) - 1] * scale;
                    p1 = (float)inptr[static_cast<ptrdiff_t>(position->mHi) + 0] * scale;
                    p2 = (float)inptr[static_cast<ptrdiff_t>(position->mHi) + 1] * scale;
                    p3 = (float)inptr[static_cast<ptrdiff_t>(position->mHi) + 2] * scale;
                    a = (3.0f * (p1-p2) - p0 + p3) / 2.0f;
                    b = 2.0f * p2 + p0 - (5.0f * p1 + p3) / 2.0f;
                    c = (p2 - p0) / 2.0f;
                    r1 = (((a * f) + b) * f + c) * f + p1;
                    position->mValue += speed->mValue;

                    f = (float)position->mLo * OO4GIG;
                    p0 = (float)inptr[static_cast<ptrdiff_t>(position->mHi) - 1] * scale;
                    p1 = (float)inptr[static_cast<ptrdiff_t>(position->mHi) + 0] * scale;
                    p2 = (float)inptr[static_cast<ptrdiff_t>(position->mHi) + 1] * scale;
                    p3 = (float)inptr[static_cast<ptrdiff_t>(position->mHi) + 2] * scale;
                    a = (3.0f * (p1-p2) - p0 + p3) / 2.0f;
                    b = 2.0f * p2 + p0 - (5.0f * p1 + p3) / 2.0f;
                    c = (p2 - p0) / 2.0f;
                    r2 = (((a * f) + b) * f + c) * f + p1;
                    position->mValue += speed->mValue;

                    f = (float)position->mLo * OO4GIG;
                    p0 = (float)inptr[static_cast<ptrdiff_t>(position->mHi) - 1] * scale;
                    p1 = (float)inptr[static_cast<ptrdiff_t>(position->mHi) + 0] * scale;
                    p2 = (float)inptr[static_cast<ptrdiff_t>(position->mHi) + 1] * scale;
                    p3 = (float)inptr[static_cast<ptrdiff_t>(position->mHi) + 2] * scale;
                    a = (3.0f * (p1-p2) - p0 + p3) / 2.0f;
                    b = 2.0f * p2 + p0 - (5.0f * p1 + p3) / 2.0f;
                    c = (p2 - p0) / 2.0f;
                    r3 = (((a * f) + b) * f + c) * f + p1;
                    position->mValue += speed->mValue;

                    f = (float)position->mLo * OO4GIG;
                    p0 = (float)inptr[static_cast<ptrdiff_t>(position->mHi) - 1] * scale;
                    p1 = (float)inptr[static_cast<ptrdiff_t>(position->mHi) + 0] * scale;
                    p2 = (float)inptr[static_cast<ptrdiff_t>(position->mHi) + 1] * scale;
                    p3 = (float)inptr[static_cast<ptrdiff_t>(position->mHi) + 2] * scale;
                    a = (3.0f * (p1-p2) - p0 + p3) / 2.0f;
                    b = 2.0f * p2 + p0 - (5.0f * p1 + p3) / 2.0f;
                    c = (p2 - p0) / 2.0f;
                    r4 = (((a * f) + b) * f + c) * f + p1;
                    position->mValue += speed->mValue;

                    out[0] = r1;
                    out[1] = r2;
                    out[2] = r3;
                    out[3] = r4;
                    len--;
                    out+=4;
                } 

                len = outlength & 3;
                while (len)
                {
                    f = position->mLo * OO4GIG;
                    p0 = (float)inptr[static_cast<ptrdiff_t>(position->mHi) - 1] * scale;
                    p1 = (float)inptr[static_cast<ptrdiff_t>(position->mHi) + 0] * scale;
                    p2 = (float)inptr[static_cast<ptrdiff_t>(position->mHi) + 1] * scale;
                    p3 = (float)inptr[static_cast<ptrdiff_t>(position->mHi) + 2] * scale;
                    a = (3.0f * (p1-p2) - p0 + p3) / 2.0f;
                    b = 2.0f * p2 + p0 - (5.0f * p1 + p3) / 2.0f;
                    c = (p2 - p0) / 2.0f;
                    r1 = (((a * f) + b) * f + c) * f + p1;

                    *out++ = r1;

                    position->mValue += speed->mValue;
                    len--;
                } 
            }
            else
            {
                while (outlength)
                {
                    int count;
                    float p0,p1,p2,p3;
                    float r;
                    float a,b,c;
                    float f = position->mLo * OO4GIG;

                    for (count = 0; count < channels; count++)
                    {
                        p0 = (float)inptr[((static_cast<ptrdiff_t>(position->mHi) - 1) * channels) + count] * scale;
                        p1 = (float)inptr[((static_cast<ptrdiff_t>(position->mHi) + 0) * channels) + count] * scale;
                        p2 = (float)inptr[((static_cast<ptrdiff_t>(position->mHi) + 1) * channels) + count] * scale;
                        p3 = (float)inptr[((static_cast<ptrdiff_t>(position->mHi) + 2) * channels) + count] * scale;
                        a = (3.0f * (p1-p2) - p0 + p3) / 2.0f;
                        b = 2.0f * p2 + p0 - (5.0f * p1 + p3) / 2.0f;
                        c = (p2 - p0) / 2.0f;
                        r = (((a * f) + b) * f + c) * f + p1;
                        *out++ = r;
                    }
                    position->mValue += speed->mValue;
                    outlength--;
                } 
            }
            break;
        }

        /*
            FLOATING POINT.
        */
        case FMOD_SOUND_FORMAT_PCMFLOAT:
        {
            float *inptr = (float *)src;

            if (channels == 1)
            {
                float f;
                float p0,p1,p2,p3;
                float r1,r2,r3,r4;
                float a,b,c;
                int len;

                len = outlength >> 2;
                while (len)
                {
                    f = (float)position->mLo * OO4GIG;
                    p0 = inptr[static_cast<ptrdiff_t>(position->mHi) - 1];
                    p1 = inptr[static_cast<ptrdiff_t>(position->mHi) + 0];
                    p2 = inptr[static_cast<ptrdiff_t>(position->mHi) + 1];
                    p3 = inptr[static_cast<ptrdiff_t>(position->mHi) + 2];
                    a = (3.0f * (p1-p2) - p0 + p3) / 2.0f;
                    b = 2.0f * p2 + p0 - (5.0f * p1 + p3) / 2.0f;
                    c = (p2 - p0) / 2.0f;
                    r1 = (((a * f) + b) * f + c) * f + p1;
                    position->mValue += speed->mValue;

                    f = (float)position->mLo * OO4GIG;
                    p0 = inptr[static_cast<ptrdiff_t>(position->mHi) - 1];
                    p1 = inptr[static_cast<ptrdiff_t>(position->mHi) + 0];
                    p2 = inptr[static_cast<ptrdiff_t>(position->mHi) + 1];
                    p3 = inptr[static_cast<ptrdiff_t>(position->mHi) + 2];
                    a = (3.0f * (p1-p2) - p0 + p3) / 2.0f;
                    b = 2.0f * p2 + p0 - (5.0f * p1 + p3) / 2.0f;
                    c = (p2 - p0) / 2.0f;
                    r2 = (((a * f) + b) * f + c) * f + p1;
                    position->mValue += speed->mValue;

                    f = (float)position->mLo * OO4GIG;
                    p0 = inptr[static_cast<ptrdiff_t>(position->mHi) - 1];
                    p1 = inptr[static_cast<ptrdiff_t>(position->mHi) + 0];
                    p2 = inptr[static_cast<ptrdiff_t>(position->mHi) + 1];
                    p3 = inptr[static_cast<ptrdiff_t>(position->mHi) + 2];
                    a = (3.0f * (p1-p2) - p0 + p3) / 2.0f;
                    b = 2.0f * p2 + p0 - (5.0f * p1 + p3) / 2.0f;
                    c = (p2 - p0) / 2.0f;
                    r3 = (((a * f) + b) * f + c) * f + p1;
                    position->mValue += speed->mValue;

                    f = (float)position->mLo * OO4GIG;
                    p0 = inptr[static_cast<ptrdiff_t>(position->mHi) - 1];
                    p1 = inptr[static_cast<ptrdiff_t>(position->mHi) + 0];
                    p2 = inptr[static_cast<ptrdiff_t>(position->mHi) + 1];
                    p3 = inptr[static_cast<ptrdiff_t>(position->mHi) + 2];
                    a = (3.0f * (p1-p2) - p0 + p3) / 2.0f;
                    b = 2.0f * p2 + p0 - (5.0f * p1 + p3) / 2.0f;
                    c = (p2 - p0) / 2.0f;
                    r4 = (((a * f) + b) * f + c) * f + p1;
                    position->mValue += speed->mValue;

                    out[0] = r1;
                    out[1] = r2;
                    out[2] = r3;
                    out[3] = r4;
                    len--;
                    out+=4;
                } 

                len = outlength & 3;
                while (len)
                {
                    f = position->mLo * OO4GIG;
                    p0 = inptr[static_cast<ptrdiff_t>(position->mHi) - 1];
                    p1 = inptr[static_cast<ptrdiff_t>(position->mHi) + 0];
                    p2 = inptr[static_cast<ptrdiff_t>(position->mHi) + 1];
                    p3 = inptr[static_cast<ptrdiff_t>(position->mHi) + 2];
                    a = (3.0f * (p1-p2) - p0 + p3) / 2.0f;
                    b = 2.0f * p2 + p0 - (5.0f * p1 + p3) / 2.0f;
                    c = (p2 - p0) / 2.0f;
                    r1 = (((a * f) + b) * f + c) * f + p1;

                    *out++ = r1;

                    position->mValue += speed->mValue;
                    len--;
                } 
            }
            else
            {
                while (outlength)
                {
                    int count;
                    float p0,p1,p2,p3;
                    float r;
                    float a,b,c;
                    float f = position->mLo * OO4GIG;

                    for (count = 0; count < channels; count++)
                    {
                        p0 = inptr[((static_cast<ptrdiff_t>(position->mHi) - 1) * channels) + count];
                        p1 = inptr[((static_cast<ptrdiff_t>(position->mHi) + 0) * channels) + count];
                        p2 = inptr[((static_cast<ptrdiff_t>(position->mHi) + 1) * channels) + count];
                        p3 = inptr[((static_cast<ptrdiff_t>(position->mHi) + 2) * channels) + count];
                        a = (3.0f * (p1-p2) - p0 + p3) / 2.0f;
                        b = 2.0f * p2 + p0 - (5.0f * p1 + p3) / 2.0f;
                        c = (p2 - p0) / 2.0f;
                        r = (((a * f) + b) * f + c) * f + p1;
                        *out++ = r;
                    }
                    position->mValue += speed->mValue;
                    outlength--;
                } 
            }
            break;
        }
        default:
        {
            break;
        }
    };
}



