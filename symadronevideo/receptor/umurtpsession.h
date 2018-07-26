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
 protected:
  void OnNewSource(RTPSourceData *dat);
  void OnBYEPacket(RTPSourceData *dat);
  void OnRTPPacket(RTPPacket *pack, const RTPTime &receivetime, const RTPAddress *senderaddress);
  void OnRTCPCompoundPacket(RTCPCompoundPacket *pack, const RTPTime &receivetime, const RTPAddress *senderaddress);
 private:
  uint32_t peerId;
  bool closed;
  bool emisor;
};

#endif
