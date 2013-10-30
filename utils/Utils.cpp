#include "Utils.h"
#include <stdlib.h>
#include <stdio.h>

uint16_t U16_AT(const uint8_t *ptr) {
    return ptr[0] << 8 | ptr[1];
}

uint16_t U16LE_AT(const uint8_t *ptr) {
    return ptr[1] << 8 | ptr[0];
}

uint32_t U32_AT(const uint8_t *ptr) {
    return ptr[0] << 24 | ptr[1] << 16 | ptr[2] << 8 | ptr[3];
}

uint32_t U32LE_AT(const uint8_t *ptr) {
    return ptr[3] << 24 | ptr[2] << 16 | ptr[1] << 8 | ptr[0];
}

uint64_t U64_AT(const uint8_t *ptr) {
    return ((uint64_t)U32_AT(ptr)) << 32 | U32_AT(ptr + 4);
}

uint64_t U64LE_AT(const uint8_t *ptr) {
    return ((uint64_t)U32LE_AT(ptr + 4)) << 32 | U32LE_AT(ptr);
}

const char *MakeFourCCString(uint32_t x) {
    char *str = (char *)malloc(5);
    str[0] = x >> 24;
    str[1] = (x >> 16) & 0xff;
    str[2] = (x >> 8) & 0xff;
    str[3] = x & 0xff;
    str[4] = '\0';

    return str;
}

uint32_t absDiff(uint32_t seq1, uint32_t seq2) {
    return seq1 > seq2 ? seq1 - seq2 : seq2 - seq1;
}

char *itos(int num, char *str) {
    sprintf(str, "%d", num);
    return str;
}

char *dtos(double num, char *str) {
    sprintf(str, "%lf", num);
    return str;
}

static const char hex_table_uc[16] = { '0', '1', '2', '3',
                                       '4', '5', '6', '7',
                                       '8', '9', 'A', 'B',
                                       'C', 'D', 'E', 'F' };
static const char hex_table_lc[16] = { '0', '1', '2', '3',
                                       '4', '5', '6', '7',
                                       '8', '9', 'a', 'b',
                                       'c', 'd', 'e', 'f' };

char *encodeToHex(char *buff, const uint8_t *src, int len, int type)
{
    int i;

    const char *hex_table = type ? hex_table_lc : hex_table_uc;

    for(i = 0; i < len; i++) {
        buff[i * 2]     = hex_table[src[i] >> 4];
        buff[i * 2 + 1] = hex_table[src[i] & 0xF];
    }

    buff[2 * len] = '\0';
    return buff;
}

uint8_t *decodeFromHex(uint8_t *data, const char *src, int len)
{
    size_t outLen = len / 2;

    uint8_t *out = data;

    uint8_t accum = 0;
    for (size_t i = 0; i < len; ++i) {
        char c = src[i];
        unsigned value;
        if (c >= '0' && c <= '9') {
            value = c - '0';
        } else if (c >= 'a' && c <= 'f') {
            value = c - 'a' + 10;
        } else if (c >= 'A' && c <= 'F') {
            value = c - 'A' + 10;
        } else {
            return NULL;
        }

        accum = (accum << 4) | value;

        if (i & 1) {
            *out++ = accum;
            accum = 0;
        }
    }
    return data;
}

void encodeBase64(const uint8_t *data, size_t size, string *out) {
    static const char base64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    out->clear();

    size_t i;
    for (i = 0; i < (size / 3) * 3; i += 3) {
        uint8_t x1 = data[i];
        uint8_t x2 = data[i + 1];
        uint8_t x3 = data[i + 2];

        out->append(1, base64[x1 >> 2]);
        out->append(1, base64[(x1 << 4 | x2 >> 4) & 0x3f]);
        out->append(1, base64[(x2 << 2 | x3 >> 6) & 0x3f]);
        out->append(1, base64[x3 & 0x3f]);
    }
    switch (size % 3) {
        case 1:
        {
            uint8_t x1 = data[i];
            out->append(1, base64[x1 >> 2]);
            out->append(1, base64[(x1 << 4) & 0x3f]);
            out->append("==");
            break;
        }
        case 2:
        {
            uint8_t x1 = data[i];
            uint8_t x2 = data[i + 1];
            out->append(1, base64[x1 >> 2]);
            out->append(1, base64[(x1 << 4 | x2 >> 4) & 0x3f]);
            out->append(1, base64[(x2 << 2) & 0x3f]);
            out->append(1, '=');
            break;
        }
	default:
		printf("Doing nothing here, have done before !\n");

    }
}

