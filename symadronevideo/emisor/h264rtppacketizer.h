#ifndef __H264RTPPacketizer
#define __H264RTPPacketizer

#include <stdint.h>
#include <list>

#define MTU 1472
#define RTPHEADER 12
#define H264RTPSTAPHEADER 3
#define MAXPAYLOAD MTU-RTPHEADER-H264RTPSTAPHEADER
#define RTPCLOCK 90000

using namespace std;

class H264RTPPacketizer {
 public:
  bool init(uint16_t fps, void *sender);
  bool send(uint8_t *buffer, uint32_t size, int64_t pts);
  void close() {};
  uint16_t getClock() { return (uint16_t)RTPCLOCK; }
  uint16_t getMaxPayload() { return (uint16_t)MAXPAYLOAD; }
 private:
  uint32_t period;
  list<uint32_t> nalu_start;
  list<uint16_t> nalu_size;
  uint8_t payload[MAXPAYLOAD];
  uint32_t payload_size;
  void* sender;
};
#endif
