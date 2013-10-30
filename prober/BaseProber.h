#ifndef _BASE_PROBER_H_

#define _BASE_PROBER_H_ 
#include "FileDataReader.h"
#include "DataBuffer.h"
#include "TrackInfo.h"

using namespace std;

class BaseProber {
public:

    BaseProber();
    ~BaseProber();

    virtual void FileParse() = 0;
    virtual void InfoDump() = 0;
    virtual int64_t getDuration(){return -1;};
    virtual int32_t getTrackCounts(){return -1;};
    virtual int32_t getVideoFrameSize(int *h, int *w){return -1;};
    virtual int32_t readData(int index, DataBuffer *&buff){return -1;};
    virtual int32_t setSeekTo(int64_t seekTime){return -1;};
    virtual TrackInfo *getTrackInfo(int index) {return NULL;}
	virtual DataBuffer *getVideoConfigBuffer() {return NULL;}
	virtual DataBuffer *getAudioConfigBuffer() {return NULL;}

};


#endif
