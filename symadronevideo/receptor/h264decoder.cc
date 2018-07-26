#include "h264decoder.h"
#include "frame.h"

H264Decoder::H264Decoder() {
  avcodec_register_all();
  codec = avcodec_find_decoder(AV_CODEC_ID_H264);
  if (!codec) {
    fprintf(stderr,"No se puede encontrar el descompresor H264!\n");
    exit(1);
  }
  context = NULL;
  frame = NULL;
}

bool H264Decoder::init() {
  context = avcodec_alloc_context3(codec);
  if (!context) {
    fprintf(stderr, "Error: no se puede reservar contexto!\n");
    exit(1);
  }
  context->pix_fmt = AV_PIX_FMT_YUV420P;
  if (avcodec_open2(context, codec, NULL) < 0) {
    fprintf(stderr, "No se puede abrir el descompresor!\n");
    exit(1);
  }
  frame = av_frame_alloc();
  if (!frame) {
    fprintf(stderr, "No se puede reservar un frame!\n");
    exit(1);
  }
  return true;
}

bool H264Decoder::decode_packet(uint8_t *au, uint32_t au_size, int64_t in_pts) {
  av_init_packet(&pkt);
  pkt.data = au;
  pkt.size = au_size;
  pkt.pts = in_pts;
  
  int ret = avcodec_send_packet(context,&pkt);
  if (ret < 0) {
    fprintf(stderr,"Error descomprimiendo frame!\n");
    exit(1);
  }
  return true;
}

bool H264Decoder::read_frame(FrameYUV **f, int64_t *out_pts) {

  int ret = avcodec_receive_frame(context,frame);
  if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) return false;
  if (ret < 0) {
    fprintf(stderr,"Error leyendo frame!\n");
    exit(2);
  }

  // Tenemos un frame
  FrameYUV *faux;
  if (!*f) {
    // Primer frame: reservo la memoria
    faux = new FrameYUV(frame->width,frame->height);
    *f = faux;
  }
  else 
    faux = *f; 

  // Copia los datos al FrameYUV de salida
  // Plano Y
  uint8_t * Y_out_ptr, * Y_in_ptr;
  Y_in_ptr = frame->data[0];
  Y_out_ptr = faux->Y();
  for (int i = 0; i < frame->height; i++) {
    memcpy(Y_out_ptr, Y_in_ptr, frame->width);
    Y_out_ptr += frame->width;
    Y_in_ptr += frame->linesize[0];
  }
  // Planos U y V
  uint8_t * U_out_ptr, *V_out_ptr, * U_in_ptr, * V_in_ptr;
  U_in_ptr = frame->data[1];
  V_in_ptr = frame->data[2];
  U_out_ptr = faux->U();
  V_out_ptr = faux->V();
  for (int i = 0; i < (frame->height)/2; i++) {
    memcpy(U_out_ptr, U_in_ptr, (frame->width)/2);
    U_out_ptr += (frame->width)/2;
    U_in_ptr += frame->linesize[1];
    memcpy(V_out_ptr, V_in_ptr, (frame->width)/2);
    V_out_ptr += (frame->width)/2;
    V_in_ptr += frame->linesize[2];
  } 
  // PTS del frame
  *out_pts = frame->pts;
  return true;
  
}

void H264Decoder::close() {
  if (context) {
    avcodec_close(context);
    av_free(context);
    context = NULL;
  }
  if (frame) {
    av_frame_free(&frame);
    frame = NULL;
  }
}

H264Decoder::~H264Decoder() {
  close();
}
