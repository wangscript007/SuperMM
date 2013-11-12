#ifndef _RMVB_FILE_PROBE_INFO_H_

#define _RMVB_FILE_PROBE_INFO_H_

#include <vector>
#include <string>
#include "BaseProber.h"
#include "FileDataReader.h"

using namespace std; 

class RMVBFileProbeInfo : public BaseProber {
public:

    RMVBFileProbeInfo(FileDataReader *reader);
    ~RMVBFileProbeInfo();

    void FileParse();
    int32_t parseChunks();
    int32_t  parsePROP(uint64_t offset, size_t size);
    int32_t  parseMDPR(uint64_t offset, size_t size);
    int32_t  parseDATA(uint64_t offset);
    int32_t  parseINDX(uint64_t offset, size_t size);
    int32_t  parseCONT(uint64_t offset);
    void InfoDump();
    int32_t readData(int index, DataBuffer *&buff);
private:
    FileDataReader *mFileDataReader;

    vector<TrackInfo> mTrackVector;

    int mTrackIndex;
    uint64_t mDataStartOffset;
};

BaseProber *ProbeRMVB(FileDataReader *reader);


#endif
