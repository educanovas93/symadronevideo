#include "umurtpsession.h"

UMURTPSession::UMURTPSession(bool emisor) : RTPSession() {
  this->emisor = emisor;
  peerId = 0;
  closed = false;
}

uint32_t UMURTPSession::getPeerSSRC() {
  return peerId;
}

bool UMURTPSession::isClosed() {
  return closed;
}

void UMURTPSession::OnNewSource(RTPSourceData *dat) {
  if (peerId != 0) return; // Ya tengo un peer seleccionado
  if (this->GetLocalSSRC() == 0) return; // Es el anuncio propio
  printf("Peer detectado: %u\n", dat->GetSSRC());
  peerId = dat->GetSSRC();
  if (!emisor) {
    // Añadir peer a la lista de destinos de la sesión para RTCP
    this->AddDestination(*(dat->GetRTPDataAddress()));
  }
}

void UMURTPSession::OnBYEPacket(RTPSourceData *dat) {
  if (peerId == dat->GetSSRC()) {
    printf("Peer cerrado: %u\n", dat->GetSSRC());
    closed = true;
    peerId == 0;
  }
}

void UMURTPSession::OnRTPPacket(RTPPacket *pack, const RTPTime &receivetime, const RTPAddress *senderaddress) {
}

void UMURTPSession::OnRTCPCompoundPacket(RTCPCompoundPacket *pack, const RTPTime &receivetime, const RTPAddress *senderaddress) {
	RTPSourceData *data = this->GetSourceInfo(peerId);
	if (data != NULL) {  
		double fractionLost = data->RR_GetFractionLost();
		double RTT = data->INF_GetRoundtripTime().GetDouble();
		printf("FractionLost: %f; RTT: %f\n", fractionLost, RTT);
	}
}
