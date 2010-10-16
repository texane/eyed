/*
** Made by texane <texane@gmail.com>
** 
** Started on  Thu Oct 11 22:03:11 2007 texane
** Last update Thu Oct 11 22:03:12 2007 texane
*/



#ifndef CBUF_H
# define CBUF_H


typedef struct
{
  unsigned char* data;
  int sz;
} cbuf_t;

void cbuf_init(cbuf_t*);
void cbuf_release(cbuf_t*);
void cbuf_push_back(cbuf_t*, unsigned char*, int);
void cbuf_pop_front(cbuf_t*, int);
void cbuf_clear(cbuf_t*);
int cbuf_size(cbuf_t*);
int cbuf_is_empty(cbuf_t*);
void cbuf_print(cbuf_t*);


#endif /* ! CBUF_H */
