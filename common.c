#include "common.h"
#include <limits.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <errno.h>
#include <time.h>

extern volatile sig_atomic_t keep_going;

void error(char *message)
{
        perror(message);
        kill(0, SIGTERM);
        exit(EXIT_FAILURE);
}

int destroy_udpBuff(udpBuffStruct* structure)
{
    if(pthread_mutex_destroy(&structure->dataMutex)) error("mutexD");
    if(pthread_mutex_destroy(&structure->NReadyMutex)) error("mutexD");
    free(structure->orderNo);
    free(structure->dataLen);
    for(iK = 0; iK < structure->noBuffers; iK++)
    {
            free(structure->data[iK]);
            free(structure->firstHeader[iK]);
    }
    free(structure->data);
    free(structure->firstHeader);
    return 1;
}

int init_udpBuff(udpBuffStruct* structure, int number_of_buffers, ssize_t bufsize)
{
    if(pthread_mutex_init(&structure->dataMutex,NULL)) error("mutexI");
    if(pthread_mutex_init(&structure->NReadyMutex,NULL)) error("mutexI");
    if((structure->data = (char **)malloc(sizeof(char*)*number_of_buffers)) == NULL) error("malloc");
    for(iK = 0; iK < number_of_buffers; iK++)
    {
        if ((structure->data[iK] = (char *) malloc (bufsize*sizeof(char))) == NULL) error("malloc");
    }
    if((structure->dataLen = (size_t *)malloc(sizeof(size_t)*number_of_buffers)) == NULL) error("malloc");
    if((structure->orderNo = (ssize_t *)malloc(sizeof(ssize_t)*number_of_buffers)) == NULL) error("malloc");
    if((structure->lastPacketNo= (unsigned int *)malloc(sizeof(unsigned int)*number_of_buffers)) == NULL) error("malloc");
    memset(structure->orderNo,0,sizeof(ssize_t)*number_of_buffers);
    structure->NReady = 0;
    structure->noBuffers = number_of_buffers;
    return 1;
}

ssize_t findEmpty(ssize_t * no, size_t buffLen)
{
        ssize_t retVal = -1;
        size_t iK;
        for (iK = 0; iK< buffLen; iK++)
        {
                if(no[iK] == 0)
                {
                        retVal = iK;
                        break;
                }
        }
        return retVal;
}

ssize_t findLowest(ssize_t * no, size_t buffLen)
{
        ssize_t retVal = -1;
        size_t iK;
        ssize_t lowVal = INT_MAX;
        for (iK = 0; iK< buffLen; iK++)
        {
                if(no[iK] < lowVal &&  no[iK] > 0)
                {
                        lowVal = no[iK];
                        retVal = iK;
                }
        }
        return retVal;
}

int fetchEmptyBuffer(udpBuffStruct* structure,char ** buff, ssize_t * index)
{
        if(pthread_mutex_lock(&structure->dataMutex)) error("lock");
        *index = findEmpty(structure->orderNo,N_UDP_BUFFERS);
        if(*index == -1)
        {
            if(pthread_mutex_unlock(&structure->dataMutex)) error("unlock");
            return -1;
        }
        *buff = structure->data[*index];
        structure->orderNo[*index] = -1; //mark as in use
        if(pthread_mutex_unlock(&structure->dataMutex)) error("unlock");
        return 0;
}

int anyBufFull(udpBuffStruct structure)
{
    int localNready;
    if(pthread_mutex_lock(&structure->NReadyMutex)) error("lock");
    localNready = structure->NReady++;
    if(pthread_mutex_unlock(&structure->NReadyMutex)) error("unlock");
    return localNready;
}

int returnBuffer(udpBuffStruct* structure, ssize_t index, ssize_t order, size_t dataLen)
{
        if(pthread_mutex_lock(&structure->dataMutex)) error("lock");
        if (structure->orderNo[index] != -1) error ("data integrity!");
        structure->orderNo[index] = order;
        structure->dataLen[index] = dataLen;
        if(pthread_mutex_lock(&structure->NReadyMutex)) error("lock");
        structure->NReady++;
        if(pthread_mutex_unlock(&structure->NReadyMutex)) error("unlock");
        if(pthread_mutex_unlock(&structure->dataMutex)) error("unlock");
        return 0;
}

int returnEmptyBuffer(udpBuffStruct* structure, ssize_t index)
{
    if(pthread_mutex_lock(&structure->dataMutex)) error("lock");
    if(structure->orderNo[index] != -1) error ("data integrity!");
    structure->orderNo[index] = 0;
    if(pthread_mutex_unlock(&structure->dataMutex)) error("unlock");
    return 0;
}

int fetchBuffer(udpBuffStruct* structure, char **buff, ssize_t * index, ssize_t * order, size_t * dataLen)
{
    struct timespec tt;
    tt.tv_sec = 0;
    tt.tv_nsec = 1000;
    int do_wait_more = 1;
    while (do_wait_more) {
        if(pthread_mutex_lock(&structure->NReadyMutex)) error("lock");
        if(structure->NReady > 0)
        {
            do_wait_more = 0;
            structure->NReady--;
            if(pthread_mutex_unlock(&structure->NReadyMutex)) error("unlock");
            break;
        }
        else if (!keep_going)
        {
            if(pthread_mutex_unlock(&structure->NReadyMutex)) error("unlock");
            return -1;
        }
        if(pthread_mutex_unlock(&structure->NReadyMutex)) error("unlock");
        nanosleep(&tt,NULL);

    if(pthread_mutex_lock(&structure->dataMutex)) error("lock");
    *index = findLowest(structure->orderNo,N_UDP_BUFFERS);
    if(*index == -1)
    {
        if(pthread_mutex_unlock(&structure->dataMutex)) error("unlock");
        return -1;
    }
    *buff = structure->data[*index];
    *order = structure->orderNo[*index];
    *dataLen = structure->dataLen[*index];
    structure->orderNo[*index] = -1; //mark as in use
    if(pthread_mutex_unlock(&structure->dataMutex)) error("unlock");
    return 0;
}
