/*
** Made by texane <texane@gmail.com>
** 
** Started on  Thu May 24 17:25:20 2007 texane
** Last update Thu May 24 17:28:05 2007 texane
*/


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "cbuf.h"


/* internal implementation
 */

static void buf_push_back(unsigned char** p_dst, int* sz_dst,
			  unsigned char* p_src, int sz_src)
{
  unsigned char* p_tmp;

  /* push src @back p_dst */
  p_tmp = malloc((*sz_dst + sz_src) * sizeof(unsigned char));
  if (p_tmp == 0)
    return ;

  /* copy */
  if (*p_dst)
    {
      memcpy((void*)p_tmp, (void*)*p_dst, *sz_dst);
      free(*p_dst);
    }
  memcpy((void*)(p_tmp + *sz_dst), (void*)p_src, sz_src);

  /* update */
  *p_dst = p_tmp;
  *sz_dst += sz_src;
}


static void buf_pop_front(unsigned char** p_buf,
			  int* sz_buf,
			  int nr_pop)
{
  unsigned char* p_tmp;

  if (*sz_buf <= 0 || *p_buf == 0 || nr_pop == 0)
    return ;

  /* handle special case */
  if ((*sz_buf - nr_pop) <= 0)
    {
      free(*p_buf);
      *p_buf = 0;
      *sz_buf = 0;
      return ;
    }

  /* pop */
  p_tmp = malloc((*sz_buf - nr_pop) * sizeof(unsigned char));
  if (p_tmp == 0)
    return ;
  memcpy((void*)p_tmp, (void*)(*p_buf + nr_pop), *sz_buf - nr_pop);

  /* release & update */
  free(*p_buf);
  *p_buf = p_tmp;
  *sz_buf -= nr_pop;
}


static void __attribute__((unused)) buf_release(unsigned char** p_buf, int* sz_buf)
{
  if (*p_buf)
    {
      free(*p_buf);
      *p_buf = 0;
    }
  *sz_buf = 0;
}


static char printable_char(unsigned char c)
{
  if ((c >= 32) && (c <= 127))
    return c;
  return '.';
}

static void buf_print(unsigned char* p_buf, int sz_buf)
{
  int i_buf;
  int i_ascii;
  char buf_ascii[32];

  printf("/* buffer[%d] */\n", sz_buf);

  i_ascii = 0;
  for (i_buf = 0; i_buf < sz_buf; ++i_buf)
    {
      if (!i_buf)
	{
	  printf("[00000000]: ");
	}
      else if ((i_buf % 16) == 0)
	{
	  buf_ascii[i_ascii] = 0;
	  i_ascii = 0;
	  printf("\t | %s\n", buf_ascii);
	  if (i_buf < sz_buf)
	    printf("[%08x]: ", i_buf);
	}
      printf("%02x ", p_buf[i_buf]);
      buf_ascii[i_ascii] = printable_char(p_buf[i_buf]);
      ++i_ascii;
    }

  if (i_ascii)
    {
      buf_ascii[i_ascii] = 0;
      printf("\t | %s\n", buf_ascii);      
    }

  printf("\n"); fflush(stdout);
}



/* cbuf public interface
 */

static void cbuf_reset(cbuf_t* buf)
{
  buf->data = 0;
  buf->sz = 0;
}


void cbuf_init(cbuf_t* buf)
{
  cbuf_reset(buf);
}


void cbuf_release(cbuf_t* buf)
{
  cbuf_clear(buf);
  cbuf_reset(buf);
}


void cbuf_push_back(cbuf_t* buf, unsigned char* data, int sz)
{
  buf_push_back(&buf->data, &buf->sz, data, sz);
}


void cbuf_pop_front(cbuf_t* buf, int count)
{
  buf_pop_front(&buf->data, &buf->sz, count);
}


void cbuf_clear(cbuf_t* buf)
{
  if (buf->data)
    free(buf->data);
  cbuf_reset(buf);
}


int cbuf_size(cbuf_t* buf)
{
  return buf->sz;
}


unsigned char* cbuf_data(cbuf_t* buf)
{
  return buf->data;
}


void cbuf_print(cbuf_t* buf)
{
  buf_print(buf->data, buf->sz);
}


int cbuf_is_empty(cbuf_t* buf)
{
  if (buf->data == 0)
    {
      return 1;
    }
  if (buf->sz == 0)
    {
      return 1;
    }

  return 0;
}


void cbuf_dup(cbuf_t* buf_to, cbuf_t* buf_from)
{
  if (buf_from->sz)
    {
      buf_to->sz = buf_from->sz;
      buf_to->data = malloc(buf_to->sz);
      memcpy(buf_to->data, buf_from->data, buf_to->sz);
    }
}
