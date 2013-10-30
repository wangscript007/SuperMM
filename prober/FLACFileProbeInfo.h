#ifndef _FLAC_FILE_PROBE_INFO_H_

#define _FLAC_FILE_PROBE_INFO_H_

#include "BaseProber.h"
#include "FileDataReader.h"

using namespace std; 

class FLACFileProbeInfo : public BaseProber {
public:

    FLACFileProbeInfo(FileDataReader *reader);
    ~FLACFileProbeInfo();

    void FileParse();
    void InfoDump();
private:
    FileDataReader *mFileDataReader;
};

BaseProber *ProbeFLAC(FileDataReader *reader);


#endif
