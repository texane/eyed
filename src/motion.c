/*
** Made by texane <texane@gmail.com>
** 
** Started on Sun Oct 26 21:36:42 2008 texane
** Updated on Sun Oct 26 21:36:53 2008 texane
*/



#include "debug.h"
#include "motion.h"



int motion_create(motion_ctx_t**, motion_callback_t, void*);
void motion_destroy(motion_ctx_t*);
void motion_add_frame(motion_ctx_t*, const uint8_t*, uint32_t, uint32_t);
