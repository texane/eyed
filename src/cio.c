/*
** Made by texane <texane@gmail.com>
** 
** Started on  Thu May 24 20:06:43 2007 texane
** Last update Mon Oct 29 15:53:36 2007 texane
*/



#include <string.h>
#include <stdlib.h>
#include "clist.h"
#include "debug.h"
#include "cio.h"
#include "socket_compat.h"



/* error implementation
 */

const char* cio_err_to_cstring(cio_err_t err)
{
  return "not_implemented";
}



/* handle implementation
 */

static void cio_handle_reset(cio_handle_t* h)
{
  h->cb_read = 0;
  h->cb_write = 0;
  h->cb_close = 0;
  socket_fd_reset(h->fd);
  memset(&h->addr_remote, 0, sizeof(struct sockaddr_in));
  h->data = 0;
  h->node = 0;
  h->do_close = 0;
  cbuf_init(&h->buf_in);
  cbuf_init(&h->buf_out);
}


cio_err_t cio_handle_init(cio_handle_t* h,
			  socket_fd_t fd,
			  struct sockaddr_in* addr_remote,
			  cio_callback_t cb_read,
			  cio_callback_t cb_write,
			  cio_callback_t cb_close,
			  void* data)
{
  cio_handle_reset(h);

  h->fd = fd;
  if (addr_remote)
    memcpy(&h->addr_remote, addr_remote, sizeof(struct sockaddr_in));
  h->cb_read = cb_read;
  h->cb_write = cb_write;
  h->cb_close = cb_close;
  h->data = data;
  cbuf_init(&h->buf_in);
  cbuf_init(&h->buf_out);

  return CIO_ERR_SUCCESS;
}


static void release_handle(clist_node_t* node,
			   void* data)
{
  data = data;

  cio_handle_release(clist_node_data(node));
}


void cio_handle_release(cio_handle_t* handle)
{
  /* fixme:
     it s possible to release the dispatcher
     without the handle descriptor closed.
     should let a chance to the handle to
     have cb_close() invoked, but we dont have
     the necessary information (ie. dispatcher
     and private data) to do so.
   */

  cbuf_release(&handle->buf_in);
  cbuf_release(&handle->buf_out);

  if (socket_fd_is_valid(handle->fd))
    DEBUG_ERROR("cio_handle_release(handle->fd != -1)\n");

  cio_handle_reset(handle);
}


void cio_handle_debug(cio_handle_t* h)
{
}


cio_err_t cio_handle_write(cio_handle_t* h, unsigned char* buf, int sz)
{
  cbuf_push_back(&h->buf_out, buf, sz);
  return CIO_ERR_SUCCESS;
}


cio_err_t cio_handle_close(cio_handle_t* h)
{
  h->do_close = 1;
  return CIO_ERR_SUCCESS;
}



/* dispatcher implementation
 */

static void cio_dispatcher_reset(cio_dispatcher_t* d)
{
  socket_fd_reset(d->fd_high);
  d->is_done = 0;
  d->cb_timeout = 0;
}


cio_err_t cio_dispatcher_init(cio_dispatcher_t* d)
{
  cio_dispatcher_reset(d);
  clist_init(&d->handles);

  return CIO_ERR_SUCCESS;
}


void cio_dispatcher_release(cio_dispatcher_t* d)
{
  clist_release(&d->handles, release_handle, NULL);
  cio_dispatcher_reset(d);
}


void cio_dispatcher_debug(cio_dispatcher_t* d)
{
}


static void load_fdsets(clist_t* l, fd_set* rds, fd_set* wrs)
{
  clist_node_t* node;
  cio_handle_t* handle;

  FD_ZERO(rds);
  FD_ZERO(wrs);

  for (node = clist_head(l);
       node != NULL;
       node = clist_node_next(node))
    {
      handle = clist_node_data(node);
      if (handle->cb_read)
	{
	  DEBUG_PRINTF("rds += %d\n", handle->fd);
	  FD_SET(handle->fd, rds);
	}
      if (handle->cb_write && cbuf_size(&handle->buf_out))
	{
	  DEBUG_PRINTF("wrs += %d\n", handle->fd);
	  FD_SET(handle->fd, wrs);
	}
    }
}

cio_err_t cio_dispatcher_dispatch(cio_dispatcher_t* d, void* ctx)
{
  fd_set rds;
  fd_set wrs;
  int nfds;
  int do_close;
  char done;
  clist_node_t* node;
  cio_handle_t* handle;
  cio_err_t err;
  struct timeval tm_timeout;

  /* do select */
  load_fdsets(&d->handles, &rds, &wrs);
  do {
    errno = 0;
    DEBUG_PRINTF("[>]select()\n");
    if (d->cb_timeout)
      {
	memcpy(&tm_timeout, &d->tm_timeout, sizeof(struct timeval));
	nfds = select(d->fd_high + 1, &rds, &wrs, 0, &tm_timeout);
      }
    else
      {
	nfds = select(d->fd_high + 1, &rds, &wrs, 0, 0);
      }
    DEBUG_PRINTF("[<]select() == %d\n", nfds);
  } while (nfds == -1 && errno == EINTR);

  /* error */
  if (nfds == -1)
    {
      d->is_done = 1;
    }
  /* timeout */
  else if (nfds == 0)
    {
      d->cb_timeout(d, ctx);
    }
  /* process fds */  
  else
    {
      done = 0;
      node = clist_head(&d->handles);
      if (node == NULL)
	done = 1;

      while (done == 0)
	{
	  do_close = 0;
	  handle = clist_node_data(node);

	  if (FD_ISSET(handle->fd, &rds))
	    {
	      DEBUG_PRINTF("FD_ISSET(%d, rds)\n", handle->fd);

	      err = CIO_ERR_SUCCESS;
	      if (handle->cb_read)
		err = handle->cb_read(handle, d, ctx);
	      if (cio_err_is_failure(err))
		do_close = 1;
	      --nfds;
	    }

	  if (FD_ISSET(handle->fd, &wrs))
	    {
	      DEBUG_PRINTF("FD_ISSET(%d, wrs)\n", handle->fd);

	      err = CIO_ERR_SUCCESS;
	      if (do_close == 0 && handle->cb_write)
		err = handle->cb_write(handle, d, ctx);
	      if (cio_err_is_failure(err))
		do_close = 1;
	      --nfds;
	    }

	  /* next node */
	  node = clist_node_next(node);
	  if (node == NULL)
	    done = 1;

	  /* close the handle + remove node */
	  if (do_close)
	    {
	      DEBUG_PRINTF("FD_ISSET(%d, close)\n", handle->fd);

	      if (handle->cb_close != NULL)
		handle->cb_close(handle, d, ctx);
	      /* remove the node */
	      clist_remove(&d->handles, handle->node, release_handle, NULL);
	    }
	}
    }

  return CIO_ERR_SUCCESS;
}


cio_err_t cio_dispatcher_add_handle(cio_dispatcher_t* d, cio_handle_t* h)
{
  clist_node_t* node;

  node = malloc(sizeof(clist_node_t));
  if (node == 0)
    return CIO_ERR_MEM;
  h->node = node;
  clist_node_init(node, (void*)h);
  clist_push_back(&d->handles, node);

#if SOCKET_SELECT_FD_HIGH
  if (h->fd > d->fd_high)
    d->fd_high = h->fd;
#else
  d->fd_high = 0;
#endif /* SOCKET_SELECT_FD_HIGH */
  
  return CIO_ERR_SUCCESS;
}


#if SOCKET_SELECT_FD_HIGH
cio_err_t cio_dispatcher_remove_handle(cio_dispatcher_t* d, cio_handle_t* handle)
{
  clist_node_t* node;
  char do_high;

  do_high = 0;

  if (handle->fd >= d->fd_high)
    do_high = 1;

  /* remove the node */
  if (clist_remove(&d->handles, handle->node, release_handle, NULL) == -1)
    return CIO_ERR_FAILURE;

  if (do_high == 1)
    {
      d->fd_high = -1;
      for (node = d->handles.head; node; node = node->next)
	{
	  if (d->fd_high <= ((cio_handle_t*)node->data)->fd)
	    d->fd_high = ((cio_handle_t*)node->data)->fd;
	}
    }
  return CIO_ERR_SUCCESS;
}
#else /* SOCKET_SELECT_FD_HIGH == 0*/
cio_err_t cio_dispatcher_remove_handle(cio_dispatcher_t* d,
				       cio_handle_t* handle)
{
  /* remove the node */
  if (clist_remove(&d->handles,
		   handle->node,
		   release_handle,
		   NULL) == -1)
    return CIO_ERR_FAILURE;

  return CIO_ERR_SUCCESS;
}
#endif /* SOCKET_SELECT_FD_HIGH */


cio_err_t cio_dispatcher_find_handle_by_fd(cio_dispatcher_t* d,
					   socket_fd_t fd,
					   cio_handle_t** res)
{
  clist_node_t* node;
  cio_handle_t* handle;

  *res = NULL;

  for (node = clist_head(&d->handles);
       node != NULL;
       node = clist_node_next(node))
    {
      handle = clist_node_data(node);
      if (handle->fd == fd)
	{
	  *res = handle;
	  return CIO_ERR_SUCCESS;
	}
    }

  return CIO_ERR_NOTFOUND;
}



/* set timeout
 */

cio_err_t cio_dispatcher_set_timeout(cio_dispatcher_t* d,
				     void (*cb_timeout)(struct cio_dispatcher*, void*),
				     unsigned long usec)
{
  d->tm_timeout.tv_sec = usec / 1000000;
  d->tm_timeout.tv_usec = usec % 1000000;
  d->cb_timeout = cb_timeout;

  return CIO_ERR_SUCCESS;
}


int cio_dispatcher_is_done(cio_dispatcher_t* d)
{
  return d->is_done;
}
