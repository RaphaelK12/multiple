#include "target_defs.h"
#include "convert_engine.h"
#include "convert_utilit.h"

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

static int16_t* koeff_v;
static const int16_t transf_koeff[] = {RKY, RKU, RKV,
                                       GKY, GKU, GKV,
                                       BKY, BKU, BKV};

void test() {
    clock_t t1, t2;
    int testnum = 100;
    uint64_t du, dv;
    const int wid = 640, hei = 480;
    Frame src_frame = {wid, hei, 16, NULL};
    Frame dst1_frame = {wid, hei, 32, NULL};
    Frame dst2_frame = {wid, hei, 32, NULL};

    bool isOk = true;
    isOk &= get_mem(&src_frame);
    isOk &= get_mem(&dst1_frame);
    isOk &= get_mem(&dst2_frame);

    if(!isOk) return;

    int fail = 0;
    for( int i = 0; i < testnum; ++i ) {
        fill_frame(wid * 2, hei , src_frame.data);
        convert_YUYV_to_RGB32_c(src_frame.data, dst1_frame.data ,wid, hei);
        convert_YUYV_to_RGB32_sse2(src_frame.data, dst2_frame.data ,wid, hei, koeff_v );
        if( !check( dst1_frame.data, dst2_frame.data , wid * 4, hei ) ) {
            ++fail;
        }
    }

    printf( "You fail %d tests", fail );

    puts( "\nUsuall process" );
    du = 0;
    for( int i = 0; i < testnum; ++i ) {
        fill_frame(wid * 2, hei , src_frame.data);
        t1 = clock();
        //printf("T1=%d\n",t1);
        convert_YUYV_to_RGB32_c(src_frame.data, dst1_frame.data ,wid, hei);
        t2 = clock();
        //printf("T2=%d\n",t2);
        du += t2 - t1;
    }
    printf( "Tics acc: %lld\n", du );

    puts( "\nVector process" );
    dv = 0;
    for( int i = 0; i < testnum; ++i ) {
        fill_frame( wid * 2, hei, src_frame.data );
        t1 = clock();
        //printf("T1=%d\n",t1);
        convert_YUYV_to_RGB32_sse2(src_frame.data, dst2_frame.data ,wid, hei, koeff_v );
        t2 = clock();
        //printf("T2=%d\n",t2);
        dv += t2 - t1;
    }
    printf( "Tics acc: %lld\n", dv );
    printf( "\nperfomance koef is %f\n", 1.*du / dv );
    clear_mem(&src_frame);
    clear_mem(&dst1_frame);
    clear_mem(&dst2_frame);
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

    //small test of asm code
    /*
    int32_t row[4];
    __m128i m1 = _mm_set_epi32(10, 20 , 30 , 40);
    __m128i m2 = multiple_add( m1, m1, m1, koeff_v, koeff_v + 8);
    _mm_storeu_si128((__m128i*)row, m2);

    for(int i =0; i < 4; ++i )
        printf("%d ", row[i]);
    */

    koeff_v = TARGET_MEMALIGN(16, 3 * 2 * 8 * sizeof(int16_t));
    if(koeff_v == NULL){
     puts("Error of alloc for koeffs");
     return 0;
     }
    if(set_koeffs( transf_koeff, koeff_v))
        test();
    else
    {
        puts("Error of transform koeffs");
    }
    //fileProcess(176,144,"out.yuv");
    TARGET_FREEALIGN(koeff_v);
    return 0;
}
