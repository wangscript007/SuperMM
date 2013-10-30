TARGET = libSuperMM_streaming.a

#FLAGS = -g -Wall -lpthread -lSuperMM_utils 
FLAGS := -g

INC_PATH = -I ../utils \

SRC_FILES = RTSPDataReader.cpp \
			SessionDescription.cpp \
			RTPConnection.cpp \

OBJS = $(SRC_FILES:.cpp=.o)

all:
	g++ -c ${SRC_FILES} ${FLAGS} ${INC_PATH} 
	ar rcs ${TARGET} ${OBJS}
	rm -rf *.o

clean:
	rm -rf ${TARGET} *.o
