#include <stdio.h>
#include <string.h>
#include <Utils.h>
#include "MP3FileProbeInfo.h"

MP3FileProbeInfo::MP3FileProbeInfo(FileDataReader *reader):
    mFileDataReader(reader),
    mFirstFramePos(0){

}

void MP3FileProbeInfo::InfoDump() {

}

static bool MPEGCheckHeader(uint32_t header) {
    if ((header & 0xffe00000) != 0xffe00000) {
        //Frame sync check.
        return false;
    }
    if (((header >> 17) & 3) == 0){
        //Layer check.
        return false;
    }
    if (((header >> 12) & 0xf) == 0xf || ((header >> 12) & 0xf) == 0) {
        //Bit rate index check.
        return false;
    }
    if (((header >> 10) & 3) == 3) {
        //Sampling rate index check.
        return false;
    }

    return true;
}
static bool parseFrameHeader(uint32_t header) {
    //printf("sync %x\n", data[0]);

    bool ret = MPEGCheckHeader(header);
    if (!ret) {
        return false;
    }

    uint8_t versionID = (header >> 19) & 3;
    uint8_t layer = (header >> 17) & 3;
    uint8_t br_index = (header >> 12) & 0x0f;
    uint8_t sr_index = (header >> 10) & 3;

    uint8_t padding = (header >> 9) & 1;

    printf("versionID %d, layer %d, br_index %d, sr_index %d, padding %d\n", versionID, layer, br_index, sr_index, padding);

    return true; 
}

ssize_t MP3FileProbeInfo::parseID3V2(uint8_t *data) {
    ssize_t id3len = 0;
    
    //Calculate the len of the id3v2 tag in the front of the file.
    id3len = ((data[6] & 0x7f) << 21)
            | ((data[7] & 0x7f) << 14)
            | ((data[8] & 0x7f) << 7)
            | (data[9] & 0x7f);

    printf("id3 tag len %d\n", (int)id3len);
    return id3len + 10;
}

ssize_t MP3FileProbeInfo::parseXINGHeader(uint64_t offset) {
    uint8_t buffer[8];
    uint64_t len = 0;

    mFileDataReader->readAt(offset + len, buffer, sizeof(buffer));
    if (memcmp("Xing", buffer, 4)) {
        return 0;
    } else {
        printf("Has xing header, and it is VBR !\n");
    }
    uint32_t vbr_flag = U32_AT(&buffer[4]);

    len += 8;
    if (vbr_flag & 0x0001) {
        uint32_t framenum;  // frame num, and 4 bytes.
        mFileDataReader->readAt(offset + len, &framenum, 4);
        len += 4;
    }

    if (vbr_flag & 0x0002) {
        uint32_t filesize;  //file size, and 4 bytes.
        mFileDataReader->readAt(offset + len, &filesize, 4);
        len += 4;
    }

    if (vbr_flag & 0x0004) {
        uint8_t toc[100];  //toc, and 100 bytes.
        mFileDataReader->readAt(offset + len, toc, 100);
    
        len += 100; 
    }

    if (vbr_flag & 0x0008) {
        uint32_t quality; //audio quality, and 4 bytes.
        mFileDataReader->readAt(offset + len, &quality, 4);
        len += 4;
    }

    return len;
}

uint64_t MP3FileProbeInfo::checkXINGHeaderPos(uint32_t header) {
    uint8_t versionID = (header >> 19) & 3;
    uint8_t stereotype = (header >> 6) & 3;

    printf("version id :%d stereotype :%d\n", versionID, stereotype);
    if ((versionID == 2) && (stereotype == 3)) {
        return 13;  //MPEG2 and single channel
    } else if ((versionID == 2) && (stereotype != 3)) {
        return 21;  //MPEG2 and not single channel
    } else if ((versionID == 3) && (stereotype == 3)) {
        return 21;  //MPEG1 and single channel
    } else if ((versionID == 3) && (stereotype != 3)) {
        return 36;  //MPEG1 and not single channel
    }
    return 0;
}

void MP3FileProbeInfo::FileParse() {
    uint8_t buffer[10];
    bool ret = false;

    uint64_t offset = 0;

    mFileDataReader->readAt(offset, buffer, sizeof(buffer));
    if (!memcmp("ID3", buffer, 3)) {
        ssize_t id3len = parseID3V2(buffer);
        offset += id3len;
        mFileDataReader->readAt(offset, buffer, sizeof(buffer));
    }

    uint32_t header = U32_AT(buffer);

    ret = parseFrameHeader(header);

    if (!ret) {
        return;
    }

   uint64_t pos = checkXINGHeaderPos(header);
   printf("xing pos %lld\n",(long long int)pos);
    if (pos) {
        ssize_t xinglen = parseXINGHeader(offset + pos);
        printf("XING LEN %d\n", (int)xinglen);
        if (xinglen) {
            offset += (pos + xinglen);
        }
    }
}

BaseProber *ProbeMP3(FileDataReader *reader) {
    //Probe MP3 file.
    uint8_t buffer[10];
    bool ret = false;

    reader->readAt(0, buffer, sizeof(buffer));
    if (!memcmp("ID3", buffer, 3)) {
        ssize_t len = 0;
        len = ((buffer[6] & 0x7f) << 21)
                | ((buffer[7] & 0x7f) << 14)
                | ((buffer[8] & 0x7f) << 7)
                | (buffer[9] & 0x7f);
        len += 10; 
        reader->readAt(len, buffer, sizeof(buffer));
    }

    uint32_t header = U32_AT(buffer);

    ret = parseFrameHeader(header);

    if (!ret) {
        return NULL;
    }
    return new MP3FileProbeInfo(reader);
}



