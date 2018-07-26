#ifndef __CAMARA
#define __CAMARA

#include "frame.h"
#include <asm/types.h>          /* for videodev2.h */
#include <linux/videodev2.h>
#include <SDL.h>
#include <SDL_thread.h>
#include <SDL_mutex.h>
#include <turbojpeg.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#define MAX_DEVICE_NAME 1024

struct buffer {
  void *start;
  size_t length;
};

class Camara {
 public:
  Camara() { last_frame = 0; frame_mutex = 0; capture_thread = 0; max_size = 0; };
  int open(char *device);      
  int init(int width, int height);//640x480 para symadrone
  int grab(Frame **f);  
  void close();
  ~Camara() { close(); };
 protected:
  virtual void convert(Frame **f) = 0;
  static int capture_loop(void *camara);
  int width;
  int height;
  SDL_Thread *capture_thread;
  uint8_t *last_frame;
  uint32_t last_size;
  uint32_t max_size;
  SDL_mutex *frame_mutex;
  bool stop;
  tjhandle tjpeg;
  uint8_t *tmp_frame;
  struct addrinfo hints, *res;
  int sockfd;

};

class CamaraYUV : public Camara {
 protected:
  void convert(Frame **f);
};


#endif
