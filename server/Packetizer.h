#include <stdint.h>
#include <list>
#include <sys/types.h>
#include "DataBuffer.h"

using namespace std;

typedef struct {
    //Little Endian
    uint8_t cc:4;      //CSRC count
    uint8_t x:1;       //Header extension flag
    uint8_t p:1;       //Padding flag
    uint8_t v:2;       //Version

    uint8_t pt:7;      //Payload type
    uint8_t m:1;       //Marker bit

    uint16_t seq;      //Sequence num
    uint32_t ts;       //Time stamp
    uint32_t ssrc;     //SSRC
    uint32_t *csrc;    //CSRC list
}RTPHeader;

/****************************************************************** 
NALU_HEADER 
+---------------+ 
|0|1|2|3|4|5|6|7| 
+-+-+-+-+-+-+-+-+ 
|F|NRI|  Type   | 
+---------------+ 
******************************************************************/  
typedef struct {  
    unsigned char TYPE:5;  
    unsigned char NRI:2;  
    unsigned char F:1;  
} NALUHeader; /* 1 byte */  

typedef struct {  
    int startcodeprefix_len;      //! 4 for parameter sets and first slice in picture, 3 for everything else (suggested)  
    size_t len;                 //! Length of the NAL unit (Excluding the start code, which does not belong to the NALU)  
    size_t max_size;            //! Nal Unit Buffer size  
    int forbidden_bit;            //! should be always FALSE  
    int nal_reference_idc;        //! NALU_PRIORITY_xxxx  
    int nal_unit_type;            //! NALU_TYPE_xxxx      
    uint8_t *buf;                    //! contains the first byte followed by the EBSP  
    uint16_t lost_packets;  //! true, if packet loss is detected  
} NALU_t;  

/****************************************************************** 
FU_INDICATOR 
+---------------+ 
|0|1|2|3|4|5|6|7| 
+-+-+-+-+-+-+-+-+ 
|F|NRI|  Type   | 
+---------------+ 
******************************************************************/  
typedef struct {  
    unsigned char TYPE:5;  
    unsigned char NRI:2;   
    unsigned char F:1;           
} FU_INDICATOR; /*1 byte */  

/****************************************************************** 
FU_HEADER 
+--------------+ 
|0|1|2|3|4|5|6|7| 
+-+-+-+-+-+-+-+-+ 
|S|E|R|  Type   | 
+---------------+ 
******************************************************************/  
typedef struct {  
    unsigned char TYPE:5;  
    unsigned char R:1;  
    unsigned char E:1;  
    unsigned char S:1;      
} FU_HEADER; /* 1 byte */  

class Packetizer {
public:
    Packetizer(const char *format, int index, uint32_t ssrc);
    ~Packetizer();

    struct PacketData{
        uint8_t *data;
        size_t size;
    };

    int packetize(DataBuffer *&buff);
    int AVCPacketize(DataBuffer *&buff);
    int getAnnexbNALU(NALU_t *nalu, uint8_t *data, size_t size, bool flag);
    int AACPacketize(DataBuffer *&buff);
    int getPacketData(uint8_t *&data, size_t *size);
    int makeRTPHeader(uint8_t *&header, uint32_t ts);
    int getPayloadType();
	int setNALLengthSize(int len);
	size_t parseNALLength(const uint8_t *data);

private:
    const char *mFormat;
    uint32_t mSequenceNum;
    uint32_t mSSRC;
    int mIndex;
	int mNALLengthSize;

    list<PacketData> mPacketList;
};
