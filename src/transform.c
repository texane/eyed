/*
** Made by texane <texane@gmail.com>
** 
** Started on Sun Oct 26 14:22:36 2008 texane
** Updated on Sun Oct 26 21:06:55 2008 texane
*/



#include <stdint.h>


static uint8_t saturate(int x)
{
  if (x < 0) x = 0;
  else if (x > 255) x = 255;
  return x;
}

static void yuv_to_rgb(uint8_t* rgb, uint8_t y, uint8_t cr, uint8_t cb)
{
  int n;

  n = (int)y + (int)(1.402 * (double)((int)cr - 128));
  rgb[0] = saturate(n);

  n =
    (int)y -
    0.34414 * (double)((int)cb - 128) -
    0.71414 * (double)((int)cr - 128);
  rgb[1] = saturate(n);

  n = (int)y + 1.772 * (double)((int)cb - 128);
  rgb[2] = saturate(n);
}


void transform_yuv420_to_rgb24(uint8_t* out_data,
			       const uint8_t* in_data,
			       uint32_t w,
			       uint32_t h)
{
  uint32_t l;
  uint32_t m;

  const int8_t* y_plane;
  const int8_t* cr_plane;
  const int8_t* cb_plane;

  int8_t cb;
  int8_t cr;

  /* planes
   */

  y_plane = (const int8_t*)in_data;
  cr_plane = y_plane + w * h;
  cb_plane = cr_plane + (w * h) / 4;

  /* save bytes per y line
   */

  m = w;

  /* bytes per rgb line
   */

  l = w * 3;

  /* 2 per 2 blocks
   */

  h /= 2;

  while (h--)
    {
      w = m / 2;

      while (w--)
	{
	  cr = *cr_plane;
	  cb = *cb_plane;

	  yuv_to_rgb(out_data, y_plane[0], cr, cb);
	  yuv_to_rgb(out_data + l, y_plane[m], cr, cb);
	  out_data += 3;
	  y_plane += 1;

	  yuv_to_rgb(out_data, y_plane[0], cr, cb);
	  yuv_to_rgb(out_data + l, y_plane[m], cr, cb);
	  out_data += 3;
	  y_plane += 1;

	  cr_plane += 1;
	  cb_plane += 1;
	}

      /* skip line
       */

      y_plane += m;

      out_data += l;
    }
}


void transform_yuyv_to_rgb24(uint8_t* out_data,
			     const uint8_t* in_data,
			     uint32_t w,
			     uint32_t h)
{
  /* y0 u0 y1 v0
   */

  const uint8_t* p = in_data;

  uint32_t i;
  uint32_t j;

  j = h;

  while (j)
    {
      i = w;

      while (i)
	{
	  yuv_to_rgb(out_data, p[0], p[1], p[3]);
	  out_data += 3;

	  yuv_to_rgb(out_data, p[2], p[1], p[3]);
	  out_data += 3;

	  p += 4;

	  i -= 2;
	}

      --j;
    }
}


/* vertical flip
 */

static void swap_lines(uint8_t* a, uint8_t* b, uint32_t w)
{
  uint32_t tmp;
  uint32_t n;

  n = w & (sizeof(uint32_t) - 1);

  w /= sizeof(uint32_t);

  while (w--)
    {
      tmp = *(uint32_t*)a;
      *(uint32_t*)a = *(uint32_t*)b;
      *(uint32_t*)b = tmp;

      a += sizeof(uint32_t);
      b += sizeof(uint32_t);
    }

  switch (n)
    {
    case 3:
      tmp = *a;
      *a = *b;
      *b = (uint8_t)tmp;
      ++a;
      ++b;

      /* passthru
       */

    case 2:
      tmp = *(uint16_t*)a;
      *(uint16_t*)a = *(uint16_t*)b;
      *(uint16_t*)b = (uint16_t)tmp;
      break;

    case 1:
      tmp = *a;
      *a = *b;
      *b = (uint8_t)tmp;
      break;
    }
}


void transform_vflip_rgb24(uint8_t* data,
			   uint32_t w,
			   uint32_t h)
{
  uint32_t j;
  uint32_t k;
  uint32_t l;

  l = w * 3;

  j = 0;
  k = (h - 1) * l;

  while (j < k)
    {
      swap_lines(data + j, data + k, l);
      j += l;
      k -= l;
    }
}
