#include "convert_engine.h"


static TARGET_INLINE YUV_Chanels load_and_unpack_YUYV( const uint8_t * mem) {
    __m128i vec = _mm_loadl_epi64( ( __m128i* )( mem ) );
    YUV_Chanels data;
    // vec = 0 0 0 0 0 0 0 0 v y u y v y u y
    data.v = _mm_set1_epi16( 255 ); // store of mask
    data.y = _mm_and_si128( data.v, vec );
    data.u = _mm_srli_epi32( data.v, 8 );
    // u =  ..... 00 00 FF 00, 00 00 FF 00,
    data.u = _mm_and_si128( data.u, vec );
    data.v = _mm_slli_epi32( data.v, 24 );
    // v =  ..... FF 00 00 00, FF 00 00 00,

    data.v = _mm_and_si128( data.v, vec );

    vec = _mm_setzero_si128();

    data.u = _mm_srli_epi32( data.u, 8 ); // ... 0 0 0 U
    data.v = _mm_srli_epi32( data.v, 24 ); // ... 0 0 0 V

    data.u =  _mm_or_si128( data.u, _mm_slli_epi32( data.u, 16 ) ); // ...  0 U 0 U
    data.v = _mm_or_si128( data.v, _mm_slli_epi32( data.v, 16 ) ); // ... 0 V 0 V

    data.y = _mm_sub_epi16( data.y, _mm_set1_epi16( 16 ) );
    data.u = _mm_sub_epi16( data.u, _mm_set1_epi16( 128 ) );
    data.v = _mm_sub_epi16( data.v, _mm_set1_epi16( 128 ) );

    data.y = _mm_unpacklo_epi16( data.y, vec ); //  [Y] = 0 0 0 Y 0 0 0 Y 0 0 0 Y 0 0 0 Y
    data.u = _mm_unpacklo_epi16( data.u, vec ); //  [U] = 0 0 0 U 0 0 0 U 0 0 0 U 0 0 0 U
    data.v = _mm_unpacklo_epi16( data.v, vec ); //  [V] = 0 0 0 V 0 0 0 V 0 0 0 V 0 0 0 V
    return data;
}
static TARGET_INLINE __m128i multiple_add( const __m128i chan_a , const __m128i chan_b, const __m128i chan_c, const int16_t* koeffs_ab, const int16_t* koeffs_c ) {
    __m128i mul_res = _mm_load_si128( ( __m128i* ) koeffs_ab );
    __m128i data = _mm_slli_epi32( chan_a, 16 );
    data = _mm_or_si128( data, chan_b ); // data = 0 a, 0 b, 0 a, 0 b, 0 a, 0 b, 0 a, 0 b
    mul_res = _mm_madd_epi16( mul_res, data ); // mul = s 0 a*b , s 0 a*b, s 0 a*b, s 0 a*b

    __m128i mul_res_last = _mm_load_si128( ( __m128i* ) koeffs_c );; // epi16, not epi32 - problem of signed int
    mul_res_last = _mm_madd_epi16( mul_res_last, chan_c );

    mul_res = _mm_add_epi32( mul_res, mul_res_last ); // (ka * a + kb * b) + (kc * c)
    mul_res = _mm_srai_epi32( mul_res, 10 ); // shifting
    return mul_res;
}
static TARGET_INLINE __m128i  clip_32i( __m128i data , const int32_t max_val ) {
    __m128i check_val = _mm_set1_epi32( max_val );
    __m128i mask = _mm_cmplt_epi32( data, check_val ); // if (reg[i] >= value)
    __m128i res = _mm_and_si128( data, mask );
    mask = _mm_andnot_si128( mask, check_val ); // reg[i] = value;
    res = _mm_or_si128( res , mask );

    check_val = _mm_setzero_si128();
    mask = _mm_cmpgt_epi32( data, check_val ); // if(reg[i] <= 0)
    res = _mm_and_si128( res, mask ); //  reg[i] = 0
    return res;
}
static TARGET_INLINE __m128i pack_and_write_RGB(uint8_t* mem, RGB_Chanels rgb_data){
    //ALFA
    __m128i vec = _mm_set1_epi32( 3 );
    //RED
    rgb_data.r = _mm_slli_epi32( rgb_data.r, 2 );
    vec = _mm_or_si128( vec, rgb_data.r );
    //GREEN
    rgb_data.g = _mm_slli_epi32( rgb_data.g , 12 );
    vec = _mm_or_si128( vec, rgb_data.g );
    //BLUE
    rgb_data.b = _mm_slli_epi32( rgb_data.b, 22 );
    vec = _mm_or_si128( vec, rgb_data.b );
    _mm_store_si128( ( __m128i* )( mem ), vec );
}
TARGET_INLINE void convert_YUYV_to_RGB32_sse2( uint8_t* src , uint8_t*  dst , const int width , const int height , const int16_t* koeffs_table ) {


    int j;
    for( int i = 0; i < height; ++i ) {
        for( j = 0; j + 3 < width; j += 4 ) {

            YUV_Chanels yuv_data = load_and_unpack_YUYV( src + j * 2);
            RGB_Chanels rgb_data;

            rgb_data.r = multiple_add( yuv_data.y, yuv_data.u, yuv_data.v, koeffs_table, koeffs_table + 8 );
            rgb_data.r = clip_32i( rgb_data.r, 1023 );

            rgb_data.g = multiple_add( yuv_data.y, yuv_data.u, yuv_data.v, koeffs_table + 16, koeffs_table + 24 );
            rgb_data.g = clip_32i( rgb_data.g, 1023 );

            rgb_data.b = multiple_add( yuv_data.y, yuv_data.u, yuv_data.v, koeffs_table + 32, koeffs_table + 40 );
            rgb_data.b = clip_32i( rgb_data.b, 1023 );

            pack_and_write_RGB(( dst + j * 4 ), rgb_data);
        }
        int last = width - j ;
        if( last > 0 )
            convert_YUYV_to_RGB32_c( src + 2 * j, dst + 4 * j, last, 1 );

        src += 2 * width;
        dst += 4 * width;
    }

}
void  convert_YUYV_to_RGB32_c( const uint8_t* src, uint8_t* dst,  int width, int height ) {
    int Y, V, U;
    U = 0;
    V = 0;
    int R, G, B;
    for( int i = 0; i < height; i++ ) {
        for( int j = 0; j < width; j++ ) {
            Y = src[2 * j] - 16;
            if( ( ( j >> 1 ) << 1 ) == j ) {
                U = src[2 * j + 1] - 128;
                V = src[2 * j + 3] - 128;
            }

            B = ( 4768 * Y + 8809 * U + 11 * V ) >> 10;

            if( B > 0x3FF ) B = 0x3FF;
            if( B < 0 ) B = 0;

            G = ( 4826 * Y - 775 * U - 2670 * V ) >> 10;
            if( G > 0x3FF ) G = 0x3FF;
            if( G < 0 ) G = 0;

            R = ( 4778 * Y + 2 * U + 6931 * V ) >> 10;

            if( R > 0x3FF ) R = 0x3FF;
            if( R < 0 ) R = 0;

            //memcpy(dst+4 * j,&B,4);

            dst[4 * j] = 3;
            dst[4 * j] |= ( R & 0x3F ) << 2;

            dst[4 * j + 1] = ( R >> 6 );
            dst[4 * j + 1] |= ( G & 0xF ) << 4;

            dst[4 * j + 2] = ( G >> 4 );
            dst[4 * j + 2] |= ( B & 3 ) << 6;

            dst[4 * j + 3] = ( B >> 2 );
        }
        src += 2 * width;
        dst += 4 * width;
    }
}

