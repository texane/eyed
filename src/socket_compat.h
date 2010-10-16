/*
** Made by texane <texane@gmail.com>
** 
** Started on Sun Oct 26 22:41:12 2008 texane
** Updated on Sun Oct 26 23:46:27 2008 texane
*/



#ifndef SOCKET_COMPAT_H_INCLUDED
# define SOCKET_COMPAT_H_INCLUDED



#ifdef WIN32

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>

#define SOCKET_SELECT_FD_HIGH 0

typedef SOCKET socket_fd_t;

#define socket_fd_close( s ) do { closesocket(s); } while (0)
#define socket_fd_is_invalid( s ) ((s) == INVALID_SOCKET)
#define socket_fd_is_valid( s ) (!socket_fd_is_invalid(s))
#define socket_fd_reset( s ) do { (s) = INVALID_SOCKET; } while (0)
#define socket_last_error( e ) do { (e) = WSALastError(); } while (0)

#define SHUT_RDWR 2


#else /* if LINUX */


#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define SOCKET_SELECT_FD_HIGH 1

typedef int socket_fd_t;

#define socket_fd_close( s ) do { close(s); } while (0)
#define socket_fd_is_invalid( s ) ((s) == -1)
#define socket_fd_is_valid( s ) (!socket_fd_is_invalid(s))
#define socket_fd_reset( s ) do { (s) = -1; } while (0)
#define socket_last_error( e ) do { (e) = errno; } while (0)


#endif /* LINUX */



#endif /* ! SOCKET_COMPAT_H_INCLUDED */
