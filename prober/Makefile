TARGET = libSuperMM_prober.a

#FLAGS = -g -Wall -lpthread -lSuperMM_utils 
FLAGS := -g

INC_PATH = -I ../utils \

SRC_FILES = BaseProber.cpp \
			Register.cpp \
			FileDataReader.cpp \
			MP3FileProbeInfo.cpp \
			MP4FileProbeInfo.cpp \
			APEFileProbeInfo.cpp \
			AVIFileProbeInfo.cpp \
			FLVFileProbeInfo.cpp \
			MKVFileProbeInfo.cpp \
			ASFFileProbeInfo.cpp \
			WAVEFileProbeInfo.cpp \
			FLACFileProbeInfo.cpp \
			SampleTables.cpp \

OBJS = $(SRC_FILES:.cpp=.o)

all:
	g++ -c ${SRC_FILES} ${FLAGS} ${INC_PATH} 
	ar rcs ${TARGET} ${OBJS}
	rm -rf *.o

clean:
	rm -rf ${TARGET} *.o
