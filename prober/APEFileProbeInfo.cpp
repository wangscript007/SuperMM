#include <stdio.h>
#include <stdlib.h>

#include "Utils.h"
#include "APEFileProbeInfo.h"

#define APE_FLAG_8_BIT                 1
#define APE_FLAG_CRC                   2
#define APE_FLAG_HAS_PEAK_LEVEL        4
#define APE_FLAG_24_BIT                8
#define APE_FLAG_HAS_SEEK_ELEMENTS    16
#define APE_FLAG_CREATE_WAV_HEADER    32

#define APE_MAX_TAGS_SIZE 200

APEFileProbeInfo::APEFileProbeInfo(FileDataReader *reader)
    :mFileDataReader(reader),
     mApeHeaderData(NULL),
     mSeekTable(NULL),
     mAPEFrames(NULL) {

}

APEFileProbeInfo::~APEFileProbeInfo() {
    if (mApeHeaderData != NULL) {
        free(mApeHeaderData);
        mApeHeaderData = NULL;
    }

    if (mSeekTable != NULL) {
        free(mSeekTable);
        mSeekTable = NULL;
    }

    if (mAPEFrames != NULL) {
        free(mAPEFrames);
        mAPEFrames = NULL;
    }
}

void APEFileProbeInfo::FileParse() {
    printf("Parse APE file\n");
    uint64_t data_offset = 0;
    uint8_t buff[2];

    mApeHeaderData = (ApeHeaderData *)malloc(sizeof(ApeHeaderData));

    size_t n = mFileDataReader->readAt(data_offset + 4, buff, sizeof(buff));
    mApeHeaderData->version = U16LE_AT(buff);

    if (mApeHeaderData->version >= 3980) {
        uint8_t descriptor[46];

        //Get descriptor data.
        if (mFileDataReader->readAt(data_offset + 6, descriptor, sizeof(descriptor)) < sizeof(descriptor)) {
            return;
        }

        mApeHeaderData->descriptorlength = U32LE_AT(&descriptor[2]);
        mApeHeaderData->headerlength     = U32LE_AT(&descriptor[6]);
        mApeHeaderData->seektablelength  = U32LE_AT(&descriptor[10]);
        mApeHeaderData->wavheaderlength  = U32LE_AT(&descriptor[14]);

        uint8_t header[mApeHeaderData->headerlength];
        data_offset += mApeHeaderData->descriptorlength;

        //Get header data.
        if (mFileDataReader->readAt(data_offset, header, sizeof(header)) < sizeof(header)) {
            return;
        }

        mApeHeaderData->compressiontype  = U16LE_AT(&header[0]);
        mApeHeaderData->formatflags      = U16LE_AT(&header[2]);
        mApeHeaderData->blocksperframe   = U32LE_AT(&header[4]);
        mApeHeaderData->finalframeblocks = U32LE_AT(&header[8]);
        mApeHeaderData->totalframes      = U32LE_AT(&header[12]);
        mApeHeaderData->bitspersample    = U16LE_AT(&header[16]);
        mApeHeaderData->channels         = U16LE_AT(&header[18]);
        mApeHeaderData->samplerate       = U32LE_AT(&header[20]);

        data_offset += mApeHeaderData->headerlength;
    } else {
        uint8_t header[34];

        mApeHeaderData->descriptorlength = 0;
        mApeHeaderData->headerlength = 32;

        data_offset += 6;
        if (mFileDataReader->readAt(data_offset, header, sizeof(header)) < sizeof(header)) {
            return;
        }

        mApeHeaderData->compressiontype  = U16LE_AT(&header[0]);
        mApeHeaderData->formatflags      = U16LE_AT(&header[2]);
        mApeHeaderData->channels         = U16LE_AT(&header[4]);
        mApeHeaderData->samplerate       = U32LE_AT(&header[6]);
        mApeHeaderData->wavheaderlength  = U32LE_AT(&header[10]);
        mApeHeaderData->totalframes      = U32LE_AT(&header[18]);
        mApeHeaderData->finalframeblocks = U32LE_AT(&header[22]);

        if (mApeHeaderData->formatflags & APE_FLAG_HAS_PEAK_LEVEL) {
            mApeHeaderData->headerlength += 4;
        }

        if (mApeHeaderData->formatflags & APE_FLAG_HAS_SEEK_ELEMENTS) {
            if (mApeHeaderData->formatflags & APE_FLAG_HAS_PEAK_LEVEL) {
                mApeHeaderData->seektablelength = U32LE_AT(&header[30]);
            } else {
                mApeHeaderData->seektablelength = U32LE_AT(&header[26]);
            }
            mApeHeaderData->headerlength += 4;
            mApeHeaderData->seektablelength *= sizeof(int32_t);
        } else {
            mApeHeaderData->seektablelength = mApeHeaderData->totalframes * sizeof(int32_t);
        }

        if (mApeHeaderData->formatflags & APE_FLAG_8_BIT) {
            mApeHeaderData->bitspersample = 8;
        } else if (mApeHeaderData->formatflags & APE_FLAG_24_BIT) {
            mApeHeaderData->bitspersample = 24;
        } else {
            mApeHeaderData->bitspersample = 16;
        }

        if (mApeHeaderData->version >= 3950) {
            mApeHeaderData->blocksperframe = 73728 * 4;
        } else if (mApeHeaderData->version >= 3900 || (mApeHeaderData->version >= 3800
                        && mApeHeaderData->compressiontype >= 4000)) {
            mApeHeaderData->blocksperframe = 73728;
        } else {
            mApeHeaderData->blocksperframe = 9216;
        }

        data_offset += mApeHeaderData->headerlength;

        if (mApeHeaderData->formatflags & APE_FLAG_CREATE_WAV_HEADER) {
            data_offset += mApeHeaderData->wavheaderlength;
        }
    }

    mAPEFrames = (ApeFrame *)malloc(mApeHeaderData->totalframes * sizeof(ApeFrame));

    //Seek table, from which can get every frame start position,
    //and can calculate the frame size and skip values.
    if (mApeHeaderData->seektablelength > 0) {
        mSeekTable = (uint32_t *)malloc(mApeHeaderData->seektablelength);
        for (uint32_t i = 0; i < mApeHeaderData->seektablelength/sizeof(uint32_t); i++) {
            if (mFileDataReader->readAt(data_offset + i * 4, &mSeekTable[i],
                                            sizeof(uint32_t)) < sizeof(uint32_t)) {
                return;
            }
        }
    }
    mAPEFrames[0].pos = mApeHeaderData->descriptorlength
                        + mApeHeaderData->headerlength
                        + mApeHeaderData->seektablelength
                        + mApeHeaderData->wavheaderlength;
    mAPEFrames[0].nblocks = mApeHeaderData->blocksperframe;
    mAPEFrames[0].skip = 0;
    mAPEFrames[0].pts = 0;

    for (uint32_t i = 1; i < mApeHeaderData->totalframes; i++) {
        mAPEFrames[i].pos = mSeekTable[i];
        mAPEFrames[i].nblocks = mApeHeaderData->blocksperframe;
        mAPEFrames[i-1].size = mAPEFrames[i].pos - mAPEFrames[i-1].pos;
        mAPEFrames[i].skip = (mAPEFrames[i].pos - mAPEFrames[0].pos) & 3;
        mAPEFrames[i].pts = mAPEFrames[i-1].pts + mApeHeaderData->blocksperframe/4608;
    }

    //The frame size and block num of the last frame.
    mAPEFrames[mApeHeaderData->totalframes - 1].size = mApeHeaderData->finalframeblocks * 8;
    mAPEFrames[mApeHeaderData->totalframes - 1].nblocks = mApeHeaderData->finalframeblocks;

    //If the skip value is not zero, need to adjust the frame position and frame size.
    for (uint32_t i = 0; i < mApeHeaderData->totalframes; i++) {
        if (mAPEFrames[i].skip) {
            mAPEFrames[i].pos -= mAPEFrames[i].skip;
            mAPEFrames[i].size += mAPEFrames[i].skip;
        }
        mAPEFrames[i].size = (mAPEFrames[i].size + 3) & ~3;
    }
}

void APEFileProbeInfo::InfoDump() {

}

BaseProber *ProbeAPE(FileDataReader *reader) {
    //Probe APE file.
    printf("Probe APE file\n");
    uint8_t header[4];
    ssize_t n = reader->readAt(0, header, sizeof(header));

    if (n < (ssize_t)sizeof(header)) {
        return NULL;
    }

    if (header[0] == 'M' && header[1] == 'A' && header[2] == 'C' && header[3] == ' ') {
        return new APEFileProbeInfo(reader);
    }

    return NULL;
}



