#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <netdb.h>
#include <errno.h>
#include <string>

#include "Utils.h"
#include "RTSPDataReader.h"

static int parsingRTSPURL(char const* url, char* username, char* password, char* host,int* portNum, char* path) 
{
		// Parse the URL as "rtsp://[<username>[:<password>]@]<server-host-or-name>[:<port>][/<path>]"
	uint32_t const prefixLength = 7;
	char const* from = &url[prefixLength];
    char const* tmpPos;

    if ((tmpPos = strchr(from, '@')) != NULL) {
		// We found <username> (and perhaps <password>).
            
		char const* usernameStart = from;
	    char const* passwordStart = NULL;
        char const* p = tmpPos;

        if ((tmpPos = strchr(from, ':')) != NULL && tmpPos < p) {
            passwordStart = tmpPos;
			uint32_t passwordLen = p - passwordStart;
            strncpy(password, passwordStart, passwordLen);

			password[passwordLen] = '\0'; //Set the ending character.
        }
            
        uint32_t usernameLen = 0;
        if (passwordStart != NULL) {
		    usernameLen = tmpPos - usernameStart;
        } else {
            usernameLen = p - usernameStart;    
        }
				
        strncpy(username, usernameStart, usernameLen);
		username[usernameLen] = '\0';  //Set the ending character.

		from = p + 1; // skip the '@'
    }

    const char* pathStart = NULL;
    if ((tmpPos = strchr(from, '/')) != NULL) {
        uint32_t pathLen = strlen(tmpPos + 1);  //Skip '/'
        strncpy(path, tmpPos + 1, pathLen + 1);
        pathStart = tmpPos;
    }

	// Next, will parse the host and port.
    tmpPos = strchr(from, ':');
    if (tmpPos == NULL) {
        if (pathStart == NULL) {
            uint32_t hostLen = strlen(from);
            strncpy(host, from, hostLen + 1);  //Already include '\0'
        } else {
            uint32_t hostLen = pathStart - from;
            strncpy(host, from, hostLen);
		    host[hostLen] = '\0';   //Set the ending character.
        }
	    *portNum = 3333; // Has not the specified port, and will use the default value
    } else if (tmpPos != NULL) {
        uint32_t hostLen = tmpPos - from;

        strncpy(host, from, hostLen);
		host[hostLen] = '\0';  //Set the ending character.
        *portNum = strtoul(tmpPos + 1, NULL, 10); 
    }

	return 1;
}

/*static int makeSocketBlocking(int s, bool blocking) {
    int flags = fcntl(s, F_GETFL, 0);
    if (flags == -1) {
        return flags;
    }

    if (blocking) {
        flags &= ~O_NONBLOCK;
    } else {
        flags |= O_NONBLOCK;
    }

    flags = fcntl(s, F_SETFL, flags);

    return flags;
}
*/
RTSPDataReader::RTSPDataReader(const char *url)
    :mSocket(-1),
     mNextCSeq(0),
     mURL(url),
     mSessionURL(url),
     mSessionDescription(NULL){

    mTracksInfo.push_back(new trackInfo());

    char* host = new char[100];
    char* username = new char[100];
    char* password = new char[100];
    char* path = new char[100];

    memset(host,0,100);

    int port;
    parsingRTSPURL(url,username,password,host,&port,path) ;
 //   printf("username %s, password %s, host %s, portnum %d, path %s\n", username, password, host, port, path);


    if (!RTSPConnect(host, port)) {
        printf("Connect successfully --!\n");
        mNextCSeq = 1;
    }

    if (!sendRTSPRequest(REQUEST_DESCRIPE, -1)) {
        uint8_t *data;
        size_t size;
        recvRTSPResponse(data, &size);
        parsingResponse(data, &size, 0);

        mSessionDescription = new SessionDescription((const char *)data, size);
    }

    mRTPConnection = new RTPConnection(mSessionDescription);

    mRTPConnection->setCallBack(CallBackFunc);

    for (int i = 1; i < mSessionDescription->tracksCount(); i++) {
  //      printf("  i %d\n", i);
        setupTrack(i);
    }

    if (!sendRTSPRequest(REQUEST_PLAY, -1)) {
        uint8_t *data;
        size_t size;
        recvRTSPResponse(data, &size);
        parsingResponse(data, &size, 0);
    }
    
    mRTPConnection->loopGetData();

    while(1) {
        pthread_mutex_lock(&mRTSPMutex);
        pthread_cond_wait(&mRTSPCond, &mRTSPMutex);
        pthread_mutex_unlock(&mRTSPMutex);

        if(mCallBackFlag == 0x01) {
            sendRTSPRequest(REQUEST_TEARDOWN, -1);
            break;
        }
    }
}

RTSPDataReader::~RTSPDataReader() {
    if (mSessionDescription != NULL) {
        delete mSessionDescription;
    }

    if (mRTPConnection != NULL) {
        delete mRTPConnection;
    }

    vector<trackInfo *>::iterator it = mTracksInfo.begin();

    while (it != mTracksInfo.end()) {
        trackInfo *info = (*it);

        delete info;
        it++;
    }
}

int RTSPDataReader::mCallBackFlag = 0;
pthread_mutex_t RTSPDataReader::mRTSPMutex = PTHREAD_MUTEX_INITIALIZER; 
pthread_cond_t RTSPDataReader::mRTSPCond = PTHREAD_COND_INITIALIZER;

void RTSPDataReader::CallBackFunc(int flag) {
    mCallBackFlag = flag;

    pthread_mutex_lock(&mRTSPMutex);
    pthread_cond_signal(&mRTSPCond);
    pthread_mutex_unlock(&mRTSPMutex);
}

int RTSPDataReader::setupTrack(int index) {
    mTracksInfo.push_back(new trackInfo());
    trackInfo *info = mTracksInfo[index];

    mRTPConnection->makePortPair(&info->RTPSocket, &info->RTCPSocket, &info->RTPPort);
    sendRTSPRequest(REQUEST_SETUP, index);

    mRTPConnection->addStream(info->RTPSocket, info->RTCPSocket, index);
    uint8_t *data;
    size_t size;
    recvRTSPResponse(data, &size);
    parsingResponse(data, &size, index);

    findHeader(index, "Session", &mSessionID);

    return 0;
}

int RTSPDataReader::makeTrackURL(string *trackurl, int index) {
    string trackID;

    mSessionDescription->findAttribute(index, "a=control", &trackID);

    trackurl->append(mSessionURL);
    trackurl->append("/");
    trackurl->append(trackID);

//    printf("track url %s\n",trackurl->c_str());
    return 0;
}

bool RTSPDataReader::findHeader(int index, string key, string *value) {

    value->clear();
    trackInfo *info = mTracksInfo[index];

    map<string, string>::iterator it = info->mHeader.find(key);

    if (it == info->mHeader.end()) {
        return false;
    }

    *value = it->second;

    return true;
}

int RTSPDataReader::RTSPConnect(char *host, int port) {
//    printf("host %s, port %d\n", host, port);
    struct hostent *ent = gethostbyname(host);
    if (ent == NULL) {
        printf("Unknown host %s\n", host);
        return -1;
    }

    if((mSocket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("socket create failed!\n");
        return -1;
    }

    //makeSocketBlocking(mSocket, false);

    struct sockaddr_in remote;
    memset(remote.sin_zero, 0, sizeof(remote.sin_zero));
    remote.sin_family = AF_INET;
    remote.sin_addr.s_addr = *(in_addr_t *)ent->h_addr;
    remote.sin_port = htons(port);

    int err = connect(mSocket, (const struct sockaddr *)&remote, sizeof(remote));
    if (err ==-1) {
        printf("Connect fail errno %d!\n", errno);
        return -1;
    }

    return 0;
}

int RTSPDataReader::sendRTSPRequest(uint32_t req, int index) {
    string *msg = new string();

    switch (req) {
        case REQUEST_DESCRIPE:
        {
            msg->append("DESCRIBE ");
            msg->append(mSessionURL);
            msg->append(" RTSP/1.0\r\n");
            msg->append("Accept: application/sdp\r\n");
            msg->append("\r\n");

            break;
        }
        case REQUEST_SETUP:
        {
            msg->append("SETUP ");
            string trackurl;
            makeTrackURL(&trackurl, index);
            msg->append(trackurl);
            msg->append(" RTSP/1.0\r\n");

            trackInfo *info = mTracksInfo[index];
            uint32_t rtpPort = info->RTPPort;
            msg->append("Transport: RTP/AVP/UDP;unicast;client_port=");

            char str[16];
            msg->append(itos(rtpPort, str));
            msg->append("-");
            msg->append(itos(rtpPort + 1, str));
            msg->append("\r\n");
            if (index > 1) {
                msg->append("Session: ");
                msg->append(mSessionID);
                msg->append("\r\n");
            }
            msg->append("\r\n");
            break;
        }
        case REQUEST_PLAY:
        {
            msg->append("PLAY ");
            msg->append(mSessionURL);
            msg->append(" RTSP/1.0\r\n");
            msg->append("Session: ");
            msg->append(mSessionID);
            msg->append("\r\n");
            msg->append("\r\n");
            break;
        }
        case REQUEST_TEARDOWN:
        {
            msg->append("TEARDOWN ");
            msg->append(mSessionURL);
            msg->append(" RTSP/1.0\r\n");
            msg->append("Session: ");
            msg->append(mSessionID);
            msg->append("\r\n");
            msg->append("\r\n");
            break;
        }
        default:
            printf("Unknown Request !\n");
    }

    ssize_t i = msg->find("\r\n\r\n");
    int32_t cseq = mNextCSeq++;

    string cseqHeader = "CSeq: ";

    char str[16];
    cseqHeader.append(itos(cseq, str));
    cseqHeader.append("\r\n");

    msg->insert(i + 2, cseqHeader.c_str());
    printf("%s\n", msg->c_str());

    size_t numSent = 0;
    size_t len = msg->size();

    const char *pos = msg->c_str();
    while (numSent < len) {
        ssize_t n = send(mSocket, pos + numSent, len - numSent, 0);

//        printf("n %d len %d\n", n, len);
        numSent += (size_t)n;
    }

    return 0;
}

int RTSPDataReader::parsingResponse(uint8_t *&data, size_t *size, int index) {
    string *tmpdesc = new string((const char *)data);
    printf("%s\n", data);
    
    trackInfo *info = mTracksInfo[index];

    int32_t pos = tmpdesc->find("\r\n", 0);
    string statusline(tmpdesc->c_str(), pos);

    mStatusLine.assign(statusline.c_str());
    tmpdesc->erase(0, pos + 2);

    while (tmpdesc->size() > 0) {
        pos = tmpdesc->find("\r\n", 0);

        string *line = new string(tmpdesc->c_str(), pos);
       // printf("-- %s\n", line->c_str());

        char c = (line->c_str())[0];
        if (c == 'v') {
	        data = (uint8_t *)(const_cast<char *>(tmpdesc->c_str()));
            *size = tmpdesc->size();
	        break;
        } else {
            int32_t tmppos = line->find(":", 0);
            if (tmppos >= 0) {
                string key(line->c_str(), tmppos);
                string value(line->c_str() + tmppos + 1, line->size() - tmppos - 1);
//                printf("key %s, value %s\n", key.c_str(), value.c_str());
                info->mHeader.insert(pair<string, string>(key, value));
            } else {
            }
        }

	    delete line;
        tmpdesc->erase(0, pos + 2);
    }
    return 0;
}

int RTSPDataReader::recvRTSPResponse(uint8_t *&data, size_t *size) {
    size_t len = 4096;
    data = (uint8_t *)malloc(len);
    *size = 0;

    while (1) {
        ssize_t n = recv(mSocket, data + *size, len - *size, 0);

        if (n < 0 && errno == EINTR) {
            continue;
        } else if (n <= (ssize_t)len && n > 0) {
            *size += n;
            break;
        }

        *size += n;
    }

    return 0;
}

ssize_t RTSPDataReader::readAt(off64_t offset, void *data, size_t size) {
    return 0;
}

