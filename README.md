# multiple
Color conversion via SSE2.
Example of using multiply function for conversion from YUV to RGB.

unpack_YUYV(uint8_t * mem, __m128i* y, __m128i* u ,__m128i* v)
 is function for extraction color channels from data stream.

__m128i madd3(__m128i * chan_a,  __m128i *chan_b, __m128i *chan_c, int16_t koef_a, int16_t koef_b,int16_t koef_c)
 is function for multiple on koef_s and add together channels

check_Up_Bound(__m128i *reg, uint32_t value)
  is function for limiting of upper bound.

lowBoundZero(__m128i *reg)
  is function for limmiting of lower bound ( new value is zero )

process_vector(uint8_t *src,uint8_t *dst,int width,int height, int16_t * koeffs_table)
  is main function for using every upper functions. Arguments are pointer to source, pointer to destination, intetgers, meaning width and height
  and , of course, main argument is pointer to 3*3 table of koeffs.
