
#ifndef _REGISTER_H_

#define _REGISTER_H_

#include "BaseProber.h"
#include "FileDataReader.h"
#include <list>  

using namespace std; 

class Register {
public:

    Register(FileDataReader *reader);
    ~Register();

    void RegisterAllProber();

    typedef BaseProber *(*ProberFunc)(FileDataReader *reader);
    BaseProber *probe();

private:
    void RegisterProber(ProberFunc func);
    list<ProberFunc> mProberList;

    FileDataReader *mFileDataReader;

};

#endif
