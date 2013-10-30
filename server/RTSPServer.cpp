#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <errno.h>
#include <fcntl.h>
#include <string>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "RTSPServer.h"
#include "SDPData.h"

#define SERVPORT 3333
#define BACKLOG 10

#define DEFAULT_PATH "/opt/SuperMM/files/"

#define LOG_TAG "RTSPServer"
#include "Utils.h"

RTSPServer::RTSPServer(int sockfd)
    :mBaseProber(NULL),
     mSocketFd(-1),
     mNextCSeq(0),
     mGetPlayRequest(false){

    mRequestData = (uint8_t *)malloc(4096);

    //set session id
    mSessionID = new char[17];
    for(int i = 0; i < 2; i++) {
        sprintf(mSessionID + i * 8, "%d", rand());
    }
    mSessionID[16] = '\0';

    mSocketFd = sockfd;
}

void RTSPServer::startRTSPServer() {
    while (1) {
        int res = recvRTSPRequest();
        if (res == -1) {
            break;
        }

        res = parsingRecvedData();

	 if (res == REQUEST_TEARDOWN) {
            break;
        }

	 clearRTSPAttribute();
    }
}

RTSPServer::~RTSPServer() {
    close(mSocketFd);

    if (mRTPSender != NULL) {
        delete mRTPSender;
    }

    if (mBaseProber != NULL) {
	 delete mBaseProber;
    }
	
    delete mSessionID;
    free(mRequestData);
}

int RTSPServer::recvRTSPRequest() {
    printf("recv requent\n");

	isFirstLine = true;
    while (1) {
		string line;
		line.clear();

        while (1) {
            char c;
            size_t n = recv(mSocketFd, &c, 1, 0);
//printf("%c -----------\n",c);

            if (n == -1) {
                printf("errno %d\n", errno);
                return -1;
            }

            if (c == '\n') {
                break;
            } else if (c == '\r') {
				continue;
			}
			line.append(1,c);
        }
		if (line.empty()) {
			break;
		} else {
			if (isFirstLine) {
				parseMethodLine(line);
				isFirstLine = false;
			} else {
				parseRTSPAttribute(line);
			}
		}
    }
//    memcpy(mRequestData, data, pos);

    return 0;
}

static uint32_t generateSSRC() {
	return rand();
}

int RTSPServer::parsingRecvedData() {
    SenderInfo *info = NULL;

    int32_t methodtype = checkMethodType();
    printf("method type : %d\n", methodtype);

	string cseq;
	findRTSPAttribute("CSeq", &cseq);
    printf("%s\n", cseq.c_str());
    
    string res;

    switch (methodtype) {
        case REQUEST_DESCRIPE:
        {
            string url(mMethodLine.URL.c_str());

            int32_t pathpos = url.rfind("/", mMethodLine.URL.size());
            string path(url.c_str() + pathpos + 1);
            path.insert(0, DEFAULT_PATH);
            printf("path ----- %s\n", path.c_str());

            Register *reg = new Register(new FileDataReader(path.c_str())); //New one Register instance.
            mBaseProber = reg->probe();

            if(NULL == mBaseProber) {
                LOG("Fail to find the match prober !\n");
                return -1;
            }
            mBaseProber->FileParse();

            res.assign("RTSP/1.0 200 OK");
            res.append("\r\n");
            res.append("Server: YGZ/5.5.5 (Build/489.16; Platform/Linux; Release/YuZhao; state/beta; )");
            res.append("\r\n");

            res.append("CSeq: ");
            res.append(cseq);

            res.append("\r\n");
            res.append("Data: ");
            res.append("2013 Agust 12");
            res.append("\r\n");
            res.append("Content-Type: ");
            res.append("application/sdp");
            res.append("\r\n");

            SDPData *sdpdata = new SDPData(mBaseProber, url);
            mRTPSender = new RTPSender(mBaseProber);
            string sdp = sdpdata->getSDPData();

            char buff[50];
            sprintf(buff, "Content-Length: %d", sdp.size());
            res.append(buff);
            res.append("\r\n");
            res.append("Content-Base: ");
            res.append(url);
            res.append("/");
            res.append("\r\n");
            res.append("\r\n");

            res.append(sdp);
//            res.append("\r\n");

	     if (reg != NULL) {
			delete reg;
	     }
		 
	     if (sdpdata != NULL) {
			delete sdpdata;
	     }
		 
            break;
        }
        case REQUEST_SETUP:
        {
            int32_t tmppos = -1;
			tmppos = mMethodLine.URL.find("trackID=",0);
            int32_t trackID = atoi(mMethodLine.URL.c_str() + tmppos + 8);

			string transport;
			findRTSPAttribute("Transport", &transport);
            tmppos = transport.find("client_port=", 0);

            int rtpport = atoi(transport.c_str() + tmppos + 12);
            printf("trackID %d rtpport %d\n",trackID, rtpport);

            mRTPSender->addSenderInfo(trackID, rtpport);

            res.assign("RTSP/1.0 200 OK");
            res.append("\r\n");
            res.append("Server: YGZ/5.5.5 (Build/489.16; Platform/Linux; Release/YuZhao; state/beta; )");
            res.append("\r\n");
            res.append("CSeq: ");
            res.append(cseq);
            res.append("\r\n");
//            res.append("Data: 2013 Agust 12");
//            res.append("\r\n");

            char buff[200];
            sprintf(buff, "Session: %s", mSessionID);
            res.append(buff);
            res.append("\r\n");

            res.append("Cache-Control: must-revalidate");
            res.append("\r\n");

	     	info = mRTPSender->getSenderInfo(trackID);
	     	info->SSRC = generateSSRC();
            sprintf(buff, "Transport: %s;source=192.168.1.100;server_port=%d-%d;ssrc=%x", 
                    transport.c_str(), info->RTPPort, info->RTPPort + 1, info->SSRC);
            res.append(buff);

            res.append("\r\n");
            res.append("\r\n");

            break;
        }
        case REQUEST_PLAY:
        {
	     int32_t pos1;
	     int32_t pos2;
            string url(mMethodLine.URL.c_str());
		 
	     int64_t starttime = 0;
	     int64_t endtime = 0;
		 
            res.assign("RTSP/1.0 200 OK");
            res.append("\r\n");
            res.append("CSeq: ");
            res.append(cseq);
            res.append("\r\n");

            char buff[200];
            sprintf(buff, "Session: %s\r\n", mSessionID);
            res.append(buff);
			
	     string range;
	     if (findRTSPAttribute("Range", &range)) {
	     		pos1 = range.find("=", 0);
	     		pos2 = range.find("-", 0);

	     		starttime = atoi(range.c_str() + pos1 + 1);
	     		endtime = atoi(range.c_str() + pos2 + 1);
		 	printf("starttime %lld, endtime %lld\n", starttime, endtime);
			if (starttime > 0) {
				starttime = mRTPSender->setSeekTo(starttime);
			}
			
			buff[200];
			if (endtime > 0) {
            			sprintf(buff, "Range: npt=%lld-%lld\r\n", starttime, endtime);
			} else {
				sprintf(buff, "Range: npt=%lld- \r\n", starttime);
			}
			res.append(buff);
			
	     } else {
	     		res.append("Range: npt=0-\r\n");
	     }
		 
            res.append("RTP-Info: ");
	     sprintf(buff, "url=%s/trackID=1;seq=0;rtptime=0,", url.c_str());
	     res.append(buff);
	     sprintf(buff, "url=%s/trackID=0;seq=0;rtptime=0,", url.c_str());
	     res.append(buff);
            res.append("\r\n");
            res.append("\r\n");
			
	     mRTPSender->sendData(starttime, endtime);
            break;
        }
        case REQUEST_TEARDOWN:
        {
            res.assign("RTSP/1.0 200 OK");
            res.append("\r\n");
            res.append("CSeq: ");
            res.append(cseq);
            res.append("\r\n");
            res.append("\r\n");
            break;
        }
        case REQUEST_OPTIONS:
        {
            res.assign("RTSP/1.0 200 OK");
            res.append("\r\n");
            res.append("CSeq: ");
            res.append(cseq);
            res.append("\r\n");
            res.append("Public: DESCRIBE, SETUP, TEARDOWN, PLAY, PAUSE");
            res.append("\r\n");
            res.append("\r\n");
            break;
        }
        case REQUEST_PAUSE:
        {
            res.assign("RTSP/1.0 200 OK");
            res.append("\r\n");
            res.append("CSeq: ");
            res.append(cseq);
            res.append("\r\n");
            res.append("\r\n");

	     mRTPSender->resetAll(false);
            break;
        }
        default:
            printf("Unknown method type !\n");
    }

    sendResponse(res);

    if (methodtype == REQUEST_SETUP) {
       mRTPSender->recvData(info->TrackIndex); 
    }

    return methodtype;
}

int RTSPServer::parseMethodLine(string line) {
	printf("parse method line\n");
	string str(line.c_str());

	int pos = str.find(" ", 0);
	mMethodLine.Method.assign(str.c_str(), pos);
	str.erase(0, pos + 1);

	pos = str.find(" ", 0);
	mMethodLine.URL.assign(str.c_str(), pos);
	str.erase(0, pos + 1);

	mMethodLine.Version.assign(str.c_str());

	return 0;
}

int RTSPServer::checkMethodType() {
	const char *type = mMethodLine.Method.c_str();
    if (!strncmp(type, "DESCRIBE", 8)) {
        return REQUEST_DESCRIPE;
    } else if (!strncmp(type, "SETUP", 5)) {
        return REQUEST_SETUP;
    } else if (!strncmp(type, "PLAY", 4)) {
        return REQUEST_PLAY;
    } else if (!strncmp(type, "TEARDOWN", 8)) {
        return REQUEST_TEARDOWN;
    } else if (!strncmp(type, "OPTIONS", 7)) {
        return REQUEST_OPTIONS;
    } else if (!strncmp(type, "PAUSE", 7)) {
        return REQUEST_PAUSE;
    }

    return 0;
}

int RTSPServer::sendResponse(string res) {
    uint8_t *data = (uint8_t *)(res.c_str());
    size_t size = res.size();

    size_t sendlen = 0;
    while (sendlen < size) {
        size_t n = send(mSocketFd, data, size - sendlen, 0);
        if (n < 0) {
            return -1;
        }

        sendlen += n;
    }

    printf("Socket fd %d, size %d Send bytes %d errno %d\n", mSocketFd, size, sendlen, errno);

    printf("-------------------------------------------------------------------1\n");
    printf("%s\n", (char *)data);
    printf("-------------------------------------------------------------------2\n");

    return 0;
}

bool RTSPServer::findRTSPAttribute(string str, string *value) {
    map<string, string>::iterator it = mRTSPAttribute.find(str);

    if (it == mRTSPAttribute.end()) {
        return false;
    }

    *value = it->second;

    return true;
}

int RTSPServer::parseRTSPAttribute(string str) {
	int pos = str.find(":", 0);

	string key(str.c_str(), pos);
	string value(str.c_str() + pos + 2, str.size() - pos -2);
	printf("---------    %s:%s\n", key.c_str(), value.c_str());

	 mRTSPAttribute.insert(map<string, string>::value_type(key, value));

	return 0;
}

int RTSPServer::clearRTSPAttribute() {
	printf("clear rtsp attribute IN\n");
	if (!mRTSPAttribute.empty()) {
		map<string, string>::iterator it = mRTSPAttribute.begin();

		while (it != mRTSPAttribute.end()) {
			mRTSPAttribute.erase(it);
			it++;
		}
	}
	printf("clear rtsp attribute OUT\n");
	return 0;
}

void startServer() {
    int sockfd,client_fd;

    struct sockaddr_in my_addr;
    struct sockaddr_in remote_addr;

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        printf("socket error");  
        return;
    }

    my_addr.sin_family=AF_INET;  
    my_addr.sin_port=htons(SERVPORT);  
    my_addr.sin_addr.s_addr = INADDR_ANY;  
    bzero(&(my_addr.sin_zero),8); 

    if (bind(sockfd, (struct sockaddr *)&my_addr, sizeof(struct sockaddr)) == -1) {  
        printf("bind error");  
        return; 
    } 
    if (listen(sockfd, BACKLOG) == -1) {  
        printf("listen error");  
        return;
    }

    while(1) {
        socklen_t size = sizeof(struct sockaddr_in);  
        if ((client_fd = accept(sockfd, (struct sockaddr *)&remote_addr, &size)) != -1) {  
            printf("received a connection from %s\n", inet_ntoa(remote_addr.sin_addr));
            pthread_t tid;

            pthread_create(&tid, NULL, processEntry, (void *)(&client_fd));
        }
    }
    close(sockfd);
}

void *processEntry(void *param) {
    int sockfd = *(int *)param;
    RTSPServer *server = new RTSPServer(sockfd);

    if (server == NULL) {
        printf("Create RTSP Server fail\n");
        return NULL;
    }

    server->startRTSPServer();

    delete server;
}
