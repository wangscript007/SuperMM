#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "FLVFileProbeInfo.h"
#include "Utils.h"

const char *flvAudioCodecName[] = {
        "Linear PCM, platform endian",
        "ADPCM",
        "MP3",
        "Linear PCM, little endian",
        "Nellymoser 16 kHz mono",
        "Nellymoser 8 kHz mono",
        "Nellymoser",
        "G.711 A-law logarithmic PCM",
        "G.711 mu-law logarithmic PCM",
        "reserved",
        "AAC",
        "reserved",
        "reserved",
        "reserved",
        "Speex",
        "MP3 8 kHz",
        "Device-specific sound"
};

const char *flvVideoCodecName[] = {
        "reserved",
        "JPEG",
        "Sorenson H.263",
        "Screen video",
        "On2 VP6",
        "On2 VP6 with alpha channel",
        "Screen video version 2",
        "H.264"
};

const char *flvVideoFrameType[] = {
        "reserved",
        "keyframe",
        "inter frame",
        "disposable inter frame",
        "generated keyframe",
        "video info/command frame"
};

FLVFileProbeInfo::FLVFileProbeInfo(FileDataReader *reader)
    :mHasAudio(false),
     mHasVideo(false),
     mVersion(0),
     mTrackIndex(-1),
     mVideoStartPosition(-1),
     mAudioStartPosition(-1),
     mFoundScriptData(false),
     mFoundFilePositions(false),
     mFoundTimes(false),
     mKeyFrames(NULL),
     mFileDataReader(reader) {

	 mKeyFrames = (keyFrame *)malloc(sizeof(keyFrame));

}

void FLVFileProbeInfo::FileParse() {
    printf("Parse FLV file\n");
    uint8_t header[9];
    ssize_t n = mFileDataReader->readAt(0, header, sizeof(header));

    if (n < (ssize_t)sizeof(header)) {
        return;
    }
    //First parse the flv header
    bool ret = HeaderDecode(header);
    if (ret < 0) {
        return;
    }

    uint64_t offset = 4 + 9;
    ssize_t tagsize = 0;

    uint64_t filesize = mFileDataReader->getFileSize();

    while (offset < filesize) {
        tagsize = TagDecode(offset);
        if (mVideoStartPosition > 0 && mAudioStartPosition > 0 && mFoundScriptData) {
            break;
        }
        offset += (tagsize + 4);
    }
}

void FLVFileProbeInfo::InfoDump() {
    printf("FLV Version : %d\n", mVersion);
    if (mHasAudio) {
        printf("This FLV file has Audio track !\n");
    }

    if (mHasVideo) {
        printf("This FLV file has Video track !\n");
    }
}

bool FLVFileProbeInfo::HeaderDecode(uint8_t *data) {

    mVersion = data[3];
    int8_t flag = data[4];

    if (flag & 0x4) {
        mHasAudio = true;
        TrackInfo trackinfo;
        trackinfo.TrackFlag = 0x08;
        mTrackIndex++;
        trackinfo.TrackIndex = mTrackIndex;
	 trackinfo.isAudio = true;
        mTrackVector.push_back(trackinfo);
    }

    if (flag & 1) {
        mHasVideo = true;
        TrackInfo trackinfo;
        trackinfo.TrackFlag = 0x09;
        mTrackIndex++;
        trackinfo.TrackIndex = mTrackIndex;
	  trackinfo.isAudio = false;
        mTrackVector.push_back(trackinfo);
    }

    return true;
}

void FLVFileProbeInfo::parseAudioFrameFirstByte(uint64_t offset) {
    uint8_t byte;
    uint8_t codecnameID;
    const char *codecname = NULL;
    mFileDataReader->readAt(offset, &byte, 1);

    codecnameID = byte >> 4;
    
    codecname = flvAudioCodecName[codecnameID];
    //printf("audio codec name : %s\n", codecname);
    
    vector<TrackInfo>::iterator it = mTrackVector.begin();
    while (it != mTrackVector.end()) {
        if (it->TrackFlag == 0x08) {
            break;
        }
        it++;
    }

    switch (codecnameID) {
        case 10:
            it->format.assign("audio/mp4a-latm");
            break;
        default:
            printf("Unknown Audio ID \n");
    }
}

void FLVFileProbeInfo::parseVideoFrameFirstByte(uint64_t offset) {
    uint8_t byte;
    uint8_t codecnameID;
    uint8_t frametypeID;
    const char *codecname = NULL;
    const char *frametype = NULL;
    mFileDataReader->readAt(offset, &byte, 1);

    codecnameID = byte & 0xf;
    frametypeID = byte >> 4;
    codecname = flvVideoCodecName[codecnameID];
    frametype = flvVideoFrameType[frametypeID];
    //printf("viode codec name : %s, frame type : %s\n", codecname, frametype);
    
    vector<TrackInfo>::iterator it = mTrackVector.begin();
    while (it != mTrackVector.end()) {
        if (it->TrackFlag == 0x09) {
            break;
        }
        it++;
    }

    switch (codecnameID) {
        case 2:
            it->format.assign("video/flv-sh263");
            break;
        case 4:
            it->format.assign("video/flv-vp6");
            break;
        case 7:
            it->format.assign("video/avc");
            break;
        default:
            printf("Unknown Video ID \n");

    }
}

void FLVFileProbeInfo::parseScriptData(uint64_t offset, ssize_t size) {
    uint8_t data[size];
    ssize_t n = mFileDataReader->readAt(offset, data, size);

    if (n < size) {
        return;
    }
    uint64_t pos = 0;
    
    //parse the first amf packet

    uint8_t type = data[pos];
    if (type != 0x02) {
        return;
    }

    uint16_t len = U16_AT(&data[pos + 1]);

    //parse the second amf packet
    pos += 1 + 2 + len;
    parseAMFObject(data, &pos, size);

    mFoundScriptData = true;
    //map<string, int8_t*>::iterator it = mMixedArray.begin();

    //while (it != mMixedArray.end()) {
    //    printf("-----name %s\n", it->first.c_str());
    //    it++;
    //}
}

int64_t FLVFileProbeInfo::getDuration() {
    int8_t *value;
    double duration;
    if (findElement("duration", value)) {
        memcpy(&duration, value, 8);
    }
    
    return (int64_t)duration;
}

int32_t FLVFileProbeInfo::getVideoFrameSize(int *h, int *w) {
    double height, width;
    int8_t *value;

    if (findElement("width", value)) {
        memcpy(&width, value, 8);
        printf("-------- %lld\n", (int64_t)width);
        *w = (int)width;
    }
    
    if (findElement("height", value)) {
        memcpy(&height, value, 8);
        *h = (int)height;
    }
    
    return 0;
}

int32_t FLVFileProbeInfo::getTrackCounts() {
    return mTrackVector.size();
}

TrackInfo *FLVFileProbeInfo::getTrackInfo(int index) {
    return &mTrackVector[index];
}

bool FLVFileProbeInfo::findElement(string str, int8_t* &value) {
    map<string, int8_t*>::iterator it = mMixedArray.find(str);

    if (it == mMixedArray.end()) {
        printf("----------------1\n");
        return false;
    }

    value = it->second;

    return true;
}

void toLittleEndian(void *buff, uint32_t len)
{
    uint32_t i;
    uint8_t arr[16] = {0}, *tmp = (uint8_t *)buff;

    memcpy(arr, tmp, len);
    for(i = 0; i < len; i ++) {
        tmp[i] = arr[len - i - 1];
    }
}

void FLVFileProbeInfo::parseAMFObject(uint8_t *data, uint64_t *pos, ssize_t max_size) {
        uint8_t value[128] = {};
//printf("pos %lld, max size %d\n", *pos, max_size);
        uint16_t valuelen = 0;

        int8_t type = data[*pos];
        printf("type %d\n",type);
        memset(value, 0, sizeof(value));
        switch (type) {
            case 0x00:   //double  8 bytes.
            {
                valuelen = 8;
                if (mInsertPos.second == true && (mInsertPos.first)->second == NULL) {
                    (mInsertPos.first)->second = (int8_t *)malloc(valuelen);
                    memcpy((mInsertPos.first)->second, data + *pos + 1, valuelen);
                    toLittleEndian((mInsertPos.first)->second, valuelen);
                }
                *pos += 1 + valuelen;
                break;
            }
            case 0x01:   //bool
                valuelen = 1;
                if (mInsertPos.second == true && (mInsertPos.first)->second == NULL) {
                    (mInsertPos.first)->second = (int8_t *)malloc(valuelen);
                    memcpy((mInsertPos.first)->second, data + *pos + 1, valuelen);
                }
                *pos += 1 + valuelen;
                break;
            case 0x02:   //string
                valuelen = U16_AT(&data[*pos + 1]);
                if (mInsertPos.second == true && (mInsertPos.first)->second == NULL) {
                    (mInsertPos.first)->second = (int8_t *)malloc(valuelen + 1);
                    memcpy((mInsertPos.first)->second, data + *pos + 3, valuelen);
                    value[valuelen] = '\0';
                }
                *pos += 1 + 2 + valuelen;
                break;
            case 0x03:   //object
                *pos += 1;
                while(*pos < (uint64_t)max_size) {
                    uint8_t name[128] = {};
                    uint16_t namelen = U16_AT(&data[*pos]);

                    if (namelen <= 0) {
                        break;
                    }

                    memset(name, 0, 128);
                    memcpy(name, &data[*pos + 2], namelen);
                    name[namelen] = '\0';
                     if (!memcmp(name, "filepositions", 13)) {
				mFoundFilePositions = true;
                     } else if (!memcmp(name, "times", 5)) {
				mFoundTimes = true;
                     }
                    *pos += 2 + namelen;
                    parseAMFObject(data, pos, max_size);
                }
                break;
            case 0x08:   //mixed array
                uint32_t mixedarraynum;
                mixedarraynum = U32_AT(&data[*pos + 1]);
                *pos += 1 + 4;
                for (uint32_t i = 0; i < mixedarraynum; i++) {
                    uint16_t namelen = U16_AT(&data[*pos]);
                    string name((char *)(data + *pos + 2), namelen);
                    if (strncmp(name.c_str(), "keyframes", 8)) {
                        mInsertPos = mMixedArray.insert(map<string, int8_t*>::value_type(name, NULL));
                    }
                    *pos += 2 + namelen;
                    printf("name %s\n", name.c_str());
                    parseAMFObject(data, pos, max_size);
                }
                break;
            case 0x0a:   //array
                uint32_t arraynum;
                arraynum = U32_AT(&data[*pos + 1]);
                printf("arraynum :%d\n",arraynum);
                *pos += 1 + 4;
		  if (mFoundFilePositions) {
		  	mKeyFrames->keyFramePositionNum = arraynum;
		  	mKeyFrames->keyFramePositions = (int64_t *)malloc(arraynum * sizeof(int64_t));
                	for (uint32_t i = 0; i < arraynum; i++) {
				int8_t tmp[8];
				double value;
				memcpy(tmp, data + *pos + 1, 8);
                    		toLittleEndian(tmp, 8);
				memcpy(&value, tmp, 8);
				mKeyFrames->keyFramePositions[i] = (int64_t)value;
//				printf("mKeyFramePositons[%d]: %lld\n", i, mKeyFramePositons[i]);
				*pos += (8 + 1);
                	}
			mFoundFilePositions = false;
                } else if (mFoundTimes) {
                	mKeyFrames->keyFrameTimeNum = arraynum;
		  	mKeyFrames->keyFrameTimes = (int64_t *)malloc(arraynum * sizeof(int64_t));
                	for (uint32_t i = 0; i < arraynum; i++) {
				int8_t tmp[8];
				double value;
				memcpy(tmp, data + *pos + 1, 8);
                    		toLittleEndian(tmp, 8);
				memcpy(&value, tmp, 8);
				mKeyFrames->keyFrameTimes[i] = (int64_t)value;
//				printf("mKeyFrameTimes[%d]: %lld\n", i, mKeyFrameTimes[i]);
				*pos += (8 + 1);
                	}
			mFoundTimes = false;
                } else {
                	for (uint32_t i = 0; i < arraynum; i++) {
				parseAMFObject(data, pos, max_size);
                	}
                }
                break;
            default:
                printf("Unknown type !\n");
        }
}

ssize_t FLVFileProbeInfo::TagDecode(uint64_t offset) {
    ssize_t tagsize = 0;
    uint8_t tagheader[11];
    ssize_t n = mFileDataReader->readAt(offset, tagheader, sizeof(tagheader));

    if (n < (ssize_t)sizeof(tagheader)) {
        if (!n) {
            printf("Reach the end of the file !");
            return 0;
        } else {
            return -1;
        }
    }

    uint8_t tagtype = tagheader[0];
   // printf("tag type %x\n", tagtype);
    uint32_t datasize = U32_AT(tagheader) & 0xffffff;
  //  printf("data size %d\n",datasize);
    int32_t timestamp = (U32_AT(&tagheader[4]) >> 8) | (tagheader[7] << 24);
   // printf("timestamp %d %x\n",timestamp, tagheader[7]);

    uint64_t dataoffset = offset + 11;
    if (tagtype == 0x08) {
        //Audio
        parseAudioFrameFirstByte(dataoffset);
    } else if (tagtype == 0x09) {
        //Video
        parseVideoFrameFirstByte(dataoffset);
    } else if (tagtype == 0x12) {
        //Script
        parseScriptData(dataoffset, datasize);
    }

    for (int i = 0; i < (int)mTrackVector.size(); i++) {
        if (mTrackVector[i].TrackFlag == tagtype) {
            if (!strncmp(mTrackVector[i].format.c_str(), "video/avc", 9)) {
                uint8_t buff[4]; 
                mFileDataReader->readAt(dataoffset + 1, buff, 4);
                    //printf("%x %x %x %x %x %x %x %x %x\n", buff[0], buff[1], buff[2], buff[3], buff[4], buff[5], buff[6], buff[7], buff[8]);
                int8_t AVCPacketType = buff[0];
		  if (AVCPacketType == 0){
			mVideoConfigBuffer = new DataBuffer();
			mVideoConfigBuffer->size = datasize -1 - 4;
			mVideoConfigBuffer->data = (uint8_t *)malloc(mVideoConfigBuffer->size);
			mFileDataReader->readAt(dataoffset + 5, mVideoConfigBuffer->data, mVideoConfigBuffer->size);
			break;
                }
            } else if (!strncmp(mTrackVector[i].format.c_str(), "audio/mp4a-latm", 15)) {
                uint8_t packettype;
                mFileDataReader->readAt(dataoffset + 1, &packettype, 1);
		  if (packettype == 0) {
			mAudioConfigBuffer = new DataBuffer();
			mAudioConfigBuffer->size = datasize - 2;
			mAudioConfigBuffer->data = (uint8_t *)malloc(mAudioConfigBuffer->size);		
			mFileDataReader->readAt(dataoffset + 2, mAudioConfigBuffer->data, mAudioConfigBuffer->size);
			break;
		  }
            }
	     if (tagtype == 0x08) {
			mAudioStartPosition = offset;
	     	} else if (tagtype == 0x09) {
			mVideoStartPosition = offset;
	     	}

            break;
        }
    }

    tagsize = datasize + 11;
    return tagsize;
}

DataBuffer *FLVFileProbeInfo::getVideoConfigBuffer() {
	uint8_t *data = mVideoConfigBuffer->data;
	printf("111111 %x %x %x %x %x %x\n", data[0], data[1], data[2], data[3], data[4], data[5]);
	
	return mVideoConfigBuffer;
}

DataBuffer *FLVFileProbeInfo::getAudioConfigBuffer() {

	return mAudioConfigBuffer;
}

int32_t FLVFileProbeInfo::setSeekTo(int64_t seekTime) {
	int index = 0;
	int64_t timeus;
		
	for (int i = 0; i < mKeyFrames->keyFrameTimeNum; i++) {
		timeus = mKeyFrames->keyFrameTimes[i];
		if (timeus > seekTime) {
			index = i;
			break;
		}
	}
	
	printf("seekTime %lld, timeus %lld\n", seekTime, timeus);
	mVideoStartPosition = mKeyFrames->keyFramePositions[index];

	uint8_t data[11];
	
	mFileDataReader->readAt(mVideoStartPosition, data, 11);
    	uint32_t size = U32_AT(data) & 0xffffff;

	mAudioStartPosition = mVideoStartPosition;
	updateReadPosition(true, size);
	
	return timeus;
}

int32_t FLVFileProbeInfo::readData(int index, DataBuffer *&buff) {
//printf("FLVFileProbeInfo::readData IN %d\n", mTrackVector[index].isAudio);
	int64_t offset = 0;
	if (mTrackVector[index].isAudio) {
		offset = mAudioStartPosition;
	} else {
		offset = mVideoStartPosition;
	}

	uint8_t data[11];
	
	mFileDataReader->readAt(offset, data, 11);
	uint8_t type = data[0];
    	uint32_t size = U32_AT(data) & 0xffffff;
    	int32_t timeUS = (U32_AT(&data[4]) >> 8) | (data[7] << 24);

	offset += (11 + 1);
	buff->size = size - 1;
	buff->timestamp = timeUS * 1000;
	if (!strncmp(mTrackVector[index].format.c_str(), "video/avc", 9)) {
                uint8_t tmp[4]; 
                mFileDataReader->readAt(offset, tmp, 4);
		int8_t AVCPacketType = tmp[0];
//		printf("AVCPacketType %d\n", AVCPacketType);
		  int32_t cto = U32_AT(tmp) & 0xffffff;
                buff->timestamp += cto * 1000;
		  buff->size -= 4;
		  offset += 4;
	} else if (!strncmp(mTrackVector[index].format.c_str(), "audio/mp4a-latm", 15)) {
		  buff->size -= 1;
		  offset += 1;
	}
		
	buff->data = (uint8_t *)malloc(buff->size);
	mFileDataReader->readAt(offset, buff->data, buff->size);

//printf("FLVFileProbeInfo::readData OUT type %d, size %d, timeUS %lld\n", type, size, timeUS);
	updateReadPosition(mTrackVector[index].isAudio, size);
    return 0;
}

int32_t FLVFileProbeInfo::updateReadPosition(bool audio, size_t size) {
	int64_t offset = 0;
	uint8_t tagtype = 0;
	uint8_t buff[11];
	if (audio) {
		offset = mAudioStartPosition;
		while (tagtype != 0x08) {
			offset += (11 + size + 4);
			mFileDataReader->readAt(offset, buff, 11);
			tagtype = buff[0];
    			size = U32_AT(buff) & 0xffffff;
		}

		mAudioStartPosition = offset;	
	} else {
		offset = mVideoStartPosition;
		while (tagtype != 0x09) {
			offset += (11 + size + 4);
			mFileDataReader->readAt(offset, buff, 11);
			tagtype = buff[0];
    			size = U32_AT(buff) & 0xffffff;
		}

		mVideoStartPosition = offset;	
	}
	return 0;
}

BaseProber *ProbeFLV(FileDataReader *reader) {
    //Probe FLV file.
    uint8_t header[3];
    ssize_t n = reader->readAt(0, header, sizeof(header));

    if (n < (ssize_t)sizeof(header)) {
        return NULL;
    }

    if (header[0] == 'F' && header[1] == 'L' && header[2] == 'V') {
        return new FLVFileProbeInfo(reader);
    }

    return NULL;
}

