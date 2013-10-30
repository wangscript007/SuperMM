#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "MP4FileProbeInfo.h"
#include "Utils.h"

MP4FileProbeInfo::MP4FileProbeInfo(FileDataReader *reader):
    mFileDataReader(reader),
    mTrackIndex(-1),
    mCurrTrackInfo(NULL),
    mCurrSampleTables(NULL){

}

MP4FileProbeInfo::~MP4FileProbeInfo() {
    
}

void MP4FileProbeInfo::InfoDump() {

}

void MP4FileProbeInfo::FileParse() {
    printf("Parse MP4 file\n");
    uint64_t offset = 0;
    uint32_t depth = 0;
    bool parsedone = false;

    while (!parsedone) {
        parsedone = parseChunk(&offset, depth);
    }
}

int64_t MP4FileProbeInfo::getDuration() {
    return mTrackVector[0].Duration;
}

int32_t MP4FileProbeInfo::getTrackCounts() {
    return mTrackVector.size();
}

TrackInfo *MP4FileProbeInfo::getTrackInfo(int index) {
    return &mTrackVector[index];
}

int32_t MP4FileProbeInfo::getVideoFrameSize(int *h, int *w) {
    uint32_t num = mTrackVector.size();

    for (int i = 0; i < num; i++) {
        if (!strncmp(mTrackVector[i].format.c_str(), "video", 5)) {
            *h = mTrackVector[i].Height;
            *w = mTrackVector[i].Width;
            break;
        }
    }

    return 0;
}

bool MP4FileProbeInfo::parseChunk(uint64_t *offset, uint32_t depth) {
    uint32_t size;
    uint32_t type;

    uint8_t header[8];

    int n = mFileDataReader->readAt(*offset, header, sizeof(header));
    if (n == 0) {
        //Parse is over.
        printf("Parse is over !\n");
        return true;
    }

    //uint64_t end_offset = *offset + size;
    size = U32_AT(header);
    type = U32_AT(&header[4]);
        
#if 1
    char space[40] = "--------------------------------";

    sprintf(space + depth * 4, "%s", MakeFourCCString(type));
    printf("%s %d\n", space, size);

#endif

    switch (type) {
        case FOURCC('f', 't', 'y', 'p'):
            *offset += size;
            break;
        case FOURCC('m', 'o', 'o', 'v'):
        {
            uint64_t end_offset;
            end_offset = *offset + size;
            *offset += 8;
            while(*offset < end_offset) {
                parseChunk(offset, depth + 1);
            }
            break;
        }
        case FOURCC('t', 'r', 'a', 'k'):
        {
            mCurrTrackInfo = new TrackInfo();

            mTrackIndex++;
            mCurrTrackInfo->TrackIndex = mTrackIndex;

            uint64_t end_offset;
            end_offset = *offset + size;
            *offset += 8;
            while(*offset < end_offset) {
                parseChunk(offset, depth + 1);
            }
            mTrackVector.push_back(*mCurrTrackInfo);
            break;
        }
        case FOURCC('m', 'v', 'h', 'd'):
            *offset += size;
            break;
        case FOURCC('t', 'k', 'h', 'd'):
            *offset += size;
            break;
        case FOURCC('e', 'd', 't', 's'):
        {
            uint64_t end_offset;
            end_offset = *offset + size;
            *offset += 8;
            while(*offset < end_offset) {
                parseChunk(offset, depth + 1);
            }
            break;
        }
        case FOURCC('m', 'd', 'i', 'a'):
        {
            uint64_t end_offset;
            end_offset = *offset + size;
            *offset += 8;
            while(*offset < end_offset) {
                parseChunk(offset, depth + 1);
            }
            break;
        }
        case FOURCC('m', 'd', 'h', 'd'):
        {
            uint8_t version;
            mFileDataReader->readAt(*offset + 8, &version, 1);

            printf("version %d\n", version);
            uint32_t timescale = 0;
            uint32_t duration = 0;

            mFileDataReader->readAt(*offset + 8 + 12, &timescale, 4);
            mFileDataReader->readAt(*offset + 8 + 16, &duration, 4);

            timescale = SWAP32(timescale);
            duration = SWAP32(duration);
            mCurrTrackInfo->Duration = duration / timescale;
            mCurrTrackInfo->TimeScale = timescale;
            printf("duration %lld timescale %d \n", mCurrTrackInfo->Duration, timescale);

            *offset += size;
            break;
        }
        case FOURCC('m', 'i', 'n', 'f'):
        {
            uint64_t end_offset;
            end_offset = *offset + size;
            *offset += 8;
            while(*offset < end_offset) {
                parseChunk(offset, depth + 1);
            }
            break;
        }
        case FOURCC('s', 't', 'b', 'l'):
        {
            mCurrSampleTables = NULL;
            mCurrSampleTables = new SampleTables(mFileDataReader);

            uint64_t end_offset;
            end_offset = *offset + size;
            *offset += 8;
            while(*offset < end_offset) {
                parseChunk(offset, depth + 1);
            }

            mCurrTrackInfo->PrivData = (void *)mCurrSampleTables;
            break;
        }
        case FOURCC('s', 't', 'c', 'o'):
        case FOURCC('c', 'o', '6', '4'):
        {
            mCurrSampleTables->parseChunkOffsetTable(type, *offset + 8, size - 8);
            *offset += size;
            break;
        }
        case FOURCC('s', 't', 's', 'z'):
        case FOURCC('s', 't', 'z', '2'):
        {
            mCurrSampleTables->parseSampleSizeTable(type, *offset + 8, size - 8);
            *offset += size;
            break;
        }
        case FOURCC('s', 't', 's', 'c'):
        {
            mCurrSampleTables->parseSampleToChunkTable(*offset + 8, size - 8);
            *offset += size;
            break;
        }
        case FOURCC('s', 't', 's', 's'):
        {
            mCurrSampleTables->parseSyncSampleTable(*offset + 8, size - 8);
            *offset += size;
            break;
        }
        case FOURCC('s', 't', 't', 's'):
        {
            mCurrSampleTables->parseSampleTimeTable(*offset + 8, size - 8);
            *offset += size;
            break;
        }
        case FOURCC('c', 't', 't', 's'):
        {
            mCurrSampleTables->parseSampleCompTimeTable(*offset + 8, size - 8);
            *offset += size;
            break;
        }
        case FOURCC('s', 't', 's', 'd'):
        {
            uint8_t buff[8];
            mFileDataReader->readAt(*offset + 8, buff, sizeof(buff));

          //  uint32_t version = buff[0];
          //  uint32_t flag = U32_AT(buff) >> 8;

            uint32_t entry_num = U32_AT(&buff[4]);
    //        printf("entry num %d\n", entry_num);

            *offset += 16;
            for (uint32_t i = 0; i < entry_num; i++) {
                parseChunk(offset, depth + 1);
            }
            break;
        }
        case FOURCC('m', 'p', '4', 'a'):
        {
            uint64_t end_offset;
            end_offset = *offset + size;

            //Reserved six bytes;
            uint8_t buffer[28]; 
            mFileDataReader->readAt(*offset + 8, buffer, sizeof(buffer));

            uint16_t version = U16_AT(&buffer[8]);
//            printf("version %d\n", version);

            *offset += 8;

            switch (version) {
                case 0:
                    *offset += 28;
                    break;
                case 1:
                    *offset += 28 + 16;  //16 extended bytes based on version 0;
                    break;
                case 2:
                    *offset += 64;
                    break;
                default:
                    printf("Unknown version for audio !\n");
            }

            while(*offset < end_offset) {
                parseChunk(offset, depth + 1);
            }

            mCurrTrackInfo->format.assign("audio/mp4a-latm");

            break;
        }
        case FOURCC('e', 's', 'd', 's'):
        {
            mAudioConfigBuffer = new DataBuffer();
            mAudioConfigBuffer->size = size - 8;
            mAudioConfigBuffer->data = (uint8_t *)malloc(size - 8);
            mFileDataReader->readAt(*offset + 8, mAudioConfigBuffer->data, size - 8);
            *offset += size;
            break;
        }
        case FOURCC('a', 'v', 'c', '1'):
        {

            uint64_t end_offset;
            end_offset = *offset + size;

            uint8_t buffer[78]; 
            mFileDataReader->readAt(*offset + 8, buffer, sizeof(buffer));

            uint16_t width = U16_AT(&buffer[6 + 18]);
            uint16_t height = U16_AT(&buffer[6 + 20]);

            mCurrTrackInfo->Width = width;
            mCurrTrackInfo->Height = height;
            printf("width %d, height %d\n", width, height);
            *offset += (8 + 78);
            while(*offset < end_offset) {
                parseChunk(offset, depth + 1);
            }

            mCurrTrackInfo->format.assign("video/avc");
            break;
        }
        case FOURCC('a', 'v', 'c', 'C'):
        {
            mVideoConfigBuffer = new DataBuffer();
            mVideoConfigBuffer->size = size - 8;
            mVideoConfigBuffer->data = (uint8_t *)malloc(size - 8);
            mFileDataReader->readAt(*offset + 8, mVideoConfigBuffer->data, size - 8);
            *offset += size;
            break;
        }
        default:
            *offset += size;

    }

    return false;
}

DataBuffer *MP4FileProbeInfo::getVideoConfigBuffer() {
    return mVideoConfigBuffer; 
}

DataBuffer *MP4FileProbeInfo::getAudioConfigBuffer() {
    return mAudioConfigBuffer;
}

int32_t MP4FileProbeInfo::readData(int index, DataBuffer *&buff) {
    SampleTables *table = (SampleTables *)mTrackVector[index].PrivData;
    printf("-----read data \n");

    SampleInfo *sampleinfo = new SampleInfo();
    table->getRequestSample(sampleinfo);

    if (sampleinfo == NULL) {
        buff = NULL;
        return 0;
    }

    buff->size = sampleinfo->size;
    buff->data = (uint8_t *)malloc(buff->size);
    printf("timestamp %d timescale %d\n", sampleinfo->time, mTrackVector[index].TimeScale);
    buff->timestamp = sampleinfo->time * 1000000 / mTrackVector[index].TimeScale;
    printf("timestamp %ld\n", buff->timestamp);

    mFileDataReader->readAt(sampleinfo->offset, buff->data, buff->size);

    return 0;
}

typedef struct {
    uint32_t type;
    const char *desc;
    const char *mime;
}ftypDesc;

ftypDesc ftypDescMap[] =   {{FOURCC('3', 'g', '2', 'a'), "3GPP2 Media (.3G2) compliant with 3GPP2 C.S0050-0 V1.0", "video/3gpp2"},
                            {FOURCC('3', 'g', '2', 'b'), "3GPP2 Media (.3G2) compliant with 3GPP2 C.S0050-A V1.0.0", "video/3gpp2"},
                            {FOURCC('m', 'p', '4', '1'), "MP4 v1 [ISO 14496-1:ch13]", "video/mp4"},
                            {FOURCC('m', 'p', '4', '2'), "MP4 v2 [ISO 14496-14]", "video/mp4"},
                            {FOURCC('a', 'v', 'c', '1'), "MP4 Base w/ AVC ext [ISO 14496-12:2005]", "video/mp4"}};

static bool isCompatibleBrand(uint32_t fourcc) {
    for(uint32_t i = 0; i < sizeof(ftypDescMap) / sizeof(ftypDescMap[0]); i++) {
        if (fourcc == ftypDescMap[i].type) {
            printf("ftyp %s, desc %s, mime %s\n", MakeFourCCString(fourcc), ftypDescMap[i].desc, ftypDescMap[i].mime);
            return true;
        }
    }

    return false;
}

BaseProber *ProbeMP4(FileDataReader *reader) {

    printf("probe MP4 file !\n");
    uint64_t offset = 0;
    uint8_t header[8];

    reader->readAt(offset, header, sizeof(header));

    uint32_t size = U32_AT(header);
    uint32_t type = U32_AT(&header[4]);

    if (type != FOURCC('f', 't', 'y', 'p')) {
        return NULL;
    }
    
    //Main brand.
    uint32_t mainbrand;
    reader->readAt(offset + 8, &mainbrand, 4);
    mainbrand = SWAP32(mainbrand);
    printf("main brand %s\n", MakeFourCCString(mainbrand));

    //Version.
    uint32_t version;
    reader->readAt(offset + 12, &version, 4);
    version = SWAP32(version);
    printf("brand version : %d\n",version);

    //Compatible brand.
    uint32_t compbrandnum = (size - 16) / 4;

    for (uint32_t i = 0; i < compbrandnum; i++) {
        uint32_t compbrand;
        reader->readAt(offset + 16 + i * 4, &compbrand, 4);
        compbrand = SWAP32(compbrand);

        if (!isCompatibleBrand(compbrand)) {
            printf("not compile !\n");
        }
    }

    return new MP4FileProbeInfo(reader);
}

