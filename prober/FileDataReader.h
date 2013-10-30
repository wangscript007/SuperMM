#ifndef _FILE_DATA_READER_H_

#define _FILE_DATA_READER_H_

#include <stdint.h>
#include <sys/types.h>

using namespace std;

class FileDataReader {
public:

    FileDataReader(const char *filename);
    ssize_t readAt(off64_t offset, void *data, size_t size);

    uint64_t getFileSize();

private:
    int mFd;
    int64_t mOffset;
    int64_t mLength;
};

#endif
