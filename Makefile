TARGET = SuperMM
FLAGS = -g -Wall -lpthread

#MACROS = -DIS_X86_32BIT

SRC_FILES = Super_Main.cpp \

INC_PATH = -I streaming \
		   -I prober \
		   -I utils \
		   -I server \

#LIB_PATH = -L src/server \
		   -L src/streaming \
		   -L src/utils \
		   -L src/prober \

#subsystem:
#	cd server && ${MAKE}
#	cd streaming && ${MAKE}
#	cd prober && ${MAKE}
#	cd utils && ${MAKE}

LIBS = server/libSuperMM_server.a \
	   utils/libSuperMM_utils.a \
	   streaming/libSuperMM_streaming.a \
	   prober/libSuperMM_prober.a \

all :
	cd server && ${MAKE}
	cd streaming && ${MAKE}
	cd prober && ${MAKE}
	cd utils && ${MAKE}
	g++ $(SRC_FILES) ${LIBS} -o ${TARGET} ${FLAGS} ${INC_PATH}

#${LIB_PATH}

clean:
	rm -rf ${TARGET} *.o
