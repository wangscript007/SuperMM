#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include "Utils.h"
#include "MKVFileProbeInfo.h"

MKVFileProbeInfo::MKVFileProbeInfo(FileDataReader *reader)
    :mFileDataReader(reader) {

}

static uint8_t getLength(uint8_t data) {
    uint8_t tmpdata = floor(log2(data));
    uint8_t length = 8 - tmpdata;
                   
    return length;
}

static uint64_t getNumber(uint8_t *data, uint8_t len) {
    uint64_t number = data[0] ^ (1 << (8 - len));
    for (uint8_t n = 1; n < len; n++) {
        number = (number << 8) | data[n];
    }

    return number;
}

static uint64_t getID(uint8_t *data, uint8_t len) {
    uint64_t number = 0;
    for (uint8_t n = 0; n < len; n++) {
        number = (number << 8) | data[n];
    }

    return number;
}

void MKVFileProbeInfo::FileParse() {
    printf("Parse MKV file\n");
    uint64_t filesize = mFileDataReader->getFileSize();

    parseElements(0, filesize, 0);

}

void MKVFileProbeInfo::parseElements(uint64_t offset, uint64_t max_size, uint32_t depth) {
    uint8_t data;
    uint8_t buff[8];
    uint64_t pos = 0;

    while (pos < max_size) {
        //Parse ID.
        mFileDataReader->readAt(offset + pos, &data, 1);
        uint8_t len = getLength(data);

        mFileDataReader->readAt(offset + pos, buff, len);
        
        uint64_t ID = getID(buff, len);
        pos += len;
        
        //Parse data size and data.
        mFileDataReader->readAt(offset + pos, buff, 1);
        len = getLength(buff[0]);

        mFileDataReader->readAt(offset + pos, buff, len);
        uint64_t size = getNumber(buff, len);

        pos += len;
        //printf("Top level element ID %x, size %lld, offset %lld, %x\n", ID, size, offset + pos, offset + pos);
#if 1
    char space[40] = "                                ";
 
    uint32_t i = 0;
    bool found = false;
    for (; i < sizeof(EleInfoArray) / sizeof(EleInfoArray[0]); i++) {
        if (EleInfoArray[i].ID == ID) {
            found  =  true;
            break;
        }
    }

    if (found) {
        sprintf(space + depth * 4, "%s", EleInfoArray[i].name);
        printf("%s, %lu, %lu\n", space, ID, size);
    }
#endif
        if (ID == EBML_HEADER_ID) {
            parseElements(offset + pos, size, depth + 1);
        } else if (ID == MATROSKA_SEGMENT_ID) {
            parseElements(offset + pos, size, depth + 1); 
        }
        
        //SEEKHEAD ELEMENT
        if (ID == MATROSKA_SEEKHEAD_ID) {
            parseElements(offset + pos, size, depth + 1); 
        } else if (ID == MATROSKA_SEEKENTRY_ID) {
            parseElements(offset + pos, size, depth + 1); 
        } else if (ID == MATROSKA_SEEKID_ID) {
            uint64_t seekid;
            readData_uint(offset + pos, size, &seekid);
        //    printf("seek id : %x\n", seekid);
        } else if (ID == MATROSKA_SEEKPOSITION_ID) {
            uint64_t seekposition;
            readData_uint(offset + pos, size, &seekposition);
    //        printf("seek position : %x\n", seekposition);
        }

        //CLUSTER ELEMENT
        if (ID == MATROSKA_CLUSTER_ID) {
         //   parseElements(offset + pos, size, depth + 1); 
        } else if (ID == MATROSKA_SIMPLEBLOCK_ID) {
        //    parseSimpleBlock(offset + pos, size);
        }

        //CUES ELEMENT
        if (ID == MATROSKA_CUES_ID) {
            parseElements(offset + pos, size, depth + 1); 
        } else if (ID == MATROSKA_POINTENTRY_ID) {
            parseElements(offset + pos, size, depth + 1); 
        } else if (ID == MATROSKA_CUETIME_ID) {
            uint64_t cuetime;
            readData_uint(offset + pos, size, &cuetime);
         //   printf("cue time : %x\n", cuetime);
        } else if (ID == MATROSKA_CUETRACKPOSITION_ID) {
            parseElements(offset + pos, size, depth + 1); 
        }

        if (ID == MATROSKA_TRACKS_ID) {
            parseElements(offset + pos, size, depth + 1); 
        } else if (ID == MATROSKA_TRACKENTRY_ID) {
            parseTrackEntry(offset + pos, size);
        } 
        pos += size; 
    }
}

void MKVFileProbeInfo::parseSimpleBlock(uint64_t offset, uint64_t size) {
        uint8_t buff[8];

        uint64_t tmppos = 0;
        uint8_t tracknum;
        //track num
        mFileDataReader->readAt(offset + tmppos, buff, 1);

        tracknum = getNumber(buff, 1);
 //           printf("track num %x\n", tracknum);
            
        //timecode
        tmppos += 1;
        mFileDataReader->readAt(offset + tmppos, buff, 1);
        uint8_t timecodelen = getLength(buff[0]);
          printf(" timecode len %d\n", timecodelen);

        mFileDataReader->readAt(offset + tmppos, buff, timecodelen);
        //uint64_t timecode = getNumber(buff, timecodelen);
   //         printf(" timecode %d\n", timecode);

        tmppos += timecodelen;

            //flags
        uint8_t flags;
        mFileDataReader->readAt(offset + tmppos, &flags, 1);
        //printf("flag %x\n",buff[0]);

        uint8_t lacingtype = int(flags & 0x06) >> 1;
//        printf("lacing type %d\n", lacingtype);

        tmppos += 1;

        switch (lacingtype) {
            case 0: //no lacing
            {
                //uint64_t framesize = size - (tmppos + 1);
                break;
            }
            case 1: //Xiph lacing
            {
                uint8_t framenum;
                mFileDataReader->readAt(offset + tmppos, &framenum, 1);
//                printf("frame num --------------- %d\n", framenum);
                break;
            }
            case 2: //fixed size lacing
                break;
            case 3: //EBML lacing
                break;
            default:
                printf("Unknown lacing !\n");
        }
}

void MKVFileProbeInfo::parseTrackEntry(uint64_t offset, uint64_t size) {

}

void MKVFileProbeInfo::readData_uint(uint64_t offset, uint64_t size, uint64_t *data) {
    uint8_t tmp;

    *data = 0;
    for (uint8_t i = 0; i < (uint8_t)size; i++) {
        mFileDataReader->readAt(offset + i, &tmp, 1);
        *data = (*data << 8) | tmp;
    }
}

void MKVFileProbeInfo::InfoDump() {

}

BaseProber *ProbeMKV(FileDataReader *reader) {
    //Probe MKV file.
    printf("Probe MKV file\n");
    uint8_t buff[8];
    uint64_t pos = 0;
    reader->readAt(pos, buff, 4);

    if(U32_AT(buff) != 0x1A45DFA3) {
        return NULL;
    }

    pos += 4;

    reader->readAt(pos, buff, 1);

    uint8_t len = getLength(buff[0]);

    reader->readAt(pos, buff, len);

    uint64_t number = getNumber(buff, len);

//    printf("number %x\n", number);
    pos += len;
    uint64_t end_pos = pos + number;
    while (pos < end_pos) {
        //printf("pos %lld\n",pos);
        //Parse ID.
        reader->readAt(pos, buff, 1);
        len = getLength(buff[0]);

        reader->readAt(pos, buff, len);
        
        uint64_t ID = getID(buff, len);

        pos += len;
        
        //Parse data size and data
        reader->readAt(pos, buff, 1);
        len = getLength(buff[0]);

        reader->readAt(pos, buff, len);
        uint64_t size = getNumber(buff, len);

        pos += len;

        if (ID == 0x4282) {
            char *doctype = (char *)malloc(size + 1);
            reader->readAt(pos, doctype, size);
            if (!strncmp(doctype, "matroska", 8)) {
                printf("This file is Matroska !\n");
            } else if (!strncmp(doctype, "webm", 4)){
                printf("This file is WebM !\n");
            }
            return new MKVFileProbeInfo(reader);
        } else {
            pos += size;
        }
    }

    return NULL; 
}
