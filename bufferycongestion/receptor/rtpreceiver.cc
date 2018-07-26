#include "rtpreceiver.h"
#include <string.h>
#include <jrtplib3/rtperrors.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <iostream>

using namespace std;

bool RTPReceiver::init(uint16_t localPort, uint32_t clock) {
  sessParams.SetOwnTimestampUnit(1.0/clock);
  sessParams.SetAcceptOwnPackets(false);
  transParams.SetPortbase(localPort);

  sessParams.SetMinimumRTCPTransmissionInterval(RTPTime(1,0));
  sessParams.SetUseHalfRTCPIntervalAtStartup(true);
  sess = new UMURTPSession(false); // Receptor

  int status = sess->Create(sessParams,&transParams);
  if (status < 0) {
    cerr << "RTPReceiver init error: " << RTPGetErrorString(status) << endl;
    return false;
  }

  cout << "SSRC: " << sess->GetLocalSSRC() << endl;
  packet = NULL;

  return true;
}

bool RTPReceiver::receive() {
  if (packet) sess->DeletePacket(packet);
  sess->BeginDataAccess();
  if (sess->GotoFirstSourceWithData()) {
    do {
      if (sess->GetCurrentSourceInfo()->GetSSRC() == sess->getPeerSSRC()) 
	packet = sess->GetNextPacket();
    } while (!packet && sess->GotoNextSourceWithData());
  }
  sess->EndDataAccess();
  return (packet != NULL);
}

void RTPReceiver::freePacket() {
  if (packet) sess->DeletePacket(packet);
  packet = NULL;
}

const uint8_t *RTPReceiver::getPayloadData() {
  if (!packet) return NULL;
  return packet->GetPayloadData();
}

uint32_t RTPReceiver::getPayloadSize() {
  if (!packet) return 0;
  return packet->GetPayloadLength();
}

uint32_t RTPReceiver::getTimestamp() {
  if (!packet) return 0;
  return packet->GetTimestamp();
}

bool RTPReceiver::getMark() {
  if (!packet) return false;
  return packet->HasMarker();
}

uint32_t RTPReceiver::getSeqNum() {
  return packet->GetExtendedSequenceNumber();
}

uint8_t RTPReceiver::getPayloadType() {
  return packet->GetPayloadType();
}

bool RTPReceiver::isClosed() {
  return sess->isClosed();
}

void RTPReceiver::close() {
  const char *razon = "Cierre normal";
  sess->BYEDestroy(RTPTime(3,0),razon,strlen(razon));
  delete sess;
}

