#ifndef _SAMPLE_TABLES_H_

#define _SAMPLE_TABLES_H_

#include <list>
#include "FileDataReader.h"

using namespace std;

typedef struct {
    uint32_t FirstChunk;
    uint32_t SamplesPerChunk;
    uint32_t SampleDescID;
}SampleToChunkEntry;

typedef struct {
    uint32_t SampleCount;
    uint32_t SampleDuration;
}SampleTimeEntry;

typedef struct {
    uint32_t SampleCount;
    uint32_t CompositionOffset ;
}SampleCompTimeEntry;

typedef struct {
    uint64_t offset;
    size_t size;
    uint64_t time;
}SampleInfo;

class SampleTables {
public:
    SampleTables(FileDataReader *reader);
    ~SampleTables();

    int32_t parseSampleToChunkTable(uint64_t offset, size_t size);
    int32_t parseSampleTimeTable(uint64_t offset, size_t size);
    int32_t parseSyncSampleTable(uint64_t offset, size_t size);
    int32_t parseSampleCompTimeTable(uint64_t offset, size_t size);
    int32_t parseChunkOffsetTable(uint32_t type, uint64_t offset, size_t size);
    int32_t parseSampleSizeTable(uint32_t type, uint64_t offset, size_t size);

    int32_t buildSampleInfoList();

    uint32_t getRequestSample(SampleInfo *&info);
    uint32_t findChunkFromSTSCTable();
    uint64_t findTimeFromSTTSTable();
private:
    FileDataReader *mFileDataReader;

    list<SampleInfo> mSampleInfoList;
    uint32_t mCurrentSampleIndex;
    uint32_t mCurrentChunkOffsetIndex;
    uint32_t mCurrentSampleTimeIndex;
    uint32_t mCurrentSampleCompTimeIndex;
    uint32_t mCurrentSampleToChunkIndex;
    uint32_t mStartSampleIndex_stsc;
    uint32_t mStartSampleIndex_stts;
    uint32_t mStartSampleIndex_ctts;
    uint32_t mTotalSampleNum;
    uint64_t mLastSampleTime;

    //stco
    uint32_t mNumChunkOffsetsEntry;
    uint64_t *mChunkOffsetTable;
    
    //stsz
    uint32_t mDefaultSampleSize;
    uint32_t mNumSampleSizesEntry;
    uint32_t *mSampleSizeTable;

    //stsc
    uint32_t mNumSampleToChunkEntry;
    SampleToChunkEntry *mSampleToChunkTable;

    //stts
    uint32_t mNumSampleTimeEntry;
    SampleTimeEntry *mSampleTimeTable;

    //stss
    uint32_t mNumSyncSampleEntry;
    uint32_t *mSyncSampleTable;

    //ctts
    uint32_t mNumSampleCompTimeEntry;
    SampleCompTimeEntry *mSampleCompTimeTable;
};

#endif
