#ifndef _MKV_FILE_PROBE_INFO_H_

#define _MKV_FILE_PROBE_INFO_H_

#include "BaseProber.h"
#include "FileDataReader.h"

using namespace std; 

#define EBML_HEADER_ID             0x1A45DFA3
#define MATROSKA_SEGMENT_ID        0x18538067

//SEEKHEAD
#define MATROSKA_SEEKHEAD_ID       0x114D9B74
#define MATROSKA_SEEKENTRY_ID      0x4DBB
#define MATROSKA_SEEKID_ID         0x53AB
#define MATROSKA_SEEKPOSITION_ID   0x53AC

//CLUSTERS
#define MATROSKA_CLUSTER_ID        0x1F43B675
#define MATROSKA_SIMPLEBLOCK_ID    0xA3

//CUES
#define MATROSKA_CUES_ID           0x1C53BB6B
#define MATROSKA_POINTENTRY_ID     0xBB
#define MATROSKA_CUETIME_ID        0xB3
#define MATROSKA_CUETRACKPOSITION_ID 0xB7
#define MATROSKA_CUETRACK_ID       0xF7
#define MATROSKA_CUECLUSTERPOSITION_ID 0xF1
#define MATROSKA_CUEBLOCKNUMBER_ID 0x5378

//TRACKS
#define MATROSKA_TRACKS_ID         0x1654AE6B
#define MATROSKA_TRACKENTRY_ID     0xAE

typedef struct {
    uint64_t ID;
    const char * name;
}EleInfo;

static EleInfo EleInfoArray[] = {{EBML_HEADER_ID, "EBML Header"},
                          {MATROSKA_SEGMENT_ID, "Matroska Segment"},
                          //SEEKHEAK
                          {MATROSKA_SEEKHEAD_ID, "Matroska SeekHead"},
                          {MATROSKA_SEEKENTRY_ID, "Matroska SeekEntry"},
                          {MATROSKA_SEEKID_ID, "Matroska SeekID"},
                          {MATROSKA_SEEKPOSITION_ID, "Matroska SeekPosition"},
                          //CLUSTER
                          {MATROSKA_CLUSTER_ID, "Matroska Cluster"},
                          {MATROSKA_SIMPLEBLOCK_ID, "Matroska SimpleBlock"},
                          //CUE
                          {MATROSKA_CUES_ID, "Matroska Cues"},
                          {MATROSKA_POINTENTRY_ID, "Matroska PointEntry"},
                          //TRACK
                          {MATROSKA_TRACKS_ID, "Matroska Track"},
                          {MATROSKA_TRACKENTRY_ID, "Matroska TrackEntry"}};

class MKVFileProbeInfo : public BaseProber {
public:

    MKVFileProbeInfo(FileDataReader *reader);
    ~MKVFileProbeInfo();

    void FileParse();
    void InfoDump();

    void parseElements(uint64_t pos, uint64_t max_size, uint32_t depth);
    void parseSimpleBlock(uint64_t offset, uint64_t size);
    void parseTrackEntry(uint64_t offset, uint64_t size);
    void readData_uint(uint64_t offset, uint64_t size, uint64_t *data);
private:
    FileDataReader *mFileDataReader;
};

BaseProber *ProbeMKV(FileDataReader *reader);


#endif
