/*
** Made by texane <texane@gmail.com>
** 
** Started on Sun Oct 26 22:27:06 2008 texane
** Updated on Mon Oct 27 02:21:53 2008 texane
*/



#include <string.h>
#include <stdlib.h>
#include <unistd.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include "eyed.h"
#include "transform.h"
#include "cam.h"
#include "bmp.h"
#include "cio.h"
#include "debug.h"



/* capture a frame
 */

struct capture_ctx
{
  uint32_t count;
  bmp_t* bmp;
  uint8_t has_captured;
};


static cam_err_t on_frame(cam_dev_t* dev, cam_frame_t* frame, void* ctx)
{
  struct capture_ctx* capture_ctx = ctx;
  uint32_t width;
  uint32_t height;
  cam_err_t err;
  void* data;

  if (!capture_ctx->count)
    {
      /* wait for a complete frame
       */

      capture_ctx->count = 1;

      err = CAM_ERR_CONTINUE;

      goto on_error;
    }

  switch (frame->format)
    {
    case CAM_FORMAT_YUV420_160_120:
      width = 160;
      height = 120;
      break;

    case CAM_FORMAT_YUV420_320_240:
      width = 320;
      height = 240;
      break;

    case CAM_FORMAT_YUV420_640_480:
      width = 640;
      height = 480;
      break;

    default:
      err = CAM_ERR_NOT_SUPPORTED;
      goto on_error;
      break;
    }

  if (bmp_set_format(capture_ctx->bmp, 24, width, height) == -1)
    {
      err = CAM_ERR_FAILURE;
      goto on_error;
    }

  if ((data = bmp_get_data(capture_ctx->bmp)) == NULL)
    {
      err = CAM_ERR_RESOURCE;
      goto on_error;
    }

  transform_yuv420_to_rgb24(data, frame->data, width, height);
  transform_vflip_rgb24(data, width, height);

  capture_ctx->has_captured = 1;

  /* next frame
   */

  err = CAM_ERR_SUCCESS;

 on_error:

  return err;
}



/* handle client request
 */

static void handle_request(cam_dev_t* dev, cio_handle_t* h, const char* l)
{
  cam_err_t err;
  struct capture_ctx capture_ctx;
  const uint8_t* data;
  uint32_t size;
  uint8_t hdr[256];

  capture_ctx.count = 0;
  capture_ctx.bmp = NULL;
  capture_ctx.has_captured = 0;

  if ((capture_ctx.bmp = bmp_create()) == NULL)
    goto on_error;

  err = cam_start_capture(dev,
			  CAM_FORMAT_YUV420_160_120,
			  on_frame,
			  &capture_ctx);

  if (err != CAM_ERR_SUCCESS)
    goto on_error;

  err = cam_wait_capture(dev);

  if (err != CAM_ERR_SUCCESS)
    goto on_error;

  if (!capture_ctx.has_captured)
    goto on_error;

  size = sizeof(hdr);

  if (bmp_get_header(capture_ctx.bmp, hdr, &size) == -1)
    goto on_error;

  if ((data = bmp_get_data(capture_ctx.bmp)) == NULL)
    goto on_error;

  cbuf_push_back(&h->buf_out, (void*)hdr, size);
  cbuf_push_back(&h->buf_out, (void*)data, 160 * 120 * 3);

 on_error:

  if (capture_ctx.bmp != NULL)
    bmp_destroy(capture_ctx.bmp);

  return ;
}



/* helpers
 */

static socket_fd_t create_tcp_server(unsigned short port)
{
  static const int enabled = 1;

  socket_fd_t fd;
  struct sockaddr_in addr;

  socket_fd_reset(fd);

  memset(&addr, 0, sizeof(struct sockaddr_in));

  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = INADDR_ANY;
  addr.sin_port = htons(port);

  fd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (socket_fd_is_invalid(fd))
    {
      DEBUG_ERROR("socket");
      return fd;
    }

  if (bind(fd, (struct sockaddr*)&addr, sizeof(struct sockaddr_in)))
    {
      DEBUG_ERROR("bind()\n");

      socket_fd_close(fd);
      socket_fd_reset(fd);
      return fd;
    }

  listen(fd, 5);

  setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &enabled, sizeof(enabled));

  return fd;
}


static void close_tcp_socket(socket_fd_t fd)
{
  shutdown(fd, SHUT_RDWR);
  socket_fd_close(fd);
  socket_fd_reset(fd);
}



/* eyed server
 */

struct eyed_server
{
  cam_dev_t* cam_dev;
  cio_handle_t handle;
  cio_dispatcher_t disp;
};


static cio_err_t on_client_write(cio_handle_t* h, cio_dispatcher_t* d, void* p)
{
  int count;

  DEBUG_ENTER();

  count = send(h->fd, h->buf_out.data, cbuf_size(&h->buf_out), 0);
  if (count <= 0)
    return CIO_ERR_CLOSE;

  cbuf_pop_front(&h->buf_out, count);

  return CIO_ERR_SUCCESS;
}


static cio_err_t on_client_read(cio_handle_t* h, cio_dispatcher_t* d, void* p)
{
  struct eyed_server* server = p;
  int count;
  unsigned char buf[512];

  DEBUG_ENTER();

  count = recv(h->fd, buf, sizeof(buf), 0);
  if (count <= 0)
    return CIO_ERR_CLOSE;

#if 0
  /* consume data
   */

  cbuf_push_back(&h->buf_in, (unsigned char*)buf, count);
  cbuf_clear(&h->buf_in);
#endif /* 0 */

  handle_request(server->cam_dev, h, NULL);
  
  return CIO_ERR_SUCCESS;
}


static cio_err_t on_client_close(cio_handle_t* h, cio_dispatcher_t* d, void* p)
{
  DEBUG_ENTER();

  close_tcp_socket(h->fd);

  return CIO_ERR_SUCCESS;
}


static cio_err_t on_server_accept(cio_handle_t* h, cio_dispatcher_t* d, void* p)
{
  socket_fd_t new_fd;
  struct sockaddr_in inaddr;
  socklen_t addrlen;
  cio_handle_t* new_handle;
  cio_err_t err;

  DEBUG_ENTER();

  /* new connection
   */

  addrlen = sizeof(struct sockaddr_in);
  new_fd = accept(h->fd, (struct sockaddr*)&inaddr, &addrlen);
  if (socket_fd_is_invalid(new_fd))
    return CIO_ERR_SUCCESS;

  new_handle = malloc(sizeof(cio_handle_t));
  if (new_handle == NULL)
    {
      close_tcp_socket(new_fd);
      return CIO_ERR_SUCCESS;
    }

  err = cio_handle_init(new_handle,
			new_fd,
			&inaddr,
			on_client_read,
			on_client_write,
			on_client_close,
			p);

  if (cio_err_is_failure(err))
    {
      free(new_handle);
      close_tcp_socket(new_fd);
      return CIO_ERR_SUCCESS;
    }

  err = cio_dispatcher_add_handle(d, new_handle);
  if (cio_err_is_failure(err))
    {
      cio_handle_release(new_handle);
      free(new_handle);
      close_tcp_socket(new_fd);
      return CIO_ERR_SUCCESS;
    }

  return CIO_ERR_SUCCESS;
}


static cio_err_t on_server_close(cio_handle_t* h, cio_dispatcher_t* d, void* p)
{
  DEBUG_ENTER();

  close_tcp_socket(h->fd);

  return CIO_ERR_SUCCESS;
}



/* exported
 */

void eyed_loop(unsigned short port)
{
  cio_err_t err;
  socket_fd_t fd;
  struct eyed_server server;

  server.cam_dev = NULL;

  if (cio_dispatcher_init(&server.disp) != CIO_ERR_SUCCESS)
    return ;

  if (cam_open_dev(&server.cam_dev, 0) != CAM_ERR_SUCCESS)
    goto on_error;

  fd = create_tcp_server(port);

  if (socket_fd_is_invalid(fd))
    goto on_error;

  err = cio_handle_init(&server.handle,
			fd,
			NULL,
			on_server_accept,
			NULL,
			on_server_close,
			&server);

  if (cio_err_is_failure(err))
    goto on_error;

  /* no longer belongs to us
   */

  socket_fd_reset(fd);

  err = cio_dispatcher_add_handle(&server.disp, &server.handle);
  if (cio_err_is_failure(err))
    goto on_error;

  while (!cio_dispatcher_is_done(&server.disp))
    cio_dispatcher_dispatch(&server.disp, (void*)&server);

 on_error:

  if (!socket_fd_is_invalid(fd))
    socket_fd_close(fd);

  if (server.cam_dev != NULL)
    cam_close_dev(server.cam_dev);

  cio_dispatcher_release(&server.disp);
}
