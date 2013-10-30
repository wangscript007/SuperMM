#ifndef _AVC_VIDEO_CONFIG_H_
#define _AVC_VIDEO_CONFIG_H_

#include <stdint.h>
#include <sys/types.h>
#include <stddef.h>
#include "BitsReader.h"

#include <string>

using namespace std;

typedef struct {
	string *ProfileLevelID;
	string *SpropParameterSets;
}VideoConfig;

class AVCVideoConfig {
public:
	AVCVideoConfig();
	~AVCVideoConfig();

    VideoConfig *parseVideoConfig(uint8_t *data, size_t size);
    uint32_t parseSPS(uint8_t *data, size_t size);
    uint32_t parsePPS(uint8_t *data, size_t size);
private:
	VideoConfig *mVideoConfig;
	string*mSPSInfo;
	string*mPPSInfo;
};

#endif
