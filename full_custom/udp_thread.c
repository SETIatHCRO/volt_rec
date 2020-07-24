#include "udp_thread.h"
#include "data_formats.h"
#include "common.h"
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>

extern volatile sig_atomic_t keep_going;

void * udp_recv_work(void * pointer)
{
        udpDataStruct * data;
        data = (udpDataStruct*) pointer;

        int socket =initSocket(PRINT_DEBUG,data->port);


        unsigned char* rxbuff;  // The buffer for received datagrams
        const int rxlen = RX_MSG_LEN;       // The size fo the datagram buffer
        if((rxbuff = (unsigned char*) malloc(rxlen)) == NULL) error("malloc");
        struct sockaddr_in fromsock;           // Buffer that holds the source addr
        socklen_t fromsocksize = sizeof(fromsock);   // Size of the fromsock buffer

        ssize_t currentBuffNo;
        char * currentBuff;
        size_t order = 1; //cant be 0
        ssize_t dataSize;
        int iResult;
        int iK;
        while(keep_going)
        {
            //printf("foo1\n");
            if(fetchEmptyBuffer(data->udpBuff,&currentBuff, &currentBuffNo) == -1)
            {
                //no space to write, waiting, but in fact we should take the oldest data and overwrite
                struct timespec tt;
                tt.tv_sec = 0;
                tt.tv_nsec = 100;
                nanosleep(&tt,NULL);
                continue;
            }
            dataSize = 0;
            for (iK=0;iK<N_READS_DEFAULT; iK++)
            {
                //printf("foo2\n");
                iResult = recvfrom(socket, (char*)rxbuff, rxlen, 0, (struct sockaddr *)&fromsock, &fromsocksize);
                //printf("foo3\n");
                if(iResult > 0)
                {
                        if(order == 1 && iK == 0)
                        {
                            //no hazard because no data in buffer yet
                            if(RX_MSG_LEN*N_READS_DEFAULT > UDP_BUFFER_SIZE) //probably
                            {
                                fprintf(stderr,"Data buffer is not sufficient, closing\n");
                                error("data allocation");
                            }
                        }
                        else if(iResult != RX_MSG_LEN) {
                                fprintf(stderr,"data of size %d received, expected %d\n",iResult,RX_MSG_LEN);
                                keep_going = 0;
                                break;
                        }
                        memcpy(currentBuff+dataSize, rxbuff, iResult);
                        dataSize += iResult;
                }
                else if (iResult == 0)
                {
                    //kill(0,SIGINT);
                    returnBuffer(data->udpBuff, currentBuffNo, order, dataSize);
                    keep_going = 0;
                    break;
                }
                else
                {
                    perror("recvfrom");
                    kill(0,SIGINT);
                    break;
                }
            }
            returnBuffer(data->udpBuff, currentBuffNo, order, dataSize);
            order++;
        }
        free(rxbuff);
        close(socket);
        return NULL;
}

int initSocket(int debugPrint, const short portNum)
{
    int sockfd;
    struct sockaddr_in saServer;

    // Creating socket file descriptor
    if ( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) ERR("socket");

    if(debugPrint){printf("Making port assignments...\n");}

    memset(&saServer, 0, sizeof(struct sockaddr_in));
    saServer.sin_family = AF_INET;
    saServer.sin_port = htons(portNum);
    saServer.sin_addr.s_addr = htonl(INADDR_ANY);

    int flag = 1;
    //If connection breaks, we are able to re-run it much quicker than we would have to use previously
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR,&flag, sizeof(flag))) ERR("setsockopt (REUSE)");

    flag = 1<<30; //2^28
    if (setsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, &flag, sizeof(flag))) ERR("setsockopt (RCVBUF)");

    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 500000;

    if(setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (struct timeval *)&timeout, sizeof(struct timeval))) ERR("setsockopt (TIMEO)")


    // Bind the socket with the server address
    if ( bind(sockfd, (struct sockaddr *)&saServer, sizeof(saServer)) < 0 ) ERR("BIND");

    return sockfd;
}
