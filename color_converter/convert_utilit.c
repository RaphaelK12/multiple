#include "convert_utilit.h"
bool set_koeffs( const int16_t* src, int16_t*  dst) {
    // INVERSE ORDER!!!
    // a and b
    //if ( sizeof(src) < sizeof(int16_t) * 9) return false;
    //if ( sizeof(dst) < sizeof(int16_t) * 3 * 2 * 8) return false;
    for( int i = 0; i < 3; ++i ) {
        dst[2 * i * 8 + 7] = src[i * 3 + 0];
        dst[2 * i * 8 + 6] = src[i * 3 + 1];
        dst[2 * i * 8 + 5] = src[i * 3 + 0];
        dst[2 * i * 8 + 4] = src[i * 3 + 1];
        dst[2 * i * 8 + 3] = src[i * 3 + 0];
        dst[2 * i * 8 + 2] = src[i * 3 + 1];
        dst[2 * i * 8 + 1] = src[i * 3 + 0];
        dst[2 * i * 8 + 0] = src[i * 3 + 1];
    }

    //c
    for( int i = 0; i < 3; ++i ) {
        dst[( 2 * i + 1 ) * 8  + 7] = 0;
        dst[( 2 * i + 1 ) * 8  + 6] = src[i * 3 + 2];
        dst[( 2 * i + 1 ) * 8  + 5] = 0;
        dst[( 2 * i + 1 ) * 8  + 4] = src[i * 3 + 2];
        dst[( 2 * i + 1 ) * 8  + 3] = 0;
        dst[( 2 * i + 1 ) * 8  + 2] = src[i * 3 + 2];
        dst[( 2 * i + 1 ) * 8  + 1] = 0;
        dst[( 2 * i + 1 ) * 8  + 0] = src[i * 3 + 2];
    }
    return true;
}


bool get_mem(Frame * f){
    int wid = f->width;
    int hei = f->height;
    int siz = (wid * hei* (f->bits) ) / 8 ;
    uint8_t* p = TARGET_MEMALIGN(16, siz);
    if(p != NULL) f->data = p;
        else return false;
    return true;
}
void clear_mem(Frame * f){
    if(f->data != NULL)
    TARGET_FREEALIGN(f->data);
}
bool check( const uint8_t* a, const uint8_t* b, const int width, const int height) {
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
bool fill_frame( const int wid, const intŝ hei, uint8_t* res ) {
    //srand(time(NULL));
    if(sizeof(res) < wid * hei) return false;
    for( int y = 0; y < hei; ++y )
        for( int x = 0; x < wid; ++x ) {
            res[ y * wid + x] = rand() % 256;
        }
    return true;
}
