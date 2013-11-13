#ifndef _TRACK_INFO_H_
#define _TRACK_INFO_H_

#include <string>
#include <list>
#include "DataBuffer.h"

using namespace std;

struct TrackInfo {
    int TrackFlag;
    int TrackIndex;
    int StreamNum;
    string format;
    bool isAudio;
    uint64_t Duration;
    uint32_t TimeScale;
    uint32_t Width;
    uint32_t Height;

    void *PrivData;
};

#endif
