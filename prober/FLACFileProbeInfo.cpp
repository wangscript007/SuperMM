#include <stdio.h>
#include <string.h>
#include "Utils.h"
#include "FLACFileProbeInfo.h"

FLACFileProbeInfo::FLACFileProbeInfo(FileDataReader *reader) 
    :mFileDataReader(reader){

}

void FLACFileProbeInfo::FileParse() {
    printf("Parse FLAC file\n");
    uint64_t offset = 0;
    uint8_t buffer[4];
    ssize_t n = mFileDataReader->readAt(offset, buffer, sizeof(buffer));

    if (n < (ssize_t)sizeof(buffer)) {
        return;
    }

    //Meta data
    offset += 4;
    bool metadata_end = false;
    while (!metadata_end) {
        mFileDataReader->readAt(offset, buffer, sizeof(buffer));
        if (buffer[0] & 0x80) {
            metadata_end = true;
        }
        uint16_t metadata_type = buffer[0] & 0x7f;
        printf("metadata type %d\n",metadata_type);
        uint32_t metadata_size = U32_AT(buffer) & 0xffffff;
        printf("metadata size %d\n",metadata_size);
        offset += (4 + metadata_size);
    }

    //Frame
    mFileDataReader->readAt(offset, buffer, sizeof(buffer));

    //uint32_t datalen = U32_AT(buffer) & 0x00ffffff;
    printf("--- %x %x\n", buffer[0],buffer[1]);
}

void FLACFileProbeInfo::InfoDump() {

}

BaseProber *ProbeFLAC(FileDataReader *reader) {
    //Probe FLAC file.
    printf("Probe FLAC file\n");
    uint8_t header[4];
    ssize_t n = reader->readAt(0, header, sizeof(header));

    if (n < (ssize_t)sizeof(header)) {
        return NULL;
    }

    if (!memcmp("fLaC", header, 4)) {
        return new FLACFileProbeInfo(reader);
    }

    return NULL;
}



