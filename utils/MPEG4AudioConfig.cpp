#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "MPEG4AudioConfig.h"
#include "BitsReader.h"
#include "Utils.h"

MPEG4AudioConfig::MPEG4AudioConfig()
	:mAudioConfig(NULL){
	mAudioConfig = (AudioConfig *)malloc(sizeof(AudioConfig));
	memset(mAudioConfig, 0, sizeof(AudioConfig));
}

MPEG4AudioConfig::~MPEG4AudioConfig() {
	if(mAudioConfig != NULL) {
		free(mAudioConfig);
	}
}

AudioConfig *MPEG4AudioConfig::parseAudioConfig (uint8_t *data, size_t size, int syncextension) {
//	printf("------------   size %d\n", size);
	BitsReader *reader = new BitsReader(data, size);

	mAudioConfig->ObjectType = getObjectType(reader);

	mAudioConfig->SamplingIndex = reader->getBits(4);
	mAudioConfig->SampleRate = getSampleRate(reader, mAudioConfig->SamplingIndex);

	mAudioConfig->ChanConfig = reader->getBits(4);

	if (mAudioConfig->ObjectType == AOT_SBR || (mAudioConfig->ObjectType == AOT_PS && ! (reader->showBits(6) & 0x03 && !(reader->showBits(9) & 0x3F)))) {
		// check for W6132 Annex YYYY draft MP3onMP4
		if (mAudioConfig->ObjectType == AOT_PS) {
			mAudioConfig->PS = 1;
		}
		mAudioConfig->ExtObjectType = AOT_SBR;
		mAudioConfig->SbrFlag= 1;

		mAudioConfig->ExtSamplingIndex = reader->getBits(4);
		mAudioConfig->ExtSampleRate = getSampleRate(reader, mAudioConfig->ExtSamplingIndex);

		mAudioConfig->ObjectType = getObjectType(reader);
		if (mAudioConfig->ObjectType == AOT_ER_BSAC) {
			mAudioConfig->ExtChanConfig = reader->getBits(4);
		}
	} else {
		mAudioConfig->ExtObjectType = AOT_NULL;
		mAudioConfig->ExtSampleRate = 0;
	}

    if (mAudioConfig->ObjectType == AOT_ALS) {
		reader->skipBits(5);
		if (reader->showLongBits(24) != MKBETAG('\0','A','L','S')) {
			reader->skipBits(24);
		}

		if (parseConfigALS(reader)) {
		      return NULL;
		}
	}

	if (mAudioConfig->ExtObjectType != AOT_SBR && syncextension) {
		while (reader->getBitsLeft() > 15) {
			if (reader->showBits(11) == 0x2b7) { // sync extension
				reader->getBits(11);
				mAudioConfig->ExtObjectType = getObjectType(reader);
				if (mAudioConfig->ExtObjectType == AOT_SBR && (mAudioConfig->SbrFlag = reader->getBits(1)) == 1) {
					mAudioConfig->ExtSampleRate = getSampleRate(reader, mAudioConfig->ExtSamplingIndex);
					if (mAudioConfig->ExtSampleRate == mAudioConfig->SampleRate)
						mAudioConfig->SbrFlag = -1;
				}
				if (reader->getBitsLeft() > 11 && reader->getBits(11) == 0x548)
					mAudioConfig->PS = reader->getBits(1);
				break;
			} else {
				reader->getBits(1); // skip 1 bit
			}
		}
	}

	//PS requires Sbrflag
	if (!mAudioConfig->SbrFlag) {
		mAudioConfig->PS = 0;
	}
	//Limit implicit PS to the HE-AACv2 Profile
	if ((mAudioConfig->PS == -1 && mAudioConfig->ObjectType != AOT_AAC_LC) || mAudioConfig->Channels & ~0x01) {
		mAudioConfig->PS = 0;
	}
//	printf("&&&&left number %d\n", reader->getBitsLeft());
//////////////////////////////////////////////////////////////////////////////////
	//get profile level.
	getLatmContext2ProfileLevel();
//	getLatmContext2Config();
	getExtraData2Config(data, size);
    return mAudioConfig;
}

int MPEG4AudioConfig::getObjectType(BitsReader *reader) {
	int ObjectType = reader->getBits(5);
	if (ObjectType == AOT_ESCAPE) {
		int ExtObjectType = reader->getBits(6);
		ObjectType = 32 + ExtObjectType;
	}
	
	return ObjectType;
}

int MPEG4AudioConfig::getSampleRate(BitsReader *reader, int index) {
	const int mpeg4audio_sample_rates[16] = {
		96000, 88200, 64000, 48000, 44100, 32000,
	    24000, 22050, 16000, 12000, 11025, 8000, 7350
	};

	int samplerate;
	if (index == 0xf) {
		samplerate = reader->getBits(24);
	} else {
		samplerate = mpeg4audio_sample_rates[index];
	}

	return samplerate;
}

int MPEG4AudioConfig::parseConfigALS(BitsReader *reader) {
	if (reader->getBitsLeft() < 112) {
	    return -1;
	}

    if (reader->getLongBits(32) != MKBETAG('A','L','S','\0')) {
	    return -1;
	}

	// override AudioSpecificConfig channel configuration and sample rate
	// which are buggy in old ALS conformance files
	mAudioConfig->SampleRate = reader->getLongBits(32);

	// skip number of samples
	reader->skipBits(32);

	//read number of channels
	mAudioConfig->ChanConfig = 0;
	mAudioConfig->Channels = reader->getBits(16) + 1;

	return 0;
}

int MPEG4AudioConfig::getLatmContext2ProfileLevel() {
	int profilelevel = 0x2b;

	if (mAudioConfig->SampleRate <= 24000) {
		if (mAudioConfig->Channels <= 2) {
			profilelevel = 0x28;
		}
	} else if (mAudioConfig->SampleRate <= 48000) {
		if (mAudioConfig->Channels <= 2) {
			profilelevel = 0x29;
		} else if (mAudioConfig->Channels <= 5) {
			profilelevel = 0x2a;
		}
	} else if (mAudioConfig->SampleRate <= 96000) {
		if (mAudioConfig->Channels <= 5) {
			profilelevel = 0x2b;
		}
	}

	mAudioConfig->ProfileLevel = profilelevel;
	return 0;
}

int MPEG4AudioConfig::getLatmContext2Config() {
	uint8_t config[6];

	config[0] = 0x40;
	config[1] = 0;
	config[2] = 0x20 | mAudioConfig->SamplingIndex;
	config[3] = mAudioConfig->Channels << 4;
	config[4] = 0x3f;
	config[5] = 0xc0;
printf("mAudioConfig->Channels %d\n", mAudioConfig->Channels);

	mAudioConfig->ConfigStr = (char *)malloc(6*2+1);

	encodeToHex(mAudioConfig->ConfigStr, config, 6, 1);

	return 0;
}

int MPEG4AudioConfig::getExtraData2Config(uint8_t *data, size_t size) {

    mAudioConfig->ConfigStr = (char *)malloc(size * 2 + 1);

    encodeToHex(mAudioConfig->ConfigStr, data, size, 0);

    return 0;
}

