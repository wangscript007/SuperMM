TARGET = libSuperMM_utils.a

#FLAGS = -g -Wall -lpthread 
FLAGS := -g

#INC_PATH = -I ../prober \

SRC_FILES = Utils.cpp \
			BitsReader.cpp \
			MPEG4AudioConfig.cpp \
			AVCVideoConfig.cpp \

OBJS = $(SRC_FILES:.cpp=.o)

all:
	g++ -c ${SRC_FILES} ${FLAGS} ${INC_PATH} 
	ar rcs ${TARGET} ${OBJS}
	rm -rf *.o

clean:
	rm -rf ${TARGET} *.o
