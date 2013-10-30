
#ifndef _MP3_FILE_PROBE_INFO_H_

#define _MP3_FILE_PROBE_INFO_H_

#include "BaseProber.h"
#include "FileDataReader.h"

using namespace std; 

class MP3FileProbeInfo : public BaseProber {
public:

    MP3FileProbeInfo(FileDataReader *reader);
    ~MP3FileProbeInfo();

    void FileParse();
    void InfoDump();

    ssize_t parseID3V2(uint8_t *data);
    ssize_t parseID3V1();
    ssize_t parseXINGHeader(uint64_t offset);
    uint64_t checkXINGHeaderPos(uint32_t);

private:
    FileDataReader *mFileDataReader;
    uint64_t mFirstFramePos;
};


BaseProber *ProbeMP3(FileDataReader *reader);


#endif
