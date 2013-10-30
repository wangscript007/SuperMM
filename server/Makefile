TARGET = libSuperMM_server.a

FLAGS = -g

INC_PATH = -I ../utils \
		   -I ../prober \

SRC_FILES = RTSPServer.cpp \
			SDPData.cpp \
			RTPSender.cpp \
			Packetizer.cpp \

OBJS = $(SRC_FILES:.cpp=.o)

all:
	g++ -c ${SRC_FILES} ${FLAGS} ${INC_PATH} 
	ar rcs ${TARGET} ${OBJS}
	rm -rf *.o

clean:
	rm -rf ${TARGET} *.o
