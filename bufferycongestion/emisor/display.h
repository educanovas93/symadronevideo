#ifndef __DISPLAY
#define __DISPLAY

#include "frame.h"
#include <SDL.h>

class Display {
 public:
  static bool init();
  static void quit();
  void clear();
  bool poll_quit();
  unsigned long now();
  Display(char *caption, int width, int height, bool *ret);       
  ~Display();
  virtual bool display(Frame *f) = 0;
 protected:      
  SDL_Window *window;
  SDL_Renderer *renderer;
  int width;
  int height;
  SDL_Event sdl_event;
};

struct Rect {
  int x,y,w,h;
};

class OverlayYUV;

class DisplayYUV : public Display {
  friend class OverlayYUV;
 public:
  DisplayYUV(char *caption, int width, int height, bool *ret);
  ~DisplayYUV();
  bool display(Frame *f);
 protected:
  OverlayYUV *overlay;
};

class OverlayYUV {
 public:
  OverlayYUV(DisplayYUV *d, Rect *r, bool *ret);
  ~OverlayYUV();
  bool display(Frame *f);
 protected:
  SDL_Texture *texture;
  SDL_Rect rect;
  DisplayYUV *disp;
};

#endif
