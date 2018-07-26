#ifndef __RTPReceiver
#define __RTPReceiver

#include <jrtplib3/rtpsession.h>
#include <jrtplib3/rtpudpv4transmitter.h>
#include <jrtplib3/rtpipv4address.h>
#include <jrtplib3/rtpsessionparams.h>

#include "umurtpsession.h"

using namespace jrtplib;

class RTPReceiver {
 public:
  bool init(uint16_t localPort, uint32_t clock);
  bool receive();
  void freePacket();
  const uint8_t *getPayloadData();
  uint32_t getPayloadSize();
  uint8_t getPayloadType();
  uint32_t getTimestamp();
  bool getMark();
  uint32_t getSeqNum();
  bool isClosed();
  void close();

 private:
  UMURTPSession *sess;
  RTPUDPv4TransmissionParams transParams;
  RTPSessionParams sessParams;
  RTPPacket *packet;
  uint32_t timestamp;
};
#endif
