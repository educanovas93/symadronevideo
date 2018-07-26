#include "camara.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <fcntl.h> 
#include <unistd.h>

#define CLEAR(x) memset (&(x), 0, sizeof (x))

#define GRAB_DEVICE "/dev/video0"

void Camara::errno_msg(const char * s) {
  fprintf (stderr, "%s error %d, %s\n",
	   s, errno, strerror (errno));
}

int Camara::xioctl(int fd, int request, void *arg) {
  int r;
  do r = ioctl (fd, request, arg);
  while (-1 == r && EINTR == errno);
  return r;
}

int Camara::open(char *device) {
  struct stat st;       
  if (device == NULL) strcpy(dev,GRAB_DEVICE);   
  else strcpy(dev,device);
      
  if (-1 == stat (dev, &st)) {
    fprintf (stderr, "No se puede identificar '%s': %d, %s\n",
	     dev, errno, strerror (errno));
    return -1;
  }

  if (!S_ISCHR (st.st_mode)) {
    fprintf (stderr, "%s no es un dispositivo\n", dev);
    return -1;
  }

  grab_fd = ::open(dev, O_RDWR /* required */, 0);  

  if (-1 == grab_fd) {
    fprintf (stderr, "No se puede abrir '%s': %d, %s\n",
	     dev, errno, strerror (errno));
    return -1;
  }

  buffers = 0;
  n_buffers = 0;
   
  return 0;
}

int Camara::init(int _width, int _height) {

  if (grab_fd == -1) {
    fprintf(stderr, "Dispositivo no abierto\n");
    return -1;
  }

  if (-1 == xioctl (grab_fd, VIDIOC_QUERYCAP, &grab_cap)) {
    errno_msg("xioctl VIDIOC_QUERYCAP");      
    return -1;
  }

  if (!(grab_cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
    fprintf (stderr, "%s no es un dispositivo de captura de video\n",dev);
    return -1;
  }
    
  printf("Info. driver: %s, card: %s\n",grab_cap.driver,grab_cap.card);

  if (!(grab_cap.capabilities & V4L2_CAP_STREAMING)) {
    fprintf (stderr, "%s el dispositivo no soporta lectura con mmap\n",dev);
    return -1;
  }

  CLEAR (grab_fmt);

  width  = _width;
  height = _height;    

  grab_fmt.type                = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  grab_fmt.fmt.pix.width       = width; 
  grab_fmt.fmt.pix.height      = height;
  grab_fmt.fmt.pix.pixelformat = getMode(); // Método virtual
  grab_fmt.fmt.pix.field       = V4L2_FIELD_NONE;

  if (-1 == xioctl (grab_fd, VIDIOC_S_FMT, &grab_fmt)) {
    errno_msg("xioctl VIDIOC_S_FMT");
    return -1;
  }

  if (-1 == xioctl (grab_fd, VIDIOC_G_FMT, &grab_fmt)) {
    errno_msg("xioctl VIDIOC_G_FMT");
    return -1;
  }

  if ((grab_fmt.fmt.pix.pixelformat != getMode()) && (grab_fmt.fmt.pix.pixelformat != V4L2_PIX_FMT_JPEG)) {
    printf("Error: formato incompatible: ");
    uint32_t f = grab_fmt.fmt.pix.pixelformat;
    uint8_t *p = (uint8_t*)&f;
    printf("%c%c%c%c\n",p[0],p[1],p[2],p[3]);
    return -1;
  }

  // Hará falta un decodificador JPEG si el driver no captura en otro formato
  tjpeg = tjInitDecompress();

  // Buffers de imágenes temporales
  last_frame = 0;
  tmp_frame = 0;

  // Compruebo cuantos buffers tengo disponibles
  CLEAR (grab_req);

  grab_req.count  = 4;
  grab_req.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  grab_req.memory = V4L2_MEMORY_MMAP;

  if (-1 == xioctl (grab_fd, VIDIOC_REQBUFS, &grab_req)) {
    errno_msg("xioctl VIDIOC_REQBUFS");
    return -1;
  }

  if (grab_req.count < 2) {
    fprintf (stderr, "Memoria insuficiente en %s\n",dev);
    return -1;
  }

  buffers = new struct buffer[grab_req.count];

  if (!buffers) {
    fprintf (stderr, "Memoria agotada!\n");
    return -1;
  }

  for (n_buffers = 0; n_buffers < grab_req.count; ++n_buffers) {
        
    CLEAR (grab_buf);
    grab_buf.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    grab_buf.memory = V4L2_MEMORY_MMAP;
    grab_buf.index  = n_buffers;

    if (-1 == xioctl (grab_fd, VIDIOC_QUERYBUF, &grab_buf)) {
      errno_msg("xioctl VIDIOC_QUERYBUF");
      return -1;
    }
    buffers[n_buffers].length = grab_buf.length;
    buffers[n_buffers].start =
      mmap (NULL /* donde quiera el driver */,
	    grab_buf.length,
	    PROT_READ | PROT_WRITE /* necesario */,
	    MAP_SHARED /* recomendado */,
	    grab_fd, grab_buf.m.offset);

    if (MAP_FAILED == buffers[n_buffers].start) {
      fprintf(stderr, "Fallo en mmap\n");
      return -1;
    }                
  }

  // Inicio la captura...
   
  for (int i = 0; i < n_buffers; ++i) {
    CLEAR (grab_buf);
    grab_buf.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    grab_buf.memory = V4L2_MEMORY_MMAP;
    grab_buf.index  = i;

    if (-1 == xioctl (grab_fd, VIDIOC_QBUF, &grab_buf)) {
      errno_msg("xioctl VIDIOC_QBUF");
      return -1;
    }
  }
      
  enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

  if (-1 == xioctl (grab_fd, VIDIOC_STREAMON, &type)) {
    errno_msg ("xioctl VIDIOC_STREAMON");
    return -1;
  }
       
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
  if (grab_fd != -1) { 
    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    if (-1 == xioctl (grab_fd, VIDIOC_STREAMOFF, &type))  
      errno_msg ("xioctl VIDIOC_STREAMOFF");

    for (int i = 0; i < n_buffers; ++i)
      if (-1 == munmap (buffers[i].start, buffers[i].length)) 
	errno_msg("munmap");
      
    if (-1 == ::close (grab_fd))
      errno_msg("close");

    grab_data = 0;
    grab_fd = -1;   

    tjDestroy(tjpeg); 	
  }
  if (buffers) {
    delete [] buffers;
    buffers = 0;
  }
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
    FD_SET (camara->grab_fd, &fds);

    /* Timeout. */
    tv.tv_sec = 2;
    tv.tv_usec = 0;

    r = select (camara->grab_fd + 1, &fds, NULL, NULL, &tv);

    if (r == -1) {
      if (EINTR == errno) continue;
      camara->errno_msg ("select camera\n");
      break;
    }

    if (r == 0) {
      fprintf (stderr, "select timeout\n");
      break;
    }

    // Consultar el buffer que esta disponible
    CLEAR (camara->grab_buf);

    camara->grab_buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    camara->grab_buf.memory = V4L2_MEMORY_MMAP;

    if (-1 == camara->xioctl (camara->grab_fd, VIDIOC_DQBUF, &(camara->grab_buf))) {
      if (errno == EAGAIN) continue;
      camara->errno_msg ("xioctl VIDIOC_DQBUF");
      break;
    }

    // Copiar al buffer del último frame la imagen capturada   
    SDL_mutexP(camara->frame_mutex);
    if (camara->last_frame == 0) {
      camara->last_size = camara->buffers[(camara->grab_buf).index].length;
      camara->last_frame = new uint8_t[camara->last_size];
    }
    memcpy (camara->last_frame,
	    camara->buffers[(camara->grab_buf).index].start,
	    camara->last_size);
    SDL_mutexV(camara->frame_mutex);      
  
    // Volver a solicitar capturar el buffer que acabo de copiar   

    if (-1 == camara->xioctl (camara->grab_fd, VIDIOC_QBUF, &(camara->grab_buf))) {
      camara->errno_msg ("VIDIOC_QBUF");
      break;        
    }
  }
  camara->stop = true;
}

uint32_t CamaraYUV::getMode() {
  // Este modo tiene más posibilidades de funcionar que el modo YUV420.
  return V4L2_PIX_FMT_YUYV;
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

  if (grab_fmt.fmt.pix.pixelformat == V4L2_PIX_FMT_JPEG) {
    int JPEGwidth, JPEGheight, JPEGSubSamp, res;

    // Formato de captura JPEG
    // Uso TurboJPEG para convertirlo a YUYV en formato planar!
    
    if (tmp_frame == 0) {
      // Primero, recupero la informacion de la imagen
      res = tjDecompressHeader2 (tjpeg, last_frame, last_size, 
				     &JPEGwidth, &JPEGheight, &JPEGSubSamp);
      if (res < 0) {
	fprintf(stderr, "convert: %s!\n",tjGetErrorStr());
	return;
      }
      if (width != JPEGwidth || height != JPEGheight) {
	fprintf(stderr, "convert: diferent source size %dx%d\n", 
		JPEGwidth, JPEGheight);
	return;
      }
      if (JPEGSubSamp != TJ_422) {
	fprintf(stderr, "convert: source subsampling not 422, but %d\n",JPEGSubSamp);
	return;
      }

      tmp_frame = new uint8_t[TJBUFSIZEYUV(JPEGwidth,JPEGheight,JPEGSubSamp)];
    }

    res = tjDecompressToYUV(tjpeg,last_frame,last_size,tmp_frame,0);
    if (res < 0) {
      fprintf(stderr, "convert: %s!\n",tjGetErrorStr());
      return;
    }

    // Conversion YVYUP a YUV420P
    uint8_t * fin_Y = tmp_frame;
    memcpy(Y,fin_Y,fout->sizeY);

    uint8_t * fin_U = fin_Y + fout->sizeY;
    uint8_t * fin_V = fin_U + width/2*height; 

    for (int y = 0; y < height; y++) {
      for (int x = 0; x < width/2; x++) {
	if (!(y % 2)) {
	  U[y/2 * width/2 + x] = fin_U[y*width/2 + x];
	  V[y/2 * width/2 + x] = fin_V[y*width/2 + x];
	}
      }
    }
  }
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
}
