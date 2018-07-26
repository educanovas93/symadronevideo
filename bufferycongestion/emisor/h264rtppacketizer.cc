#include <stdio.h>
#include "h264rtppacketizer.h"
#include "rtpsender.h"

bool H264RTPPacketizer::init(uint16_t fps, void *sender) {
	this->sender = sender;
	this->period = RTPCLOCK / fps;
}


bool H264RTPPacketizer::send(uint8_t *buffer, uint32_t size, int64_t pts) {
	uint32_t inicio, fin, ceros;
	inicio = 0;
	fin = 0;
	ceros = 0;

	while (fin < size) {
		switch ( buffer[fin++] ) {
			case 0x00: 
				ceros++;
				break;
			case 0x01:
				if (ceros >= 2)  {
					// Comienzo de una NALU
					nalu_start.push_back(fin);
					if (inicio != 0) 							nalu_size.push_back(fin-inicio-ceros-1);
					inicio = fin;
				}
			default:
				ceros = 0;
		}
	}	
	nalu_size.push_back(fin-inicio);


	uint16_t aux;
	while (nalu_start.size() != 0) {
		aux = nalu_size.front();
		nalu_size.pop_front();
		inicio = nalu_start.front();
		nalu_start.pop_front();
		//printf("NALU size=%d start=%d\n",aux,inicio);
		((RTPSender*)sender)->send(&(buffer[inicio]),aux,period*pts,
			(nalu_start.size() == 0 ? true : false));
	}

	return true;
}
