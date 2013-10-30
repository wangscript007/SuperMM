#ifndef _AVI_FILE_PROBE_INFO_H_

#define _AVI_FILE_PROBE_INFO_H_

#include "BaseProber.h"
#include "FileDataReader.h"

using namespace std; 

typedef struct {
    uint32_t dwMicroSecPerFrame; // frame display rate (or 0)
    uint32_t dwMaxBytesPerSec; // max. transfer rate
    uint32_t dwPaddingGranularity; // pad to multiples of this
    uint32_t dwFlags; // the ever-present flags
    uint32_t dwTotalFrames; // # frames in file
    uint32_t dwInitialFrames;
    uint32_t dwStreams;
    uint32_t dwSuggestedBufferSize;
    uint32_t dwWidth;
    uint32_t dwHeight;
    uint32_t dwReserved[4];
}MainAVIHeader;

typedef struct {
    uint32_t fccType;
    uint32_t fccHandler;
    uint32_t dwFlags;
    uint16_t wPriority;
    uint16_t wLanguage;
    uint32_t dwInitialFrames;
    uint32_t dwScale;
    uint32_t dwRate; /* dwRate / dwScale == samples/second */
    uint32_t dwStart;
    uint32_t dwLength;
    uint32_t dwSuggestedBufferSize;
    uint32_t dwQuality;
    uint32_t dwSampleSize;

    struct {
        uint16_t left;
        uint16_t top;
        uint16_t right;
        uint16_t bottom;
    }rcFrame;
}AVIStreamHeader;

typedef struct {
    uint32_t ckid;
    uint32_t dwFlags;
    uint32_t dwChunkOffset;
    uint32_t dwChunkLength;
}OldStyleIndex;

class AVIFileProbeInfo : public BaseProber {
public:

    AVIFileProbeInfo(FileDataReader *reader);
    ~AVIFileProbeInfo();

    void FileParse();
    void InfoDump();

    bool parseChunk(uint64_t *offset, uint32_t depth);
    void parseMainAVIHeader(uint64_t offset, ssize_t size);
    void parseAVIStreamHeader(uint64_t offset, ssize_t size);
    void parseOldStyleIndex(uint64_t offset, ssize_t size);
    void parseOpenDMLIndex(uint64_t offset, ssize_t size);
private:
    FileDataReader *mFileDataReader;
    MainAVIHeader *mMainAVIHeader;
    AVIStreamHeader *mAVIStreamHeader;
    OldStyleIndex *mOldStyleIndex;
};

BaseProber *ProbeAVI(FileDataReader *reader);


#endif
