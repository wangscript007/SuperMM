#ifndef _APE_FILE_PROBE_INFO_H_

#define _APE_FILE_PROBE_INFO_H_

#include "BaseProber.h"
#include "FileDataReader.h"

using namespace std; 

typedef struct {
    int64_t pos;
    int32_t nblocks;
    size_t size;
    int32_t skip;
    int64_t pts;
} ApeFrame;

typedef struct {
    //Descriptor data.
    uint16_t version;
    uint32_t descriptorlength;
    uint32_t headerlength;
    uint32_t seektablelength;
    uint32_t wavheaderlength;
    //Header data.
    uint32_t compressiontype;
    uint16_t formatflags;
    uint32_t blocksperframe;
    uint32_t finalframeblocks;
    uint32_t totalframes;
    uint16_t bitspersample;
    uint16_t channels;
    uint32_t samplerate;
    uint64_t durationUS;
} ApeHeaderData;

class APEFileProbeInfo : public BaseProber {
public:

    APEFileProbeInfo(FileDataReader *reader);
    ~APEFileProbeInfo();

    void FileParse();
    void InfoDump();
private:
    FileDataReader *mFileDataReader;
    ApeHeaderData *mApeHeaderData;

    uint32_t *mSeekTable;
    ApeFrame *mAPEFrames;
};

BaseProber *ProbeAPE(FileDataReader *reader);


#endif
