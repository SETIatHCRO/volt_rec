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
        int nOfPackets;
        uint64_t noMissing=0;
        int iK;
        ssize_t order;
        ssize_t offset;
        work_local_struct localStruct[N_FILE_BUFFERS_AT_A_TIME];
        int first_loop_done = 0;
        uint64_t currHeader;
        uint64_t currPacketNo;
        uint64_t nextStructPacketNo;
        int64_t lastSentPacketNo;
        int currChanNo, currAntId, currPacketRest;
        int structIndex;


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
                                //printf("Ant: %ld chan: %ld pkt: %ld version: %ld\n",GET_ANT_NO(currHeader),GET_CHAN_NO(currHeader),GET_PCKT_NO(currHeader),GET_VERSION_NO(currHeader));
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
                        nextStructPacketNo = (pkt_start + N_FILE_BUFFERS_AT_A_TIME*TIME_INT_PER_BUFFER) & PCKT_NO_MASK;
                        first_loop_done = 1;
                }
                for(iK = 0; iK < nOfPackets; iK++)
                {
                        currHeader = be64toh( *((uint64_t *)(input + iK*RX_MSG_LEN)));
                        currPacketNo =GET_PACKET_FILTER( GET_PCKT_NO(currHeader) );
                        currPacketRest = GET_PACKET_REST( GET_PCKT_NO(currHeader) );
                        currAntId = getAntIndex(antNumbers, GET_ANT_NO(currHeader), nAnts);
                        currChanNo = (GET_CHAN_NO(currHeader) - firstChan)/CHANS_PER_PACKET;
                        if( IS_VERSION_VOLT(currHeader) && (currAntId != -1) && (currAntId < MAX_NO_ANTS) && (currChanNo >= 0) && (currChanNo < nchans) )
                        {
                            //we work wit the assumption that there are no 2 packets with the same header
                            structIndex = findStructIndex(localStruct, N_FILE_BUFFERS_AT_A_TIME, currPacketNo);
                            if (structIndex == -1) {
                                //packet does not exist in local Struct, either too old or too new.
                                //if too new, that means we have a bit of a problem, but not checking it here
                                continue;
                            }
                            offset = (currPacketRest*nAnts*nchans +currAntId *nchans + currChanNo) * RX_DATA_LEN;
                            memcpy(localStruct[structIndex].data + offset,input + iK*RX_MSG_LEN + RX_BUFF_LEN ,RX_DATA_LEN);
                            localStruct[structIndex].nToFull--;

                        }
                        //else we ignore that packet
                }
                returnEmptyBuffer(data->udpBuff,indexIn);
                //sending all that are full, and all earlier. Also, if number of started buffers is all-1, we are sending those
                int nStarted = 0;
                for (iK = 0; iK < N_FILE_BUFFERS_AT_A_TIME; iK++)
                {
                        //if buffer is full, we are sending it
                        if(localStruct[iK].nToFull == 0)
                        {
                                lastSentPacketNo = localStruct[iK].packetNo;
                                returnBuffer(&data->fileBuff->udpBuff,localStruct[iK].index,localStruct[iK].order,dataLenOut);
                                //waiting as long as we can grab next one
                                while(fetchEmptyBuffer(&data->fileBuff->udpBuff,&localStruct[iK].data, &localStruct[iK].index));
                                localStruct[iK].nToFull = packetsPerBlock;
                                memset(localStruct[iK].data,0,dataLenOut*sizeof(char));
                                localStruct[iK].order = next_order++;
                                localStruct[iK].packetNo = nextStructPacketNo;
                                nextStructPacketNo = (nextStructPacketNo + TIME_INT_PER_BUFFER) & PCKT_NO_MASK;
                        }
                        else if (localStruct[iK].nToFull < packetsPerBlock)
                        {
                                nStarted++;
                        }

                        /*
                        //sending "too old" packets
                        //if we have "high" packet no and last setn was very low (e.g. last sent was 2 and we have MAX-10,
                        else if ( ( (lastSentPacketNo < N_FILE_BUFFERS_AT_A_TIME/2) && (localStruct[iK].packetNo < (PCKT_NO_MASK + lastSentPacketNo - N_FILE_BUFFERS_AT_A_TIME/2+1) ) && (localStruct[iK].packetNo > (PCKT_NO_MASK >> 1)) )
                                  //or if we have "high enough" last send and the packet number of a buffer is lower than half of the queue, we are sending partially empty buffer
                                  || ( (lastSentPacketNo >= N_FILE_BUFFERS_AT_A_TIME/2 ) && (localStruct[iK].packetNo < (lastSentPacketNo - N_FILE_BUFFERS_AT_A_TIME/2) ) ) )
                        {
                                noMissing += localStruct[iK].nToFull;
                                returnBuffer(&data->fileBuff->udpBuff,localStruct[iK].index,localStruct[iK].order,dataLenOut);
                                while(fetchEmptyBuffer(&data->fileBuff->udpBuff,&localStruct[iK].data, &localStruct[iK].index));
                                localStruct[iK].nToFull = packetsPerBlock;
                                memset(localStruct[iK].data,0,dataLenOut*sizeof(char));
                                localStruct[iK].order = next_order++;
                                localStruct[iK].packetNo = nextStructPacketNo;
                                nextStructPacketNo = (nextStructPacketNo + TIME_INT_PER_BUFFER) & PCKT_NO_MASK;
                                //if we have all packets with missing data, we are enforcing that things move forward by adjusting this
                                lastSentPacketNo = (nextStructPacketNo - TIME_INT_PER_BUFFER*N_FILE_BUFFERS_AT_A_TIME/4) & PCKT_NO_MASK;
                        }
                        */
                }
                //if we have most started, we mark last sent as half of the buffer size
                if (nStarted > N_FILE_BUFFERS_AT_A_TIME -2)
                {
                        lastSentPacketNo = (nextStructPacketNo - TIME_INT_PER_BUFFER*N_FILE_BUFFERS_AT_A_TIME/2) & PCKT_NO_MASK;
                }
                //now we are sending all "earlier" data
                for (iK = 0; iK < N_FILE_BUFFERS_AT_A_TIME; iK++)
                {
                    //TODO - fix this if to deal with very high packet numbers
                        if( (localStruct[iK].packetNo <  lastSentPacketNo ))
                        {
                                noMissing += localStruct[iK].nToFull;
                                fprintf(stderr,"missing %ld packets\n",localStruct[iK].nToFull);
                                returnBuffer(&data->fileBuff->udpBuff,localStruct[iK].index,localStruct[iK].order,dataLenOut);
                                while(fetchEmptyBuffer(&data->fileBuff->udpBuff,&localStruct[iK].data, &localStruct[iK].index));
                                localStruct[iK].nToFull = packetsPerBlock;
                                memset(localStruct[iK].data,0,dataLenOut*sizeof(char));
                                localStruct[iK].order = next_order++;
                                localStruct[iK].packetNo = nextStructPacketNo;
                                nextStructPacketNo = (nextStructPacketNo + TIME_INT_PER_BUFFER) & PCKT_NO_MASK;
                        }
                }

        }

        for(iK = 0; iK < N_FILE_BUFFERS_AT_A_TIME; iK++)
        {
            returnEmptyBuffer(&data->fileBuff->udpBuff, localStruct[iK].index);
        }
        fprintf(stderr,"totalMissing: %ld\n", noMissing);
        return NULL;
}

int findStructIndex(work_local_struct *localStruct, int structLen, uint64_t currPacketNo )
{
    int iK;
    for (iK = 0; iK < structLen; iK++)
    {
        if(localStruct[iK].packetNo == currPacketNo)
        {
            return iK;
        }
    }
    return -1;
}
