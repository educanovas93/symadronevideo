CXX         = c++
LIBS	    = sdl2 libavcodec libavutil x11 vdpau libva libva-drm libva-x11 jrtplib jthread
CFLAGS      = -g -c $$(pkg-config $(LIBS) --cflags) 
LDFLAGS     = $$(pkg-config $(LIBS) --libs) -lturbojpeg
OBJ         = captura.o camara.o display.o h264encoder.o h264rtppacketizer.o rtpsender.o umurtpsession.o

all: captura 

clean:
	rm -f *.o captura

captura: $(OBJ)
	$(CXX) -o $@ $(OBJ) $(LDFLAGS) 

%.o : %.cc %.h
	$(CC) -o $@ $(CFLAGS) $<

captura.o : captura.cc
	$(CC) -o $@ $(CFLAGS) $<

