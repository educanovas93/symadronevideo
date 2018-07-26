#ifndef __UMURTPSession
#define __UMURTPSession

#include <jrtplib3/rtpsession.h>
#include <jrtplib3/rtpsourcedata.h>

using namespace jrtplib;

class UMURTPSession : public RTPSession {
 public:
  UMURTPSession(bool emisor);
  uint32_t getPeerSSRC();
  bool isClosed();
  uint32_t getRTPFriendlyRate(uint32_t bitrate_max, uint64_t time_inicio);
  double timeval_diff(struct timeval *a, struct timeval *b);
 protected:
  void OnNewSource(RTPSourceData *dat);
  void OnBYEPacket(RTPSourceData *dat);
  void OnRTPPacket(RTPPacket *pack, const RTPTime &receivetime, const RTPAddress *senderaddress);
  void OnRTCPCompoundPacket(RTCPCompoundPacket *pack, const RTPTime &receivetime, const RTPAddress *senderaddress);
 private:
  uint32_t peerId;
  bool closed;
  bool emisor;
  bool new_rtcp;
  FILE * fichero;
  struct timeval iniTime; 
  struct timeval endTime;
  double average_lost;
  uint32_t bitrate_act;
  uint32_t threshold;
  bool first_time;
};

#endif