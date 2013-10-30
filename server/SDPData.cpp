#include <string.h>
#include <stdio.h>
#include "Utils.h"
#include "SDPData.h"
#include "PayloadType.h"

SDPData::SDPData(BaseProber *prober, string url)
    :mBaseProber(prober){
    //set v=0
    mSDPData.append("v=0");
    mSDPData.append("\r\n");

    //set o=
    mSDPData.append("o=SuperMM");
    mSDPData.append("\r\n");

    //set s=
    mSDPData.append("s=");
    int32_t pathpos = url.rfind("/", url.size());
    string path(url.c_str() + pathpos);
    mSDPData.append(path);
    mSDPData.append("\r\n");

    //set u=
    mSDPData.append("u=http:///");  
    mSDPData.append("\r\n");

    //set e=
    mSDPData.append("e=admin@");
    mSDPData.append("\r\n");

    //set c=
    mSDPData.append("c=IN IP4 0.0.0.0");
    mSDPData.append("\r\n");

    //set b=
    mSDPData.append("b=AS:96");
    mSDPData.append("\r\n");

    //set a=range
    int64_t duration = prober->getDuration();
    mSDPData.append("a=range:npt=0-  ");
    char buff[100];
    mSDPData.append(dtos((double)duration, buff));
    mSDPData.append("\r\n");

	makeVideoAttribute();
	makeAudioAttribute();
}

int SDPData::makeVideoAttribute() {
    ////////video
    int pt = -1;
    int trackIndex;
    string str;
    char buff[100];

    for (int i = 0; i < mBaseProber->getTrackCounts(); i++) {
        TrackInfo *info = mBaseProber->getTrackInfo(i);
        if (!strncmp(info->format.c_str(), "video", 5)) {
            trackIndex = info->TrackIndex;
            pt = getPayloadInfo(info->format, trackIndex);
            break;
        }
    }
    memset(buff, 0, sizeof(buff));
    sprintf(buff, "m=video 0 RTP/AVP %d\r\n", pt);
    mSDPData.append(buff);
    mSDPData.append("b=AS:76\r\n");

    memset(buff, 0, sizeof(buff));
    sprintf(buff, "a=control:trackID=%d\r\n", trackIndex);
    mSDPData.append(buff);

	//video config info.
	
    VideoConfig *Vconfig = NULL;
    DataBuffer *vbuf = mBaseProber->getVideoConfigBuffer();
    TrackInfo *info = mBaseProber->getTrackInfo(trackIndex);
    if (!strncmp(info->format.c_str(), "video/avc", 15)) {
        AVCVideoConfig *videoconfig = new AVCVideoConfig();

        mVideoConfigInfo = videoconfig->parseVideoConfig(vbuf->data, vbuf->size);
        mSDPData.append(makeMediaAttribute(trackIndex, pt));
        delete videoconfig;
    } else {
        mSDPData.append(makeMediaAttribute(trackIndex, pt));
    }

    int height, width;
    mBaseProber->getVideoFrameSize(&height, &width);
    memset(buff, 0, sizeof(buff));
    sprintf(buff, "a=framesize:%d %d-%d\r\n", pt, height, width);
    mSDPData.append(buff);

	return 0;
}

int SDPData::makeAudioAttribute() {
    ////////audio
    int pt = -1;
    int trackIndex;
    string str;
    char buff[100];

    for (int i = 0; i < mBaseProber->getTrackCounts(); i++) {
        TrackInfo *info = mBaseProber->getTrackInfo(i);
        if (!strncmp(info->format.c_str(), "audio", 5)) {
            trackIndex = info->TrackIndex;
            pt = getPayloadInfo(info->format, trackIndex);
            break;
        }
    }

    memset(buff, 0, sizeof(buff));
    sprintf(buff, "m=audio 0 RTP/AVP %d\r\n", pt);
    mSDPData.append(buff);
    mSDPData.append("b=AS:76\r\n");

    memset(buff, 0, sizeof(buff));
    sprintf(buff, "a=control:trackID=%d\r\n", trackIndex);
    mSDPData.append(buff);
	
	//audio config info.
    AudioConfig *Aconfig = NULL;
    DataBuffer *abuf = mBaseProber->getAudioConfigBuffer();

    TrackInfo *info = mBaseProber->getTrackInfo(trackIndex);
    if (!strncmp(info->format.c_str(), "audio/mp4a-latm", 15)) {
        MPEG4AudioConfig *audioconfig = new MPEG4AudioConfig();

        mAudioConfigInfo = audioconfig->parseAudioConfig(abuf->data, abuf->size, 1);
        mSDPData.append(makeMediaAttribute(trackIndex, pt));
        delete audioconfig;
    } else {
        mSDPData.append(makeMediaAttribute(trackIndex, pt));
    }

	return 0;
}

string SDPData::getSDPData() {
    return mSDPData;
}

int SDPData::getPayloadInfo(string format, int index) {
    int pt = -1;
    size_t len = strlen(format.c_str());

    for (int i = 0; i < sizeof(PayloadTypeArray)/sizeof(PayloadType); i++) {
        if (!strncmp(PayloadTypeArray[i].codecmime, format.c_str(), len)) {
            pt = PayloadTypeArray[i].pltype;
            return pt;
        }
    }

    //Not found the match payload info.
    pt = 96 + index;
    return pt;
}

string SDPData::makeMediaAttribute(int index, int pt) {
    char buff[500];

    TrackInfo *info = mBaseProber->getTrackInfo(index);
    if (!strncmp(info->format.c_str(), "video/avc", 9)) {
        sprintf(buff, "a=rtpmap:%d H264/90000\r\na=fmtp:%d profile-level-id=%s;sprop-parameter-sets=%s\r\n",
			pt, pt, mVideoConfigInfo->ProfileLevelID->c_str(), mVideoConfigInfo->SpropParameterSets->c_str());
    } else if(!strncmp(info->format.c_str(), "audio/mp4a-latm", 9)) {
        sprintf(buff, "a=rtpmap:%d mpeg4-generic/8000/2\r\na=fmtp:%d profile-level-id=%d;mode=AAC-hbr;sizelength=13;indexlength=3;indexdeltalength=3;config=%s\r\n", 
			pt, pt, mAudioConfigInfo->ProfileLevel, mAudioConfigInfo->ConfigStr);
//        sprintf(buff, "a=rtpmap:%d MP4A-LATM/8000/2\r\na=fmtp:%d profile-level-id=%d;cpresent=0;config=%s\r\n", 
//			pt, pt, mAudioConfigInfo->ProfileLevel, mAudioConfigInfo->ConfigStr);
    }

    int size = strlen(buff);
    string str(buff, size);
    return str;
}
