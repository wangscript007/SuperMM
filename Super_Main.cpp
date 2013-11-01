#include <iostream>
#include <stdio.h>
#include <string.h>
#include "Register.h"
#include "FileDataReader.h"
#include "RTSPDataReader.h"
#include "RTSPServer.h"
#include "Utils.h"
#include <stdlib.h>
using namespace std; 

int main(int argc,char *argv[]) {

    if (!strncmp(argv[1], "rtspserver", 10)) {
        printf("Start server ! \n");
        startServer();
    } else if (!strncmp(argv[1], "test", 4)) {
        printf("Test some function here !!!!!!!!!!!!!!!!\n");
	 const uint8_t src[4] = {99,105,224,7};
	 char *buff = (char *)malloc(8 + 1);
	 buff = encodeToHex(buff, src, 4, 1);
	 printf("%s\n", buff);
	 uint8_t *data = (uint8_t *)malloc(4);
	 data = decodeFromHex(data, buff, 8);
	 printf("%d, %d, %d, %d\n", data[0], data[1], data[2], data[3]);
    } else {
        const char *url = argv[1];
        printf("file name :%s\n",argv[1]);

        FileDataReader *filereader = NULL;
        Register *reg = NULL;
        BaseProber *prober = NULL;
        RTSPDataReader *streamreader = NULL;

        if (!strncmp(url, "rtsp://", 7)) {
            streamreader = new RTSPDataReader(url);

            delete streamreader;
        } else {
            filereader = new FileDataReader(url); //New one FileDataReader instance.

            reg = new Register(filereader); //New one Register instance.

            prober = reg->probe();
            if (prober == NULL) {
                printf("Have not found the required prober !\n");
                return -1;

                delete filereader;
                delete reg;
            }
            prober->FileParse();
            prober->InfoDump();

/*            while(1) {
                DataBuffer *buff = new DataBuffer();
                prober->readData(1, buff);
                if (buff == NULL) {
                    delete buff;
                    break;
                }
                printf("buff->size %d\n", buff->size);
                free(buff->data);
                delete buff;
            }
*/
            delete filereader;
            delete reg;
        }
    }
    return 0;
}
