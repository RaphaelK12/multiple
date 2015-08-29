#include "convert_engine.h"
#define RKY  4778 // Koeff of  Y for red  processing
#define RKU  2      //  Koeff of  U for red processing
#define RKV  6931   //  Koeff of    V for red processing

#define GKY  4826 // Koeff of Y for green processing
#define GKU  -775 // Koeff of U for green processing
#define GKV  -2670// Koeff of V for green processing

#define BKY  4768// Koeff of Y for blue processing
#define BKU  8809// Koeff of U for blue processing
#define BKV    11// Koeff of V for blue processing
//B = (BKY * (Y-16) + BKU * (U-128) + BKV * (V-128)) >> 10;
//G = (GKY * (Y-16) + GKU * (U-128) + GKV * (V-128)) >> 10;
//R = (RKY * (Y-16) + RKU * (U-128) + RKV * (V-128)) >> 10;


static const int16_t transf_koeff[] = {RKY, RKU, RKV,  GKY, GKU, GKV,  BKY, BKU, BKV};
static int16_t* koeff_v;

int16_t* set_koeffs( const int16_t * k ) {
    int16_t* mat = TARGET_MEMALIGN( 16,  3 * 2 * 8 * sizeof( int16_t ) );

    // INVERSE ORDER!!!
    // a and b
    for( int i = 0; i < 3; ++i ) {
        mat[2 * i * 8 + 7] = k[i * 3 + 0];
        mat[2 * i * 8 + 6] = k[i * 3 + 1];
        mat[2 * i * 8 + 5] = k[i * 3 + 0];
        mat[2 * i * 8 + 4] = k[i * 3 + 1];
        mat[2 * i * 8 + 3] = k[i * 3 + 0];
        mat[2 * i * 8 + 2] = k[i * 3 + 1];
        mat[2 * i * 8 + 1] = k[i * 3 + 0];
        mat[2 * i * 8 + 0] = k[i * 3 + 1];
    }

    //c
    for( int i = 0; i < 3; ++i ) {
        mat[( 2 * i + 1 ) * 8  + 7] = 0;
        mat[( 2 * i + 1 ) * 8  + 6] = k[i * 3 + 2];
        mat[( 2 * i + 1 ) * 8  + 5] = 0;
        mat[( 2 * i + 1 ) * 8  + 4] = k[i * 3 + 2];
        mat[( 2 * i + 1 ) * 8  + 3] = 0;
        mat[( 2 * i + 1 ) * 8  + 2] = k[i * 3 + 2];
        mat[( 2 * i + 1 ) * 8  + 1] = 0;
        mat[( 2 * i + 1 ) * 8  + 0] = k[i * 3 + 2];
    }
    return mat;
}

void static TARGET_INLINE load_and_unpack_YUYV( const uint8_t * mem, __m128i* y, __m128i* u , __m128i* v ) {
    __m128i vec = _mm_loadl_epi64( ( __m128i* )( mem ) );
    // vec = 0 0 0 0 0 0 0 0 v y u y v y u y
    *v = _mm_set1_epi16( 255 ); // store of mask
    *y = _mm_and_si128( *v, vec );
    *u = _mm_srli_epi32( *v, 8 );
    // u =  ..... 00 00 FF 00, 00 00 FF 00,
    *u = _mm_and_si128( *u, vec );
    *v = _mm_slli_epi32( *v, 24 );
    // v =  ..... FF 00 00 00, FF 00 00 00,

    *v = _mm_and_si128( *v, vec );

    vec = _mm_setzero_si128();

    *u = _mm_srli_epi32( *u, 8 ); // ... 0 0 0 U
    *v = _mm_srli_epi32( *v, 24 ); // ... 0 0 0 V

    *u =  _mm_or_si128( *u, _mm_slli_epi32( *u, 16 ) ); // ...  0 U 0 U
    *v = _mm_or_si128( *v, _mm_slli_epi32( *v, 16 ) ); // ... 0 V 0 V

    *y = _mm_sub_epi16( *y, _mm_set1_epi16( 16 ) );
    *u = _mm_sub_epi16( *u, _mm_set1_epi16( 128 ) );
    *v = _mm_sub_epi16( *v, _mm_set1_epi16( 128 ) );

    *y = _mm_unpacklo_epi16( *y, vec ); //  [Y] = 0 0 0 Y 0 0 0 Y 0 0 0 Y 0 0 0 Y
    *u = _mm_unpacklo_epi16( *u, vec ); //  [U] = 0 0 0 U 0 0 0 U 0 0 0 U 0 0 0 U
    *v = _mm_unpacklo_epi16( *v, vec ); //  [V] = 0 0 0 V 0 0 0 V 0 0 0 V 0 0 0 V
}
__m128i static TARGET_INLINE multiple_add( const __m128i chan_a , const __m128i chan_b, const __m128i chan_c, const int16_t* koeffs_ab, const int16_t* koeffs_c ) {
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
void static TARGET_INLINE clip_32i( __m128i* data , const int32_t max_val ) {
    __m128i check_val = _mm_set1_epi32( max_val );
    __m128i mask = _mm_cmplt_epi32( *data, check_val ); // if (reg[i] >= value)
    *data = _mm_and_si128( *data, mask );
    mask = _mm_andnot_si128( mask, check_val ); // reg[i] = value;
    *data = _mm_or_si128( *data, mask );

    check_val = _mm_setzero_si128();
    mask = _mm_cmpgt_epi32( *data, check_val ); // if(reg[i] <= 0)
    *data = _mm_and_si128( *data, mask ); //  reg[i] = 0
}
void convert_YUYV_to_RGB32_sse2( uint8_t* src , uint8_t*  dst , const int width , const int height , const int16_t* koeffs_table ) {
    __m128i y;
    __m128i u;
    __m128i v;
    int j;
    for( int i = 0; i < height; ++i ) {
        for( j = 0; j + 3 < width; j += 4 ) {

            load_and_unpack_YUYV( src + j * 2, &y, &u, &v );

            //ALFA
            __m128i vec = _mm_set1_epi32( 3 );

            //RED

            __m128i buf = multiple_add( y, u, v, koeffs_table, koeffs_table + 8 );
            clip_32i( &buf, 1023 );

            buf = _mm_slli_epi32( buf, 2 );
            //koeffs_table += 3;
            vec = _mm_or_si128( vec, buf );

            //G component
            buf = multiple_add( y, u, v, koeffs_table + 16, koeffs_table + 24 );
            clip_32i( &buf, 1023 );

            buf = _mm_slli_epi32( buf, 12 );
            vec = _mm_or_si128( vec, buf );

            //B component
            buf = multiple_add( y, u, v, koeffs_table + 32, koeffs_table + 40 );
            clip_32i( &buf, 1023 );

            buf = _mm_slli_epi32( buf, 22 );
            vec = _mm_or_si128( vec, buf );

            _mm_store_si128( ( __m128i* )( dst + j * 4 ), vec );

        }
        int last = width - j ;
        if( last > 0 )
            convert_YUYV_to_RGB32_c( src + 2 * j, dst + 4 * j, last, 1 );

        src += 2 * width;
        dst += 4 * width;
    }

}
void convert_YUYV_to_RGB32_c( const uint8_t* src, uint8_t* dst,  int width, int height ) {
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

bool check( uint8_t* a, uint8_t* b, int width, int height ) {
    for( int y = 0; y < height; ++y )
        for( int x = 0 ; x < width ; ++x ) {
            if( a[y * width + x] != b[y * width + x] ) {
                printf( "a= %d  b= %d\n", a[y * width + x], b[y * width + x] );
                printf( "error pos is %d %d\n", y, x );
                return false;
            }
        }
    return true;
}
void createframe( int wid, int hei, uint8_t* res ) {
    //srand(time(NULL));
    for( int y = 0; y < hei; ++y )
        for( int x = 0; x < wid; ++x ) {
            res[ y * wid + x] = rand() % 256;
        }
}
int test() {
    clock_t t1, t2;
    int testnum = 100;
    uint64_t du, dv;
    int wid = 640, hei = 480;
    //uint8_t test[]={10,11,12,20,21,22,30,31,32,40,41,42,50,51,52,60};
    //uint8_t test[]={200,200,230,240,250,260,270,280,290,100,110,120,130,140,150,160};
    //uint8_t test[] = {100,200,130,40,50,60,70,80,90,100,110,120,130,140,150,160};

    //uint32_t restest[ 4 ];
    //memset(restest,0,16);
    uint8_t* src = ( uint8_t* ) memalign( 16 , wid * 2 * hei );
    uint8_t* dst1 = ( uint8_t* ) memalign( 16, wid * hei * 4 );
    uint8_t* dst2 = ( uint8_t* ) memalign( 16, wid * hei * 4 );
    //puts("Usuall");
    /*
    convert_YUYV_to_RGB32_c(test,(uint8_t*)restest,4,1);
    for(int i=0;i<4;++i)
    {
        printf("%4d ",restest[i]);
    }
    puts("Vector");
    memset(restest,0,16);
    //convert_YUYV_to_RGB32_sse2(test,(uint8_t*)restest,4,1, koeff_v);

    for(int i=0;i<4;++i)
    {
        printf("%4d ",restest[i]);
    }
    puts("GO!");
    */
    int fail = 0;
    for( int i = 0; i < testnum; ++i ) {
        createframe( wid * 2, hei, src );
        convert_YUYV_to_RGB32_c( src, dst1, wid, hei );
        convert_YUYV_to_RGB32_sse2( src , dst2 , wid, hei , koeff_v );
        if( !check( ( uint8_t* )dst1, ( uint8_t* )dst2, wid * 4, hei ) ) {
            ++fail;
        }
    }

    printf( "You fail %d tests", fail );

    puts( "\nUsuall process" );
    du = 0;
    for( int i = 0; i < testnum; ++i ) {
        createframe( wid * 2, hei, src );
        t1 = clock();
        //printf("T1=%d\n",t1);
        convert_YUYV_to_RGB32_c( src, dst1, wid, hei );
        t2 = clock();
        //printf("T2=%d\n",t2);
        du += t2 - t1;
    }
    printf( "Tics acc: %lld\n", du );

    puts( "\nVector process" );
    dv = 0;
    for( int i = 0; i < testnum; ++i ) {
        createframe( wid * 2, hei, src );
        t1 = clock();
        //printf("T1=%d\n",t1);
        convert_YUYV_to_RGB32_sse2( src, dst2, wid, hei, koeff_v );
        t2 = clock();
        //printf("T2=%d\n",t2);
        dv += t2 - t1;
    }
    printf( "Tics acc: %lld\n", dv );
    free( src );
    printf( "\nperfomance koef is %f\n", 1.*du / dv );
    return 0;
}
void fileProcess( int wid, int hei, char * name ) {
    FILE* f1 = fopen( name, "rb" );
    if( !f1 ) {
        puts( "Error of reading" );
        return ;
    }
    uint8_t *src = ( uint8_t* ) malloc( wid * hei * 2 );
    uint8_t *dst = ( uint8_t* ) malloc( wid * hei * 4 );
    fread( src, 1, wid * hei * 2, f1 );
    fclose( f1 );
    convert_YUYV_to_RGB32_sse2( src, dst, wid, hei, transf_koeff );
    FILE* f2 = fopen( "out.rgb", "wb" );
    fwrite( dst , 1, wid * hei * 4, f1 );
    fclose( f2 );

}
int main() {
    srand( time( NULL ) );
    koeff_v = set_koeffs( transf_koeff );

    //small test of asm code
    /*
    int32_t row[4];
    __m128i m1 = _mm_set_epi32(10, 20 , 30 , 40);
    __m128i m2 = multiple_add( m1, m1, m1, koeff_v, koeff_v + 8);
    _mm_storeu_si128((__m128i*)row, m2);

    for(int i =0; i < 4; ++i )
        printf("%d ", row[i]);
    */
    test();
    //fileProcess(176,144,"out.yuv");
    return 0;
}
