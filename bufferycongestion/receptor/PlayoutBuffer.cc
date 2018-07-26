#include <iostream>
#include "PlayoutBuffer.h"

PlayoutBuffer::PlayoutBuffer(){
	seq = 0;
	frame_mutex = SDL_CreateMutex();
	primero = true;
}

PlayoutBuffer::~PlayoutBuffer(){
	if (frame_mutex != 0) {
	    SDL_DestroyMutex(frame_mutex);
	    frame_mutex = 0;
	}
}

bool PlayoutBuffer::insertFrame(uint8_t *buffer, uint32_t size, int64_t pts, uint32_t seqNum){
	if(primero)
		primero = false;
	if (seq > seqNum)
		return false;
	FrameBuffer frame;
	frame.buffer = (uint8_t *) malloc(size);
	memcpy(frame.buffer, buffer, size);
	frame.size = size;
	frame.pts = pts;
	frame.seqNum = seqNum;
	SDL_mutexP(frame_mutex);
	listaFrames.push_back(frame);
	SDL_mutexV(frame_mutex);
	seq = seqNum;
	return true;
}

bool PlayoutBuffer::getFrame(uint8_t **buffer, uint32_t *size, int64_t *pts){
	SDL_mutexP(frame_mutex);
	if(listaFrames.size() == 0)
		return false;
	FrameBuffer f = listaFrames.front();
	*buffer = f.buffer;
	*size = f.size;
	*pts = f.pts;
	listaFrames.pop_front();
	SDL_mutexV(frame_mutex);	
	return true;
}

void PlayoutBuffer::getTimeInBuffer(int64_t *pts){
		*pts = (listaFrames.size()*1000)/30;
}
