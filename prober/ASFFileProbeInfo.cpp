#include <stdio.h>
#include <string.h>
#include "Utils.h"
#include "ASFFileProbeInfo.h"

const AsfGuid ASF_Header_Object = {
    0x30, 0x26, 0xB2, 0x75, 0x8E, 0x66, 0xCF, 0x11, 0xA6, 0xD9, 0x00, 0xAA, 0x00, 0x62, 0xCE, 0x6C
};

const AsfGuid ASF_Data_Object = {
    0x36, 0x26, 0xb2, 0x75, 0x8E, 0x66, 0xCF, 0x11, 0xa6, 0xd9, 0x00, 0xaa, 0x00, 0x62, 0xce, 0x6c
};

const AsfGuid ASF_Simple_Index_Object = {
    0x90, 0x08, 0x00, 0x33, 0xB1, 0xE5, 0xCF, 0x11, 0x89, 0xF4, 0x00, 0xA0, 0xC9, 0x03, 0x49, 0xCB
};

static inline int AsfGuidCmp(const void *g1, const void *g2) {
    return memcmp(g1, g2, sizeof(AsfGuid));
}
ASFFileProbeInfo::ASFFileProbeInfo(FileDataReader *reader)
    :mFileDataReader(reader){

}

void ASFFileProbeInfo::FileParse() {
    uint64_t offset = 0;

    uint64_t size = mFileDataReader->getFileSize();
    while (size >= OBJECT_HEAD_LEN) {
        uint8_t buffer[OBJECT_HEAD_LEN];
        mFileDataReader->readAt(offset, buffer, sizeof(buffer));

        uint64_t objsize = U64LE_AT(buffer + OBJECT_ID_LEN);

       // printf("%x %x %x %x\n", buffer[0], buffer[1], buffer[2], buffer[3]);

        if (!AsfGuidCmp(buffer, ASF_Header_Object)) {
            parseHeaderObject(offset + OBJECT_HEAD_LEN, size - OBJECT_HEAD_LEN);
        } else if (!AsfGuidCmp(buffer, ASF_Data_Object)) {
            parseDataObject(offset + OBJECT_HEAD_LEN, size - OBJECT_HEAD_LEN);
        } else if (!AsfGuidCmp(buffer, ASF_Simple_Index_Object)) {
            parseSimpleIndexObject(offset + OBJECT_HEAD_LEN, size - OBJECT_HEAD_LEN);
        }
        offset += objsize;
        size -= objsize;
    }

    printf("Parse ASF file\n");
}

void ASFFileProbeInfo::InfoDump() {

}

void ASFFileProbeInfo::parseHeaderObject(uint64_t offset, uint64_t size) {
    uint64_t pos = 0;

    uint32_t objectnum;

    mFileDataReader->readAt(offset + pos, &objectnum, sizeof(objectnum));
    printf("object number :%d\n", objectnum);
    pos += 6;

    for (uint32_t i = 0; i < objectnum; i++) {
        uint8_t buffer[OBJECT_HEAD_LEN];
        mFileDataReader->readAt(offset + pos, buffer, sizeof(buffer));

        uint64_t objsize = U64LE_AT(buffer + OBJECT_ID_LEN);
        printf("%x %x %x %x\n", buffer[0], buffer[1], buffer[2], buffer[3]);

        pos += objsize;
    }
}

void ASFFileProbeInfo::parseDataObject(uint64_t offset, uint64_t size) {

}

void ASFFileProbeInfo::parseSimpleIndexObject(uint64_t offset, uint64_t size) {

}

BaseProber *ProbeASF(FileDataReader *reader) {
    //Probe ASF file.
    uint8_t buffer[16];
    reader->readAt(0, buffer, sizeof(buffer));

    if (AsfGuidCmp(buffer, ASF_Header_Object)) {
        return NULL;
    }
    printf("Probe ASF file\n");

    return new ASFFileProbeInfo(reader);
}



