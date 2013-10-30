#ifndef _TRP_CONNECTION_H_

#define _TRP_CONNECTION_H_

#include <vector>
#include <list>

#include "SessionDescription.h"

#define REPORT_BYE  0x01

using namespace std;

typedef void (*callback)(int flag);

class RTPConnection {
public:
    RTPConnection(SessionDescription *sedec);

    struct packetInfo {
        uint8_t *data;
        size_t size;
        uint32_t rtpTime;
        uint32_t srcId;
        uint32_t seqNum;
    };

    struct streamInfo {
        int mRTPSocket;
        int mRTCPSocket;
        int mIndex;
        bool mIsEOS;
        uint32_t mHighestSeqNumber;
        int32_t mNumBuffersReceived;
        string format;

        list<packetInfo *> mPacketList;
    };

    RTPConnection();
    ~RTPConnection();

    void makePortPair(int *rtpSocket, int *rtcpSocket, uint32_t *rtpPort);
    void addStream(int rtpSocket, int rtcpSocket, int index);
    void setCallBack(callback func);

    int loopGetData();
    static void *recvEntry(void *param);

    int recvData(streamInfo *s, bool receiveRTP);
    int parsingRTPData(packetInfo *& info);
    int parsingRTCPData(uint8_t *buff, size_t size);
    int pushRTPPacket(streamInfo *s, packetInfo *info);

    bool checkEOS();

    callback mCallBackFunc;
private:
    vector<streamInfo *> mStreamsInfo;
    
    SessionDescription *mSessionDescription;
};

#endif
