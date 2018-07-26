#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "rtpreceiver.h"
#include "h264decoder.h"
#include "display.h"
#include "PlayoutBuffer.h"

bool stop;

void sighand(int sig) {
  stop = true;
}

int displayLoop(void * buffer) {
  H264Decoder dec;
  dec.init();
  FrameYUV *f = NULL;
  FILE * file = fopen("salida.264", "wb");

  PlayoutBuffer * buf = (PlayoutBuffer *) buffer;

  if (!DisplayYUV::init()) exit(1);

  bool retb;
  DisplayYUV *display = NULL;

  sleep(2);

  while(!stop) {
		int64_t tiempo;
		uint8_t *buffer;
    uint32_t size;
		int64_t pts;
		buf->getTimeInBuffer(&tiempo);
    if(tiempo>3 && buf->getFrame(&buffer, &size, &pts)) {
      printf("saco  size = %ld pts = %d\n", size, pts);
      fwrite(buffer, size, 1, file);
      if(!dec.decode_packet(buffer, size, pts)){
        perror("Fallo decoder");
        exit(1);
      }
      free(buffer);
      // Intento leer un frame decodificado
      if (dec.read_frame(&f, &pts)) {
        // Tengo un frame!
        //	fwrite(f->data,1,f->size,fyuv);
        if(display == NULL) {
            bool retb = false;
            display = new DisplayYUV((char *)"Receptor",f->width,f->height,&retb);
            if(!retb) exit(1);
          }
        display->display(f);
        //usleep((1.0/30.0)*1000000);
      }
    }
	}
  fclose(file);
	if (display != NULL) 
		delete display;
}

int main(int argc, char *argv[]) {
  if (argc != 3) {
    fprintf(stderr, "Uso: %s puerto fps\n", argv[0]);
    exit(1);
  }

  RTPReceiver receiver;
  PlayoutBuffer bufferFrames;

  bool ret = receiver.init(atoi(argv[1]),90000); // Frecuencia reloj RTP 
  if (!ret) exit(1);

  stop = false;
  signal(SIGINT, sighand);
  signal(SIGQUIT, sighand);

  const uint8_t *payload;
  uint32_t payload_size;
  uint32_t fps = atoi(argv[2]);
  uint32_t frame_size = 0;
  uint32_t frame_max_size = 0;
  uint8_t * frame = 0;
  uint32_t frame_timestamp;
  uint32_t frame_pts;
  uint32_t timestamp0 = 0;


  SDL_Thread * frame_thread = SDL_CreateThread(displayLoop, "Hilo del buffer de reproduccion", (void*) &bufferFrames);


  uint8_t aux[4] = {0x00, 0x00, 0x00, 0x01};
  FILE *fout = fopen("out.264","wb");
  FILE *fyuv = fopen("out.yuv","wb");

  while(!stop) {  
    if (!receiver.getPayloadData()) {
      // No hay paquete RTP disponible.
      // Compruebo si el otro extremo sigue emitiendo...
      if (receiver.isClosed()) 
	// Ha cerrado el otro extremo.
	break;
      // No se tiene constancia de que el otro extremo haya cerrado.
      // Intento obtener un paquete RTP nuevo...
      if (!receiver.receive()) {
	// No lo he conseguido. Espero el minimo posible antes de reintentar
	usleep(1);
      }
      else {
	// Recibido un paquete. Imprimo su TS
	printf("TS: %u\n",receiver.getTimestamp());
	if (!timestamp0) timestamp0 = frame_timestamp = receiver.getTimestamp();
	else if (frame_timestamp != receiver.getTimestamp()) {
		//Nuevo Frame!
		frame_pts = (frame_timestamp - timestamp0)*fps/90000;
		printf("PTS: %d\n", frame_pts);
		//Actualizo
		frame_timestamp = receiver.getTimestamp();
    printf("meto  size = %ld pts = %d\n", frame_size, receiver.getSeqNum());
		bufferFrames.insertFrame(frame, frame_size, frame_pts, receiver.getSeqNum());
			// Reset del frame_size
			frame_size = 0;
	}
      }
    }
    else {
      // Hay un paquete RTP disponible
      payload = receiver.getPayloadData();
      payload_size = receiver.getPayloadSize();
      //manejo de memoria
      //Aumenta con el tamaño de NALU y marca de 0x00000001
      frame_size += payload_size+4;
      if (frame_max_size < frame_size){
        //Realloc
	    frame = (uint8_t *) realloc(frame, frame_size);
	    frame_max_size = frame_size;
      }
      //Copiar marca y NALU al buffer del frame
      memcpy(frame+frame_size-payload_size-4,&aux,4);
      memcpy(frame+frame_size-payload_size,payload, payload_size);
      receiver.freePacket();
    }

  }
  free(frame);
  fclose(fout);
  fclose(fyuv);

	int status;
  	if (frame_thread != 0) {
   		SDL_WaitThread(frame_thread,&status);      
    	frame_thread = 0;
  	} 
  receiver.close();
}
