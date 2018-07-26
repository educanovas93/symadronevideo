#ifndef __CAMARA
#define __CAMARA

#include "frame.h"
#include <asm/types.h>          /* for videodev2.h */
#include <linux/videodev2.h>
#include <SDL.h>
#include <SDL_thread.h>
#include <SDL_mutex.h>
#include <turbojpeg.h>
#define MAX_DEVICE_NAME 1024

struct buffer {
  void *start;
  size_t length;
};

class Camara {
 public:
  Camara() { grab_fd = -1; grab_data = 0; last_frame = 0; frame_mutex = 0; capture_thread = 0; buffers = 0; };
  int open(char *device);      
  int init(int width, int height);
  int grab(Frame **f);      
  void close();
  ~Camara() { close(); };
 protected:
  void errno_msg(const char * s);
  int xioctl(int fd, int request, void *arg);
  virtual void convert(Frame **f) = 0;
  virtual uint32_t getMode() = 0;
  static int capture_loop(void *camara);
  int width;
  int height;      
  int grab_fd;
  struct buffer *buffers;
  int n_buffers;
  struct v4l2_capability grab_cap;
  struct v4l2_format grab_fmt;
  struct v4l2_requestbuffers grab_req;
  struct v4l2_buffer grab_buf;
  char dev[MAX_DEVICE_NAME];
  unsigned char *grab_data;
  SDL_Thread *capture_thread;
  uint8_t *last_frame;
  uint32_t last_size;
  SDL_mutex *frame_mutex;
  bool stop;
  tjhandle tjpeg;
  uint8_t *tmp_frame;
};

class CamaraYUV : public Camara {
 protected:
  void convert(Frame **f);
  uint32_t getMode();
};

#endif
