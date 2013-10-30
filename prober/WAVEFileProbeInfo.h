#ifndef _WAVE_FILE_PROBE_INFO_H_

#define _WAVE_FILE_PROBE_INFO_H_

#include "BaseProber.h"
#include "FileDataReader.h"

using namespace std; 

class WAVEFileProbeInfo : public BaseProber {
public:

    WAVEFileProbeInfo(FileDataReader *reader);
    ~WAVEFileProbeInfo();

    void FileParse();
    void InfoDump();
};

BaseProber *ProbeWAVE(FileDataReader *reader);


#endif
