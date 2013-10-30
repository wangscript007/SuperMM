#ifndef _SESSION_DESCRIPTION_H_

#define _SESSION_DESCRIPTION_H_

#include <stdint.h>
#include <sys/types.h>
#include <string>
#include <map>
#include <vector>

using namespace std;

class SessionDescription {
public:

    SessionDescription(const char *data, size_t size);
    ~SessionDescription();

    int parsingSDP(string *data, size_t size);
    bool getDurationUs(int64_t *durationUs);
    bool parseNTPRange(const char *s, float *npt1, float *npt2);
    bool findAttribute(size_t index, string key, string *value);
    int tracksCount();
private:
    typedef map<string, string> Attribs;
    vector<string> mFormat;
    vector<Attribs> mTracks;
};

#endif
