#ifndef __DATA_FORMATS_RECORDER__
#define __DATA_FORMATS_RECORDER__

#include <pthread.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <time.h>
#include <stdint.h>

#define PACKET_HEADER_SIZE (64)
#define N_READS_DEFAULT (32*5)
#define N_UDP_BUFFERS (10)
#define N_FILE_BUFFERS (20)
#define N_FILE_BUFFERS_AT_A_TIME (6)
//this should be RX_MSG_LEN * N_READS_DEFAULT
#define UDP_BUFFER_SIZE (1000000000)
//this should be MAX_NO_ANTS * RX_DATA_LEN * MAX_CHANNEL_PACKETS * TIME_INT_PER_BUFFER
#define FILE_BUFFER_SIZE (1000000000)

#define MAX_ORDER_MISSES (200)

//should be 2^n
#define TIME_INT_PER_BUFFER 128
#define GET_PACKET_FILTER(x) (x & ~(0x07f))
#define GET_PACKET_REST(x) (x & 0x07f)

#define RX_DATA_LEN (8192)
#define RX_BUFF_LEN (8)
#define RX_MSG_LEN (RX_DATA_LEN+RX_BUFF_LEN)

#define MAX_NO_ANTS (4)
#define MAX_CHANNEL_PACKETS (32)

#define DEFAULT_PORT 4015

#define SAMPLES_PER_PACKET (16)
#define POLS_PER_PACKET (2)
#define CHANS_PER_PACKET (256)

#define DEFAULT_SAVE_PATTERN "recording"
#define FILE_EXTENSION "bin"
#define MAX_FILE 128

#define PRINT_DEBUG 0

#define ANT_NO_MASK (0x003f)
#define CHAN_NO_MASK (0x0fff)
#define PCKT_NO_MASK (0x03fffffffff)
#define VERSION_NO_MASK (0x00ff)
#define VOLT_VERSION_MASK (0x0080)

#define GET_ANT_NO(x) ((x) & ANT_NO_MASK)
#define GET_CHAN_NO(x) ((x >> 6) & CHAN_NO_MASK)
#define GET_PCKT_NO(x) ((x >> 18) & PCKT_NO_MASK)
#define GET_VERSION_NO(x) ((x >> 56) & VERSION_NO_MASK)
#define IS_VERSION_VOLT(x) ((x >> 56) & VOLT_VERSION_MASK)

#ifndef MIN
#define MIN(a,b) (((a)<(b))?(a):(b))
#endif
#ifndef MAX
#define MAX(a,b) (((a)>(b))?(a):(b))
#endif

typedef struct __udpBuffStruct
{
        char ** data;
        size_t* dataLen;
        ssize_t * orderNo;
        pthread_mutex_t dataMutex;
        //probably cond variable would be better here, but lets start with that
        pthread_mutex_t NReadyMutex;
        size_t NReady;
        int noBuffers;
} udpBuffStruct;

typedef struct __fileBuffStruct
{
        uint32_t nAnts;
        uint32_t antNumbers[MAX_NO_ANTS];
        uint32_t nchans;
        uint32_t firstChan;
        uint64_t packetStart;
        udpBuffStruct udpBuff;
} fileBuffStruct;


typedef struct __udpDataStruct
{
    udpBuffStruct * udpBuff;
    short port;
} udpDataStruct;

typedef struct __processorDataStruct
{
    udpBuffStruct * udpBuff;
    fileBuffStruct * fileBuff;
} procDataStruct;

typedef struct __fileDataStruct
{
        fileBuffStruct * fileBuff;
        char file_part[MAX_FILE];
} fileDataStruct;

#endif
