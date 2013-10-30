#ifndef _APE_FILE_PROBE_INFO_H_

#define _APE_FILE_PROBE_INFO_H_

#include "BaseProber.h"
#include "FileDataReader.h"

using namespace std; 

class APEFileProbeInfo : public BaseProber {
public:

    APEFileProbeInfo(FileDataReader *reader);
    ~APEFileProbeInfo();

    void FileParse();
    void InfoDump();
};

BaseProber *ProbeAPE(FileDataReader *reader);


#endif
