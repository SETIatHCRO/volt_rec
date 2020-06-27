#include "processing_thread.h"
#include "data_fromats.h"
#include "common.h"
#include <stdlib.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <unistd.h>
#include <stddef.h>

extern volatile sig_atomic_t keep_going;

void * worker_work(void * pointer)
{
        procDataStruct * data;
        data = (procDataStruct *) pointer;

        ssize_t indexIn, indexOut;
        char * input;
        ssize_t next_order;
        size_t dataLen;
        size_t dataLenOut;
        int msize;
        int no_missing,nOfPackets;
        int iK;
        work_local_struct localStruct[N_FILE_BUFFERS_AT_A_TIME];
        int first_loop_done = 0;
        uint64_t currHeader;

        uint32_t nAnts = data->fileBuff->nAnts;
        uint32_t nchans = data->fileBuff->nchans;
        uint32_t firstChan = data->fileBuff->firstChan;

        if ( (nAnts < 1) || (nAnts > MAX_NO_ANTS ) ||  (nchans < 1) || (nchans > MAX_CHANNEL_PACKETS) || (N_FILE_BUFFERS <= N_FILE_BUFFERS_AT_A_TIME ))
        {
                fprintf(stderr,"bad data, closing\n");
                kill(0,SIGINT);
                return NULL;
        }

        ssize_t packetsPerBlock = nAnts * nchans;
        //making sure the data is within limits

        for(iK = 0; iK < N_FILE_BUFFERS_AT_A_TIME; iK++)
        {
            if(fetchEmptyBuffer(&data->fileBuff->udpBuff,&localStruct[iK].data, &localStruct->index))
            {
                    struct timespec tt;
                    tt.tv_sec = 0;
                    tt.tv_nsec = 100;
                    nanosleep(&tt,NULL);
                    continue;
            }
            localStruct[iK].nToFull = packetsPerBlock;
            localStruct[iK].order = iK+1;
        }
        next_order = N_FILE_BUFFERS_AT_A_TIME+1;

        while(keep_going)
        {
                if(fetchBuffer(data->udpBuff, &input, &indexIn, &order, &dataLen))
                {
                        struct timespec tt;
                        tt.tv_sec = 0;
                        tt.tv_nsec = 50;
                        nanosleep(&tt,NULL);
                        continue;
                }
                //we have both buffers ready
                //printf("input %ld, order %ld\n", indexIn, order);
                nOfPackets = dataLen/RX_MSG_LEN;

                if (!nOfPackets) {
                        returnEmptyBuffer(data->udpBuff,indexIn);
                        continue;
                }
                if (!first_loop_done) {
                        //scanning what is the lowest packet ID

                        first_loop_done = 1;
                        for(iK = 0; iK < nOfPackets; iK++)
                        {
                                memcpy(&currHeader, input + iK*RX_MSG_LEN, sizeof(uint64_t));
                                printf("Ant: %d chan: %d pkt: %d version: %d\n",GET_ANT_NO(currHeader),GET_CHAN_NO(currHeader),GET_PCKT_NO(currHeader),GET_VERSION_NO(currHeader));
                        }
                        keep_going = false;
                }

                returnEmptyBuffer(data->udpBuff,indexIn);
        }

        for(iK = 0; iK < N_FILE_BUFFERS_AT_A_TIME; iK++)
        {
            returnEmptyBuffer(&data->fileBuff->udpBuff, localStruct->index);
        }
        return NULL;
}

