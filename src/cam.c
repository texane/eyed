/*
** Made by texane <texane@gmail.com>
** 
** Started on Sun Oct 26 02:47:44 2008 texane
** Updated on Mon Oct 27 02:05:10 2008 texane
*/



#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/fcntl.h>
#include <linux/videodev2.h>
#include "debug.h"
#include "cam.h"



/* cam device type
 */

struct frame_buffer
{
  void* data;
  uint32_t size;
  struct v4l2_buffer v4l2;
};


struct cam_dev
{
  int fd;
  cam_format_t format;
  int is_mmap_enabled;
#define MAX_FB_COUNT 2
  struct frame_buffer fbs[MAX_FB_COUNT];
  size_t fb_count;
};



/* internal functions
 */

static cam_err_t set_format
(
 int fd,
 uint32_t fourcc,
 uint32_t width,
 uint32_t height
)
{
  struct v4l2_format format;

  format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

  if (ioctl(fd, VIDIOC_G_FMT, &format) == -1)
  {
    DEBUG_ERROR("ioctl(VIDIOC_G_FMT)\n");
    return CAM_ERR_COM;
  }

  format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  format.fmt.pix.width = width;
  format.fmt.pix.height = height;
  format.fmt.pix.pixelformat = fourcc;

  if (ioctl(fd, VIDIOC_S_FMT, &format) == -1)
  {
    DEBUG_ERROR("ioctl(VIDIOC_S_FMT)\n");
    return CAM_ERR_COM;
  }

  return CAM_ERR_SUCCESS;
}


static int expand_format
(
 cam_format_t format,
 uint32_t* fourcc,
 uint32_t* width,
 uint32_t* height
)
{
  int res;

#define EXPAND_FORMAT_CASE(c, w, h)		\
 case CAM_FORMAT_ ## c ## _ ## w ## _ ## h:	\
   *fourcc = V4L2_PIX_FMT_ ## c;		\
   *width = w;					\
   *height = h;					\
   break

  res = 0;

  switch (format)
    {
      EXPAND_FORMAT_CASE(YUV420, 160, 120);
      EXPAND_FORMAT_CASE(YUV420, 320, 240);
      EXPAND_FORMAT_CASE(YUV420, 640, 480);

      EXPAND_FORMAT_CASE(YUYV, 160, 120);
      EXPAND_FORMAT_CASE(YUYV, 320, 240);
      EXPAND_FORMAT_CASE(YUYV, 640, 480);

    default:
      res = -1;
      break;
    }

  return res;
}


static uint32_t format_to_frame_size(cam_format_t format)
{
  uint32_t c;
  uint32_t w;
  uint32_t h;
  uint32_t n;

  if (expand_format(format, &c, &w, &h) == -1)
    return 0;

  switch (c)
  {
  case V4L2_PIX_FMT_YUV420:

    n = w * h + (w * h) / 2;

    break;

  case V4L2_PIX_FMT_YUYV:

    /* bits per pixel = 16
     */

    n = w * h * 2;

    break;

  default:

    /* unknown format
     */

    n = 0;

    break;
  }

  return n;
}


static cam_err_t __attribute__((unused)) set_fps(int fd, uint8_t fps)
{
  struct v4l2_streamparm parm;

  memset(&parm, 0, sizeof(parm));

  parm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

  if (ioctl(fd, VIDIOC_G_PARM, &parm) == -1)
  {
    DEBUG_ERROR("ioctl(VIDIOC_G_PARM)\n");
    return CAM_ERR_COM;
  }

  parm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  parm.parm.capture.timeperframe.numerator = 0;
  parm.parm.capture.timeperframe.denominator =  10000000 / fps;

  if (ioctl(fd, VIDIOC_S_PARM, &parm) == -1)
  {
    DEBUG_ERROR("ioctl(VIDIOC_S_PARM)\n");
    return CAM_ERR_COM;
  }

  return CAM_ERR_SUCCESS;
}


/* mmap disabled capture loop
 */

static cam_err_t start_mmap_disabled(cam_dev_t* dev)
{
  cam_err_t err;
  int flags;

  if (!(dev->fbs[0].size = format_to_frame_size(dev->format)))
  {
    err = CAM_ERR_NOT_SUPPORTED;
    goto on_error;
  }

  if ((dev->fbs[0].data = malloc(dev->fbs[0].size)) == NULL)
  {
    err = CAM_ERR_RESOURCE;
    goto on_error;
  }

  dev->fb_count = 1;

  if (!((flags = fcntl(dev->fd, F_GETFL)) & O_NONBLOCK))
    fcntl(dev->fd, flags | O_NONBLOCK);

  err = CAM_ERR_SUCCESS;

 on_error:
  return err;
}


static cam_err_t stop_mmap_disabled(cam_dev_t* dev)
{
  if (dev->fb_count)
  {
    free(dev->fbs[0].data);
    dev->fb_count = 0;
  }

  return CAM_ERR_SUCCESS;
}


static cam_err_t capture_mmap_disabled
(
 cam_dev_t* dev,
 cam_frame_callback_t on_frame,
 void* context
)
{
  cam_err_t err = CAM_ERR_FAILURE;

  ssize_t nread;
  fd_set rds;
  int n;
  struct cam_frame frame;

  FD_ZERO(&rds);
  FD_SET(dev->fd, &rds);

  n = select(dev->fd + 1, &rds, NULL, NULL, NULL);
  if (n == -1)
  {
    if (errno != EINTR) goto on_error;
  }
  else if (n == 1)
  {
    nread = read(dev->fd, dev->fbs[0].data, dev->fbs[0].size);
    if (nread == -1)
    {
      err = CAM_ERR_COM;
      goto on_error;
    }
    else if (nread > 0)
    {
      frame.format = dev->format;
      frame.data = dev->fbs[0].data;
      frame.size = (uint32_t)nread;

      err = on_frame(dev, &frame, context);
    }
  }

  err = CAM_ERR_SUCCESS;

 on_error:

  return err;
}


/* mmap enabled capture loop
 */

static int map_frame_buffer
(
 struct cam_dev* dev,
 uint32_t index,
 struct frame_buffer* buf
)
{
  buf->data = MAP_FAILED;
  buf->size = 0;

  memset(&buf->v4l2, 0, sizeof(struct v4l2_buffer));
  buf->v4l2.index = index;
  buf->v4l2.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  buf->v4l2.memory = V4L2_MEMORY_MMAP;

  if (ioctl(dev->fd, VIDIOC_QUERYBUF, &buf->v4l2) == -1)
  {
    DEBUG_ERROR("ioctl(VIDIOC_QUERYBUF)");
    return -1;
  }

  buf->data = mmap
  (
   NULL,
   buf->v4l2.length,
   PROT_READ | PROT_WRITE,
   MAP_SHARED,
   dev->fd,
   0
  );

  if (buf->data == MAP_FAILED)
  {
    DEBUG_ERROR("mmap()\n");
    return -1;
  }

  buf->size = buf->v4l2.length;

  return 0;
}


static void unmap_frame_buffer(struct frame_buffer* buf)
{
  if (buf->data != MAP_FAILED)
  {
    munmap(buf->data, buf->size);
    buf->data = MAP_FAILED;
    buf->size = 0;
  }
}


static cam_err_t start_mmap_enabled(cam_dev_t* dev)
{
  static const enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  struct v4l2_requestbuffers req;
  cam_err_t err;
  size_t i;
  int flags;

  /* request buffers
   */

  memset(&req, 0, sizeof(req));
  req.count = 1;
  req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  req.memory = V4L2_MEMORY_MMAP;

  if (ioctl(dev->fd, VIDIOC_REQBUFS, &req) == -1)
  {
    DEBUG_ERROR("ioctl(VIDIOC_REQBUFS)\n");
    err = CAM_ERR_COM;
    goto on_error;
  }

  if (req.count < 1 || req.count > MAX_FB_COUNT)
  {
    DEBUG_ERROR("req.count: %u\n", req.count);
    err = CAM_ERR_RESOURCE;
    goto on_error;
  }

  /* alloc frame buffer
   */

  for (i = 0; i < req.count; ++i)
  {
    dev->fbs[i].data = MAP_FAILED;

    if (map_frame_buffer(dev, 0, &dev->fbs[i]) == -1)
    {
      err = CAM_ERR_RESOURCE;
      goto on_error;
    }

    ++dev->fb_count;

    /* queue buffer
     */

    if (ioctl(dev->fd, VIDIOC_QBUF, &dev->fbs[i].v4l2) == -1)
    {
      DEBUG_ERROR("ioctl(VIDIOC_QBUF)\n");
      err = CAM_ERR_COM;
      goto on_error;
    }
  }

  /* start streaming
   */

  if (ioctl(dev->fd, VIDIOC_STREAMON, &type) == -1)
  {
    DEBUG_ERROR("ioctl(VIDIOC_STREAMON)\n");
    err = CAM_ERR_COM;
    goto on_error;
  }

  if (!((flags = fcntl(dev->fd, F_GETFL)) & O_NONBLOCK))
    fcntl(dev->fd, flags | O_NONBLOCK);

  err = CAM_ERR_SUCCESS;

 on_error:
  return err;
}


static cam_err_t stop_mmap_enabled(cam_dev_t* dev)
{
  static const enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  size_t i;

  for (i = 0; i < dev->fb_count; ++i)
    unmap_frame_buffer(&dev->fbs[i]);
  dev->fb_count = 0;

  ioctl(dev->fd, VIDIOC_STREAMOFF, &type);

  return CAM_ERR_SUCCESS;
}


static cam_err_t capture_mmap_enabled
(
 cam_dev_t* dev,
 cam_frame_callback_t on_frame,
 void* context
)
{
  cam_err_t err = CAM_ERR_FAILURE;
  struct cam_frame frame;
  struct v4l2_buffer buffer;
  unsigned int bug_count = 0;

  /* run capture
   */

 redo_select:
  {
    /* this fix is required due to some hardware */
    /* then driver related bug that makes ioctl */
    /* blocks forever. restart stream on timeout. */
    fd_set rds;
    int n;
    struct timeval tm;
    FD_ZERO(&rds);
    FD_SET(dev->fd, &rds);
    tm.tv_sec = 0;
    tm.tv_usec = 500000;
    n = select(dev->fd + 1, &rds, NULL, NULL, &tm);
    if (n != 1)
    {
      stop_mmap_enabled(dev);
      start_mmap_enabled(dev);
      if (++bug_count == 4)
      {
	err = CAM_ERR_FAILURE;
	goto on_error;
      }
      goto redo_select;
    }
  }

  memset(&buffer, 0, sizeof(buffer));
  buffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  buffer.memory = V4L2_MEMORY_MMAP;

  if (ioctl(dev->fd, VIDIOC_DQBUF, &buffer) == -1)
  {
    DEBUG_ERROR("ioctl(VIDIOC_DQBUF)\n");
    err = CAM_ERR_COM;
    goto on_error;
  }

  frame.format = dev->format;
  frame.data = dev->fbs[buffer.index].data;
  frame.size = dev->fbs[buffer.index].size;

  err = on_frame(dev, &frame, context);

  if (ioctl(dev->fd, VIDIOC_QBUF, &buffer) == -1)
  {
    DEBUG_ERROR("ioctl(VIDIOC_QBUF)\n");
    err = CAM_ERR_COM;
    goto on_error;
  }

  err = CAM_ERR_SUCCESS;

 on_error:
  return err;
}


/* exported
 */

cam_err_t cam_open_dev(cam_dev_t** res, cam_format_t format, uint8_t id)
{
  cam_dev_t* dev;
  cam_err_t err;
  int fd;
#define DEV_PATH "/dev/videoNNN"
  char filename[sizeof(DEV_PATH)];
  struct v4l2_capability cap;

  dev = NULL;
  fd = -1;

  snprintf(filename, sizeof(filename), "/dev/video%d", id);

  if ((fd = open(filename, O_RDWR)) == -1)
  {
    err = CAM_ERR_NOT_FOUND;
    goto on_error;
  }

  if (ioctl(fd, VIDIOC_QUERYCAP, &cap) == -1)
  {
    DEBUG_ERROR("ioctl(VIDIOC_QUERYCAP)\n");
    err = CAM_ERR_COM;
    goto on_error;
  }

  if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE))
  {
    err = CAM_ERR_NOT_SUPPORTED;
    goto on_error;
  }

  if ((dev = malloc(sizeof(struct cam_dev))) == NULL)
  {
    err = CAM_ERR_RESOURCE;
    goto on_error;
  }

  dev->fd = fd;
  dev->format = format;
  dev->is_mmap_enabled = 0;
  dev->fb_count = 0;

  /* set format */
  {
    uint32_t width;
    uint32_t height;
    uint32_t fourcc;

    if (expand_format(format, &fourcc, &width, &height) == -1)
    {
      DEBUG_ERROR("");
      err = CAM_ERR_NOT_SUPPORTED;
      goto on_error;
    }

    err = set_format(dev->fd, fourcc, width, height);
    if (err != CAM_ERR_SUCCESS)
    {
      DEBUG_ERROR("");
      goto on_error;
    }
  }

  if (cap.capabilities & V4L2_CAP_STREAMING)
  {
    dev->is_mmap_enabled = 1;
    err = start_mmap_enabled(dev);
  }
  else
  {
    dev->is_mmap_enabled = 0;
    err = start_mmap_disabled(dev);
  }

  if (err != CAM_ERR_SUCCESS)
    goto on_error;

  *res = dev;

  return CAM_ERR_SUCCESS;

 on_error:

  cam_close_dev(dev);

  *res = NULL;

  return err;
}


void cam_close_dev(cam_dev_t* dev)
{
  if (dev->is_mmap_enabled)
    stop_mmap_enabled(dev);
  else
    stop_mmap_disabled(dev);

  close(dev->fd);

  free(dev);
}


cam_err_t cam_capture
(
 cam_dev_t* dev,
 cam_frame_callback_t on_frame,
 void* context
)
{
  cam_err_t err;

  if (dev->is_mmap_enabled)
    err = capture_mmap_enabled(dev, on_frame, context);
  else
    err = capture_mmap_disabled(dev, on_frame, context);

  return err;
}


cam_err_t cam_list_formats
(
 cam_dev_t* dev,
 cam_format_callback_t on_format,
 void* context
)
{
#if 1

  cam_err_t err = CAM_ERR_SUCCESS;

  struct v4l2_fmtdesc desc;

  on_format = on_format;

  desc.index = 0;

  while (1)
  {
    desc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    if (ioctl(dev->fd, VIDIOC_ENUM_FMT, &desc) == -1)
    {
      if (errno != EINVAL)
      {
	DEBUG_ERROR("ioctl(VIDIOC_ENUM_FMT)\n");
	err = CAM_ERR_COM;
      }

      break;
    }

    printf("%s:0x%08x\n", desc.description, desc.pixelformat);

    ++desc.index;
  }

  return err;

#else

  dev = dev;
  on_format = on_format;
  context = context;

  return CAM_ERR_NOT_IMPLEMENTED;

#endif
}
