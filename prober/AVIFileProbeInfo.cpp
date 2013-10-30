#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "Utils.h"
#include "AVIFileProbeInfo.h"

AVIFileProbeInfo::AVIFileProbeInfo(FileDataReader *reader)
    :mFileDataReader(reader),
     mMainAVIHeader(NULL),
     mAVIStreamHeader(NULL),
     mOldStyleIndex(NULL){
    mMainAVIHeader = (MainAVIHeader *)malloc(sizeof(MainAVIHeader));

    if (mMainAVIHeader == NULL) {
        printf("Failure to malloc for MainAVIHeader\n");
        return;
    } 

    mAVIStreamHeader = (AVIStreamHeader *)malloc(sizeof(AVIStreamHeader));

    if (mAVIStreamHeader == NULL) {
        printf("Failure to malloc for AVIStreamHeader\n");
        return;
    }
}

void AVIFileProbeInfo::FileParse() {
    bool parsedone = false;
    uint64_t offset = 0;

    while (!parsedone) {
        parsedone = parseChunk(&offset, 1);
    }

    printf("Parse AVI file\n");
}

void AVIFileProbeInfo::InfoDump() {

}

void AVIFileProbeInfo::parseMainAVIHeader(uint64_t offset, ssize_t size) {
    ssize_t n = mFileDataReader->readAt(offset, mMainAVIHeader, size);
    
    if (n < size) {
        return;
    }

//    printf("align bytes %d\n",mMainAVIHeader->dwPaddingGranularity);
//    printf("width : %d height : %d\n", mMainAVIHeader->dwWidth, mMainAVIHeader->dwHeight);
}

void AVIFileProbeInfo::parseAVIStreamHeader(uint64_t offset, ssize_t size) {
    ssize_t n = mFileDataReader->readAt(offset, mAVIStreamHeader, sizeof(AVIStreamHeader));
//    printf("----n %d      %d\n",n, sizeof(AVIStreamHeader));

    if (n < (ssize_t)sizeof(AVIStreamHeader)) {
        return;
    }

    mAVIStreamHeader->fccType = SWAP32(mAVIStreamHeader->fccType);
//    printf("fcc type :%s\n",MakeFourCCString(mAVIStreamHeader->fccType));
    mAVIStreamHeader->fccHandler = SWAP32(mAVIStreamHeader->fccHandler);
//    printf("fcc handler :%s\n",MakeFourCCString(mAVIStreamHeader->fccHandler));
}

void AVIFileProbeInfo::parseOldStyleIndex(uint64_t offset, ssize_t size) {
    uint64_t pos = 0;
    uint32_t structsize = sizeof(OldStyleIndex);

    mOldStyleIndex = (OldStyleIndex *)malloc(structsize);
    
    while (pos < (uint32_t)size) {
        mFileDataReader->readAt(offset + pos, mOldStyleIndex, structsize);
        
//        printf("id %s\n",MakeFourCCString(SWAP32(mOldStyleIndex->ckid)));

        pos += structsize;
    }
}

void AVIFileProbeInfo::parseOpenDMLIndex(uint64_t offset, ssize_t size) {

}

bool AVIFileProbeInfo::parseChunk(uint64_t *offset, uint32_t depth) {
    uint8_t buffer[12];
    ssize_t n = mFileDataReader->readAt(*offset, buffer, sizeof(buffer));

    if (!n) {
       return true; 
    }

    uint32_t type = U32_AT(buffer);
    uint32_t size = U32LE_AT(&buffer[4]);

#if 1
    char space[40] = "--------------------------------";
    sprintf(space + depth * 4, "%s", MakeFourCCString(type));
#endif

    if (type == FOURCC('R', 'I', 'F', 'F') || type == FOURCC('L', 'I', 'S', 'T')) {
        uint32_t subtype = U32_AT(&buffer[8]);

#if 1
        sprintf(space + depth * 4 + 4, "----%s", MakeFourCCString(subtype));
        printf("%s  %d\n", space, size);
#endif

        if (subtype == FOURCC('m', 'o','v','i')) {
            *offset += (size + 8);
        } else {
            *offset += 12;
            uint64_t end_offset = *offset + size - 4;
            while (*offset < end_offset) {
//                printf("offset %x\n", *offset);
                parseChunk(offset, depth + 1);
            }
        }
    } else {
        printf("%s  %d\n", space, size);
        switch (type) {
            case FOURCC('s', 't', 'r', 'h'):
                parseAVIStreamHeader(*offset + 8, size);
                break;
            case FOURCC('a', 'v', 'i', 'h'):
                parseMainAVIHeader(*offset + 8, size);
                break;
            case FOURCC('i', 'd', 'x', '1'):
                parseOldStyleIndex(*offset + 8, size);
                break;
            case FOURCC('i', 'n', 'd', 'x'):
                parseOpenDMLIndex(*offset + 8, size);
                break;
            default:
                break;
                //Do nothing !
        }
        if (size & 1) {
            ++size;
        }
        *offset += (size + 8);
    }
    return false;
}

BaseProber *ProbeAVI(FileDataReader *reader) {
    //Probe AVI file.
    printf("Probe AVI file\n");
    uint8_t buffer[12];
    ssize_t n = reader->readAt(0, buffer, sizeof(buffer));

    if (n < (ssize_t)sizeof(buffer)) {
        return NULL;
    }

    if (!memcmp(buffer, "RIFF", 4) && !memcmp(&buffer[8], "AVI ", 4)) {
        return new AVIFileProbeInfo(reader);
    }

    return NULL;
}



