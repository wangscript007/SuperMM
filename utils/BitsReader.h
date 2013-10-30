#ifndef _GET_BITS_H_
#define _GET_BITS_H_

#include <stdint.h>
#include <stddef.h>

class BitsReader {
public:
	BitsReader(uint8_t *data, size_t size);
	~BitsReader();

	int getBits(int num);
	int showBits(int num);
	int skipBits(int num);
	int getLongBits(int num);
	int showLongBits(int num);
	size_t getBitsLeft();

private:
	uint8_t *mStartData;
	int mBitPosition;
	size_t mBitsCount;

};

#endif
