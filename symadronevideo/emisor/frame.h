#ifndef __FRAME
#define __FRAME

#include <stdlib.h>
#include <string.h>

class Frame {
 public:
 Frame(int _w, int _h) : width(_w), height(_h), data(0) {};
  ~Frame() { if (data != 0) free(data); };
  unsigned char *data;
  int width;
  int height;
  int size;      
  void clear() { memset(data,0,size); };
};

class FrameYUYV : public Frame
{
 public:
 FrameYUYV(int _w, int _h): Frame(_w,_h) {
    size = _w*_h + 2 * (_w/2 * _h);
    data = (unsigned char *)malloc(size);
    clear();
  }      
};

class FrameYUV : public Frame
{
 public:
 FrameYUV(int _w, int _h): Frame(_w,_h) {
    sizeY = _w*_h;
    sizeUV = (_w>>1)*(_h>>1);
    size = sizeY + (sizeUV*2);
    data = (unsigned char *)malloc(size);
  }      
  unsigned char *Y() {
    return data;      
  }
  unsigned char *U() {
    return data + sizeY;
  }
  unsigned char *V() {
    return data + sizeY + sizeUV;
  }            
  int sizeUV;
  int sizeY;
};

#endif
