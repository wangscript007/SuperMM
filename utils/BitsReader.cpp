
#include "BitsReader.h"
#include "Utils.h"

BitsReader::BitsReader(uint8_t *data, size_t size)
	:mStartData(data),
	 mBitPosition(0){
	mBitsCount = size * 8;
}

BitsReader::~BitsReader() {

}

int BitsReader::getBits(int num) {
	int value;
	value = U32_AT(mStartData + (mBitPosition >> 3)) << (mBitPosition & 7);
	value >>= (32 - num);

	mBitPosition += num;
	return value;
}

int BitsReader::getLongBits(int num) {
	int value;
    if (!num) {
        return 0;
    } else if (num <= 25) {
        value = getBits(num);
    } else {
		value = getBits(16) << (num - 16);
		value |= getBits(num - 16);
	}

	mBitPosition += num;
	return value;
}

int BitsReader::showBits(int num) {
	int value;
	value = U32_AT(mStartData + (mBitPosition >> 3)) << (mBitPosition & 7);
	value >>= (32 - num);

	return value;
}

int BitsReader::showLongBits(int num) {
	int value;
    if (!num) {
        return 0;
    } else if (num <= 25) {
        value = getBits(num);
    } else {
		value = getBits(16) << (num - 16);
		value |= getBits(num - 16);
	}

	return value;
}

int BitsReader::skipBits(int num) {
	mBitPosition += num;
	
	return 0;
}

size_t BitsReader::getBitsLeft() {
	return mBitsCount - mBitPosition;
}
