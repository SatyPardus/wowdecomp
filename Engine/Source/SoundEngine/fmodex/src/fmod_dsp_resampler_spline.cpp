#include "fmod_settings.h"
#include <stddef.h>

#include "fmod_dsp_resampler_spline.h"

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
// VERIFY: fc91bda7 manual
// OFFSET: 0x0093F9B0
void FMOD_Resampler_Spline(float *out, int outlength, void *src, FMOD_SOUND_FORMAT srcformat, FMOD_UINT64P *position, FMOD_SINT64P *speed, int channels)
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
                while (outlength)
                {
                    float f = position->mLo * OO4GIG;
                    float r;
                    float p0 = (float)inptr[static_cast<ptrdiff_t>(position->mHi) - 2] * scale;
                    float p1 = (float)inptr[static_cast<ptrdiff_t>(position->mHi) - 1] * scale;
                    float p2 = (float)inptr[static_cast<ptrdiff_t>(position->mHi) + 0] * scale;
                    float p3 = (float)inptr[static_cast<ptrdiff_t>(position->mHi) + 1] * scale;
                    float p4 = (float)inptr[static_cast<ptrdiff_t>(position->mHi) + 2] * scale;
                    float p5 = (float)inptr[static_cast<ptrdiff_t>(position->mHi) + 3] * scale;

                    r = p2 + 0.04166666666f * f * ((p3-p1) * 16.0f+(p0-p4) * 2.0f
                        + f * ((p3 + p1) * 16.0f - p0 - p2 * 30.0f - p4
                        + f * (p3 *  66.0f - p2 *  70.0f - p4 * 33.0f + p1 * 39.0f + p5 *  7.0f - p0 * 9.0f
                        + f * (p2 * 126.0f - p3 * 124.0f + p4 * 61.0f - p1 * 64.0f - p5 * 12.0f + p0 * 13.0f
                        + f * ((p3 - p2) * 50.0f + (p1 - p4) * 25.0f + (p5 - p0) * 5.0f)))));

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
                    float f = position->mLo * OO4GIG;

                    for (count = 0; count < channels; count++)
                    {
                        float r;
                        float p0 = (float)inptr[((static_cast<ptrdiff_t>(position->mHi) - 2) * channels) + count] * scale;
                        float p1 = (float)inptr[((static_cast<ptrdiff_t>(position->mHi) - 1) * channels) + count] * scale;
                        float p2 = (float)inptr[((static_cast<ptrdiff_t>(position->mHi) + 0) * channels) + count] * scale;
                        float p3 = (float)inptr[((static_cast<ptrdiff_t>(position->mHi) + 1) * channels) + count] * scale;
                        float p4 = (float)inptr[((static_cast<ptrdiff_t>(position->mHi) + 2) * channels) + count] * scale;
                        float p5 = (float)inptr[((static_cast<ptrdiff_t>(position->mHi) + 3) * channels) + count] * scale;

                        r = p2 + 0.04166666666f * f * ((p3-p1) * 16.0f+(p0-p4) * 2.0f
                            + f * ((p3 + p1) * 16.0f - p0 - p2 * 30.0f - p4
                            + f * (p3 *  66.0f - p2 *  70.0f - p4 * 33.0f + p1 * 39.0f + p5 *  7.0f - p0 * 9.0f
                            + f * (p2 * 126.0f - p3 * 124.0f + p4 * 61.0f - p1 * 64.0f - p5 * 12.0f + p0 * 13.0f
                            + f * ((p3 - p2) * 50.0f + (p1 - p4) * 25.0f + (p5 - p0) * 5.0f)))));

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
                while (outlength)
                {
                    float f = position->mLo * OO4GIG;
                    float r;
                    float p0 = (float)inptr[static_cast<ptrdiff_t>(position->mHi) - 2] * scale;
                    float p1 = (float)inptr[static_cast<ptrdiff_t>(position->mHi) - 1] * scale;
                    float p2 = (float)inptr[static_cast<ptrdiff_t>(position->mHi) + 0] * scale;
                    float p3 = (float)inptr[static_cast<ptrdiff_t>(position->mHi) + 1] * scale;
                    float p4 = (float)inptr[static_cast<ptrdiff_t>(position->mHi) + 2] * scale;
                    float p5 = (float)inptr[static_cast<ptrdiff_t>(position->mHi) + 3] * scale;

                    r = p2 + 0.04166666666f * f * ((p3-p1) * 16.0f+(p0-p4) * 2.0f
                        + f * ((p3 + p1) * 16.0f - p0 - p2 * 30.0f - p4
                        + f * (p3 *  66.0f - p2 *  70.0f - p4 * 33.0f + p1 * 39.0f + p5 *  7.0f - p0 * 9.0f
                        + f * (p2 * 126.0f - p3 * 124.0f + p4 * 61.0f - p1 * 64.0f - p5 * 12.0f + p0 * 13.0f
                        + f * ((p3 - p2) * 50.0f + (p1 - p4) * 25.0f + (p5 - p0) * 5.0f)))));

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
                    float f = position->mLo * OO4GIG;

                    for (count = 0; count < channels; count++)
                    {
                        float r;
                        float p0 = (float)inptr[((static_cast<ptrdiff_t>(position->mHi) - 2) * channels) + count] * scale;
                        float p1 = (float)inptr[((static_cast<ptrdiff_t>(position->mHi) - 1) * channels) + count] * scale;
                        float p2 = (float)inptr[((static_cast<ptrdiff_t>(position->mHi) + 0) * channels) + count] * scale;
                        float p3 = (float)inptr[((static_cast<ptrdiff_t>(position->mHi) + 1) * channels) + count] * scale;
                        float p4 = (float)inptr[((static_cast<ptrdiff_t>(position->mHi) + 2) * channels) + count] * scale;
                        float p5 = (float)inptr[((static_cast<ptrdiff_t>(position->mHi) + 3) * channels) + count] * scale;

                        r = p2 + 0.04166666666f * f * ((p3-p1) * 16.0f+(p0-p4) * 2.0f
                            + f * ((p3 + p1) * 16.0f - p0 - p2 * 30.0f - p4
                            + f * (p3 *  66.0f - p2 *  70.0f - p4 * 33.0f + p1 * 39.0f + p5 *  7.0f - p0 * 9.0f
                            + f * (p2 * 126.0f - p3 * 124.0f + p4 * 61.0f - p1 * 64.0f - p5 * 12.0f + p0 * 13.0f
                            + f * ((p3 - p2) * 50.0f + (p1 - p4) * 25.0f + (p5 - p0) * 5.0f)))));

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
                    float f = position->mLo * OO4GIG;
                    float r;
                    FMOD_INT24 *s0 = &inptr[static_cast<ptrdiff_t>(position->mHi) - 2];
                    FMOD_INT24 *s1 = &inptr[static_cast<ptrdiff_t>(position->mHi) - 1];
                    FMOD_INT24 *s2 = &inptr[static_cast<ptrdiff_t>(position->mHi) + 0];
                    FMOD_INT24 *s3 = &inptr[static_cast<ptrdiff_t>(position->mHi) + 1];
                    FMOD_INT24 *s4 = &inptr[static_cast<ptrdiff_t>(position->mHi) + 2];
                    FMOD_INT24 *s5 = &inptr[static_cast<ptrdiff_t>(position->mHi) + 3];
                    float  p0 = (float)((int)(((unsigned int)s0->val[0] <<  8) | ((unsigned int)s0->val[1] << 16) | ((unsigned int)s0->val[2] << 24)) >> 8) * scale;
                    float  p1 = (float)((int)(((unsigned int)s1->val[0] <<  8) | ((unsigned int)s1->val[1] << 16) | ((unsigned int)s1->val[2] << 24)) >> 8) * scale;
                    float  p2 = (float)((int)(((unsigned int)s2->val[0] <<  8) | ((unsigned int)s2->val[1] << 16) | ((unsigned int)s2->val[2] << 24)) >> 8) * scale;
                    float  p3 = (float)((int)(((unsigned int)s3->val[0] <<  8) | ((unsigned int)s3->val[1] << 16) | ((unsigned int)s3->val[2] << 24)) >> 8) * scale;
                    float  p4 = (float)((int)(((unsigned int)s4->val[0] <<  8) | ((unsigned int)s4->val[1] << 16) | ((unsigned int)s4->val[2] << 24)) >> 8) * scale;
                    float  p5 = (float)((int)(((unsigned int)s5->val[0] <<  8) | ((unsigned int)s5->val[1] << 16) | ((unsigned int)s5->val[2] << 24)) >> 8) * scale;


                    r = p2 + 0.04166666666f * f * ((p3-p1) * 16.0f+(p0-p4) * 2.0f
                        + f * ((p3 + p1) * 16.0f - p0 - p2 * 30.0f - p4
                        + f * (p3 *  66.0f - p2 *  70.0f - p4 * 33.0f + p1 * 39.0f + p5 *  7.0f - p0 * 9.0f
                        + f * (p2 * 126.0f - p3 * 124.0f + p4 * 61.0f - p1 * 64.0f - p5 * 12.0f + p0 * 13.0f
                        + f * ((p3 - p2) * 50.0f + (p1 - p4) * 25.0f + (p5 - p0) * 5.0f)))));

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
                    float f = position->mLo * OO4GIG;

                    for (count = 0; count < channels; count++)
                    {
                        float r;
                        FMOD_INT24 *s0 = &inptr[((static_cast<ptrdiff_t>(position->mHi) - 2) * channels) + count];
                        FMOD_INT24 *s1 = &inptr[((static_cast<ptrdiff_t>(position->mHi) - 1) * channels) + count];
                        FMOD_INT24 *s2 = &inptr[((static_cast<ptrdiff_t>(position->mHi) + 0) * channels) + count];
                        FMOD_INT24 *s3 = &inptr[((static_cast<ptrdiff_t>(position->mHi) + 1) * channels) + count];
                        FMOD_INT24 *s4 = &inptr[((static_cast<ptrdiff_t>(position->mHi) + 2) * channels) + count];
                        FMOD_INT24 *s5 = &inptr[((static_cast<ptrdiff_t>(position->mHi) + 3) * channels) + count];
                        float  p0 = (float)((int)(((unsigned int)s0->val[0] <<  8) | ((unsigned int)s0->val[1] << 16) | ((unsigned int)s0->val[2] << 24)) >> 8) * scale;
                        float  p1 = (float)((int)(((unsigned int)s1->val[0] <<  8) | ((unsigned int)s1->val[1] << 16) | ((unsigned int)s1->val[2] << 24)) >> 8) * scale;
                        float  p2 = (float)((int)(((unsigned int)s2->val[0] <<  8) | ((unsigned int)s2->val[1] << 16) | ((unsigned int)s2->val[2] << 24)) >> 8) * scale;
                        float  p3 = (float)((int)(((unsigned int)s3->val[0] <<  8) | ((unsigned int)s3->val[1] << 16) | ((unsigned int)s3->val[2] << 24)) >> 8) * scale;
                        float  p4 = (float)((int)(((unsigned int)s4->val[0] <<  8) | ((unsigned int)s4->val[1] << 16) | ((unsigned int)s4->val[2] << 24)) >> 8) * scale;
                        float  p5 = (float)((int)(((unsigned int)s5->val[0] <<  8) | ((unsigned int)s5->val[1] << 16) | ((unsigned int)s5->val[2] << 24)) >> 8) * scale;

                        r = p2 + 0.04166666666f * f * ((p3-p1) * 16.0f+(p0-p4) * 2.0f
                            + f * ((p3 + p1) * 16.0f - p0 - p2 * 30.0f - p4
                            + f * (p3 *  66.0f - p2 *  70.0f - p4 * 33.0f + p1 * 39.0f + p5 *  7.0f - p0 * 9.0f
                            + f * (p2 * 126.0f - p3 * 124.0f + p4 * 61.0f - p1 * 64.0f - p5 * 12.0f + p0 * 13.0f
                            + f * ((p3 - p2) * 50.0f + (p1 - p4) * 25.0f + (p5 - p0) * 5.0f)))));

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
                while (outlength)
                {
                    float f = position->mLo * OO4GIG;
                    float r;
                    float p0 = (float)inptr[static_cast<ptrdiff_t>(position->mHi) - 2] * scale;
                    float p1 = (float)inptr[static_cast<ptrdiff_t>(position->mHi) - 1] * scale;
                    float p2 = (float)inptr[static_cast<ptrdiff_t>(position->mHi) + 0] * scale;
                    float p3 = (float)inptr[static_cast<ptrdiff_t>(position->mHi) + 1] * scale;
                    float p4 = (float)inptr[static_cast<ptrdiff_t>(position->mHi) + 2] * scale;
                    float p5 = (float)inptr[static_cast<ptrdiff_t>(position->mHi) + 3] * scale;

                    r = p2 + 0.04166666666f * f * ((p3-p1) * 16.0f+(p0-p4) * 2.0f
                        + f * ((p3 + p1) * 16.0f - p0 - p2 * 30.0f - p4
                        + f * (p3 *  66.0f - p2 *  70.0f - p4 * 33.0f + p1 * 39.0f + p5 *  7.0f - p0 * 9.0f
                        + f * (p2 * 126.0f - p3 * 124.0f + p4 * 61.0f - p1 * 64.0f - p5 * 12.0f + p0 * 13.0f
                        + f * ((p3 - p2) * 50.0f + (p1 - p4) * 25.0f + (p5 - p0) * 5.0f)))));

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
                    float f = position->mLo * OO4GIG;

                    for (count = 0; count < channels; count++)
                    {
                        float r;
                        float p0 = (float)inptr[((static_cast<ptrdiff_t>(position->mHi) - 2) * channels) + count] * scale;
                        float p1 = (float)inptr[((static_cast<ptrdiff_t>(position->mHi) - 1) * channels) + count] * scale;
                        float p2 = (float)inptr[((static_cast<ptrdiff_t>(position->mHi) + 0) * channels) + count] * scale;
                        float p3 = (float)inptr[((static_cast<ptrdiff_t>(position->mHi) + 1) * channels) + count] * scale;
                        float p4 = (float)inptr[((static_cast<ptrdiff_t>(position->mHi) + 2) * channels) + count] * scale;
                        float p5 = (float)inptr[((static_cast<ptrdiff_t>(position->mHi) + 3) * channels) + count] * scale;

                        r = p2 + 0.04166666666f * f * ((p3-p1) * 16.0f+(p0-p4) * 2.0f
                            + f * ((p3 + p1) * 16.0f - p0 - p2 * 30.0f - p4
                            + f * (p3 *  66.0f - p2 *  70.0f - p4 * 33.0f + p1 * 39.0f + p5 *  7.0f - p0 * 9.0f
                            + f * (p2 * 126.0f - p3 * 124.0f + p4 * 61.0f - p1 * 64.0f - p5 * 12.0f + p0 * 13.0f
                            + f * ((p3 - p2) * 50.0f + (p1 - p4) * 25.0f + (p5 - p0) * 5.0f)))));

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
                while (outlength)
                {
                    float f = position->mLo * OO4GIG;
                    float r;
                    float p0 = inptr[static_cast<ptrdiff_t>(position->mHi) - 2];
                    float p1 = inptr[static_cast<ptrdiff_t>(position->mHi) - 1];
                    float p2 = inptr[static_cast<ptrdiff_t>(position->mHi) + 0];
                    float p3 = inptr[static_cast<ptrdiff_t>(position->mHi) + 1];
                    float p4 = inptr[static_cast<ptrdiff_t>(position->mHi) + 2];
                    float p5 = inptr[static_cast<ptrdiff_t>(position->mHi) + 3];

                    r = p2 + 0.04166666666f * f * ((p3-p1) * 16.0f+(p0-p4) * 2.0f
                        + f * ((p3 + p1) * 16.0f - p0 - p2 * 30.0f - p4
                        + f * (p3 *  66.0f - p2 *  70.0f - p4 * 33.0f + p1 * 39.0f + p5 *  7.0f - p0 * 9.0f
                        + f * (p2 * 126.0f - p3 * 124.0f + p4 * 61.0f - p1 * 64.0f - p5 * 12.0f + p0 * 13.0f
                        + f * ((p3 - p2) * 50.0f + (p1 - p4) * 25.0f + (p5 - p0) * 5.0f)))));

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
                    float f = position->mLo * OO4GIG;

                    for (count = 0; count < channels; count++)
                    {
                        float r;
                        float p0 = inptr[((static_cast<ptrdiff_t>(position->mHi) - 2) * channels) + count];
                        float p1 = inptr[((static_cast<ptrdiff_t>(position->mHi) - 1) * channels) + count];
                        float p2 = inptr[((static_cast<ptrdiff_t>(position->mHi) + 0) * channels) + count];
                        float p3 = inptr[((static_cast<ptrdiff_t>(position->mHi) + 1) * channels) + count];
                        float p4 = inptr[((static_cast<ptrdiff_t>(position->mHi) + 2) * channels) + count];
                        float p5 = inptr[((static_cast<ptrdiff_t>(position->mHi) + 3) * channels) + count];

                        r = p2 + 0.04166666666f * f * ((p3-p1) * 16.0f+(p0-p4) * 2.0f
                            + f * ((p3 + p1) * 16.0f - p0 - p2 * 30.0f - p4
                            + f * (p3 *  66.0f - p2 *  70.0f - p4 * 33.0f + p1 * 39.0f + p5 *  7.0f - p0 * 9.0f
                            + f * (p2 * 126.0f - p3 * 124.0f + p4 * 61.0f - p1 * 64.0f - p5 * 12.0f + p0 * 13.0f
                            + f * ((p3 - p2) * 50.0f + (p1 - p4) * 25.0f + (p5 - p0) * 5.0f)))));

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



