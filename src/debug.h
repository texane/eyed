/*
** Made by texane <texane@gmail.com>
** 
** Started on Sun Oct 26 14:48:36 2008 texane
** Updated on Sun Oct 26 16:21:22 2008 texane
*/



#ifndef DEBUG_H_INCLUDED
# define DEBUG_H_INCLUDED



#if _DEBUG

# include <stdio.h>

# define DEBUG_ENTER() printf("[>] %s\n", __FUNCTION__)
# define DEBUG_LEAVE() printf("[<] %s\n", __FUNCTION__)
# define DEBUG_PRINTF(s, ...) printf("[?] %s_%u: " s, __FUNCTION__, __LINE__, ## __VA_ARGS__)
# define DEBUG_ERROR(s, ...) printf("[!] %s_%u: " s, __FUNCTION__, __LINE__, ## __VA_ARGS__)

#else

# define DEBUG_ENTER()
# define DEBUG_LEAVE()
# define DEBUG_PRINTF(s, ...)
# define DEBUG_ERROR(s, ...)

#endif /* ! _DEBUG */



#endif /* ! DEBUG_H_INCLUDED */
