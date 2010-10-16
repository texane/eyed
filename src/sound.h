/*
** Made by fabien le mentec <texane@gmail.com>
** 
** Started on  Tue Jun 23 14:29:07 2009 texane
** Last update Tue Jun 23 14:49:08 2009 texane
*/



#ifndef SOUND_H_INCLUDED
# define SOUND_H_INCLUDED



struct sound_ctx;



int sound_initialize(struct sound_ctx**);
void sound_cleanup(struct sound_ctx*);
void sound_play(struct sound_ctx*, const char*);



#endif /* ! SOUND_H_INCLUDED */
