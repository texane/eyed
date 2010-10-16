/*
** Made by texane <texane@gmail.com>
** 
** Started on  Sun Jun 03 17:10:08 2007 texane
** Last update Mon Oct 29 15:37:21 2007 texane
*/



#ifndef CIO_H_INCLUDED
# define CIO_H_INCLUDED



#include <time.h>
#include "clist.h"
#include "cbuf.h"
#include "socket_compat.h"


/* forward declarations */
struct cio_handle;
struct cio_dispatcher;


/* error */
typedef enum
  {
    CIO_ERR_SUCCESS = 0,
    CIO_ERR_NOTIMPL,
    CIO_ERR_MEM,
    CIO_ERR_NOTFOUND,
    CIO_ERR_CLOSE,
    CIO_ERR_FAILURE
  } cio_err_t;

const char* cio_err_to_cstring(cio_err_t);
#define cio_err_is_success( e ) ((e) == CIO_ERR_SUCCESS)
#define cio_err_is_failure( e ) (!cio_err_is_success(e))


/* callback */
typedef cio_err_t (*cio_callback_t)(struct cio_handle*, struct cio_dispatcher*, void*);


/* cio handle */
typedef struct cio_handle
{
  socket_fd_t fd;
  struct sockaddr_in addr_remote;
  cio_callback_t cb_read;
  cio_callback_t cb_write;
  cio_callback_t cb_close;
  cbuf_t buf_in;
  cbuf_t buf_out;
  void* data;
  clist_node_t* node;
  char do_close;
} cio_handle_t;

cio_err_t cio_handle_init(cio_handle_t*, socket_fd_t, struct sockaddr_in*, cio_callback_t, cio_callback_t, cio_callback_t, void*);
void cio_handle_release(cio_handle_t*);
cio_err_t cio_handle_write(cio_handle_t*, unsigned char*, int);
cio_err_t cio_handle_close(cio_handle_t*);
void cio_handle_debug(cio_handle_t*);


/* cio dispatcher */
typedef struct cio_dispatcher
{
  int fd_high;
  clist_t handles;
  char is_done;
  struct timeval tm_timeout;
  void (*cb_timeout)(struct cio_dispatcher*, void*);
} cio_dispatcher_t;

cio_err_t cio_dispatcher_init(cio_dispatcher_t*);
void cio_dispatcher_release(cio_dispatcher_t*);
cio_err_t cio_dispatcher_dispatch(cio_dispatcher_t*, void*);
cio_err_t cio_dispatcher_add_handle(cio_dispatcher_t*, cio_handle_t*);
cio_err_t cio_dispatcher_find_handle_by_fd(cio_dispatcher_t*, socket_fd_t, cio_handle_t**);
void cio_dispatcher_debug(cio_dispatcher_t*);
cio_err_t cio_dispatcher_set_timeout(cio_dispatcher_t*, void (*)(struct cio_dispatcher*, void*), unsigned long);
int cio_dispatcher_is_done(cio_dispatcher_t*);


#endif /* CIO_H_INCLUDED */
