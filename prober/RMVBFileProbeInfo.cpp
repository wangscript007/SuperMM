#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "Utils.h"
#include "RMVBFileProbeInfo.h"

#define CHUNK_HEADER_LENGTH 10

RMVBFileProbeInfo::RMVBFileProbeInfo(FileDataReader *reader)
    :mFileDataReader(reader){

}

RMVBFileProbeInfo::~RMVBFileProbeInfo() {

}

void RMVBFileProbeInfo::FileParse() {

    parseChunks();

}

int32_t  RMVBFileProbeInfo::parseChunks() {
    uint64_t filesize = mFileDataReader->getFileSize();

    uint64_t offset = 0;
    while (offset < filesize) {
    
        uint8_t header[10];
        uint32_t chunk_type;
        uint32_t chunk_size;
        uint16_t chunk_version;

        ssize_t n = mFileDataReader->readAt(offset, header, sizeof(header));

        chunk_type = U32_AT(header);
        chunk_size = U32_AT(header + 4);
        chunk_version = U16_AT(header + 8);

#if 1
    char space[4];

    sprintf(space, "%s", MakeFourCCString(chunk_type));
    printf("chunk type: %s  size is %d\n", space, chunk_size);

#endif

        switch(chunk_type) {
            case FOURCC('.', 'R', 'M', 'F'):
            {
                uint8_t buffer[8];

                mFileDataReader->readAt(offset + CHUNK_HEADER_LENGTH, buffer, sizeof(buffer));

                uint32_t file_version = U32_AT(buffer);
                uint32_t headers_num = U32_AT(buffer + 4);

                offset += chunk_size;
                break;
            }
            case FOURCC('M', 'D', 'P', 'R'):
                //This chunk contains some information about the general properties of a 
                //RealMedia file. Only one PROP chunk can be present in a file. 
                parseMDPR(offset + CHUNK_HEADER_LENGTH, chunk_size - CHUNK_HEADER_LENGTH);

                offset += chunk_size;
                break;
            case FOURCC('P', 'R', 'O', 'P'):
                //This chunk contains some information about the general properties of a 
                //RealMedia file. Only one PROP chunk can be present in a file. 
                parsePROP(offset + CHUNK_HEADER_LENGTH, chunk_size - CHUNK_HEADER_LENGTH);

                offset += chunk_size;
                break;
            case FOURCC('D', 'A', 'T', 'A'):
                //This chunk contains a group of data packets. Packets from each stream are 
                //interleaved, except for multirate files.
                parseDATA(offset + CHUNK_HEADER_LENGTH);
                
                offset += chunk_size;
                break;
            case FOURCC('I', 'N', 'D', 'X'):
                //This chunk contains index entries. It comes after all the DATA chunks. An index 
                //chunk contains data for a single stream, A file can have more than one INDX chunk.
                parseINDX(offset + CHUNK_HEADER_LENGTH, chunk_size - CHUNK_HEADER_LENGTH);
                
                offset += chunk_size;
                break;
            case FOURCC('C', 'O', 'N', 'T'):
                //This chunk contains some text information (like title, author, ...) about the
                //content of the file. This header has an informative purpose only and it's not needed 
                //to demux the file. 
                parseCONT(offset + CHUNK_HEADER_LENGTH);
                offset += chunk_size;
                break;
            default:
                offset += chunk_size;
        }

    }

    return 0;
}

int32_t  RMVBFileProbeInfo::parsePROP(uint64_t offset, size_t size) {
    uint8_t buff[size];

    size_t n = mFileDataReader->readAt(offset, buff, sizeof(buff));

    if (n < size) {
        return -1;
    }

    uint32_t max_bit_rate = U32_AT(buff);
    uint32_t ave_bit_rate = U32_AT(buff + 4);
    uint32_t max_packet_size = U32_AT(buff + 8);
    uint32_t ave_packet_size = U32_AT(buff + 12);
    uint32_t packets_num = U32_AT(buff + 16);

    printf("packets_num %d\n", packets_num);

    uint32_t first_index_offset = U32_AT(buff + 28);
    uint32_t first_data_offset = U32_AT(buff + 32);

    mDataStartOffset = first_data_offset;

    uint16_t streams_num = U16_AT(buff + 36);
    printf("stream num: %d size %d %x %x\n", streams_num, size, buff[36], buff[37]);
    uint16_t flags = U16_AT(buff + 38);

    return 0;
}

bool RMVBFileProbeInfo::checkValidStream(char *mime) {
    if(!strncmp(mime, "video", 5) || !strncmp(mime, "audio", 5)) {
        return true;
    }

    return false;
}

int32_t  RMVBFileProbeInfo::parseMDPR(uint64_t offset, size_t size) {
    uint8_t buff[size];

    size_t n = mFileDataReader->readAt(offset, buff, sizeof(buff));

    if (n < size) {
        return -1;
    }

    uint16_t stream_num = U16_AT(buff);
    uint32_t max_bit_rate = U32_AT(buff + 2);
    uint32_t ave_bit_rate = U32_AT(buff + 6);
    uint32_t max_packet_size = U32_AT(buff + 10);
    uint32_t ave_packet_size = U32_AT(buff + 14);

    //Stream start offset in ms, 4 bytes.
    //Preroll in ms, 4 bytes.

    uint32_t stream_duration = U32_AT(buff + 26);

    uint64_t pos = 30;
    uint8_t description_len = buff[pos];
    pos++;
    char description[description_len + 1];
    memcpy(description, buff + pos, description_len);
    description[description_len] = '\0';
    pos += description_len;

    uint8_t mime_len = buff[pos];
    pos++;
    char mime[mime_len + 1];
    memcpy(mime, buff + pos, mime_len);
    mime[mime_len] = '\0';
    pos += mime_len;
    printf("mime %s\n", mime);

    if (checkValidStream(mime)) {
        TrackInfo info;
        info.TrackIndex = mTrackIndex;
        info.StreamNum = stream_num;
        info.format.append(mime);

        mTrackVector.push_back(info);
        mTrackIndex++;
    }

    return 0;
}

int32_t  RMVBFileProbeInfo::parseDATA(uint64_t offset) {
    printf("%lld, %lld\n", mDataStartOffset, offset);

    uint8_t buff[8];
    size_t n = mFileDataReader->readAt(offset, buff, sizeof(buff));

    uint32_t packets_num = U32_AT(buff);
    uint32_t offset_next_DATA = U32_AT(buff + 4);

    //update mDataStartOffset
    mDataStartOffset = offset + 8;

    return 0;
}

int32_t  RMVBFileProbeInfo::parseINDX(uint64_t offset, size_t size) {
    uint8_t buff[10];
    size_t n = mFileDataReader->readAt(offset, buff, sizeof(buff));

    if (n < sizeof(buff)) {
        return -1;
    }
    uint32_t number_entries = U32_AT(buff);
    printf("number_entries %d\n", number_entries);

    uint64_t position = offset + 10;
    for (uint32_t i = 0; i < number_entries; i++) {
        uint8_t data[14];
        //parse index entry, every entry is about 14 bytes.
        n = mFileDataReader->readAt(position, data, sizeof(data));

        if (n < sizeof(data)) {
            return -1;
        }

        uint32_t timestamp = U32_AT(data + 2);
        uint32_t packetoffset = U32_AT(data + 6);
        position += 14;
    }

    return 0;
}

int32_t  RMVBFileProbeInfo::parseCONT(uint64_t offset) {
    uint16_t length = 0;
    uint64_t position = offset;
    char string[100];
    uint8_t buff[2];

    //Title string
    mFileDataReader->readAt(position, buff, sizeof(buff));
    length = U16_AT(buff);
    if (length > 0) {
        mFileDataReader->readAt(position + 2, string, length);
        string[length] = '\0';
        position += (length + 2);
    }

    //Author string
    mFileDataReader->readAt(position, buff, sizeof(buff));
    length = U16_AT(buff);
    if (length > 0) {
        mFileDataReader->readAt(position + 2, string, length);
        string[length] = '\0';
        position += (length + 2);
    }

    //Cotyright string
    mFileDataReader->readAt(position, buff, sizeof(buff));
    length = U16_AT(buff);
    if (length > 0) {
        mFileDataReader->readAt(position + 2, string, length);
        string[length] = '\0';
        position += (length + 2);
    }

    //Comment string
    mFileDataReader->readAt(position, buff, sizeof(buff));
    length = U16_AT(buff);
    if (length > 0) {
        mFileDataReader->readAt(position + 2, string, length);
        string[length] = '\0';
    }

    return 0;
}

void RMVBFileProbeInfo::InfoDump() {

}

int32_t RMVBFileProbeInfo::readData(int index, DataBuffer *&buff) {

    return 0;
}

BaseProber *ProbeRMVB(FileDataReader *reader) {
    //Probe RMVB file.
    printf("Probe RMVB file\n");
    uint8_t header[4];
    ssize_t n = reader->readAt(0, header, sizeof(header));

    if (n < (ssize_t)sizeof(header)) {
        return NULL;
    }

    if (header[0] == '.' && header[1] == 'R' && header[2] == 'M' && header[3] == 'F') {
        return new RMVBFileProbeInfo(reader);
    }

    return NULL;
}



