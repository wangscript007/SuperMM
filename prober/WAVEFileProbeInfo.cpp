#include <stdio.h>
#include <string.h>
#include "WAVEFileProbeInfo.h"

WAVEFileProbeInfo::WAVEFileProbeInfo(FileDataReader *reader) {

}

void WAVEFileProbeInfo::FileParse() {
    printf("Parse WAVE file\n");
}

void WAVEFileProbeInfo::InfoDump() {

}

BaseProber *ProbeWAVE(FileDataReader *reader) {
    //Probe WAVE file.

    uint8_t buffer[12];
    ssize_t n = reader->readAt(0, buffer, sizeof(buffer));
    if (n < (ssize_t)sizeof(buffer)) {
        return NULL;
    }

    if (!memcmp(buffer, "RIFF", 4) && !memcmp(&buffer[8], "WAVE", 4)) {
        return new WAVEFileProbeInfo(reader);
    }

    return NULL;
}



