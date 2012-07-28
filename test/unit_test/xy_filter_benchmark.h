#ifndef __XY_FILTER_BENCHMARK_F0ED9F75_C0A7_4B0E_910E_7A82ECE2D7AD_H__
#define __XY_FILTER_BENCHMARK_F0ED9F75_C0A7_4B0E_910E_7A82ECE2D7AD_H__

#include <gtest/gtest.h>
#include <wtypes.h>
#include <math.h>
#include <xmmintrin.h>

typedef const UINT8 CUINT8, *PCUINT8;

//////////////////////////////////////////////////////////////////////

// transpose test

void xy_filter_c_v0(float *dst, int width, int height, int stride, const float *filter, int filter_width)
{
    ASSERT( stride>=4*(width+filter_width) );

    int xx_fix = width > filter_width ? 0 : filter_width - width;
    const float *filter_start = filter;

    BYTE* end = reinterpret_cast<BYTE*>(dst) + height*stride;
    BYTE* dst_byte = reinterpret_cast<BYTE*>(dst);
    for( ; dst_byte<end; dst_byte+=stride )
    {
        float *dst_f = reinterpret_cast<float*>(dst_byte); 
        float *dst2 = dst_f - filter_width;
        float *dst_endr = dst_f + width;
        float *dst_end0 = dst_endr - filter_width;
        float *dst_endl = dst_f - xx_fix;
        ASSERT(xx_fix==0 || dst_end0==dst_endl);

        ASSERT(filter_start == filter);
        filter_start += filter_width;
        const float *filter_end = filter_start;
        for (;dst2<dst_endl;dst2++, filter_start--)//left margin
        {
            const float *src = dst_f;
            float sum = 0;
            for(const float* f=filter_start;f<filter_end;f++, src++)
            {
                sum += src[0] * f[0];
            }
            *dst2 = sum;
        }
        for (;dst2<dst_f;dst2++, filter_start--, filter_end--)//if width < filter_width
        {
            const float *src = dst_f;
            float sum = 0;
            for(const float* f=filter_start;f<filter_end;f++, src++)
            {
                sum += src[0] * f[0];
            }
            *dst2 = sum;
        }
        ASSERT(filter_start==filter);
        for (;dst2<dst_end0;dst2++)
        {
            const float *src = dst2;

            float sum = 0;
            for(const float* f=filter_start;f<filter_end;f++, src++)
            {
                sum += src[0] * f[0];
            }
            *dst2 = sum;
        }
        for (;dst2<dst_endr;dst2++, filter_end--)//right margin
        {
            const float *src = dst2;
            float sum = 0;
            for(const float* f=filter;f<filter_end;f++, src++)
            {
                sum += src[0] * f[0];
            }
            *dst2 = sum;
        }
    }
}

void xy_filter_sse_v0(float *dst, int width, int height, int stride, const float *filter, int filter_width)
{
#ifdef XY_FILTER_4
#  undef XY_FILTER_4
#endif
    ASSERT( stride>=4*(width+filter_width) );
    ASSERT( ((stride|(4*width)|(4*filter_width)|reinterpret_cast<int>(dst)|reinterpret_cast<int>(filter))&15)==0 );

    int xx_fix = width > filter_width ? 0 : filter_width - width;
    const float *filter_start = filter;
    BYTE* dst_byte = reinterpret_cast<BYTE*>(dst);
    BYTE* end = dst_byte + height*stride;
    for( ; dst_byte<end; dst_byte+=stride )
    {
        float *dst_f = reinterpret_cast<float*>(dst_byte); 
        float *dst2 = dst_f - filter_width;
        float *dst_endr = dst_f + width;
        float *dst_end0 = dst_endr - filter_width;
        float *dst_endl = dst_f - xx_fix;
        ASSERT(xx_fix==0 || dst_end0==dst_endl);

        ASSERT(filter_start == filter);
        filter_start += filter_width;
        const float *filter_end = filter_start;

        for (;dst2<dst_endl;dst2+=4)//left margin
        {
            const float *src = dst_f;
            filter_start -= 4;

            //filter 4
            __m128 src4 = _mm_setzero_ps();/*1 2 3 4*/
            __m128 sum = _mm_setzero_ps();
            for(const float* f=filter_start;f<filter_end;f+=4,src+=4)
            {   
                __m128 src_5_8 = _mm_load_ps(src);/*5 6 7 8*/
                __m128 f4 = _mm_load_ps(f);

#define XY_FILTER_4(src4, src_5_8, f4, sum) \
    __m128 f4_1 = _mm_unpacklo_ps(f4,f4);\
    f4_1 = _mm_unpacklo_ps(f4_1, f4_1);\
    f4_1 = _mm_mul_ps(f4_1, src4);\
    sum = _mm_add_ps(sum, f4_1);\
    __m128 src_3_6 = _mm_shuffle_ps(src4, src_5_8, _MM_SHUFFLE(1,0,3,2));/*3 4 5 6*/\
    f4_1 = _mm_unpackhi_ps(f4,f4);\
    f4_1 = _mm_unpacklo_ps(f4_1,f4_1);\
    f4_1 = _mm_mul_ps(f4_1, src_3_6);\
    sum = _mm_add_ps(sum, f4_1);\
    src4 = _mm_shuffle_ps(src4, src_3_6, _MM_SHUFFLE(2,1,2,1));/*2 3 4 5*/\
    f4_1 = _mm_unpacklo_ps(f4,f4);\
    f4_1 = _mm_unpackhi_ps(f4_1, f4_1);\
    f4_1 = _mm_mul_ps(f4_1, src4);\
    sum = _mm_add_ps(sum, f4_1);\
    src_3_6 = _mm_shuffle_ps(src_3_6, src_5_8, _MM_SHUFFLE(2,1,2,1));/*4 5 6 7*/\
    f4_1 = _mm_unpackhi_ps(f4,f4);\
    f4_1 = _mm_unpackhi_ps(f4_1, f4_1);\
    f4_1 = _mm_mul_ps(f4_1, src_3_6);\
    sum = _mm_add_ps(sum, f4_1)

                { XY_FILTER_4(src4, src_5_8, f4, sum); }
                src4 = src_5_8;
            }
            //store result
            _mm_stream_ps(dst2, sum);
        }
        for (;dst2<dst_f;dst2+=4)//if width < filter_width
        {
            const float *src = dst_f;
            filter_start-=4;
            filter_end-=4;

            __m128 src4 = _mm_setzero_ps();/*1 2 3 4*/
            __m128 sum = _mm_setzero_ps();
            __m128 src_5_8, f4;
            for(const float* f=filter_start;f<filter_end;f+=4,src+=4)
            {   
                src_5_8 = _mm_load_ps(src);/*5 6 7 8*/
                f4 = _mm_load_ps(f);

                { XY_FILTER_4(src4, src_5_8, f4, sum); }
                src4 = src_5_8;
            }
            src_5_8 = _mm_setzero_ps();
            f4 = _mm_load_ps(filter_end);
            { XY_FILTER_4(src4, src_5_8, f4, sum); }
            //store result
            _mm_stream_ps(dst2, sum);
        }
        ASSERT(filter_start == filter);
        for (;dst2<dst_end0;dst2+=4)
        {
            const float *src = dst2;

            //filter 4
            __m128 src4 = _mm_load_ps(src);/*1 2 3 4*/
            __m128 sum = _mm_setzero_ps();
            for(const float* f=filter_start;f<filter_end;f+=4)
            {
                src+=4;
                __m128 src_5_8 = _mm_load_ps(src);/*5 6 7 8*/
                __m128 f4 = _mm_load_ps(f);

                { XY_FILTER_4(src4, src_5_8, f4, sum); }
                src4 = src_5_8;
            }
            //store result
            _mm_stream_ps(dst2, sum);
        }
        for (;dst2<dst_endr;dst2+=4)//right margin
        {
            const float *src = dst2;
            filter_end-=4;

            //filter 4
            __m128 src4 = _mm_load_ps(src);//1 2 3 4
            __m128 sum = _mm_setzero_ps();
            __m128 src_5_8, f4;
            for(const float* f=filter_start;f<filter_end;f+=4)
            {
                src+=4;
                src_5_8 = _mm_load_ps(src);//5 6 7 8
                f4 = _mm_load_ps(f);

                { XY_FILTER_4(src4, src_5_8, f4, sum); }

                src4 = src_5_8;
                //move new 4 in_n_out to old 4 in_n_out
            }
            src_5_8 = _mm_setzero_ps();
            f4 = _mm_load_ps(filter_end);
            { XY_FILTER_4(src4, src_5_8, f4, sum); }
            //store result
            _mm_stream_ps(dst2, sum);
        }
    }
#undef XY_FILTER_4
}

void xy_filter_sse_v1(float *dst, int width, int height, int stride, const float *filter, int filter_width)
{
#ifdef XY_FILTER_4
#  undef XY_FILTER_4
#endif
    ASSERT( stride>=4*(width+filter_width) );
    ASSERT( ((stride|(4*width)|(4*filter_width)|reinterpret_cast<int>(dst)|reinterpret_cast<int>(filter))&15)==0 );

    int xx_fix = width > filter_width ? 0 : filter_width - width;
    const float *filter_start = filter;
    BYTE* dst_byte = reinterpret_cast<BYTE*>(dst);
    BYTE* end = dst_byte + height*stride;
    for( ; dst_byte<end; dst_byte+=stride )
    {
        float *dst_f = reinterpret_cast<float*>(dst_byte); 
        float *dst2 = dst_f - filter_width;
        float *dst_endr = dst_f + width;
        float *dst_end0 = dst_endr - filter_width;
        float *dst_endl = dst_f - xx_fix;
        ASSERT(xx_fix==0 || dst_end0==dst_endl);

        ASSERT(filter_start == filter);
        filter_start += filter_width;
        const float *filter_end = filter_start;

        for (;dst2<dst_endl;dst2+=4)//left margin
        {
            const float *src = dst_f;
            filter_start -= 4;

            //filter 4
            __m128 src4 = _mm_setzero_ps();/*1 2 3 4*/
            __m128 sum = _mm_setzero_ps();
            for(const float* f=filter_start;f<filter_end;f+=4,src+=4)
            {   
                __m128 src_5_8 = _mm_load_ps(src);/*5 6 7 8*/
                __m128 f4 = _mm_load_ps(f);

#define XY_FILTER_4(src4, src_5_8, f4, sum) \
    __m128 f4_1 = _mm_shuffle_ps(f4, f4, _MM_SHUFFLE(0,0,0,0));\
    f4_1 = _mm_mul_ps(f4_1, src4);\
    sum = _mm_add_ps(sum, f4_1);\
    __m128 src_3_6 = _mm_shuffle_ps(src4, src_5_8, _MM_SHUFFLE(1,0,3,2));/*3 4 5 6*/\
    f4_1 = _mm_shuffle_ps(f4, f4, _MM_SHUFFLE(2,2,2,2));\
    f4_1 = _mm_mul_ps(f4_1, src_3_6);\
    sum = _mm_add_ps(sum, f4_1);\
    src4 = _mm_shuffle_ps(src4, src_3_6, _MM_SHUFFLE(2,1,2,1));/*2 3 4 5*/\
    f4_1 = _mm_shuffle_ps(f4, f4, _MM_SHUFFLE(1,1,1,1));\
    f4_1 = _mm_mul_ps(f4_1, src4);\
    sum = _mm_add_ps(sum, f4_1);\
    src_3_6 = _mm_shuffle_ps(src_3_6, src_5_8, _MM_SHUFFLE(2,1,2,1));/*4 5 6 7*/\
    f4_1 = _mm_shuffle_ps(f4, f4, _MM_SHUFFLE(3,3,3,3));\
    f4_1 = _mm_mul_ps(f4_1, src_3_6);\
    sum = _mm_add_ps(sum, f4_1)

                { XY_FILTER_4(src4, src_5_8, f4, sum); }
                src4 = src_5_8;
            }
            //store result
            _mm_stream_ps(dst2, sum);
        }
        for (;dst2<dst_f;dst2+=4)//if width < filter_width
        {
            const float *src = dst_f;
            filter_start-=4;
            filter_end-=4;

            __m128 src4 = _mm_setzero_ps();/*1 2 3 4*/
            __m128 sum = _mm_setzero_ps();
            __m128 src_5_8, f4;
            for(const float* f=filter_start;f<filter_end;f+=4,src+=4)
            {   
                src_5_8 = _mm_load_ps(src);/*5 6 7 8*/
                f4 = _mm_load_ps(f);

                { XY_FILTER_4(src4, src_5_8, f4, sum); }
                src4 = src_5_8;
            }
            src_5_8 = _mm_setzero_ps();
            f4 = _mm_load_ps(filter_end);
            { XY_FILTER_4(src4, src_5_8, f4, sum); }
            //store result
            _mm_stream_ps(dst2, sum);
        }
        ASSERT(filter_start == filter);
        for (;dst2<dst_end0;dst2+=4)
        {
            const float *src = dst2;

            //filter 4
            __m128 src4 = _mm_load_ps(src);/*1 2 3 4*/
            __m128 sum = _mm_setzero_ps();
            for(const float* f=filter_start;f<filter_end;f+=4)
            {
                src+=4;
                __m128 src_5_8 = _mm_load_ps(src);/*5 6 7 8*/
                __m128 f4 = _mm_load_ps(f);

                { XY_FILTER_4(src4, src_5_8, f4, sum); }
                src4 = src_5_8;
            }
            //store result
            _mm_stream_ps(dst2, sum);
        }
        for (;dst2<dst_endr;dst2+=4)//right margin
        {
            const float *src = dst2;
            filter_end-=4;

            //filter 4
            __m128 src4 = _mm_load_ps(src);//1 2 3 4
            __m128 sum = _mm_setzero_ps();
            __m128 src_5_8, f4;
            for(const float* f=filter_start;f<filter_end;f+=4)
            {
                src+=4;
                src_5_8 = _mm_load_ps(src);//5 6 7 8
                f4 = _mm_load_ps(f);

                { XY_FILTER_4(src4, src_5_8, f4, sum); }

                src4 = src_5_8;
                //move new 4 in_n_out to old 4 in_n_out
            }
            src_5_8 = _mm_setzero_ps();
            f4 = _mm_load_ps(filter_end);
            { XY_FILTER_4(src4, src_5_8, f4, sum); }
            //store result
            _mm_stream_ps(dst2, sum);
        }
    }
#undef XY_FILTER_4
}

void xy_filter_sse_v2(float *dst, int width, int height, int stride, const float *filter, int filter_width)
{
#ifdef XY_FILTER_4
#  undef XY_FILTER_4
#endif
    ASSERT( stride>=4*(width+filter_width) );
    ASSERT( ((stride|(4*width)|(4*filter_width)|reinterpret_cast<int>(dst)|reinterpret_cast<int>(filter))&15)==0 );

    int xx_fix = width > filter_width ? 0 : filter_width - width;
    const float *filter_start = filter;
    BYTE* dst_byte = reinterpret_cast<BYTE*>(dst);
    BYTE* end = dst_byte + height*stride;
    for( ; dst_byte<end; dst_byte+=stride )
    {
        float *dst_f = reinterpret_cast<float*>(dst_byte); 
        float *dst2 = dst_f - filter_width;
        float *dst_endr = dst_f + width;
        float *dst_end0 = dst_endr - filter_width;
        float *dst_endl = dst_f - xx_fix;
        ASSERT(xx_fix==0 || dst_end0==dst_endl);

        ASSERT(filter_start == filter);
        filter_start += filter_width;
        const float *filter_end = filter_start;

        for (;dst2<dst_endl;dst2+=4)//left margin
        {
            const float *src = dst_f;
            filter_start -= 4;

            //filter 4
            __m128 src4 = _mm_setzero_ps();/*1 2 3 4*/
            __m128 sum = _mm_setzero_ps();
            for(const float* f=filter_start;f<filter_end;f+=4,src+=4)
            {   
                __m128 src_5_8 = _mm_load_ps(src);/*5 6 7 8*/
                __m128 f4 = _mm_load_ps(f);

#define XY_FILTER_4(src4, src_5_8, f4, sum) \
    __m128 f4_1 = _mm_shuffle_ps(f4, f4, _MM_SHUFFLE(0,0,0,0));\
    f4_1 = _mm_mul_ps(f4_1, src4);\
    sum = _mm_add_ps(sum, f4_1);\
    __m128 src_3_6 = _mm_shuffle_ps(src4, src_5_8, _MM_SHUFFLE(1,0,3,2));/*3 4 5 6*/\
    f4_1 = _mm_shuffle_ps(f4, f4, _MM_SHUFFLE(2,2,2,2));\
    f4_1 = _mm_mul_ps(f4_1, src_3_6);\
    sum = _mm_add_ps(sum, f4_1);\
    src4 = _mm_shuffle_ps(src4, src_3_6, _MM_SHUFFLE(2,1,2,1));/*2 3 4 5*/\
    f4_1 = _mm_shuffle_ps(f4, f4, _MM_SHUFFLE(1,1,1,1));\
    f4_1 = _mm_mul_ps(f4_1, src4);\
    sum = _mm_add_ps(sum, f4_1);\
    src_3_6 = _mm_shuffle_ps(src_3_6, src_5_8, _MM_SHUFFLE(2,1,2,1));/*4 5 6 7*/\
    f4_1 = _mm_shuffle_ps(f4, f4, _MM_SHUFFLE(3,3,3,3));\
    f4_1 = _mm_mul_ps(f4_1, src_3_6);\
    sum = _mm_add_ps(sum, f4_1)

                { XY_FILTER_4(src4, src_5_8, f4, sum); }
                src4 = src_5_8;
            }
            //store result
            _mm_store_ps(dst2, sum);
        }
        for (;dst2<dst_f;dst2+=4)//if width < filter_width
        {
            const float *src = dst_f;
            filter_start-=4;
            filter_end-=4;

            __m128 src4 = _mm_setzero_ps();/*1 2 3 4*/
            __m128 sum = _mm_setzero_ps();
            __m128 src_5_8, f4;
            for(const float* f=filter_start;f<filter_end;f+=4,src+=4)
            {   
                src_5_8 = _mm_load_ps(src);/*5 6 7 8*/
                f4 = _mm_load_ps(f);

                { XY_FILTER_4(src4, src_5_8, f4, sum); }
                src4 = src_5_8;
            }
            src_5_8 = _mm_setzero_ps();
            f4 = _mm_load_ps(filter_end);
            { XY_FILTER_4(src4, src_5_8, f4, sum); }
            //store result
            _mm_store_ps(dst2, sum);
        }
        ASSERT(filter_start == filter);
        for (;dst2<dst_end0;dst2+=4)
        {
            const float *src = dst2;

            //filter 4
            __m128 src4 = _mm_load_ps(src);/*1 2 3 4*/
            __m128 sum = _mm_setzero_ps();
            for(const float* f=filter_start;f<filter_end;f+=4)
            {
                src+=4;
                __m128 src_5_8 = _mm_load_ps(src);/*5 6 7 8*/
                __m128 f4 = _mm_load_ps(f);

                { XY_FILTER_4(src4, src_5_8, f4, sum); }
                src4 = src_5_8;
            }
            //store result
            _mm_store_ps(dst2, sum);
        }
        for (;dst2<dst_endr;dst2+=4)//right margin
        {
            const float *src = dst2;
            filter_end-=4;

            //filter 4
            __m128 src4 = _mm_load_ps(src);//1 2 3 4
            __m128 sum = _mm_setzero_ps();
            __m128 src_5_8, f4;
            for(const float* f=filter_start;f<filter_end;f+=4)
            {
                src+=4;
                src_5_8 = _mm_load_ps(src);//5 6 7 8
                f4 = _mm_load_ps(f);

                { XY_FILTER_4(src4, src_5_8, f4, sum); }

                src4 = src_5_8;
                //move new 4 in_n_out to old 4 in_n_out
            }
            src_5_8 = _mm_setzero_ps();
            f4 = _mm_load_ps(filter_end);
            { XY_FILTER_4(src4, src_5_8, f4, sum); }
            //store result
            _mm_store_ps(dst2, sum);
        }
    }
#undef XY_FILTER_4
}

static __forceinline void xy_filter_4_inline(__m128& src4, const __m128& src_5_8, const __m128& f4, __m128& sum)
{
    __m128 f4_1 = _mm_shuffle_ps(f4, f4, _MM_SHUFFLE(0,0,0,0));
    f4_1 = _mm_mul_ps(f4_1, src4);
    sum = _mm_add_ps(sum, f4_1);
    __m128 src_3_6 = _mm_shuffle_ps(src4, src_5_8, _MM_SHUFFLE(1,0,3,2));/*3 4 5 6*/
    f4_1 = _mm_shuffle_ps(f4, f4, _MM_SHUFFLE(2,2,2,2));
    f4_1 = _mm_mul_ps(f4_1, src_3_6);
    sum = _mm_add_ps(sum, f4_1);
    src4 = _mm_shuffle_ps(src4, src_3_6, _MM_SHUFFLE(2,1,2,1));/*2 3 4 5*/
    f4_1 = _mm_shuffle_ps(f4, f4, _MM_SHUFFLE(1,1,1,1));
    f4_1 = _mm_mul_ps(f4_1, src4);
    sum = _mm_add_ps(sum, f4_1);
    src_3_6 = _mm_shuffle_ps(src_3_6, src_5_8, _MM_SHUFFLE(2,1,2,1));/*4 5 6 7*/
    f4_1 = _mm_shuffle_ps(f4, f4, _MM_SHUFFLE(3,3,3,3));
    f4_1 = _mm_mul_ps(f4_1, src_3_6);
    sum = _mm_add_ps(sum, f4_1);
}

void xy_filter_sse_v3(float *dst, int width, int height, int stride, const float *filter, int filter_width)
{
    ASSERT( stride>=4*(width+filter_width) );
    ASSERT( ((stride|(4*width)|(4*filter_width)|reinterpret_cast<int>(dst)|reinterpret_cast<int>(filter))&15)==0 );

    int xx_fix = width > filter_width ? 0 : filter_width - width;
    const float *filter_start = filter;
    BYTE* dst_byte = reinterpret_cast<BYTE*>(dst);
    BYTE* end = dst_byte + height*stride;
    for( ; dst_byte<end; dst_byte+=stride )
    {
        float *dst_f = reinterpret_cast<float*>(dst_byte); 
        float *dst2 = dst_f - filter_width;
        float *dst_endr = dst_f + width;
        float *dst_end0 = dst_endr - filter_width;
        float *dst_endl = dst_f - xx_fix;
        ASSERT(xx_fix==0 || dst_end0==dst_endl);

        ASSERT(filter_start == filter);
        filter_start += filter_width;
        const float *filter_end = filter_start;

        for (;dst2<dst_endl;dst2+=4)//left margin
        {
            const float *src = dst_f;
            filter_start -= 4;

            //filter 4
            __m128 src4 = _mm_setzero_ps();/*1 2 3 4*/
            __m128 sum = _mm_setzero_ps();
            for(const float* f=filter_start;f<filter_end;f+=4,src+=4)
            {   
                __m128 src_5_8 = _mm_load_ps(src);/*5 6 7 8*/
                __m128 f4 = _mm_load_ps(f);

                xy_filter_4_inline(src4, src_5_8, f4, sum);

                src4 = src_5_8;
            }
            //store result
            _mm_store_ps(dst2, sum);
        }
        for (;dst2<dst_f;dst2+=4)//if width < filter_width
        {
            const float *src = dst_f;
            filter_start-=4;
            filter_end-=4;

            __m128 src4 = _mm_setzero_ps();/*1 2 3 4*/
            __m128 sum = _mm_setzero_ps();
            __m128 src_5_8, f4;
            for(const float* f=filter_start;f<filter_end;f+=4,src+=4)
            {   
                src_5_8 = _mm_load_ps(src);/*5 6 7 8*/
                f4 = _mm_load_ps(f);

                xy_filter_4_inline(src4, src_5_8, f4, sum);
                src4 = src_5_8;
            }
            src_5_8 = _mm_setzero_ps();
            f4 = _mm_load_ps(filter_end);
            xy_filter_4_inline(src4, src_5_8, f4, sum);
            //store result
            _mm_store_ps(dst2, sum);
        }
        ASSERT(filter_start == filter);
        for (;dst2<dst_end0;dst2+=4)
        {
            const float *src = dst2;

            //filter 4
            __m128 src4 = _mm_load_ps(src);/*1 2 3 4*/
            __m128 sum = _mm_setzero_ps();
            for(const float* f=filter_start;f<filter_end;f+=4)
            {
                src+=4;
                __m128 src_5_8 = _mm_load_ps(src);/*5 6 7 8*/
                __m128 f4 = _mm_load_ps(f);

                xy_filter_4_inline(src4, src_5_8, f4, sum);
                src4 = src_5_8;
            }
            //store result
            _mm_store_ps(dst2, sum);
        }
        for (;dst2<dst_endr;dst2+=4)//right margin
        {
            const float *src = dst2;
            filter_end-=4;

            //filter 4
            __m128 src4 = _mm_load_ps(src);//1 2 3 4
            __m128 sum = _mm_setzero_ps();
            __m128 src_5_8, f4;
            for(const float* f=filter_start;f<filter_end;f+=4)
            {
                src+=4;
                src_5_8 = _mm_load_ps(src);//5 6 7 8
                f4 = _mm_load_ps(f);

                xy_filter_4_inline(src4, src_5_8, f4, sum);

                src4 = src_5_8;
                //move new 4 in_n_out to old 4 in_n_out
            }
            src_5_8 = _mm_setzero_ps();
            f4 = _mm_load_ps(filter_end);
            xy_filter_4_inline(src4, src_5_8, f4, sum);
            //store result
            _mm_store_ps(dst2, sum);
        }
    }
}

class XyFilterTest : public ::testing::Test 
{
public:
    static const int MAX_FILTER_LENGTH = 128;
    static const int MAX_DATA_BUFF_SIZE = 512*1024;

    float filter_f_base[MAX_FILTER_LENGTH+3];
    float *filter_f;

    float data_base[MAX_DATA_BUFF_SIZE+3];
    float *data;
    int w, h, pitch;
    int filter_width;

protected:
    virtual void SetUp() 
    {
    }

    void FillRandData(int w, int h, int filter_width)
    {
        this->w = w;
        this->h = h;        
        this->filter_width = filter_width;
        ASSERT(w%4==0);
        ASSERT(this->filter_width  <= MAX_FILTER_LENGTH);
        //ASSERT(this->filter_width%4==0);
        
        this->pitch = ((w+filter_width+3)&~3)*sizeof(float);
        ASSERT(this->pitch*this->h <= MAX_DATA_BUFF_SIZE);

        data = data_base + filter_width;
        if ((int)(data)&15)
        {
            data = (float*)(((int)data + 15) & ~15);
        }
        filter_f = filter_f_base;
        if ((int)(filter_f)&15)
        {
            filter_f = (float*)(((int)filter_f + 15) & ~15);
        }

        for (int i=0;i<h;i++)
        {
            float *data2 = (float*)((PUINT8)data + i*pitch);
            for (int j=0;j<w+filter_width;j++)
            {
                data2[j] = rand()*0.1f/0xFF;
            }
        }
        float sum = 0;
        for (int i=0;i<filter_width;i++)
        {
            filter_f[i] = abs(rand());
            sum += filter_f[i];
        }
        for (int i=0;i<filter_width;i++)
        {
            filter_f[i] /= sum;
        }
        for (int i=filter_width;i<((filter_width+3)&~3);i++)
        {
            filter_f[i] = 0;
        }
    }
};

#define FilterTest(width, height, FILTER_LENGTH, loop_num, function) \
TEST_F(XyFilterTest, function ## _ ## width ## _ ## height ## _ ## FILTER_LENGTH ## _ ## loop_num ) \
{\
    FillRandData(width, height, FILTER_LENGTH);\
    for(int i=0;i<loop_num;i++)\
    {\
        function(data, w, h, pitch, filter_f, (FILTER_LENGTH+3)&~3);\
    }\
    /*ASSERT_EQ(data[-filter_width],0)*/;/*anit compiler optimize*/\
    ASSERT_EQ(0,0);\
}

#define DUAL_TEST(width, height, FILTER_LENGTH, loop_num) \
    FilterTest(width, height, FILTER_LENGTH, loop_num, xy_filter_sse_v2) \
    FilterTest(width, height, FILTER_LENGTH, loop_num, xy_filter_sse_v3)

DUAL_TEST(128, 16, 28, 20000)
    DUAL_TEST(256, 16, 28, 20000)
    DUAL_TEST(512, 16, 28, 20000)
    DUAL_TEST(128, 32, 28, 20000)
    DUAL_TEST(256, 32, 28, 20000)
    DUAL_TEST(512, 32, 28, 20000)
    DUAL_TEST(128, 64, 28, 20000)
    DUAL_TEST(256, 64, 28, 20000)
    DUAL_TEST(512, 64, 28, 20000)

//////////////////////////////////////////////////////////////////////

// transpose test

void xy_float_2_float_transpose_c_v0(float *dst, int dst_width, int dst_stride, 
    const float *src, int width, int height, int src_stride)
{
    ASSERT(dst_width >= height);
    PUINT8 dst_byte = reinterpret_cast<PUINT8>(dst);
    const float* src_end = src + width;
    PCUINT8 src2_end = reinterpret_cast<PCUINT8>(src) + height*src_stride;
    for( ; src<src_end; src++, dst_byte+=dst_stride )
    {
        PCUINT8 src2 = reinterpret_cast<PCUINT8>(src);

        float *dst2 = reinterpret_cast<float*>(dst_byte);        
        for (;src2<src2_end;src2+=src_stride,dst2++)
        {
            *dst2 = *reinterpret_cast<const float*>(src2);
        }
        float *dst2_end = reinterpret_cast<float*>(dst_byte) + dst_width;
        for (;dst2<dst2_end;dst2++)
        {
            *dst2 = 0;
        }
    }
}

void xy_float_2_float_transpose_sse_v0(float *dst, int dst_width, int dst_stride, 
    const float *src, int width, int height, int src_stride)
{
    typedef float DstT;
    typedef const float SrcT;

    ASSERT( (((int)dst|dst_stride)&15)==0 );
    ASSERT(dst_width >= height);
    PUINT8 dst_byte = reinterpret_cast<PUINT8>(dst);
    SrcT* src_end = src + width;
    PCUINT8 src2_end1 = reinterpret_cast<PCUINT8>(src) + (height&~3)*src_stride;
    PCUINT8 src2_end2 = reinterpret_cast<PCUINT8>(src) + height*src_stride;
    for( ; src<src_end; src++, dst_byte+=dst_stride )
    {
        PCUINT8 src2 = reinterpret_cast<PCUINT8>(src);

        DstT *dst2 = reinterpret_cast<DstT*>(dst_byte);
        for (;src2<src2_end1;src2+=4*src_stride,dst2+=4)
        {
            __m128 m1 = _mm_set_ps(
                *(SrcT*)(src2+3*src_stride), 
                *(SrcT*)(src2+2*src_stride), 
                *(SrcT*)(src2+src_stride), 
                *(SrcT*)(src2));
            _mm_store_ps(dst2, m1);
        }
        for (;src2<src2_end2;src2+=src_stride,dst2++)
        {
            *dst2 = *reinterpret_cast<SrcT*>(src2);
        }
        float *dst2_end = reinterpret_cast<DstT*>(dst_byte) + dst_width;
        for (;dst2<dst2_end;dst2++)
        {
            *dst2 = 0;
        }
    }
}

void float_2_byte_transpose_v0(UINT8 *dst, int dst_width, int dst_stride, 
    const float *src, int width, int height, int src_stride)
{
    ASSERT(dst_width >= height);
    PUINT8 dst_byte = reinterpret_cast<PUINT8>(dst);
    const float* src_end = src + width;
    PCUINT8 src2_end = reinterpret_cast<PCUINT8>(src) + height*src_stride;
    for( ; src<src_end; src++, dst_byte+=dst_stride )
    {
        PCUINT8 src2 = reinterpret_cast<PCUINT8>(src);

        UINT8 *dst2 = reinterpret_cast<UINT8*>(dst_byte);
        for (;src2<src2_end;src2+=src_stride,dst2++)
        {
            *dst2 = static_cast<UINT8>(*reinterpret_cast<const float*>(src2)+0.5);
        }
        UINT8 *dst2_end = reinterpret_cast<UINT8*>(dst_byte) + dst_width;
        for (;dst2<dst2_end;dst2++)
        {
            *dst2 = 0;
        }
    }
}

void float_2_byte_transpose_sse_v0(UINT8 *dst, int dst_width, int dst_stride, 
    const float *src, int width, int height, int src_stride)
{
    typedef UINT8 DstT;
    typedef const float SrcT;

    ASSERT(dst_width >= height);
    PUINT8 dst_byte = reinterpret_cast<PUINT8>(dst);
    SrcT* src_end = src + width;
    PCUINT8 src2_end00 = reinterpret_cast<PCUINT8>(src) + (height&~15)*src_stride;
    PCUINT8 src2_end = reinterpret_cast<PCUINT8>(src) + height*src_stride;
    __m128 m0 = _mm_set1_ps(0.5f);
    for( ; src<src_end; src++, dst_byte+=dst_stride )
    {
        PCUINT8 src2 = reinterpret_cast<PCUINT8>(src);

        DstT *dst2 = reinterpret_cast<DstT*>(dst_byte);
        for (;src2<src2_end00;src2+=16*src_stride,dst2+=16)
        {
            __m128 m1 = _mm_set_ps(
                *(SrcT*)(src2+3*src_stride),
                *(SrcT*)(src2+2*src_stride), 
                *(SrcT*)(src2+src_stride), 
                *(SrcT*)(src2));
            __m128 m2 = _mm_set_ps(
                *(SrcT*)(src2+7*src_stride),
                *(SrcT*)(src2+6*src_stride),
                *(SrcT*)(src2+5*src_stride),
                *(SrcT*)(src2+4*src_stride));
            __m128 m3 = _mm_set_ps(
                *(SrcT*)(src2+11*src_stride),
                *(SrcT*)(src2+10*src_stride),
                *(SrcT*)(src2+9*src_stride), 
                *(SrcT*)(src2+8*src_stride));
            __m128 m4 = _mm_set_ps(
                *(SrcT*)(src2+15*src_stride),
                *(SrcT*)(src2+14*src_stride),
                *(SrcT*)(src2+13*src_stride),
                *(SrcT*)(src2+12*src_stride));
            m1 = _mm_add_ps(m1,m0);
            m2 = _mm_add_ps(m2,m0);
            m3 = _mm_add_ps(m3,m0);
            m4 = _mm_add_ps(m4,m0);

            __m128i i1 = _mm_cvtps_epi32(m1);
            __m128i i2 = _mm_cvtps_epi32(m2);
            __m128i i3 = _mm_cvtps_epi32(m3);
            __m128i i4 = _mm_cvtps_epi32(m4);

            i1 = _mm_packs_epi32(i1,i2);
            i3 = _mm_packs_epi32(i3,i4);
            i1 = _mm_packus_epi16(i1,i3);

            _mm_store_si128((__m128i*)dst2, i1);
        }
        for (;src2<src2_end;src2+=src_stride,dst2++)
        {
            *dst2 = static_cast<DstT>(*reinterpret_cast<SrcT*>(src2)+0.5);
        }
        DstT *dst2_end = reinterpret_cast<DstT*>(dst_byte) + dst_width;
        for (;dst2<dst2_end;dst2++)
        {
            *dst2 = 0;
        }
    }
}

void byte_2_float_c_v0(float *dst, int dst_width, int dst_stride, 
    PCUINT8 src, int width, int height, int stride)
{
    PUINT8 dst_byte = reinterpret_cast<PUINT8>(dst);

    PCUINT8 src_end = src + height*stride;
    for( ; src<src_end; src+=stride, dst_byte+=dst_stride )
    {
        PCUINT8 src2 = src;
        PCUINT8 src_end = src2 + width;
        float *dst2 = reinterpret_cast<float*>(dst_byte);

        for (;src2<src_end;src2++, dst2++)
        {
            *dst2 = *src2;
        }
        float *dst2_end=reinterpret_cast<float*>(dst_byte)+dst_width;
        for (;dst2<dst2_end;dst2++)
        {
            *dst2=0;
        }
    }
}

void byte_2_float_sse_v0(float *dst, int dst_width, int dst_stride, 
    PCUINT8 src, int width, int height, int stride)
{
    ASSERT( dst_width>=width );
    ASSERT( ((reinterpret_cast<int>(dst)|dst_stride)&15)==0 );
    PUINT8 dst_byte = reinterpret_cast<PUINT8>(dst);

    PCUINT8 src_end = src + height*stride;
    for( ; src<src_end; src+=stride, dst_byte+=dst_stride )
    {
        PCUINT8 src2 = src;
        PCUINT8 src2_end0 = src2 + (width&~15); 
        float *dst2 = reinterpret_cast<float*>(dst_byte);

        for (;src2<src2_end0;src2+=16, dst2+=16)
        {
            //filter 4
            __m128i src16 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(src2));
            __m128i src16_lo = _mm_unpacklo_epi8(src16, _mm_setzero_si128()); 
            __m128i src16_hi = _mm_unpackhi_epi8(src16, _mm_setzero_si128());
            __m128i src16_lo_lo = _mm_unpacklo_epi8(src16_lo, _mm_setzero_si128());
            __m128i src16_lo_hi = _mm_unpackhi_epi8(src16_lo, _mm_setzero_si128());
            __m128i src16_hi_lo = _mm_unpacklo_epi8(src16_hi, _mm_setzero_si128());
            __m128i src16_hi_hi = _mm_unpackhi_epi8(src16_hi, _mm_setzero_si128());
            __m128 dst_f1 =  _mm_cvtepi32_ps(src16_lo_lo);
            __m128 dst_f2 =  _mm_cvtepi32_ps(src16_lo_hi);
            __m128 dst_f3 =  _mm_cvtepi32_ps(src16_hi_lo);
            __m128 dst_f4 =  _mm_cvtepi32_ps(src16_hi_hi);
            _mm_stream_ps(dst2, dst_f1);
            _mm_stream_ps(dst2+4, dst_f2);
            _mm_stream_ps(dst2+8, dst_f3);
            _mm_stream_ps(dst2+12, dst_f4);
        }
        PCUINT8 src2_end = src + width;
        for (;src2<src2_end;src2++,dst2++)
        {
            *dst2 = *src2;
        }
        float *dst2_end=reinterpret_cast<float*>(dst_byte)+dst_width;
        for (;dst2<dst2_end;dst2++)
        {
            *dst2=0;
        }
    }
}

void byte_2_float_sse_v1(float *dst, int dst_width, int dst_stride, 
    PCUINT8 src, int width, int height, int stride)
{
    ASSERT( dst_width>=width );
    ASSERT( ((reinterpret_cast<int>(dst)|dst_stride)&15)==0 );
    PUINT8 dst_byte = reinterpret_cast<PUINT8>(dst);

    PCUINT8 src_end = src + height*stride;
    for( ; src<src_end; src+=stride, dst_byte+=dst_stride )
    {
        PCUINT8 src2 = src;
        PCUINT8 src2_end0 = src2 + (width&~15); 
        float *dst2 = reinterpret_cast<float*>(dst_byte);

        for (;src2<src2_end0;src2+=16, dst2+=16)
        {
            //filter 4
            __m128i src16 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(src2));
            __m128i src16_lo = _mm_unpacklo_epi8(src16, _mm_setzero_si128()); 
            __m128i src16_hi = _mm_unpackhi_epi8(src16, _mm_setzero_si128());
            __m128i src16_lo_lo = _mm_unpacklo_epi8(src16_lo, _mm_setzero_si128());
            __m128i src16_lo_hi = _mm_unpackhi_epi8(src16_lo, _mm_setzero_si128());
            __m128i src16_hi_lo = _mm_unpacklo_epi8(src16_hi, _mm_setzero_si128());
            __m128i src16_hi_hi = _mm_unpackhi_epi8(src16_hi, _mm_setzero_si128());
            __m128 dst_f1 =  _mm_cvtepi32_ps(src16_lo_lo);
            __m128 dst_f2 =  _mm_cvtepi32_ps(src16_lo_hi);
            __m128 dst_f3 =  _mm_cvtepi32_ps(src16_hi_lo);
            __m128 dst_f4 =  _mm_cvtepi32_ps(src16_hi_hi);
            _mm_store_ps(dst2, dst_f1);
            _mm_store_ps(dst2+4, dst_f2);
            _mm_store_ps(dst2+8, dst_f3);
            _mm_store_ps(dst2+12, dst_f4);
        }
        PCUINT8 src2_end = src + width;
        for (;src2<src2_end;src2++,dst2++)
        {
            *dst2 = *src2;
        }
        float *dst2_end=reinterpret_cast<float*>(dst_byte)+dst_width;
        for (;dst2<dst2_end;dst2++)
        {
            *dst2=0;
        }
    }
}

class XyTransposeTest : public ::testing::Test 
{
public:
    static const int MAX_FILTER_LENGTH = 128;
    static const int MAX_DATA_BUFF_SIZE = 512*128;

    UINT8 data_byte_base[MAX_DATA_BUFF_SIZE+15];
    UINT8 *data_b;

    float data_base_f[2*MAX_DATA_BUFF_SIZE+3];
    float *data_f;
    float *data_f2;
    int w, h, pitch;

protected:
    virtual void SetUp() 
    {
    }

    void FillRandData(int w, int h)
    {
        this->w = w;
        this->h = h;
        ASSERT(w%4==0);

        this->pitch = ((w+3)&~3)*sizeof(float);
        ASSERT(this->pitch*this->h <= MAX_DATA_BUFF_SIZE);

        data_f = data_base_f;
        if ((int)(data_f)&15)
        {
            data_f = (float*)(((int)data_f + 15) & ~15);
        }
        for (int i=0;i<h;i++)
        {
            float *data2 = (float*)((PUINT8)data_f + i*pitch);
            for (int j=0;j<w;j++)
            {
                data2[j] = rand()*0.1f/0xFF;
            }
        }
        data_f2 = data_f + h*pitch;

        data_b = data_byte_base;
        if ((int)(data_b)&15)
        {
            data_b = (UINT8*)(((int)data_b + 15) & ~15);
        }
    }
};


#define Float2FloatTest(width, height, loop_num, function) \
TEST_F(XyTransposeTest, function ## _ ## width ## _ ## height ## _ ## loop_num ) \
{\
    FillRandData(width, height);\
    for(int i=0;i<loop_num;i++)\
    {\
        function(data_f2, height, (width+15)&~15, data_f, width, height, pitch);\
    }\
    ASSERT_EQ(data_f2[0],data_f2[0]);\
}

Float2FloatTest( 272, 80, 10000, xy_float_2_float_transpose_c_v0)
    Float2FloatTest( 272, 80, 10000, xy_float_2_float_transpose_sse_v0)


#define Float2ByteTest(width, height, loop_num, function) \
TEST_F(XyTransposeTest, function ## _ ## width ## _ ## height ## _ ## loop_num ) \
{\
    FillRandData(width, height);\
    for(int i=0;i<loop_num;i++)\
    {\
        function(data_b, height, (width+15)&~15, data_f, width, height, pitch);\
    }\
    ASSERT_EQ(data_b[0],data_b[0]);\
}

Float2ByteTest( 272, 80, 10000, float_2_byte_transpose_v0)
    Float2ByteTest( 272, 80, 10000, float_2_byte_transpose_sse_v0)


#define Byte2FloatTest(dst_width, width, height, loop_num, function) \
TEST_F(XyTransposeTest, function ## _ ## width ## _ ## height ## _ ## loop_num ) \
{\
    FillRandData(width, height);\
    for(int i=0;i<loop_num;i++)\
    {\
        function(data_f, width, (width+15)&~15, data_b, width, height, pitch);\
    }\
    ASSERT_EQ(data_b[0],data_b[0]);\
}


Byte2FloatTest( 272, 256, 64, 10000, byte_2_float_c_v0)
Byte2FloatTest( 272, 256, 64, 10000, byte_2_float_sse_v0)
Byte2FloatTest( 272, 256, 64, 10000, byte_2_float_sse_v1)

#endif // __XY_FILTER_BENCHMARK_F0ED9F75_C0A7_4B0E_910E_7A82ECE2D7AD_H__
