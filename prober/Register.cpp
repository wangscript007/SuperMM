#include <stdio.h>
#include "Register.h"
#include "MP3FileProbeInfo.h"
#include "APEFileProbeInfo.h"
#include "AVIFileProbeInfo.h"
#include "FLVFileProbeInfo.h"
#include "MP4FileProbeInfo.h"
#include "MKVFileProbeInfo.h"
#include "ASFFileProbeInfo.h"
#include "WAVEFileProbeInfo.h"
#include "FLACFileProbeInfo.h"
#include "RMVBFileProbeInfo.h"

Register::Register(FileDataReader *reader)
    :mFileDataReader(reader){
    RegisterAllProber();
}

void Register::RegisterProber(ProberFunc func) {
    //First check whether prober already in the list.
    list<ProberFunc>::iterator it = mProberList.begin();
    while (it != mProberList.end()) {
        if (*it == func) {
            return;
        }
        it++;
    }

    mProberList.push_back(func);
}

Register::~Register(){

}

void Register::RegisterAllProber() {

    RegisterProber(ProbeMP3);
    RegisterProber(ProbeAPE);
    RegisterProber(ProbeAVI);
    RegisterProber(ProbeFLV);
    RegisterProber(ProbeMP4);
    RegisterProber(ProbeMKV);
    RegisterProber(ProbeASF);
    RegisterProber(ProbeWAVE);
    RegisterProber(ProbeFLAC);
    RegisterProber(ProbeRMVB);
}

BaseProber *Register::probe() {
    BaseProber *prober = NULL;

    printf("Try to get one prober !\n");
    list<ProberFunc>::iterator it = mProberList.begin();
        
    while (it != mProberList.end()) {
        prober = (*it)(mFileDataReader);
        if (prober != NULL) {
            return prober;
        }
        it++;
    }

    return NULL;
}
