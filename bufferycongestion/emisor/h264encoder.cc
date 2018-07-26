#include "h264encoder.h"

H264Encoder::H264Encoder() {
	avcodec_register_all();
	codec = avcodec_find_encoder_by_name("libx264");
	if (!codec) {
		fprintf(stderr,"No H264!\n");
		exit(1);
	}
	context = NULL;
	frame = NULL;
}

H264Encoder::~H264Encoder() {
	close();
}

bool H264Encoder::init(int width, int height, int fps, int gop, int bperiod, int rate) {
	context = avcodec_alloc_context3(codec);
	context->width = width;
	context->height = height;
	context->pix_fmt = AV_PIX_FMT_YUV420P;
	context->time_base = (AVRational){1,fps};
	context->gop_size = context->keyint_min = gop;
	context->max_b_frames = bperiod;
	context->rc_max_rate = rate;
	context->rc_buffer_size = rate/4;
	av_opt_set(context->priv_data, "preset", "ultrafast", 0);
	av_opt_set(context->priv_data, "tune", "zerolatency", 0);
	av_opt_set(context->priv_data, "x264opts", "slice-max-size=1460", 0);

	if (avcodec_open2(context, codec, NULL) < 0) {
		fprintf(stderr, "No se puede abrir el compresor!\n");
		return false;
	}

	frame = av_frame_alloc();
	if (!frame) {
		fprintf(stderr, "No se puede reservar un frame!\n");
		return false;
	}

	frame->format = context->pix_fmt;
	frame->width = context->width;
	frame->height = context->height;

	int ret = av_frame_get_buffer(frame, 1);
	if (ret < 0) {
		fprintf(stderr, "No se puede reservar el bufer del frame!\n");
		return false;
	}

	av_init_packet(&pkt);
	pts = 0;
	
	return true;
}

bool H264Encoder::encode_frame(FrameYUV *f) {
	int ret;
	if (f != NULL) {
		memcpy(frame->data[0],f->Y(),f->sizeY);
		memcpy(frame->data[1],f->U(),f->sizeUV);
		memcpy(frame->data[2],f->V(),f->sizeUV);
		frame->pts = pts++;
		ret = avcodec_send_frame(context,frame);
	}
	else {
		ret = avcodec_send_frame(context,NULL);
	}
	
	if (ret < 0) {
		fprintf(stderr,"Error codificando frame!\n");
		return false;
	}
	return true;
}


bool H264Encoder::read_packet(uint8_t **output, uint32_t * out_size, int64_t *out_pts) {
	int ret = avcodec_receive_packet(context,&pkt);
	if (ret == AVERROR_EOF) {
		fprintf(stderr,"Ya no quedan más frames!\n");
		return false;
	}
	else if (ret == 0) {
		// pkt ha recibido un frame comprimido
		*output = pkt.data; 
		*out_size = pkt.size;
		*out_pts = pkt.pts;
	}
	else {
		*output = 0;
		*out_size = 0;
		*out_pts = 0;
	}
	return true;
}

void H264Encoder::free_packet() {
	av_packet_unref(&pkt);
}

void H264Encoder::close() {
	if (context != NULL) {
		avcodec_close(context);
		av_free(context);
	}
	if (frame != NULL) {
		av_frame_free(&frame);
	}
}






