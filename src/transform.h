/*
** Made by texane <texane@gmail.com>
** 
** Started on Sun Oct 26 14:19:40 2008 texane
** Updated on Sun Oct 26 18:59:51 2008 texane
*/



#ifndef TRANSFORM_H_INCLUDED
# define TRANSFORM_H_INCLUDED



#include <stdint.h>



void transform_yuv420_to_rgb24(uint8_t*, const uint8_t*, uint32_t, uint32_t);
void transform_yuyv_to_rgb24(uint8_t*, const uint8_t*, uint32_t, uint32_t);
void transform_vflip_rgb24(uint8_t*, uint32_t, uint32_t);



#endif /* ! TRANSFORM_H_INCLUDED */
