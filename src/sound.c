/*
** Made by fabien le mentec <texane@gmail.com>
** 
** Started on  Tue Jun 23 14:29:00 2009 texane
** Last update Wed Jun 24 01:26:14 2009 texane
*/



#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <stddef.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <alsa/asoundlib.h>
#include "debug.h"



/* wav format
 */

struct wav_header
{
#define WAV_RIFF_MAGIC "RIFF"
  uint8_t riff_magic[4];
  uint32_t file_size;

#define WAV_WAVE_MAGIC "WAVE"
  uint8_t wave_magic[4];

#define WAV_FORMAT_MAGIC "fmt "
  uint8_t fmt_magic[4];
  uint32_t block_size;

#define WAV_PCM_FORMAT 1
  uint16_t audio_format;
  uint16_t channels;
  uint32_t freq;
  uint32_t bytes_per_second;
  uint16_t bytes_per_block;
  uint16_t bytes_per_sample;

#define WAV_DATA_MAGIC "data"
  uint8_t data_magic[4];
  uint32_t data_size;
} __attribute__((packed));


struct wav_file
{
  const void* data;
  size_t size;
};


static int check_wav(const struct wav_file* wf)
{
  /* return 0 for success, -1 on error */

  const struct wav_header* const wh = wf->data;

  if (wf->size < sizeof(struct wav_header))
    return -1;

  /* pcm */

  if (wh->audio_format != WAV_PCM_FORMAT)
    return -1;

  /* magics */

#define STRCMP(A, B) memcmp(A, B, sizeof(B) - 1)

  if (STRCMP(wh->riff_magic, WAV_RIFF_MAGIC))
    return -1;

  if (STRCMP(wh->wave_magic, WAV_WAVE_MAGIC))
    return -1;

  if (STRCMP(wh->fmt_magic, WAV_FORMAT_MAGIC))
    return -1;

  if (STRCMP(wh->data_magic, WAV_DATA_MAGIC))
    return -1;

  /* sizes */

  if ((wf->size - 8) != wh->file_size)
    return -1;

  if (offsetof(struct wav_header, data_magic) -
      offsetof(struct wav_header, audio_format) !=
      wh->block_size)
    return -1;

  if ((wf->size - sizeof(struct wav_header)) < wh->data_size)
    return -1;

  return 0;
}


static void wav_close(struct wav_file* wf)
{
  if ((wf->data != MAP_FAILED) && wf->size)
    {
      munmap((void*)wf->data, wf->size);

      wf->data = MAP_FAILED;
      wf->size = 0;
    }
}


static int wav_open(struct wav_file* wf, const char* path)
{
  int fd = -1;
  int status = -1;
  struct stat st;

  /* reset wf */

  wf->data = MAP_FAILED;
  wf->size = 0;

  /* map the file */

  fd = open(path, O_RDONLY);
  if (fd == -1)
    {
      DEBUG_ERROR("open() == -1\n");
      goto on_error;
    }

  if (fstat(fd, &st) == -1)
    {
      DEBUG_ERROR("fstat() == -1\n");
      goto on_error;
    }

  wf->data = mmap(NULL, st.st_size, PROT_READ, MAP_SHARED, fd, 0);
  if (wf->data == MAP_FAILED)
    {
      DEBUG_ERROR("mmap() == MAP_FAILED\n");
      goto on_error;
    }

  wf->size = st.st_size;

  if (check_wav(wf) == -1)
    {
      DEBUG_ERROR("check_wav() == -1\n");

      wav_close(wf);
      goto on_error;
    }

  /* success */

  status = 0;

 on_error:

  if (fd != -1)
    close(fd);

  return status;
}


static unsigned int wav_get_channels(const struct wav_file* wf)
{
  return ((const struct wav_header*)wf->data)->channels;
}


static unsigned int wav_get_freq(const struct wav_file* wf)
{
  return ((const struct wav_header*)wf->data)->freq;
}


static unsigned int wav_get_samples_per_sec(const struct wav_file* wf)
{
  const struct wav_header* const wh = wf->data;

  return wh->bytes_per_second / wh->bytes_per_sample;
}


static void wav_get_data(const struct wav_file* wf, const void** data, unsigned int* size)
{
  const struct wav_header* const wh = wf->data;

  *size = wh->data_size - sizeof(struct wav_header);
  *data = (const unsigned char*)wf->data + sizeof(struct wav_header);
}



/* alsa sound wrapper
 */

struct sound_dev
{
  snd_pcm_t* handle;
};


static void clear_dev(struct sound_dev* d)
{
  d->handle = NULL;
}


void sound_dev_close(struct sound_dev* d)
{
  if (d->handle != NULL)
    snd_pcm_close(d->handle);

  clear_dev(d);
}


int sound_dev_open(struct sound_dev* d)
{
  int error;

  clear_dev(d);

  error = snd_pcm_open(&d->handle, "default", SND_PCM_STREAM_PLAYBACK, 0);
  if (error < 0)
    {
      DEBUG_ERROR("snd_pcm_open() == %d\n", error);
      return -1;
    }

  return 0;
}


static int conf_dev(struct sound_dev* d, unsigned int freq, unsigned int chans, unsigned int sps)
{
  snd_pcm_hw_params_t* params;
  int dir;

  snd_pcm_hw_params_alloca(&params);

  if (snd_pcm_hw_params_any(d->handle, params) < 0)
    goto on_error;

  if (snd_pcm_hw_params_set_access(d->handle, params, SND_PCM_ACCESS_RW_INTERLEAVED) < 0)
    goto on_error;

  if (snd_pcm_hw_params_set_format(d->handle, params, SND_PCM_FORMAT_S16_LE) < 0)
    goto on_error;

  if (snd_pcm_hw_params_set_channels(d->handle, params, chans) < 0)
    goto on_error;

  if (snd_pcm_hw_params_set_rate_near(d->handle, params, &freq, &dir) < 0)
    goto on_error;

  if (snd_pcm_hw_params(d->handle, params) < 0)
    goto on_error;

  return 0;

 on_error:

  return -1;
}


void sound_dev_play_wav(struct sound_dev* d, const struct wav_file* wf)
{
  const unsigned char* data;
  unsigned int size;
  unsigned int freq;
  unsigned int chans;
  unsigned int sps;
  int error;

  freq = wav_get_freq(wf);
  chans = wav_get_channels(wf);
  sps = wav_get_samples_per_sec(wf);

  if (conf_dev(d, freq, chans, sps) == -1)
    {
      DEBUG_ERROR("conf_dev() == -1\n");
      return ;
    }

  wav_get_data(wf, (const void**)&data, &size);

#define SIZE 1024

  while (size > SIZE)
    {
      /* in blocking mode, wait until there
	 is enough room in the ring buffer
       */

      error = snd_pcm_writei(d->handle, data, SIZE);
      if (error < 0)
	snd_pcm_prepare(d->handle);

      if (size < SIZE)
	break;

      size -= SIZE;
      data += SIZE;
    }

  snd_pcm_drain(d->handle);
}


int main(int ac, char** av)
{
  int status = -1;
  struct sound_dev sd;
  struct wav_file wf;

  if (wav_open(&wf, av[1], 1000000) == -1)
    return -1;

  if (sound_dev_open(&sd) == -1)
    goto on_error;

  while (1)
    {
      sound_dev_play_wav(&sd, &wf);
      getchar();
    }

 on_error:

  wav_close(&wf);

  sound_dev_close(&sd);

  return status;
}
