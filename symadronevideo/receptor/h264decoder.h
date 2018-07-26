#ifndef __H264Decoder
#define __H264Decoder

#include "frame.h"
extern "C" {
#include <libavcodec/avcodec.h>
}

class H264Decoder {
 public:
  H264Decoder();
  ~H264Decoder();
  bool init();
  bool decode_packet(uint8_t *au, uint32_t au_size, int64_t in_pts);
  bool read_frame(FrameYUV **f, int64_t *out_pts);
  void close();
 private:
  AVCodec *codec;
  AVCodecContext *context;
  AVFrame *frame;
  AVPacket pkt;
};

#endif
