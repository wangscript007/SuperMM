#ifndef _PAYLOAD_TYPE_H_
#define _PAYLOAD_TYPE_H_

#include <stdint.h>

typedef struct {
    int pltype;
    int mediatype;
    const char* encoding;
    const char* codecmime;
    int clockrate;
    int channels;
}PayloadType;

static PayloadType PayloadTypeArray[] = {{0, 2, "PCMU", "audio/g711-mlaw", 8000, 1}};

#endif
