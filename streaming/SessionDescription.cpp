#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "SessionDescription.h"

SessionDescription::SessionDescription(const char *data, size_t size) {

    string *desc = new string(data);

    if (parsingSDP(desc, size) < 0) {
        return;
    }

    int64_t time;
    getDurationUs(&time);

    delete desc;
}

SessionDescription::~SessionDescription() {
}

bool SessionDescription::findAttribute(size_t index, string key, string *value) {

    value->clear();

    Attribs track = mTracks[index];
    map<string, string>::iterator it = track.find(key);

    if (it == track.end()) {
        return false;
    }

    *value = it->second;

    return true;
}

bool SessionDescription::parseNTPRange(
        const char *s, float *npt1, float *npt2) {
    if (s[0] == '-') {
        return false;  // no start time available.
    }

    if (!strncmp("now", s, 3)) {
        return false;  // no absolute start time available
    }

    char *end;
    *npt1 = strtof(s, &end);

    if (end == s || *end != '-') {
        // Failed to parse float or trailing "dash".
        return false;
    }

    s = end + 1;  // skip the dash.

    if (!strncmp("now", s, 3)) {
        return false;  // no absolute end time available
    }

    *npt2 = strtof(s, &end);

    if (end == s || *end != '\0') {
        return false;
    }

    return *npt2 > *npt1;
}

int SessionDescription::tracksCount() {
   return mTracks.size(); 
}

bool SessionDescription::getDurationUs(int64_t *durationUs) {
    string value;

    if (!findAttribute(0, "a=range", &value)) {
        return false;
    }

    if (strncmp(value.c_str(), "npt=", 4)) {
       return false;
    }

    float from, to;
    if (!parseNTPRange(value.c_str() + 4, &from, &to)) {
        return false;
    }

    *durationUs = (int64_t)((to - from) * 1E6);
    printf("duration %ld\n", *durationUs);

    return true;
}

int SessionDescription::parsingSDP(string *desc, size_t size) {
    string *tmpdesc = new string(desc->c_str());
//    printf("size %d\n", tmpdesc->size());
    printf("%s\n", desc->c_str());

    mTracks.push_back(Attribs());

    while (tmpdesc->size() > 0) {
        int32_t pos = tmpdesc->find("\r\n", 0);

        string *line = new string(tmpdesc->c_str(), pos);

        char c = (line->c_str())[0];

        switch (c) {
            case 'v':
            {
                if (strcmp(line->c_str(), "v=0")) {
                    return -1;
                }
                break;
            }
            case 'a':
            case 'b':
            {
                int32_t tmppos = line->find(":", 0);
                if (tmppos >= 0) {
                    string key(line->c_str(), tmppos);
                    if (key == "a=rtpmap") {
                        ssize_t spacePos = line->find(" ", tmppos + 1);
                        if (spacePos >= 0) {
                            tmppos = spacePos;
                        }
                    }
                    string value(line->c_str() + tmppos + 1, line->size() - tmppos - 1);
                    printf("key %s, value %s\n", key.c_str(), value.c_str());
                    mTracks[mTracks.size() - 1].insert(pair<string, string>(key, value));
                }
                break;
            }
            case 'm':
            {
                mTracks.push_back(Attribs());
                mFormat.push_back(string(line->c_str() + 2, line->size() - 2));
                break;
            }
            case 'o':
            case 's':
            case 'u':
            case 'e':
            case 'c':
            case 't':
                break;
            default:
                printf("Unknown type !\n");
        }
        tmpdesc->erase(0, pos + 2);
        delete line;       
    }

    delete tmpdesc;
    return 0;
}
