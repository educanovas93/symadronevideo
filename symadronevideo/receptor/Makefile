CXX         = c++
LIBS	    = sdl2 libavcodec libavutil x11 vdpau libva libva-drm libva-x11 jrtplib jthread
CFLAGS      = -g -c -w $$(pkg-config $(LIBS) --cflags) 
LDFLAGS     = $$(pkg-config $(LIBS) --libs) -lturbojpeg
LIBSJRTPLIB = -ljrtp -ljthread

all:	receptor

clean:
	rm -f *.o receptor

receptor: umurtpsession.o rtpreceiver.o receptor.o h264decoder.o display.o
	$(CXX) -o $@ umurtpsession.o rtpreceiver.o receptor.o h264decoder.o display.o $(LDFLAGS) $(LIBSJRTPLIB)

.SUFFIXES: .o .cc .h

.cc.o:
	$(CXX) -o $@ $(CFLAGS) $<
