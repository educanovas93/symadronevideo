#include "camara.h"
#include "display.h"
#include "h264encoder.h"
#include "h264rtppacketizer.h"
#include "rtpsender.h"
#include <unistd.h>

int main(int argc, char *argv[]) {

  if (argc != 11) {
    printf("uso: %s width height fps device gop bperiod bitrate localP destP destIP\n",
		argv[0]);
    exit(1);
  }
   
  uint32_t width = atoi(argv[1]);
  uint32_t height = atoi(argv[2]);
  uint32_t fps = atoi(argv[3]);
  uint32_t gop = atoi(argv[5]);
  uint32_t bperiod = atoi(argv[6]);
  uint32_t bitrate = atoi(argv[7]);
  uint16_t localP = atoi(argv[8]);
  uint16_t destP = atoi(argv[9]);
  const char *destIP = argv[10];
   

  int16_t res;
  bool resb;

  CamaraYUV camara;
  res = camara.open(argv[4]);
  if (res <0) exit(1);
   
  res = camara.init(width,height);
  if (res <0) exit(1);

  if (!DisplayYUV::init()) exit(1);

  bool retb;
  DisplayYUV display((char *)"Comunicaciones Multimedia",width,height,&retb); 
  if (!retb) exit(1);
  
  FrameYUV *f = 0;
  uint64_t intervalo = 1000000L / fps; // Intervalo entre frames (microseg)   
  uint64_t num_frames = 0;
  uint64_t time_inicio = display.now();
  uint64_t time_fin;
  uint64_t captura_siguiente = time_inicio + intervalo;
  uint64_t ahora;
  uint64_t espera;

  display.clear();    

  FILE *fout = fopen("out.264","wb");
  H264Encoder enc;
  enc.init(width,height,fps,gop,bperiod,bitrate);
  uint8_t *output;
  uint32_t out_size;
  int64_t out_pts;
  H264RTPPacketizer pack;
  RTPSender rtp;
  rtp.init(localP,destIP,destP,pack.getClock(),bitrate,100);
  pack.init(fps,&rtp);

  while(1) {   
    if (camara.grab((Frame **)&f) < 0) { 
      fprintf(stderr, "Error capturando\n"); exit(1);
    }

    // Codificar frame y grabar en fout
    enc.encode_frame(f);
    enc.read_packet(&output,&out_size,&out_pts);
    if (out_size != 0) {
	// Hay un frame comprimido disponible
	//printf("Got a frame: size=%d pts=%ld\n",out_size,out_pts);
        pack.send(output,out_size,out_pts);
        fwrite(output,out_size,1,fout);
    }

    if (!display.display(f)) { 
      fprintf(stderr, "Error visualizando\n"); exit(1);
    }
    
    if (display.poll_quit()) break;
    ahora = display.now();
    if (captura_siguiente > ahora) {         
      espera = captura_siguiente - ahora;
      usleep(espera); 
    }      
    captura_siguiente += intervalo;
    num_frames += 1;
  }

  time_fin = display.now();
  camara.close();
  rtp.close();
  display.quit();
  if (f != 0) delete f; 
      
  double time_total = ((double)(time_fin-time_inicio))/1000000.0;
  double fps_real = (double)num_frames / time_total;
  printf("\nFrames total: %ld, Tiempo total: %.3f seg, Media: %.3f fps\n", 
         num_frames, time_total, fps_real);
   
}
