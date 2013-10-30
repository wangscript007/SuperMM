#ifndef _RTSP_DATA_READER_H_

#define _RTSP_DATA_READER_H_

#include <stdint.h>
#include <sys/types.h>
#include <pthread.h>
#include <vector>
#include "SessionDescription.h"
#include "RTPConnection.h"

#define REQUEST_DESCRIPE     0x01
#define REQUEST_SETUP        0x02
#define REQUEST_PLAY         0x04
#define REQUEST_TEARDOWN     0x08

using namespace std;

class RTSPDataReader {
public:
    struct trackInfo{
        int RTPSocket;
        int RTCPSocket;
        uint32_t RTPPort;
        map<string, string> mHeader;
    };

    RTSPDataReader(const char *url);
    ~RTSPDataReader();

    ssize_t readAt(off64_t offset, void *data, size_t size);

    int RTSPConnect(char *host, int port);
    int sendRTSPRequest(uint32_t req, int index);
    int recvRTSPResponse(uint8_t *&data, size_t *size);
    int parsingResponse(uint8_t *&data, size_t *size, int index);
    int setupTrack(int index);
    int makeTrackURL(string *trackurl, int index);
    bool findHeader(int index, string key, string *value);

    static void CallBackFunc(int flag);

    void createEntry();

private:
    int mSocket;
    
    static int mCallBackFlag;
    static pthread_mutex_t mRTSPMutex; 
    static pthread_cond_t mRTSPCond;

    uint32_t mNextCSeq;

    const char *mURL;
    const char *mSessionURL;

    string mSessionID;
    string mStatusLine;

    SessionDescription *mSessionDescription;
    RTPConnection *mRTPConnection;

    vector<trackInfo *> mTracksInfo;
};

#endif
