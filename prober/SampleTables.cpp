#include <stdlib.h>
#include <stdio.h>
#include "Utils.h"
#include "SampleTables.h"

SampleTables::SampleTables(FileDataReader *reader)
    :mFileDataReader(reader),
     mCurrentSampleIndex(0),
     mCurrentChunkOffsetIndex(0),
     mCurrentSampleTimeIndex(0),
     mCurrentSampleCompTimeIndex(0),
     mCurrentSampleToChunkIndex(0),
     mStartSampleIndex_stsc(0),
     mStartSampleIndex_stts(0),
     mStartSampleIndex_ctts(0),
     mTotalSampleNum(0),
     mLastSampleTime(0),
     mNumChunkOffsetsEntry(0),
     mChunkOffsetTable(NULL),
     mDefaultSampleSize(0),
     mNumSampleSizesEntry(0),
     mSampleSizeTable(NULL),
     mNumSampleToChunkEntry(0),
     mSampleToChunkTable(NULL),
     mNumSampleTimeEntry(0),
     mSampleTimeTable(NULL),
     mNumSyncSampleEntry(0),
     mSyncSampleTable(NULL),
     mNumSampleCompTimeEntry(0),
     mSampleCompTimeTable(NULL) {

}

SampleTables::~SampleTables() {
    if (mChunkOffsetTable != NULL) {
        delete mChunkOffsetTable;
    }

    if (mSampleSizeTable != NULL) {
        delete mSampleSizeTable;
    }

    if (mSampleToChunkTable != NULL) {
        delete mSampleToChunkTable;
    }

    if (mSampleTimeTable != NULL) {
        delete mSampleTimeTable;
    }

    if (mSyncSampleTable != NULL) {
        delete mSyncSampleTable;
    }

    if (mSampleCompTimeTable != NULL) {
        delete mSampleCompTimeTable;
    }
}

int32_t SampleTables::parseSampleToChunkTable(uint64_t offset, size_t size) {
    uint8_t header[8];
    int n = mFileDataReader->readAt(offset, header, sizeof(header));

    uint32_t version = header[0];
    uint32_t flags = U32_AT(header) >> 8;

    mNumSampleToChunkEntry = U32_AT(header + 4);

//    printf("mNumSampleToChunk %d\n", mNumSampleToChunkEntry);

    mSampleToChunkTable = new SampleToChunkEntry[mNumSampleToChunkEntry];

    for (int i = 0; i < mNumSampleToChunkEntry; i++) {
        uint8_t buff[12];
        mFileDataReader->readAt(offset + 8 + i * 12, buff, 12);

        mSampleToChunkTable[i].FirstChunk = U32_AT(buff) - 1;
        mSampleToChunkTable[i].SamplesPerChunk = U32_AT(buff + 4);
        mSampleToChunkTable[i].SampleDescID = U32_AT(buff + 8);
       // printf("mSampleToChunkTable[%d].SamplesPerChunk %d\n", i, mSampleToChunkTable[i].SamplesPerChunk);
    }

    return 0;
}

int32_t SampleTables::parseSampleTimeTable(uint64_t offset, size_t size) {
    uint8_t header[8];
    int n = mFileDataReader->readAt(offset, header, sizeof(header));

    uint32_t version = header[0];
    uint32_t flags = U32_AT(header) >> 8;

    mNumSampleTimeEntry = U32_AT(header + 4);
  //  printf("mNumSampleTime %d\n", mNumSampleTimeEntry);

    mSampleTimeTable = new SampleTimeEntry[mNumSampleTimeEntry];
    
    for (int i = 0; i < mNumSampleTimeEntry; i++) {
        uint8_t buff[8];
        mFileDataReader->readAt(offset + 8 + i * 8, buff, 8);
        
        mSampleTimeTable[i].SampleCount = U32_AT(buff);
        mSampleTimeTable[i].SampleDuration = U32_AT(buff + 4);
      //  printf("mSampleTimeTable[%d].SampleCount %d\n", i, mSampleTimeTable[i].SampleCount);
      //  printf("mSampleTimeTable[%d].SampleDuration %d\n", i, mSampleTimeTable[i].SampleDuration);
    }

    return 0;
}

int32_t SampleTables::parseSyncSampleTable(uint64_t offset, size_t size) {
    uint8_t header[8];
    int n = mFileDataReader->readAt(offset, header, sizeof(header));

    uint32_t version = header[0];
    uint32_t flags = U32_AT(header) >> 8;

    mNumSyncSampleEntry = U32_AT(header + 4);

//    printf("mNumSyncSampleEntry %d\n", mNumSyncSampleEntry);

    mSyncSampleTable = new uint32_t[mNumSyncSampleEntry * 4];

    for (int i = 0; i < mNumSyncSampleEntry; i++) {
        mFileDataReader->readAt(offset + 8 + i * 4, &mSyncSampleTable[i], 4);
        
        mSyncSampleTable[i] = SWAP32(mSyncSampleTable[i]);
    }

    return 0;
}

int32_t SampleTables::parseSampleCompTimeTable(uint64_t offset, size_t size) {
    uint8_t header[8];
    int n = mFileDataReader->readAt(offset, header, sizeof(header));

    uint32_t version = header[0];
    uint32_t flags = U32_AT(header) >> 8;

    mNumSampleCompTimeEntry = U32_AT(header + 4);

//    printf("mNumSampleCompTimeEntry %d\n", mNumSampleCompTimeEntry);

    mSampleCompTimeTable = new SampleCompTimeEntry[mNumSampleCompTimeEntry];

    for (int i = 0; i < mNumSampleCompTimeEntry; i++) {
        uint8_t buff[8];
        mFileDataReader->readAt(offset + 8 + i * 8, buff, 8);

        mSampleCompTimeTable[i].SampleCount = U32_AT(buff);
        mSampleCompTimeTable[i].CompositionOffset = U32_AT(buff + 4);
    }

    return 0;
}

int32_t SampleTables::parseChunkOffsetTable(uint32_t type, uint64_t offset, size_t size) {
    uint8_t header[8];
    int n = mFileDataReader->readAt(offset, header, sizeof(header));

    uint32_t version = U32_AT(header);
    mNumChunkOffsetsEntry = U32_AT(header + 4);

//    printf("number of chunkoffset %d\n", mNumChunkOffsetsEntry);

    uint32_t fieldLen;

    if (type == FOURCC('s', 't', 'c', 'o')) {
        fieldLen = 4;
    } else if (type == FOURCC ('c', 'o', '6', '4')){
        fieldLen = 8;
    }

    mChunkOffsetTable = new uint64_t[mNumChunkOffsetsEntry * 8];

    for (int i = 0; i < mNumChunkOffsetsEntry; i++) {
        mFileDataReader->readAt(offset + 8 + i * fieldLen, &mChunkOffsetTable[i], fieldLen);

        if (fieldLen == 4) {
            mChunkOffsetTable[i] = SWAP32(mChunkOffsetTable[i]) & 0x00000000ffffffff;
//            printf("mChunkOffsetTable[%d] %d\n", i, mChunkOffsetTable[i]);
        } else if (fieldLen == 8){
            mChunkOffsetTable[i] = SWAP64(mChunkOffsetTable[i]);
        }
    }
    return 0;
}

int32_t SampleTables::parseSampleSizeTable(uint32_t type, uint64_t offset, size_t size) {
    uint8_t header[12];
    int n = mFileDataReader->readAt(offset, header, sizeof(header));

    uint32_t version = header[0];
    uint32_t flags = U32_AT(header) >> 8;

    mDefaultSampleSize = U32_AT(&header[4]);

    if (mDefaultSampleSize) {
        printf("mDefaultSampleSize %d\n", mDefaultSampleSize);
        return 0;
    } else {
        mNumSampleSizesEntry = U32_AT(&header[8]);
        mTotalSampleNum = mNumSampleSizesEntry;
    }

    printf("mNumSampleSizes :%d\n", mNumSampleSizesEntry);

    uint32_t fieldLen;

    if (type == FOURCC('s', 't', 's', 'z')) {
        fieldLen = 4;    
    } else if (type == FOURCC('s', 't', 'z', '2')) {
        fieldLen = 2;    
    }

    mSampleSizeTable = new uint32_t[mNumSampleSizesEntry * 4];

    for (int i = 0; i < mNumSampleSizesEntry; i++) {
        mFileDataReader->readAt(offset + 12 + i * fieldLen, &mSampleSizeTable[i], fieldLen);
        
        if (fieldLen == 4) {
            mSampleSizeTable[i] = SWAP32(mSampleSizeTable[i]);
//		printf("mSampleSizeTable[%d] %d\n", i, mSampleSizeTable[i]);
        } else if (fieldLen == 2) {
            mSampleSizeTable[i] = SWAP16(mSampleSizeTable[i]) & 0x0000ffff;
        }
    }

    return 0;
}

int32_t SampleTables::buildSampleInfoList() {

    return 0;
}

uint32_t SampleTables::getRequestSample(SampleInfo *&info) {
    //printf("SampleTables::getRequestSample IN\n");

    if (mCurrentSampleIndex >= mTotalSampleNum) {
        return NULL;
    }

    uint32_t location = findChunkFromSTSCTable();

    uint64_t chunkoffset = mChunkOffsetTable[mCurrentChunkOffsetIndex];
 //printf("chunkoffset %lld, %d\n", chunkoffset, chunkoffset);
    uint64_t sampleoffset = 0;
    if (location == 0) {
        sampleoffset = chunkoffset;
    } else if (location > 0) {
        sampleoffset = chunkoffset;
        for (int i = location - 1; i > 0; i--) {
            sampleoffset += mSampleSizeTable[mCurrentSampleIndex - i];
        }
    }
    //printf("sampleoffset %lld location %d %d\n", sampleoffset, location, mCurrentChunkOffsetIndex);

    uint64_t time = findTimeFromSTTSTable();

    info->offset = sampleoffset;
    info->size = mSampleSizeTable[mCurrentSampleIndex];
    info->time = time;
//	printf("time --- %lld\n", info->time);

    mCurrentSampleIndex++;
    return 0;
}

uint32_t SampleTables::findChunkFromSTSCTable() {
    uint32_t startchunk = 0;
    uint32_t endchunk = 0;
    uint32_t sampleperchunk = 0;

    if (mCurrentSampleToChunkIndex >= mNumSampleToChunkEntry) {
        return -1;
    }

    while (1) {
        if (mCurrentSampleToChunkIndex < mNumSampleToChunkEntry - 1) {
            startchunk = mSampleToChunkTable[mCurrentSampleToChunkIndex].FirstChunk;
            endchunk = mSampleToChunkTable[mCurrentSampleToChunkIndex + 1].FirstChunk;
            sampleperchunk = mSampleToChunkTable[mCurrentSampleToChunkIndex].SamplesPerChunk;

            uint32_t EndSampleIndex = mStartSampleIndex_stsc + (endchunk - startchunk) * sampleperchunk;
            if (mCurrentSampleIndex < EndSampleIndex) {
                break;
            } else {
                mStartSampleIndex_stsc += (endchunk - startchunk) * sampleperchunk;
                mNumSampleToChunkEntry++;
            }
        } else {
            //The last SampleToChunkEntry.
            startchunk = mSampleToChunkTable[mCurrentSampleToChunkIndex].FirstChunk;
            sampleperchunk = mSampleToChunkTable[mCurrentSampleToChunkIndex].SamplesPerChunk;
            
            break;
        }
    }

    mCurrentChunkOffsetIndex = startchunk + (mCurrentSampleIndex - mStartSampleIndex_stsc) / sampleperchunk;
    uint32_t location = (mCurrentSampleIndex - mStartSampleIndex_stsc) % sampleperchunk;

    return location;
}
    
uint64_t SampleTables::findTimeFromSTTSTable() {
    uint64_t time = 0;

    SampleTimeEntry sampletime = mSampleTimeTable[mCurrentSampleTimeIndex];
    if (!mCurrentSampleIndex) {
	    time = 0;
    } else {
    	time = mLastSampleTime + sampletime.SampleDuration;
    }
    //printf("mLastSampleTime %ld, time %ld, duration %ld\n", mLastSampleTime, time, sampletime.SampleDuration);
    mLastSampleTime = time;

    if (mCurrentSampleIndex == mStartSampleIndex_stts + mSampleTimeTable[mCurrentSampleTimeIndex].SampleCount) {
        mStartSampleIndex_stts += mSampleTimeTable[mCurrentSampleTimeIndex].SampleCount;
        mCurrentSampleTimeIndex++;
    }

    //Check ctts is valid ?
/*    if (mSampleCompTimeTable != NULL) {
        SampleCompTimeEntry samplecomptime = mSampleCompTimeTable[mCurrentSampleCompTimeIndex];
        time += samplecomptime.CompositionOffset;

        if (mCurrentSampleIndex = mStartSampleIndex_ctts + mSampleCompTimeTable[mCurrentSampleCompTimeIndex].SampleCount) {
            mStartSampleIndex_ctts += mSampleCompTimeTable[mCurrentSampleCompTimeIndex].SampleCount;
            mCurrentSampleCompTimeIndex++;
        }
    }
*/
//    printf("mLastSampleTime %ld, time %ld, duration %ld\n", mLastSampleTime, time, sampletime.SampleDuration);

    return time;
}
