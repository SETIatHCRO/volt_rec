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
#include <endian.h>

extern volatile sig_atomic_t keep_going;

void * worker_work(void * pointer)
{
        procDataStruct * data;
        data = (procDataStruct *) pointer;

        ssize_t indexIn;
        char * input;
        ssize_t next_order;
        size_t dataLen;
        size_t dataLenOut;
        int no_missing,nOfPackets;
        int iK;
        ssize_t order;
        work_local_struct localStruct[N_FILE_BUFFERS_AT_A_TIME];
        int first_loop_done = 0;
        uint64_t currHeader;

        uint32_t nAnts = data->fileBuff->nAnts;
        uint32_t nchans = data->fileBuff->nchans;
        uint32_t firstChan = data->fileBuff->firstChan;
        uint32_t antNumbers[MAX_NO_ANTS];

        if ( (nAnts < 1) || (nAnts > MAX_NO_ANTS ) ||  (nchans < 1) || (nchans > MAX_CHANNEL_PACKETS) || (N_FILE_BUFFERS <= N_FILE_BUFFERS_AT_A_TIME ))
        {
                fprintf(stderr,"bad data, closing\n");
                kill(0,SIGINT);
                return NULL;
        }

        for(iK = 0; iK < MAX_NO_ANTS; iK++)
        {
            antNumbers[iK] = -1;
        }

        ssize_t packetsPerBlock = nAnts * nchans * TIME_INT_PER_BUFFER;
        //making sure the data is within limits
        dataLenOut = packetsPerBlock * RX_DATA_LEN;
        for(iK = 0; iK < N_FILE_BUFFERS_AT_A_TIME; iK++)
        {
            if(fetchEmptyBuffer(&data->fileBuff->udpBuff,&localStruct[iK].data, &localStruct[iK].index))
            {
                    struct timespec tt;
                    tt.tv_sec = 0;
                    tt.tv_nsec = 100;
                    nanosleep(&tt,NULL);
                    continue;
            }
            memset(localStruct[iK].data,0,dataLenOut*sizeof(char));
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

                        uint64_t pkt_start = LONG_MAX;
                        for(iK = 0; iK < nOfPackets; iK++)
                        {
                                //memcpy(&currHeader, input + iK*RX_MSG_LEN, sizeof(uint64_t));
                                //currHeader = be64toh(currHeader);
                                currHeader = be64toh( *((uint64_t *)(input + iK*RX_MSG_LEN)));
                                pkt_start = MIN(pkt_start,GET_PCKT_NO(currHeader));
                                if(updateAntenna(antNumbers, GET_ANT_NO(currHeader), nAnts) == -1)
                                {
                                    fprintf(stderr,"specified %d antennas, got more (curr Ant Number %ld) \n", nAnts, GET_ANT_NO(currHeader));
                                }
                                printf("Ant: %ld chan: %ld pkt: %ld version: %ld\n",GET_ANT_NO(currHeader),GET_CHAN_NO(currHeader),GET_PCKT_NO(currHeader),GET_VERSION_NO(currHeader));
                        }
                        pkt_start = GET_PACKET_FILTER(pkt_start);
                        data->fileBuff->packetStart = pkt_start;
                        for(iK = 0; iK < N_FILE_BUFFERS_AT_A_TIME; iK++)
                        {
                            localStruct[iK].packetNo = pkt_start+iK*TIME_INT_PER_BUFFER;
                        }
                        for(iK = 0; iK < nAnts; iK++)
                        {
                            data->fileBuff->antNumbers[iK] = antNumbers[iK];
                        }

                        first_loop_done = 1;
                }


                returnEmptyBuffer(data->udpBuff,indexIn);
        }

        for(iK = 0; iK < N_FILE_BUFFERS_AT_A_TIME; iK++)
        {
            returnEmptyBuffer(&data->fileBuff->udpBuff, localStruct[iK].index);
        }
        return NULL;
}

