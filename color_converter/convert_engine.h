#ifndef CONVERT_ENGINE_INCLUDED
#define CONVERT_ENGINE_INCLUDED


#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <emmintrin.h>
#include <malloc.h>
#include "target_defs.h"

typedef struct{
    __m128i r;
    __m128i g;
    __m128i b;
}RGB_Chanels;

typedef struct{
    __m128i y;
    __m128i u;
    __m128i v;
}YUV_Chanels;

YUV_Chanels static TARGET_INLINE load_and_unpack_YUYV( const uint8_t * mem);
void static TARGET_INLINE pack_and_write_RGB(uint8_t * mem, RGB_Chanels rgb_data);
__m128i static TARGET_INLINE multiple_add( const __m128i chan_a , const __m128i chan_b,
                                            const __m128i chan_c, const int16_t* koeffs_ab,
                                             const int16_t* koeffs_c );
__m128i static TARGET_INLINE clip_32i( __m128i data , const int32_t max_val );
void static TARGET_INLINE pack_and_write_RGB(uint8_t* mem, RGB_Chanels rgb_data);
void convert_YUYV_to_RGB32_c( const uint8_t* src, uint8_t* dst,  int width, int height );
void convert_YUYV_to_RGB32_sse2(uint8_t* src , uint8_t*  dst , const int width ,
                                    const int height , const int16_t* koeffs_table );

#endif // CONVERT_ENGINE_INCLUDED
