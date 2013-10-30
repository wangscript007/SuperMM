#ifndef _MP4_FILE_PROBE_INFO_H_

#define _MP4_FILE_PROBE_INFO_H_

#include <vector>
#include "BaseProber.h"
#include "FileDataReader.h"
#include "SampleTables.h"

using namespace std; 

class MP4FileProbeInfo : public BaseProber {
public:

    MP4FileProbeInfo(FileDataReader *reader);
    ~MP4FileProbeInfo();

    virtual void FileParse();
    virtual void InfoDump();

    bool parseChunk(uint64_t *offset, uint32_t depth);
    int32_t readData(int index, DataBuffer *&buff);

    int64_t getDuration();
    int32_t getTrackCounts();
    int32_t getVideoFrameSize(int *h, int *w);
    TrackInfo *getTrackInfo(int index);
	DataBuffer *getVideoConfigBuffer();
	DataBuffer *getAudioConfigBuffer();

private:
    FileDataReader *mFileDataReader;

    vector<TrackInfo> mTrackVector;
    TrackInfo *mCurrTrackInfo;
    SampleTables *mCurrSampleTables;

	DataBuffer *mVideoConfigBuffer;
	DataBuffer *mAudioConfigBuffer;

    int mTrackIndex;
};


BaseProber *ProbeMP4(FileDataReader *reader);


#endif
