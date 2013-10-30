#ifndef _DATA_BUFFER_H_

#define _DATA_BUFFER_H_

typedef struct{
    uint8_t *data;
    size_t size;

    int64_t timestamp;
}DataBuffer;

#endif
