#ifndef _UTILS_H_

#define _UTILS_H_
#include <stdint.h>
#include <sys/types.h>
 #include <string>
 
using namespace std;

#define FOURCC(c1, c2, c3, c4) \
        (c1 << 24 | c2 << 16 | c3 << 8 | c4)

#define SWAP16(x) ((((x) << 8) & 0xff00) | (((x) >> 8) & 255))
#define SWAP32(x) (((x) << 24) | (((x) >> 24) & 255) | (((x) << 8) & 0xff0000) | (((x) >> 8) & 0xff00))
#define SWAP64(x) ((uint64_t)SWAP32(x & 0xffffffff) << 32) | SWAP32(x >> 32)

#define MKTAG(a,b,c,d) ((a) | ((b) << 8) | ((c) << 16) | ((unsigned)(d) << 24))
#define MKBETAG(a,b,c,d) ((d) | ((c) << 8) | ((b) << 16) | ((unsigned)(a) << 24))

uint16_t U16_AT(const uint8_t *ptr);
uint32_t U32_AT(const uint8_t *ptr);
uint64_t U64_AT(const uint8_t *ptr);

uint16_t U16LE_AT(const uint8_t *ptr);
uint32_t U32LE_AT(const uint8_t *ptr);
uint64_t U64LE_AT(const uint8_t *ptr);

const char *MakeFourCCString(uint32_t x); 

uint32_t absDiff(uint32_t seq1, uint32_t seq2);

char *itos(int num, char *str);

char *dtos(double num, char *str);
char *encodeToHex(char *buff, const uint8_t *src, int len, int type);
uint8_t *decodeFromHex(uint8_t *data, const char *src, int len);

void encodeBase64(const uint8_t *data, size_t size, string *out);

#define LOG(...) printf(LOG_TAG" : " __VA_ARGS__)

#endif
