/*
** Made by texane <texane@gmail.com>
** 
** Started on Sun Oct 26 21:16:50 2008 texane
** Updated on Sun Oct 26 21:36:21 2008 texane
*/



#ifndef MOTION_H_INCLUDED
# define MOTION_H_INCLUDED



#include <stdint.h>



typedef struct motion_ctx motion_ctx_t;


typedef int (*motion_callback_t)(motion_ctx_t*, void*);



int motion_create(motion_ctx_t**, motion_callback_t, void*);
void motion_destroy(motion_ctx_t*);
void motion_add_frame(motion_ctx_t*, const uint8_t*, uint32_t, uint32_t);



#endif /* ! MOTION_H_INCLUDED */
