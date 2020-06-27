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
#define N_FILE_BUFFERS_AT_A_TIME (10)
//this should be RX_MSG_LEN * N_READS_DEFAULT
#define UDP_BUFFER_SIZE (100000000)
//this should be MAX_NO_ANTS * RX_DATA_LEN * MAX_CHANNEL_PACKETS
#define FILE_BUFFER_SIZE (100000000)

#define MAX_ORDER_MISSES (9)

#define RX_DATA_LEN (8192)
#define RX_MSG_LEN (RX_DATA_LEN+8)

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

#define GET_ANT_NO(x) (x & ANT_NO_MASK)
#define GET_CHAN_NO(x) ((x >> 6) & CHAN_NO_MASK)
#define GET_PCKT_NO(x) ((x >> 18) & PCKT_NO_MASK)
#define GET_VERSION_NO(x) ((x >> 56) & VERSION_NO_MASK)


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
