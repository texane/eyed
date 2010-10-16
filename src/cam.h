/*
** Made by texane <texane@gmail.com>
** 
** Started on Sun Oct 26 02:44:33 2008 texane
** Updated on Mon Oct 27 02:03:55 2008 texane
*/



#ifndef CAM_H_INCLUDED
# define CAM_H_INCLUDED



#include <stdint.h>



typedef enum cam_err
  {
    CAM_ERR_SUCCESS = 0,
    CAM_ERR_CONTINUE,
    CAM_ERR_BUSY,
    CAM_ERR_NOT_FOUND,
    CAM_ERR_COM,
    CAM_ERR_NOT_SUPPORTED,
    CAM_ERR_RESOURCE,
    CAM_ERR_LOCK,
    CAM_ERR_ALREADY,
    CAM_ERR_NOT_IMPLEMENTED,
    CAM_ERR_FAILURE
  } cam_err_t;


typedef enum cam_format
  {
    CAM_FORMAT_YUV420_160_120 = 0,
    CAM_FORMAT_YUV420_320_240,
    CAM_FORMAT_YUV420_640_480,

    CAM_FORMAT_YUYV_160_120,
    CAM_FORMAT_YUYV_320_240,
    CAM_FORMAT_YUYV_640_480,

    CAM_FORMAT_INVALID

  } cam_format_t;


typedef struct cam_frame
{
  cam_format_t format;
  const uint8_t* data;
  uint32_t size;
} cam_frame_t;


typedef struct cam_dev cam_dev_t;


typedef cam_err_t (*cam_frame_callback_t)(cam_dev_t*, cam_frame_t*, void*);
typedef cam_err_t (*cam_format_callback_t)(cam_dev_t*, cam_format_t, void*);



cam_err_t cam_open_dev(cam_dev_t**, cam_format_t, uint8_t);
void cam_close_dev(cam_dev_t*);
cam_err_t cam_capture(cam_dev_t*, cam_frame_callback_t, void*);
cam_err_t cam_list_formats(cam_dev_t*, cam_format_callback_t, void*);



#endif /* ! CAM_H_INCLUDED */
