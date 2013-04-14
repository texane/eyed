/*
** Made by texane <texane@gmail.com>
** 
** Started on Sun Oct 26 02:43:57 2008 texane
** Updated on Mon Oct 27 00:46:41 2008 texane
*/



#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include "debug.h"
#include "cam.h"



#define CONFIG_PROCESSING 1
#define CONFIG_TRANSFORM 1


#if CONFIG_PROCESSING

#include "transform.h"
#include "bmp.h"

struct capture_ctx
{
  struct timeval tms_total[2];
  struct timeval tms_processing[2];
  bmp_t* bmp;
};


static cam_err_t on_frame(cam_dev_t* dev, cam_frame_t* frame, void* ctx)
{
  struct capture_ctx* capture_ctx = ctx;

  void (*transform)(uint8_t*, const uint8_t*, uint32_t, uint32_t);
  uint32_t width;
  uint32_t height;
  cam_err_t err;
  void* data;

  gettimeofday(&capture_ctx->tms_processing[0], NULL);

  /* choose transfrom
   */

  switch (frame->format)
    {
    case CAM_FORMAT_YUV420_160_120:
    case CAM_FORMAT_YUV420_320_240:
    case CAM_FORMAT_YUV420_640_480:
      transform = transform_yuv420_to_rgb24;
      break;

    case CAM_FORMAT_YUYV_160_120:
    case CAM_FORMAT_YUYV_320_240:
    case CAM_FORMAT_YUYV_640_480:
      transform = transform_yuyv_to_rgb24;
      break;

    default:
      err = CAM_ERR_NOT_SUPPORTED;
      goto on_error;
      break;

    }

  /* choose dimension
   */

  switch (frame->format)
    {
    case CAM_FORMAT_YUV420_160_120:
    case CAM_FORMAT_YUYV_160_120:
      width = 160;
      height = 120;
      break;

    case CAM_FORMAT_YUV420_320_240:
    case CAM_FORMAT_YUYV_320_240:
      width = 320;
      height = 240;
      break;

    case CAM_FORMAT_YUV420_640_480:
    case CAM_FORMAT_YUYV_640_480:
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

#if CONFIG_TRANSFORM
  transform(data, frame->data, width, height);
#endif

  transform_vflip_rgb24(data, width, height);

  /* next frame
   */

  err = CAM_ERR_SUCCESS;

 on_error:

  gettimeofday(&capture_ctx->tms_processing[1], NULL);

  return err;
}

#else /* !CONFIG_PROCESSING */

struct capture_ctx
{
  struct timeval tms_total[2];
  struct timeval tms_processing[2];
};

static cam_err_t on_frame(cam_dev_t* dev, cam_frame_t* frame, void* ctx)
{
  struct capture_ctx* capture_ctx = ctx;

  dev = dev;
  frame = frame;
  ctx = ctx;

  gettimeofday(&capture_ctx->tms_processing[0], NULL);
  gettimeofday(&capture_ctx->tms_processing[1], NULL);

  return CAM_ERR_SUCCESS;
}

#endif


static void get_objects(cam_dev_t* dev)
{
  unsigned int i;
  cam_err_t err;
  struct timeval tms[2];
  struct timeval tmp;

#if CONFIG_PROCESSING
  struct capture_ctx context;
  memset(&context, 0, sizeof(context));
  if ((context.bmp = bmp_create()) == NULL)
  {
    DEBUG_ERROR("bmp_create() == NULL\n");
    return ;
  }
#endif

  for (i = 0; i < 2; ++i)
  {
    gettimeofday(&context.tms_total[0], NULL);
    {
#if CONFIG_PROCESSING
      err = cam_capture(dev, on_frame, &context);
#else /* !CONFIG_PROCESSING */
      err = cam_capture(dev, on_frame, &context);
#endif
    }
    gettimeofday(&context.tms_total[1], NULL);

    timersub(&context.tms_processing[1], &context.tms_processing[0], &tms[1]);
    timersub(&context.tms_total[1], &context.tms_total[0], &tmp);
    timersub(&tmp, &tms[1], &tms[0]);
#define TMVAL_TO_MSECS(T) (((float)(T).tv_sec * 1000000.f + (float)T.tv_usec) / 1000.f)
    printf("%f, %f msecs\n", TMVAL_TO_MSECS(tms[0]), TMVAL_TO_MSECS(tms[1]));
  }

#if CONFIG_PROCESSING
  if (err == CAM_ERR_SUCCESS)
  {
    bmp_store_file(context.bmp, "/tmp/foo.bmp");
    bmp_destroy(context.bmp);
  }
#endif

}


int main(int ac, char** av)
{
  cam_err_t err = CAM_ERR_FAILURE;
  cam_dev_t* dev = NULL;

#if 0
  if ((err = cam_open_dev(&dev, CAM_FORMAT_YUYV_640_480, 1)) != CAM_ERR_SUCCESS)
#else
  if ((err = cam_open_dev(&dev, CAM_FORMAT_YUYV_640_480, 0)) != CAM_ERR_SUCCESS)
#endif
  {
    printf("[!] cam_open_dev() == %u\n", err);
    goto on_error;
  }

  get_objects(dev);

 on_error:

  if (dev != NULL)
    cam_close_dev(dev);

  return 0;
}
