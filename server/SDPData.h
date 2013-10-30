#ifndef _SDP_DATA_H_
#define _SDP_DATA_H_

#include <string>
#include "BaseProber.h"
#include "MPEG4AudioConfig.h"
#include "AVCVideoConfig.h"

using namespace std;

class SDPData {
public:
    SDPData(BaseProber *prober, string url);

    string getSDPData();
	int makeVideoAttribute();
	int makeAudioAttribute();

    string makeMediaAttribute(int index, int pt);
    int getPayloadInfo(string format, int index);

private:
    BaseProber *mBaseProber;
    string mSDPData;
	VideoConfig *mVideoConfigInfo;
	AudioConfig *mAudioConfigInfo;
};

#endif
