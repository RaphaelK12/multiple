#ifndef CONVERT_UTILIT_H_INCLUDED
#define CONVERT_UTILIT_H_INCLUDED

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <emmintrin.h>
#include <malloc.h>
#include "target_defs.h"
typedef struct{
    int width;
    int height;
    int bits;
    uint8_t * data;
} Frame;
bool get_mem(Frame * f);
void clear_mem(Frame * f);
bool set_koeffs( const int16_t * src,  int16_t * dst);
bool check( const uint8_t* a, const uint8_t* b, const int width, const int height);
bool fill_frame( const int wid, const int hei, uint8_t* res );
#endif // CONVERT_UTILIT_H_INCLUDED
