#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "AVCVideoConfig.h"
#include "Utils.h"

AVCVideoConfig::AVCVideoConfig() {
	mVideoConfig = (VideoConfig *)malloc(sizeof(VideoConfig));
	mVideoConfig->ProfileLevelID = NULL;
	mVideoConfig->SpropParameterSets = NULL;
	mSPSInfo = NULL;
	mPPSInfo = NULL;
}

AVCVideoConfig::~AVCVideoConfig() {

}

VideoConfig *AVCVideoConfig::parseVideoConfig(uint8_t *data, size_t size) {
	uint32_t pos = 0;
	uint8_t configurationVersion = data[0];
	printf("----------------------------YGZHAO\n");
	if (configurationVersion != 1) {
		printf("mal formed !\n");
		return NULL;
	}

	uint8_t AVCProfileIndication = data[1];
	uint8_t profile_compatibility = data[2];
	uint8_t AVCLevelIndication = data[3];
	
	uint8_t numOfSequenceParameterSets = data[5] & 0x1f;
	printf("numOfSequenceParameterSets %d\n", numOfSequenceParameterSets);

	pos += 6;
	for (int i=0; i < numOfSequenceParameterSets; i++) {
		uint16_t sequenceParameterSetLength = data[pos] << 8 | data[pos + 1];
		parseSPS(data + pos + 2, sequenceParameterSetLength);
		pos += (sequenceParameterSetLength + 2);
	}

	uint8_t numOfPictureParameterSets = data[pos];
	pos += 1;

	for (int i=0; i < numOfPictureParameterSets; i++) {
		uint16_t pictureParameterSetLength = data[pos] << 8 | data[pos + 1];
		parsePPS(data + pos + 2, pictureParameterSetLength);
		pos += (pictureParameterSetLength + 2);
	}

	mVideoConfig->SpropParameterSets = new string();
	mVideoConfig->SpropParameterSets->append(mSPSInfo->c_str());
	mVideoConfig->SpropParameterSets->append(1, ',');
	mVideoConfig->SpropParameterSets->append(mPPSInfo->c_str());
//	printf("pos %d, size %d\n", pos, size);
	return mVideoConfig;
}

uint32_t AVCVideoConfig::parseSPS(uint8_t *data, size_t size)  {
	printf("sps %x %x %x %x %x\n", data[0], data[1], data[2], data[3], data[4]);

	if (mVideoConfig->ProfileLevelID == NULL) {
		char *str = (char *)malloc(6 + 1);
		encodeToHex(str, data + 1, 3, 0);
		mVideoConfig->ProfileLevelID = new string(str, 6);
	}

	if (mSPSInfo == NULL) {
		size_t len = ((size)+2) / 3 * 4 + 1;
		mSPSInfo = new string();
		encodeBase64(data, size, mSPSInfo);
	}

	return 0;
}

uint32_t AVCVideoConfig::parsePPS(uint8_t *data, size_t size) {
	printf("pps %x %x %x %x %x\n", data[0], data[1], data[2], data[3], data[4]);

	if (mPPSInfo == NULL) {
		size_t len = ((size)+2) / 3 * 4 + 1;
		mPPSInfo = new string();
		encodeBase64(data, size, mPPSInfo);
	}
	return 0;
}



