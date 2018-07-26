#ifndef __H264ENCODER
#define __H264ENCODER

#include "frame.h"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/opt.h>
#include <libavutil/frame.h>
}

class H264Encoder {
 public:
  H264Encoder();
  ~H264Encoder();
  bool init(int width, int height, int fps, int gop, int bperiod, int rate);
  bool encode_frame(FrameYUV *f);
  bool read_packet(uint8_t **output, uint32_t * out_size, int64_t *out_pts);
  void free_packet();	
  void close();
 private:
  AVCodec *codec;
  AVCodecContext *context;
  AVFrame *frame;
  AVPacket pkt;
  int64_t pts;
};

#endif
