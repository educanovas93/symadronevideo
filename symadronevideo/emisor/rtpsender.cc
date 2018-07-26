#include "rtpsender.h"
#include <string.h>
#include <jrtplib3/rtperrors.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <iostream>

using namespace std;

#define MTU 1472

bool RTPSender::init(uint16_t localPort, const char *destIP, uint16_t destPort, uint32_t clock, uint32_t bw, uint8_t pt) {
  sessParams.SetOwnTimestampUnit(1.0/clock);
  sessParams.SetSessionBandwidth(bw/8);
  transParams.SetPortbase(localPort); 
  
  sess = new UMURTPSession(true); // Emisor

  int status = sess->Create(sessParams,&transParams);
  if (status < 0) {
    cerr << "RTPSender init error: " << RTPGetErrorString(status) << endl;
    return false;
  }

  uint32_t ip = inet_addr(destIP);
  if (ip == INADDR_NONE) {
    cerr << "RTPSender init error: direccion ip incorrecta" << endl;
    return false;
  }
 
  RTPIPv4Address addr(ntohl(ip), destPort);
  status = sess->AddDestination(addr);
  if (status < 0) {
    cerr << "RTPSender init error: adddestination" << endl;
    return false;
  }

  cout << "SSRC: " << sess->GetLocalSSRC() << endl;
  sess->SetDefaultTimestampIncrement(0);
  sess->SetDefaultPayloadType(pt);
  sess->SetMaximumPacketSize(MTU);
  timestamp = 0;
  return true;
}

bool RTPSender::send(uint8_t *payload, uint32_t size, uint32_t ts, bool mark) {
  sess->SetDefaultMark(mark);
  uint32_t diff = ts - timestamp; // Aunque sean unsigned, calcula bien la diferencia
  timestamp = ts;
  sess->IncrementTimestamp(diff);
  int status = sess->SendPacket(payload,size);
  if (status < 0) {
    cerr << "RTPSender send error: " << RTPGetErrorString(status) << endl;
    cerr << "Packet size: " << size << endl;
    return false;
  }
  return true;
}

void RTPSender::close() {
  const char *razon = "Cierre normal";
  sess->BYEDestroy(RTPTime(3,0),razon,strlen(razon));
  delete sess;
}
