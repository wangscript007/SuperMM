#ifndef _RTP_PACKETIZER_H_
#define _RTP_PACKETIZER_H_

#include <vector>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <pthread.h>
#include "BaseProber.h"
#include "DataBuffer.h"


typedef struct {
	 pthread_t RecvTid;
	 pthread_t SendTid;
        int RTPSocket;
        int RTCPSocket;
        uint32_t RTPPort;
        int32_t TrackIndex;
        struct sockaddr_in mRemoteRTPAddr;
        struct sockaddr_in mRemoteRTCPAddr;
	 bool hasSetRemoteRTP;
	 bool hasSetRemoteRTCP;
        bool IsEnd;
        int64_t BaseTime;
        uint32_t SSRC;
}SenderInfo;

class RTPSender {
public:
    struct ThreadParam {
        void *param;
        int index;
	 int64_t starttime;
	 int64_t endtime;
    };

    enum {
	UNKNOWN,
	RTCP_SR_TYPE,
	RTCP_RR_TYPE,
	RTCP_SDES_TYPE,
	RTCP_BYE_TYPE,
	RTCP_APP_TYPE,
    };
	
    RTPSender(BaseProber *prober);
    ~RTPSender();

    SenderInfo *getSenderInfo(int32_t index);
    int addSenderInfo(int32_t index, int port);
    int recvData(int index);
    static void *recvEntry(void *param);
    int sendData(int64_t start, int64_t end);
    static void *sendEntry(void *param);
    void makePortPair(int *rtpSocket, int *rtcpSocket, uint32_t *rtpPort);
    const char *makeRTCPReport(int type);
    int parseRTCPReport(uint8_t *buff, ssize_t len);
    int parseRRReport(uint8_t *buff, ssize_t len);
    int parseSDESReport(uint8_t *buff, ssize_t len);
	
    int resetAll(bool all);
    int64_t setSeekTo(int64_t timeUS);

private:
    BaseProber *mBaseProber;

    vector<SenderInfo*> mSenderVector;
	int mNALLengthSize;
};

#endif
