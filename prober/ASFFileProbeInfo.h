#ifndef _ASF_FILE_PROBE_INFO_H_

#define _ASF_FILE_PROBE_INFO_H_

#include "BaseProber.h"
#include "FileDataReader.h"

using namespace std; 

typedef uint8_t AsfGuid[16];

#define OBJECT_ID_LEN       16
#define OBJECT_SIZE_LEN     8
#define OBJECT_HEAD_LEN     24

class ASFFileProbeInfo : public BaseProber {
public:

    ASFFileProbeInfo(FileDataReader *reader);
    ~ASFFileProbeInfo();

    void FileParse();
    void InfoDump();

    void parseHeaderObject(uint64_t offset, uint64_t size);
    void parseDataObject(uint64_t offset, uint64_t size);
    void parseSimpleIndexObject(uint64_t offset, uint64_t size);
private:
    FileDataReader *mFileDataReader;
};

BaseProber *ProbeASF(FileDataReader *reader);


#endif
