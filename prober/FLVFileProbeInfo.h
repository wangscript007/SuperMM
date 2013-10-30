#ifndef _FLV_FILE_PROBE_INFO_H_

#define _FLV_FILE_PROBE_INFO_H_

#include <map>
#include <string>
#include <list>
#include <vector>
#include "BaseProber.h"
#include "FileDataReader.h"

using namespace std; 

typedef struct {
        uint32_t keyFrameTimeNum;
        int64_t *keyFrameTimes;

        uint32_t keyFramePositionNum;
        int64_t *keyFramePositions;
} keyFrame;
	
class FLVFileProbeInfo : public BaseProber {
public:
	
    FLVFileProbeInfo(FileDataReader *reader);
    ~FLVFileProbeInfo();
    bool HeaderDecode(uint8_t *data);
    ssize_t TagDecode(uint64_t offset);

    void parseAudioFrameFirstByte(uint64_t offset);
    void parseVideoFrameFirstByte(uint64_t offset);
    void parseScriptData(uint64_t offset, ssize_t size);
    void parseAMFObject(uint8_t *data, uint64_t *pos, ssize_t size);
    bool findElement(string str, int8_t * &value);

    int64_t getDuration();
    int32_t getTrackCounts();
    int32_t getVideoFrameSize(int *h, int *w);
    int32_t readData(int index, DataBuffer *&buff);
    int32_t updateReadPosition(bool audio, size_t size);
    int32_t setSeekTo(int64_t seekTime);
    TrackInfo *getTrackInfo(int index);
	DataBuffer *getVideoConfigBuffer();
	DataBuffer *getAudioConfigBuffer();

    void FileParse();
    void InfoDump();
private:
    bool mHasAudio;
    bool mHasVideo;
    uint8_t mVersion;

    int mTrackIndex;
    FileDataReader *mFileDataReader;

    map<string, int8_t*> mMixedArray;
    pair<map<string, int8_t*>::iterator, bool> mInsertPos;

    vector<TrackInfo> mTrackVector;

	DataBuffer *mVideoConfigBuffer;
	DataBuffer *mAudioConfigBuffer;

	int64_t mVideoStartPosition;
	int64_t mAudioStartPosition;
	bool mFoundScriptData;
	bool mFoundFilePositions;
	bool mFoundTimes;

	keyFrame *mKeyFrames;

};


BaseProber *ProbeFLV(FileDataReader *reader);


#endif
