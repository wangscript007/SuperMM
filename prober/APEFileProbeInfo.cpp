#include <stdio.h>
#include "APEFileProbeInfo.h"

APEFileProbeInfo::APEFileProbeInfo(FileDataReader *reader) {

}

void APEFileProbeInfo::FileParse() {
    printf("Parse APE file\n");
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



