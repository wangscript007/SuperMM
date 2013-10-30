#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

#include "FileDataReader.h"

FileDataReader::FileDataReader(const char *filename) 
    :mFd(-1),
     mOffset(0),
     mLength(-1){
    mFd = open(filename, O_LARGEFILE | O_RDONLY);
    if (mFd >= 0) {
        mLength = lseek(mFd, 0, SEEK_END);
    } else {
        printf("Can not find the file:%s\n",filename);
    }
}

ssize_t FileDataReader::readAt(off64_t offset, void *data, size_t size) {
    if (mFd < 0) {
        return 0;
    }

    if (mLength >= 0) {
        if (offset >= mLength) {
            return 0;
        }

        int64_t available = mLength - offset;
        if ((int64_t)size > available) {
            size = available;
        }
    }

    off64_t result = lseek64(mFd, offset + mOffset, SEEK_SET);
    if (result == -1) {
        return 0;
    }

    return read(mFd, data, size);
}

uint64_t FileDataReader::getFileSize() {
    return mLength;
}
