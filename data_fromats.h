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
#define UDP_BUFFER_SIZE (100000000)

#define MAX_ORDER_MISSES (9)

#define RX_MSG_LEN (8192+8)

#define MAX_NO_ANTS (4)

#define DEFAULT_PORT 7000

#define DEFAULT_SAVE_PATTERN "recording"
#define FILE_EXTENSION "bin"
#define MAX_FILE 128

#define PRINT_DEBUG 0

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
