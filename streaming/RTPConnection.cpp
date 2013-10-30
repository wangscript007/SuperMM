#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <netdb.h>
#include <errno.h>
#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>

#include "Utils.h"
#include "RTPConnection.h"

#define LOG_TAG "RTPConnection"

RTPConnection::RTPConnection(SessionDescription *sedec)
    :mSessionDescription(sedec){

}

RTPConnection::~RTPConnection() {

}

void RTPConnection::makePortPair(int *rtpSocket, int *rtcpSocket, uint32_t *rtpPort) {
    *rtpSocket = socket(AF_INET, SOCK_DGRAM, 0);

    int size = 256 * 1024;
    setsockopt(*rtpSocket, SOL_SOCKET, SO_RCVBUF, &size, sizeof(size));

    *rtcpSocket = socket(AF_INET, SOCK_DGRAM, 0);

    size = 256 * 1024;
    setsockopt(*rtcpSocket, SOL_SOCKET, SO_RCVBUF, &size, sizeof(size));

    /* rand() * 1000 may overflow int type, use long long */
    //unsigned start = (rand() * 1000)/RAND_MAX + 15550;
    unsigned randPart = (unsigned)((rand()* 1000ll)/RAND_MAX);
    unsigned start = 15550+randPart; /* variable @start: default start port */
    unsigned startPort = start;
    unsigned endPort = 65536; /*variable @endPort: default end port is 65536 */

    if(startPort>endPort){
        startPort = start;
        endPort = 65536;
    }
    startPort &= ~1;

    for (unsigned port = startPort; port < endPort; port += 2) {
        struct sockaddr_in addr;
        memset(addr.sin_zero, 0, sizeof(addr.sin_zero));
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = htonl(INADDR_ANY);
        addr.sin_port = htons(port);

        if (bind(*rtpSocket,
                 (const struct sockaddr *)&addr, sizeof(addr)) < 0) {
            continue;
        }

        addr.sin_port = htons(port + 1);

        if (bind(*rtcpSocket,
                 (const struct sockaddr *)&addr, sizeof(addr)) == 0) {
            *rtpPort = port;
            return;
        }
    }
}

void RTPConnection::addStream(int rtpSocket, int rtcpSocket, int index) {
    streamInfo *info = new streamInfo();

    info->mHighestSeqNumber = 0;
    info->mNumBuffersReceived = 0;

    info->mRTPSocket = rtpSocket;
    info->mRTCPSocket = rtcpSocket;
    info->mIndex = index - 1;

    mSessionDescription->findAttribute(index, "a=rtpmap", &info->format);

//    LOG("--------------------------------------- %s\n", info->format.c_str());
    info->mIsEOS = false; 

    mStreamsInfo.push_back(info);    
}

void RTPConnection::setCallBack(callback func) {
    if (func == NULL) {
        return;
    }

    mCallBackFunc = func;
}

int RTPConnection::loopGetData() {
    pthread_t tid;
    int err = pthread_create(&tid, NULL, recvEntry, (void*)this);

    if (err) {
        return -1;
    }

    return 0;
}

void *RTPConnection::recvEntry(void *param) {
    RTPConnection *rtpconn = (RTPConnection *)param;
    while(1) {
        struct timeval tv;
        tv.tv_sec = 0;
        tv.tv_usec = 1000ll;

        fd_set rs;
        FD_ZERO(&rs);

        int maxSocket = -1;
        for (int i = 0; i < (int)rtpconn->mStreamsInfo.size(); i++) {
            streamInfo *info = rtpconn->mStreamsInfo[i];
            FD_SET(info->mRTPSocket, &rs);
            FD_SET(info->mRTCPSocket, &rs);

            if (info->mRTPSocket > maxSocket) {
                maxSocket = info->mRTPSocket;
            }
            if (info->mRTCPSocket > maxSocket) {
                maxSocket = info->mRTCPSocket;
            }
        }

        if (maxSocket == -1) {
            break;
        }

        int res = select(maxSocket + 1, &rs, NULL, NULL, &tv);

        if (res > 0) {
            for (int i = 0; i < (int)rtpconn->mStreamsInfo.size(); i++) {
                int err = -1;
                streamInfo *info = rtpconn->mStreamsInfo[i];

                if (FD_ISSET(info->mRTPSocket, &rs)) {
                    err = rtpconn->recvData(info, true);
                }
                if (err >= 0 && FD_ISSET(info->mRTCPSocket, &rs)) {
                    err = rtpconn->recvData(info, false);
                }
            }
        }

        //check eos
        if (rtpconn->checkEOS()) {
            int flag = 0x01;
            printf("----------------------------------------- fffffff\n");
            rtpconn->mCallBackFunc(flag);
            break;
        }
    }

    return NULL;
}

bool RTPConnection::checkEOS() {
    bool eos = true;

    for (int i = 0; i < (int)mStreamsInfo.size(); i++) {
        eos &= mStreamsInfo[i]->mIsEOS;
    }

    return eos;
}

int RTPConnection::recvData(streamInfo *s, bool receiveRTP) {
    size_t size = 65536;
    size_t recved = 0;
    uint8_t *buffer = (uint8_t *)malloc(size);

    size_t nbytes;
    do {
        nbytes = recvfrom(
            receiveRTP ? s->mRTPSocket : s->mRTCPSocket,
            buffer + recved,
            size - recved,
            0,
            NULL,
            NULL);
        recved += nbytes;
    } while (nbytes < 0 && errno == EINTR);
//    LOG("---------------- receive bytes %d\n", nbytes);

    if (nbytes <= 0) {
        return -1;
    }

    if (receiveRTP) {
        packetInfo *info = new packetInfo();
        info->data = buffer;
        info->size = recved;

        if (!parsingRTPData(info)) {
            pushRTPPacket(s, info);
        }
    } else {
        int res = parsingRTCPData(buffer, recved);
        printf("--------------------------------rtcp\n");
        if (res == REPORT_BYE) {
            s->mIsEOS = true;
        }
    }

    return 0;
}

int RTPConnection::parsingRTPData(packetInfo *& info) {
    size_t size = info->size;
    uint8_t *data = info->data;

    if (size < 12) {
        return -1;
    }

    if ((data[0] >> 6) != 2) {
//        LOG("Unuspported version !\n");
        return -1;
    }

    if (data[0] & 0x20) {
        // Padding present.

        size_t paddingLength = data[size - 1];

        if (paddingLength + 12 > size) {
            return -1;
        }

        size -= paddingLength;
    }

    int numCSRCs = data[0] & 0x0f;

    size_t payloadOffset = 12 + 4 * numCSRCs;

    if (size < payloadOffset) {
        // Not enough data to fit the basic header and all the CSRC entries.
        return -1;
    }

    if (data[0] & 0x10) {
        // Header eXtension present.

        if (size < payloadOffset + 4) {
            // Not enough data to fit the basic header, all CSRC entries
            // and the first 4 bytes of the extension header.

            return -1;
        }

        const uint8_t *extensionData = &data[payloadOffset];

        size_t extensionLength =
            4 * (extensionData[2] << 8 | extensionData[3]);

        if (size < payloadOffset + 4 + extensionLength) {
            return -1;
        }

        payloadOffset += 4 + extensionLength;
    }

    uint32_t rtpTime = U32_AT(&data[4]);
    uint32_t srcId = U32_AT(&data[8]);
    uint32_t seqNum = U16_AT(&data[2]);
//    LOG("rtp time %d, src id %d, seq num %d\n", rtpTime, srcId, seqNum);

    info->data = data + payloadOffset;
    info->size = size - payloadOffset;
    info->rtpTime = rtpTime;
    info->srcId = srcId;
    info->seqNum = seqNum;

    return 0;
}

int RTPConnection::parsingRTCPData(uint8_t *buff, size_t size) {
    //LOG("RTCP packet data size %d\n", size);
    printf("%s\n", (char *)buff);
    if (!strncmp((char *)buff, "BYE", 3)) {
        return REPORT_BYE;
    }

    return 0;
}

int RTPConnection::pushRTPPacket(streamInfo *s, packetInfo *info) {
    uint32_t seqNum = info->seqNum;

    if (s->mNumBuffersReceived++ == 0) {
        s->mHighestSeqNumber = seqNum;
        s->mPacketList.push_back(info);
        return 0;
    }

    // Only the lower 16-bit of the sequence numbers are transmitted,
    // derive the high-order bits by choosing the candidate closest
    // to the highest sequence number (extended to 32 bits) received so far.

    uint32_t seq1 = seqNum | (s->mHighestSeqNumber & 0xffff0000);
    uint32_t seq2 = seqNum | ((s->mHighestSeqNumber & 0xffff0000) + 0x10000);
    uint32_t seq3 = seqNum | ((s->mHighestSeqNumber & 0xffff0000) - 0x10000);
    uint32_t diff1 = absDiff(seq1, s->mHighestSeqNumber);
    uint32_t diff2 = absDiff(seq2, s->mHighestSeqNumber);
    uint32_t diff3 = absDiff(seq3, s->mHighestSeqNumber);

    if (diff1 < diff2) {
        if (diff1 < diff3) {
            seqNum = seq1;
        } else {
            seqNum = seq3;
        }
    } else if (diff2 < diff3) {
        seqNum = seq2;
    } else {
        seqNum = seq3;
    }

    if (seqNum > s->mHighestSeqNumber) {
        s->mHighestSeqNumber = seqNum;
    }

    info->seqNum = seqNum;

    //reorder
    list<packetInfo *>::iterator it = s->mPacketList.begin();
    while (it != s->mPacketList.end() && (*it)->seqNum < seqNum) {
        ++it;
    }

    if (it != s->mPacketList.end() && (*it)->seqNum == seqNum) {
        return -1;
    }

    //insert
    s->mPacketList.insert(it, info);
//    LOG("index %d, list size %d\n", s->mIndex, s->mPacketList.size());

    return 0;
}

