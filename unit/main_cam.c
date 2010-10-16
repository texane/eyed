/*
** Made by texane <texane@gmail.com>
** 
** Started on Sun Oct 26 02:43:57 2008 texane
** Updated on Sun Oct 26 22:13:58 2008 texane
*/



#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include "transform.h"
#include "bmp.h"
#include "cam.h"



static cam_err_t on_frame(cam_dev_t* dev, cam_frame_t* frame, void* ctx)
{
  uint32_t* count = ctx;
  uint32_t width;
  uint32_t height;
  cam_err_t err;
  bmp_t* bmp;
  void* data;
  char filename[32];

  if ((bmp = bmp_create()) == NULL)
    {
      err = CAM_ERR_RESOURCE;
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

  if (bmp_set_format(bmp, 24, width, height) == -1)
    {
      err = CAM_ERR_FAILURE;
      goto on_error;
    }

  if ((data = bmp_get_data(bmp)) == NULL)
    {
      err = CAM_ERR_RESOURCE;
      goto on_error;
    }

  transform_yuv420_to_rgb24(data, frame->data, width, height);
  transform_vflip_rgb24(data, width, height);

  snprintf(filename, sizeof(filename), "/tmp/%08x.bmp", *count);
  if (bmp_store_file(bmp, filename) == -1)
    {
      err = CAM_ERR_FAILURE;
      goto on_error;
    }

  ++*count;

  /* next frame
   */

  err = CAM_ERR_CONTINUE;

 on_error:

  if (bmp != NULL)
    bmp_destroy(bmp);

  return err;
}



int main(int ac, char** av)
{
  cam_err_t err;
  cam_dev_t* dev;
  uint32_t count;

  if ((err = cam_open_dev(&dev, 0)) != CAM_ERR_SUCCESS)
    {
      printf("cam_open_dev() == %u\n", err);
      return -1;
    }

  count = 0;

  err = cam_start_capture(dev,
			  CAM_FORMAT_YUV420_160_120,
			  on_frame,
			  &count);

  if (err != CAM_ERR_SUCCESS)
    {
      printf("cam_start_capture() == %u\n", err);
      goto on_error;
    }

  {
    char c;

    printf("any key to stop\n");
    read(0, &c, 1);
  }

  cam_stop_capture(dev);

 on_error:

  cam_close_dev(dev); 

  if (err == CAM_ERR_SUCCESS) 
    return 0;

  return -1;
}
