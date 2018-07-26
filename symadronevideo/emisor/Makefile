CXX         = c++
LIBS	    = sdl2 libavcodec libavutil x11 vdpau libva libva-drm libva-x11 jrtplib jthread
CFLAGS      = -g -c -w $$(pkg-config $(LIBS) --cflags) 
LDFLAGS     = $$(pkg-config $(LIBS) --libs) -lturbojpeg -lfl
OBJ         = captura.o camara.o display.o h264encoder.o h264rtppacketizer.o rtpsender.o umurtpsession.o lex.yy.o

all: captura 

clean:
	rm -f *.o captura

captura: $(OBJ)
	$(CXX) -o $@ $(OBJ) $(LDFLAGS) 

lex.yy.c : socket.l
	flex socket.l

lex.yy.o : lex.yy.c
	gcc -o lex.yy.o -c lex.yy.c

%.o : %.cc %.h
	$(CC) -o $@ $(CFLAGS) $<

captura.o : captura.cc
	$(CC) -o $@ $(CFLAGS) $<

