#ifndef __RTPSender
#define __RTPSender

#include <jrtplib3/rtpsession.h>
#include <jrtplib3/rtpudpv4transmitter.h>
#include <jrtplib3/rtpipv4address.h>
#include <jrtplib3/rtpsessionparams.h>

#include "umurtpsession.h"

using namespace jrtplib;

class RTPSender {
 public:
  bool init(uint16_t localPort, const char *destIP, uint16_t destPort, uint32_t clock, uint32_t bw, uint8_t pt);
  bool send(uint8_t *payload, uint32_t size, uint32_t timestamp, bool mark);
  void close();
 private:
  uint32_t timestamp;
  UMURTPSession *sess;
  RTPUDPv4TransmissionParams transParams;
  RTPSessionParams sessParams;
};
#endif
