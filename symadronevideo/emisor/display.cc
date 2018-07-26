#include "display.h"
#include <string.h>

bool Display::init() {  
  if (SDL_Init(SDL_INIT_VIDEO) < 0) {
    fprintf(stderr, "No se puede inicializar SDL: %s\n",SDL_GetError());
    return false;
  }
  return true;
}

void Display::quit() {
  SDL_Quit();
}

Display::Display(char *caption, int _width, int _height, bool *ret) {
  *ret = true;
  width = _width;
  height = _height;    
  window = SDL_CreateWindow(caption, SDL_WINDOWPOS_UNDEFINED,
			    SDL_WINDOWPOS_UNDEFINED, width, height, 0);  
  if (window == 0) {
    fprintf(stderr,"No se pudo crear la ventana\n");
    *ret = false;
  }

  renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
  if (renderer == 0) {
    fprintf(stderr,"No se pudo crear el renderizador\n");
    *ret = false;
  }
}

Display::~Display() {
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
}

void Display::clear() {
  // Color de fondo: negro
  SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
  SDL_RenderClear(renderer);
  SDL_RenderPresent(renderer);
}

bool Display::poll_quit() {
  SDL_PollEvent(&sdl_event);
  if (sdl_event.type == SDL_QUIT) return true;
  else return false;
}

unsigned long Display::now() {
  return SDL_GetTicks()*1000; // en microsegundos
}

DisplayYUV::DisplayYUV(char *caption, int width, int height, bool *ret) :
  Display(caption, width, height, ret) {
  if (*ret == false) return;
  // Overlay en toda la pantalla por defecto
  Rect rect;
  rect.x = 0;
  rect.y = 0;
  rect.w = width;
  rect.h = height;
  overlay = new OverlayYUV(this, &rect, ret); 
}

DisplayYUV::~DisplayYUV() {
  delete overlay;
}

bool DisplayYUV::display(Frame *f) {
  return overlay->display(f);
}

OverlayYUV::OverlayYUV(DisplayYUV *d, Rect *_r, bool *ret) {
  *ret = true;
  rect.x = _r->x;
  rect.y = _r->y;
  rect.w = _r->w;
  rect.h = _r->h;
  disp = d;
  texture = SDL_CreateTexture(disp->renderer,SDL_PIXELFORMAT_IYUV,
			      SDL_TEXTUREACCESS_STREAMING,
			      _r->w, _r->h);
  if (texture == NULL) {
    fprintf(stderr, "No se puede crear overlay YUV: %s\n",SDL_GetError());
    *ret = false;
  }
}

OverlayYUV::~OverlayYUV() {
  SDL_DestroyTexture(texture);
}

bool OverlayYUV::display(Frame *f) {
  bool ret = true;
  
  FrameYUV *fyuv = (FrameYUV *)f;
  void *pixels;
  int pitch;
  if (SDL_LockTexture(texture, NULL, &pixels, &pitch) < 0) {
      fprintf(stderr, "No se puede bloquear la textura: %s\n", SDL_GetError());
      ret = false;
  }
  else {
    memcpy(pixels,fyuv->Y(),fyuv->size);
    SDL_UnlockTexture(texture);
    SDL_RenderCopy(disp->renderer, texture, NULL, &rect);
    SDL_RenderPresent(disp->renderer);
  }
  
  return ret;
}
