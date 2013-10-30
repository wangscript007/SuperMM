#ifndef _RTSP_SERVER_H_

#define _RTSP_SERVER_H_

#include <sys/types.h>
#include <pthread.h>
#include <map>

#include "BaseProber.h"
#include "Register.h"
#include "FileDataReader.h"
#include "RTPSender.h"

#define REQUEST_DESCRIPE     0x01
#define REQUEST_SETUP        0x02
#define REQUEST_PLAY         0x04
#define REQUEST_TEARDOWN     0x08
#define REQUEST_OPTIONS      0x10
#define REQUEST_PAUSE      0x20

using namespace std;

class RTSPServer {
public:
    RTSPServer(int sockfd);
    ~RTSPServer();

	struct MethodLine {
		string Method;
		string URL;
		string Version;
	};

    void startRTSPServer();
    int parsingRecvedData();
    int checkMethodType();
    int sendResponse(string res);

    int recvRTSPRequest();
	bool findRTSPAttribute(string str, string *value);
	int parseRTSPAttribute(string str);
	int clearRTSPAttribute();
	int parseMethodLine(string line);

private:
    BaseProber *mBaseProber;

    RTPSender *mRTPSender;
    int mSocketFd;

    int32_t mNextCSeq;
    char *mSessionID;

    bool mGetPlayRequest;
	bool isFirstLine;
	MethodLine mMethodLine;

    uint8_t *mRequestData;

	map<string, string> mRTSPAttribute;
};

void startServer();
void *processEntry(void *param);

#endif
