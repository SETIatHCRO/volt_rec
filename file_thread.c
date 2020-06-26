#include "file_thread.h"
#include "data_fromats.h"
#include "common.h"
#include <stdlib.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <unistd.h>
#include <stddef.h>
#include <fcntl.h>

extern volatile sig_atomic_t keep_going;

void * file_writer_work(void * pointer)
{
    fileDataStruct * data;
    data = (fileDataStruct *) pointer;

    int consec_file = 0;
    int nMisses = 0;
    ssize_t lastOrd = 0, order, indexIn;
    size_t dataLen;
    char * input;
    char currFileName[MAX_FILE+20];
    int file_int;
    ssize_t wcount;
    snprintf(currFileName,MAX_FILE+20,"%s_%d.%s",data->file_part,consec_file,FILE_EXTENSION);


    if((file_int = open(currFileName,O_CREAT|O_WRONLY|O_TRUNC|S_IWUSR|S_IRUSR,0644)) < 0) error("open");
    while (keep_going || anyBufFull(data->fileBuff->udpBuff))
    {
        if(fetchBuffer(&data->fileBuff->udpBuff, &input, &indexIn, &order, &dataLen))
        {
                continue;
        }
        if (lastOrd+1 != order)
        {
            if (order <= lastOrd)
            {
                //we lost some data. that buffer should be earlier but we missed it.
                //we won't go back. report an error and discard the data
                fprintf(stderr,"lowest normalized id: %ld; my currend id %ld. Discarding data\n", order, lastOrd);
                returnEmptyBuffer(&data->fileBuff->udpBuff, indexIn);
                continue;
            }
            //we need to put it back and wait once again for the data, but we increase miss count
            nMisses++;
            if(nMisses > MAX_ORDER_MISSES) {
                //starting new file
                nMisses = 0;
                close(file_int);
                consec_file++;
                snprintf(currFileName,MAX_FILE+20,"%s_%d.%s",data->file_part,consec_file,FILE_EXTENSION);
                if((file_int = open(currFileName,O_CREAT|O_WRONLY|O_TRUNC|S_IWUSR|S_IRUSR,0644)) < 0) error("open");
                if(writeHeader(file_int,data->fileBuff)) error("writeHeader");
            }
            else
            {
                //putting data back
                returnBuffer(&data->fileBuff->udpBuff, indexIn, order, dataLen);
                //and sleeping
                struct timespec tt;
                tt.tv_sec = 0;
                tt.tv_nsec = 100;
                nanosleep(&tt,NULL);
                continue;
            }
        } else {
            //resseting miss count
            nMisses = 0;
        }
        if(lastOrd == 0) { //first package
            if(writeHeader(file_int,data->fileBuff)) error("writeHeader");
        }
        lastOrd = order;
        printf("writing %ld\n",dataLen);
        if((wcount =  write(file_int,input,dataLen))<dataLen) error("write");

        returnEmptyBuffer(&data->fileBuff->udpBuff, indexIn);
    }
    close(file_int);

    return NULL;
}

int writeHeader(int fd, fileBuffStruct * structure)
{
    if(write(fd,&structure->nAnts,sizeof(uint32_t))<sizeof(uint32_t)) error("write");
    if(write(fd,structure->antNumbers,sizeof(uint32_t)*structure->nAnts)<(structure->nAnts* sizeof(uint32_t))) error("write");
    if(write(fd,&structure->nchans,sizeof(uint32_t))<sizeof(uint32_t)) error("write");
    if(write(fd,&structure->packetStart,sizeof(uint64_t))<sizeof(uint64_t)) error("write");
    return 0;
}
