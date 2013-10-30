#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "Packetizer.h"
#include "PayloadType.h"
#include "Utils.h"

#define UDP_MAX_SIZE 1400

uint8_t const startcode2[3] = {0x00, 0x00, 0x01};
uint8_t const startcode3[4] = {0x00, 0x00, 0x00, 0x01};

Packetizer::Packetizer(const char* format, int index, uint32_t ssrc)
     :mFormat(format),
     mIndex(index){
//    mSequenceNum = rand();
	mSequenceNum = 0;

	uint32_t mSSRC = ssrc;
	printf("---------ssrc %x\n", mSSRC);
}

Packetizer::~Packetizer() {

}

int Packetizer::packetize(DataBuffer *&buff) {
    uint8_t *data = buff->data;
    printf("%s:%x %x %x %x\n", mFormat, data[0], data[1], data[2], data[3]);
    printf("size %d\n", buff->size);
    if (!strncmp(mFormat, "video/avc", 9)) {
//		printf("----------------- avc\n");
        AVCPacketize(buff);
    } else if (!strncmp(mFormat, "audio/mp4a-latm", 15)) {
        AACPacketize(buff);
    }

    return 0;
}

int Packetizer::AVCPacketize(DataBuffer *&buff) {
    size_t length = 0;
    uint8_t *tmppos = buff->data;
//	printf("-----------   buff->timestamp %d\n", buff->timestamp);
	bool hasstartcode = false;

//    if (!memcmp(tmppos, startcode2, 3) || !memcmp(tmppos, startcode3, 4)) {
    if (!memcmp(tmppos, startcode3, 4)) {
//		printf("has start code %x %x %x %x\n", tmppos[0], tmppos[1],tmppos[2],tmppos[3]);
		hasstartcode = true;
	}

    NALU_t nalu;  

    NALUHeader nalu_hdr;

    while (length < buff->size) {
//		printf("length %d, size %d\n", length, buff->size);
        memset(&nalu, 0, sizeof(NALU_t));
        int n = getAnnexbNALU(&nalu, tmppos, buff->size - length, hasstartcode);

//printf("nalu.nal_unit_type %d\n", nalu.nal_unit_type);
        if (nalu.len < UDP_MAX_SIZE) {
            nalu_hdr.F = nalu.forbidden_bit;  
            nalu_hdr.NRI = nalu.nal_reference_idc >> 5;
            nalu_hdr.TYPE = nalu.nal_unit_type; 

            uint8_t *header = NULL;
            size_t size = makeRTPHeader(header, buff->timestamp * 90000 / 1000000);

            PacketData packet;
            packet.size = size + 1 + (nalu.len - 1);
            packet.data = (uint8_t *)malloc(packet.size);
            memcpy(packet.data, header, size);
            memcpy(packet.data + size, &nalu_hdr, 1);
            memcpy(packet.data + size + 1, nalu.buf + 1, nalu.len - 1);
		delete header;
            mPacketList.push_back(packet);
        } else {
            int packetNum = (nalu.len - 1)/UDP_MAX_SIZE;
            if (nalu.len % UDP_MAX_SIZE != 0) {
                packetNum++;
            }

            int lastPackSize = nalu.len - 1 - (packetNum-1)*UDP_MAX_SIZE;  

            uint8_t *pos = nalu.buf + 1;

            FU_INDICATOR fu_ind;  
            FU_HEADER fu_hdr;  

            for (int index = 0; index < packetNum; index++) {
                fu_ind.F = nalu.forbidden_bit;  
                fu_ind.NRI = nalu.nal_reference_idc>>5;  
                fu_ind.TYPE = 28;
                
                //First FU: S = 1, E = 0, R = 0
                //Last FU:  S = 0, E = 1, R = 0
                //Other FU: S = 0, E = 0, R = 0
                if (index == 0) {
                    fu_hdr.S = 1;
                } else {
                    fu_hdr.S = 0;
                }

                if (index == packetNum -1) {
                    fu_hdr.E = 1;  
                } else {
                    fu_hdr.E = 0;
                }

                fu_hdr.R = 0;  
                fu_hdr.TYPE = nalu.nal_unit_type;

                uint8_t *header = NULL;
                size_t size = makeRTPHeader(header, buff->timestamp * 90000 / 1000000);

                PacketData packet;
                
                if (index == packetNum -1) {
                    packet.size = size + 2 + lastPackSize;
                } else {
                    packet.size = size + 2 + UDP_MAX_SIZE;
                }
                packet.data = (uint8_t *)malloc(packet.size);
                memcpy(packet.data, header, size);
                memcpy(packet.data + size, &fu_ind, 1);
                memcpy(packet.data + size + 1, &fu_hdr, 1);
		delete header;
                if (index == packetNum -1) {
                    memcpy(packet.data + size + 2, pos, lastPackSize);
                } else {
                    memcpy(packet.data + size + 2, pos, UDP_MAX_SIZE);
                }
                mPacketList.push_back(packet);
                pos += UDP_MAX_SIZE;
            }
        }
		length += n;
		tmppos += n;
    }

//	printf("----------------1\n");
    return 0;
}

int Packetizer::getAnnexbNALU(NALU_t *nalu, uint8_t *data, size_t size, bool flag) {
    uint8_t *tmp = data;
    size_t len = size;

	if (flag) {
		if (!memcmp(tmp, startcode2, 3)) {
			nalu->startcodeprefix_len = 3;
			tmp += 3;
			len -= 3;
		} else if (!memcmp(tmp, startcode3, 4)) {
			nalu->startcodeprefix_len = 4;
			tmp += 4;
			len -= 4;
		}

		bool StartCodeFound = false;
		while (!StartCodeFound) {
//	printf("----------- nalu len--1 %d\n", nalu->len);
			if (!len) {
				//reach the buff end
				nalu->len = size - nalu->startcodeprefix_len;
				break;
			}
			if (!memcmp(tmp, startcode2, 3) || !memcmp(tmp, startcode3, 4)) {
				StartCodeFound = true;
				nalu->len = (data - tmp) - nalu->startcodeprefix_len;
			}
		}
	} else {
		//Have no start code.
		//printf("get nal len\n");
		nalu->len = parseNALLength(data);
		nalu->startcodeprefix_len = mNALLengthSize;
	}
//	printf("----------- nalu len %d\n", nalu->len);

    nalu->buf = data + nalu->startcodeprefix_len;
    nalu->forbidden_bit = nalu->buf[0] & 0x80;
    nalu->nal_reference_idc = nalu->buf[0] & 0x60; // 2 bit  
    nalu->nal_unit_type = (nalu->buf[0]) & 0x1f;// 5 bit  

//	nalu->buf++;
//	nalu->len--;

    return nalu->len + nalu->startcodeprefix_len;
}

int Packetizer::AACPacketize(DataBuffer *&buff) {
    uint8_t *header = NULL;
    size_t size = makeRTPHeader(header, buff->timestamp * 8000 / 1000000);

    //printf("buff->timestamp %d\n", buff->timestamp);

        size_t datalen = buff->size;// - 7;    
//printf("------datalen %x size %d \n", datalen, size);

        PacketData packet;
        packet.size = size + datalen + 4;
        packet.data = (uint8_t *)malloc(packet.size);
        memcpy(packet.data, header, size);
	 delete header;

        packet.data[size + 0] = 0x0;
        packet.data[size + 1] = 0x10;
        packet.data[size + 2] = datalen >> 5;
        packet.data[size + 3] = (datalen & 0x1f) << 3;

        memcpy(packet.data + size + 4, buff->data, datalen);
/*for (int i = 0; i < 20; i++) {
	uint8_t *mydata = buff->data;
	printf("data[%d]: %x\n", i, mydata[i]);
}
*/
        mPacketList.push_back(packet);

    return 0;
}

int Packetizer::makeRTPHeader(uint8_t *&header, uint32_t ts) {
    RTPHeader rtpheader;

    rtpheader.cc = 1;
    rtpheader.x = 0;
    rtpheader.p = 0;
    rtpheader.v = 2;

    rtpheader.pt = getPayloadType();
    rtpheader.m = 0;

    rtpheader.seq = (mSequenceNum & 0xff) << 8 | (mSequenceNum &0xff00) >> 8;
//	printf("9999999999999999999999999       mSequenceNum %d\n", rtpheader.seq);
    rtpheader.ts = ts >> 24 | (ts >> 8) & 0xff00 | (ts << 8) & 0xff0000 | ts << 24;
    rtpheader.ssrc = mSSRC >> 24 | (mSSRC >> 8) & 0xff00 | (mSSRC << 8) & 0xff0000 | mSSRC << 24;
    rtpheader.csrc = (uint32_t *)malloc(rtpheader.cc * sizeof(uint32_t));

    size_t size1 = 16 - 4;
    size_t size2 = rtpheader.cc * sizeof(uint32_t);

    header = (uint8_t *)malloc(size1 + size2);
    memcpy(header, &rtpheader, size1);
    memcpy(header + size1, rtpheader.csrc, size2);

    mSequenceNum++;
//	printf("size 1 %d, size2 %d\n", size1, size2);
    return size1 + size2;
}

int Packetizer::getPayloadType() {
    int pt = -1;

    for (int i = 0; i < sizeof(PayloadTypeArray)/sizeof(PayloadType); i++) {
        size_t len = strlen(mFormat);
        if (!strncmp(PayloadTypeArray[i].codecmime, mFormat, len)) {
            pt = PayloadTypeArray[i].pltype;
            return pt;
        }
    }

    //Not found the match payload info.
    pt = 96 + mIndex;
    return pt;
}

int Packetizer::getPacketData(uint8_t *&data, size_t *size) {
    if (!mPacketList.empty()) {
        list<PacketData>::iterator it = mPacketList.begin();
        *size = it->size;
        memcpy(data, it->data, *size);
        mPacketList.erase(it);
        return 0;
    }

    return -1;
}

int Packetizer::setNALLengthSize(int len) {
	mNALLengthSize = len;
	return 0;
}

size_t Packetizer::parseNALLength(const uint8_t *data) {
	switch (mNALLengthSize) {
		case 1:
			return *data;
		case 2:
			return U16_AT(data);
		case 3:
		    return ((size_t)data[0] << 16) | U16_AT(&data[1]);
		case 4:
		    return U32_AT(data);
	}

	return 0;
}
