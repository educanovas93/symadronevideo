#include "camara.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <fcntl.h> 
#include <unistd.h>

extern "C" FILE *yyin;
extern "C" int yylex();
extern "C" char *yytext;


#define CLEAR(x) memset (&(x), 0, sizeof (x))




//cambiar metodo ya que no vamos a utilizar un /dev/video si no 
//el streaming en http en http://192.168.1.1/videostream.cgi

int Camara::open(char *url) {

  //coger info del host, construir socket y conectarlo
  memset(&hints, 0,sizeof hints);
  hints.ai_family=AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  getaddrinfo("192.168.1.1","80", &hints, &res);
  sockfd = socket(res->ai_family,res->ai_socktype,res->ai_protocol);
  printf("Connectando...\n");
  connect(sockfd,res->ai_addr,res->ai_addrlen);
  printf("Connectado!\n");


  char *header ="GET /videostream.cgi HTTP/1.1\r\n"
                "Host: 192.168.1.1\r\n"
                "Connection: keep-alive\r\n"
                "Authorization: Digest username=\"admin\", realm=\"ip camera\", nonce=\"fa03a58efadbb89255a0cce0e3cdae00\", uri=\"/videostream.cgi\", algorithm=MD5, response=\"fc5906d6a307ed0568bf9772bff6ca33\", qop=auth, nc=00000002, cnonce=\"b9a3a69f0a284f82\"\r\n"
                "Upgrade-Insecure-Requests: 1\r\n"
                "User-Agent: Mozilla/5.0 (Macintosh; Intel Mac OS X 10_13_4) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/66.0.3359.139 Safari/537.36\r\n"
                "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,image/apng;q=0.8\r\n"
                "Accept-Encoding: gzip, deflate\r\n"
                "Accept-Language: es-ES,es;q=0.9\r\n\r\n";

  send(sockfd,header,strlen(header),0);
  printf("GET Enviado...\n");

  FILE *f_in = fdopen(sockfd,"r");
  if (f_in == NULL){
    fprintf(stderr,"Error al leer del socket\n");
    exit(2);
  }
  yyin = f_in;
   
  return 0;
}

//de momento con 640x480 ,mas adelante hacer setparams para poner alto y ancho
int Camara::init(int _width, int _height) {

	if( _width == 640  && _height == 480 ) {
		width = _width;
		height = _height;
	}else return 1;

  // Hará falta un decodificador JPEG si el driver no captura en otro formato
  tjpeg = tjInitDecompress();

  // Buffers de imágenes temporales
  last_frame = 0;
  tmp_frame = 0;

  // Inicio la captura...
  stop = false;    
  frame_mutex = SDL_CreateMutex();    
  capture_thread = SDL_CreateThread(capture_loop, "Camera capture loop", (void*)this);
    
  return 0;   

}

void Camara::close() {
  stop = true;   
  int status;
  if (capture_thread != 0) {
    SDL_WaitThread(capture_thread,&status);      
    capture_thread = 0;
  }   
  if (frame_mutex != 0) {
    SDL_DestroyMutex(frame_mutex);
    frame_mutex = 0;
  }
  if (last_frame != 0) {
    delete [] last_frame;
    last_frame = 0;
  }
  if (tmp_frame != 0) {
    delete [] tmp_frame;
    tmp_frame = 0;
  }

    tjDestroy(tjpeg);
}

int Camara::grab(Frame **f) {
  if (stop) return -1;
  SDL_mutexP(frame_mutex);
  convert(f);      
  SDL_mutexV(frame_mutex);
  return 0;
}

int Camara::capture_loop(void *_camara) {

  Camara *camara = (Camara *)_camara;     
   
  printf("Iniciando hilo de captura\n");
    
  fd_set fds;
  struct timeval tv;
  int r;

  while (!(camara->stop)) {   

    FD_ZERO (&fds);
    FD_SET (camara->sockfd, &fds);

    /* Timeout. */
    tv.tv_sec = 2;
    tv.tv_usec = 0;

    r = select (camara->sockfd + 1, &fds, NULL, NULL, &tv);

    if (r == -1) {
      if (EINTR == errno) continue;
      fprintf(stderr,"select camera\n");
      fflush(stderr);
      break;
    }

    if (r == 0) {
      fprintf (stderr, "select timeout\n");
      fflush(stderr);
      break;
    }

    int aux = yylex();
    //printf("Bytes: %d\n",aux);

    // Copiar al buffer del último frame la imagen capturada   
    SDL_mutexP(camara->frame_mutex);
    if (camara->max_size < aux) {
      camara->last_frame = (uint8_t*)realloc(camara->last_frame,aux);
      camara->max_size = aux;
    }
    camara->last_size = aux;
    memcpy (camara->last_frame, yytext, camara->last_size);
    SDL_mutexV(camara->frame_mutex);      

  }
  camara->stop = true;
}


// Transformar a YUV420P
// Puede haber dos formatos de captura: YUYV, o JPEG.
// Si es JPEG, uso TurboJPEG para decodificar la imagen y ponerla en formato YUV420P.
// Si es YUYV, hago la transformación a YUV420P.
// El formato YUYV usa decimacion 4:2:2 y no es planar.
// El formato YUV usa decimacion 4:2:0 y es planar.
void CamaraYUV::convert(Frame **f) {
  FrameYUV *fout;
  if (*f == NULL) {
    fout = new FrameYUV(width,height);
    if (!fout) {
      fprintf(stderr, "convert: error reservando memoria de FrameYUV\n");
      exit(1);
    }
    *f = fout;
  }
  else {
    fout = (FrameYUV *)*f;
  }
  if (!last_frame) {
    // No se ha capturado todavía ningún frame.
    // fprintf(stderr, "convert: no frame captured yet!\n");
    return;
  }

  uint8_t *Y = fout->Y();
  uint8_t *U = fout->U();
  uint8_t *V = fout->V();

  int JPEGwidth, JPEGheight, JPEGSubSamp, res;

    // Formato de captura JPEG
    // Uso TurboJPEG para convertirlo a YUYV en formato planar!

  if (tmp_frame == 0) {
    // Primero, recupero la informacion de la imagen
    res = tjDecompressHeader2(tjpeg, last_frame, last_size,
                              &JPEGwidth, &JPEGheight, &JPEGSubSamp);
    if (res < 0) {
      fprintf(stderr, "convert: %s!\n", tjGetErrorStr());
      return;
    }
    if (width != JPEGwidth || height != JPEGheight) {
      fprintf(stderr, "convert: diferent source size %dx%d\n",
              JPEGwidth, JPEGheight);
      return;
    }
    if (JPEGSubSamp != TJ_422) {
      fprintf(stderr, "convert: source subsampling not 422, but %d\n", JPEGSubSamp);
      return;
    }

    tmp_frame = new uint8_t[TJBUFSIZEYUV(JPEGwidth, JPEGheight, JPEGSubSamp)];
  }

  res = tjDecompressToYUV(tjpeg, last_frame, last_size, tmp_frame, 0);
  if (res < 0) {
    fprintf(stderr, "convert: %s!\n", tjGetErrorStr());
    return;
  }

  // Conversion YVYUP a YUV420P
  uint8_t *fin_Y = tmp_frame;
  memcpy(Y, fin_Y, fout->sizeY);

  uint8_t *fin_U = fin_Y + fout->sizeY;
  uint8_t *fin_V = fin_U + width / 2 * height;

  for (int y = 0; y < height; y++) {
    for (int x = 0; x < width / 2; x++) {
      if (!(y % 2)) {
        U[y / 2 * width / 2 + x] = fin_U[y * width / 2 + x];
        V[y / 2 * width / 2 + x] = fin_V[y * width / 2 + x];
      }
    }
  }
}
/*
else {

  // Conversion YUYV a YUV420P
  for (int y = 0; y < height; y++) {
	for (int x = 0; x < width; x++) {
  Y[y*width + x] = last_frame[(y*width + x)*2];
  if (!(y % 2)) {
	if (!(x % 2))
	  U[y/2 * width/2 + x/2] = last_frame[(y*width+x)*2 + 1];
	else
	  V[y/2 * width/2 + x/2] = last_frame[(y*width+x)*2 + 1];
  }
	}
  }
}
 */

