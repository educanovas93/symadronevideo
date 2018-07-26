#ifndef __PLAYOUTBUFFER
#define __PLAYOUTBUFFER

#include <list>
#include <SDL.h>
#include <SDL_thread.h>
#include <SDL_mutex.h>

struct FrameBuffer {
	uint8_t *buffer;
	uint32_t size;
	int64_t pts;
	uint32_t seqNum;
};

class PlayoutBuffer {
public:
	PlayoutBuffer();
	~PlayoutBuffer();
	int size() {return listaFrames.size();};
	bool insertFrame(uint8_t *buffer, uint32_t size, int64_t pts, uint32_t seqNum);
	bool getFrame(uint8_t **buffer, uint32_t *size, int64_t *pts);
	void getTimeInBuffer(int64_t *pts);
protected:
	bool primero;
	SDL_mutex * frame_mutex;
	std::list<FrameBuffer> listaFrames;
	uint32_t seq;
};

#endif
