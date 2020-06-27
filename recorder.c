
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <time.h>
#include <sys/mman.h>

#include "data_fromats.h"
#include "udp_thread.h"
#include "processing_thread.h"
#include "file_thread.h"
#include "common.h"

volatile sig_atomic_t keep_going = 1;


void inthandler (int sig)
{
    keep_going = 0;
}

void setsighandler(int sig, void (*handler) (int))
{
        struct sigaction sa;

        memset(&sa, 0x00, sizeof(struct sigaction));
        sa.sa_handler = handler;
        if (sigaction(sig, &sa, NULL) == -1)
                error("sigaction");
}

int main(int argc, char *argv[])
{
        udpDataStruct udpStr;
        procDataStruct procStr;
        fileDataStruct fileStr;

        udpBuffStruct udpBuff;
        fileBuffStruct fileBuff;

        pthread_t udpRcv;
        pthread_t fileWriter;
        pthread_t worker;

        int iK;

        if (argc < 2)
        {
            printf("usage: %s port datadir nAnts nChans firstChan\n");
            return -1;
        }
        if(argc >= 4)
        {
            fileBuff.nAnts = atoi(argv[3]);
            if ( (fileBuff.nAnts > MAX_NO_ANTS ) ||(fileBuff.nAnts < 1) )
            {
                fprintf(stderr,"bad number of antennas, expected 1-%d, got %d\n", MAX_NO_ANTS, fileBuff.nAnts);
                return -1;
            }
        }
        else
        {
            fileBuff.nAnts = 1;
        }
        if(argc >= 5)
        {
            fileBuff.nchans = atoi(argv[4]);
            if ( (fileBuff.nchans >MAX_CHANNEL_PACKETS ) ||(fileBuff.nchans < 1) )
            {
                fprintf(stderr,"bad number of channels, expected 1-%d, got %d\n", MAX_CHANNEL_PACKETS, fileBuff.nchans);
                return -1;
            }
        }
        else
        {
            fileBuff.nchans = MAX_CHANNEL_PACKETS;
        }
        if(argc >= 6)
        {
            fileBuff.firstChan = atoi(argv[5]);
        }
        else
        {
            fileBuff.firstChan = 0;
        }

        if (argc >= 3)
        {
            //the last argument should be the data path thing
            strncpy(fileStr.file_part,argv[2], MAX_FILE-1);
        }
        else
        {
            snprintf(fileStr.file_part, MAX_FILE, "%s",DEFAULT_SAVE_PATTERN);
        }
        if (argc >= 2)
        {
            udpStr.port = atoi(argv[1]);
        }
        else {
            udpStr.port = DEFAULT_PORT;
        }

        init_udpBuff(&udpBuff, N_UDP_BUFFERS, UDP_BUFFER_SIZE);
        init_udpBuff(&fileBuff.udpBuff, N_FILE_BUFFERS, FILE_BUFFER_SIZE);
/*
        if(pthread_mutex_init(&udpBuff.dataMutex,NULL)) error("mutexI");
        if(pthread_mutex_init(&udpBuff.NReadyMutex,NULL)) error("mutexI");
        if((udpBuff.data = (char **)malloc(sizeof(char*)*N_UDP_BUFFERS)) == NULL) error("malloc");
        for(iK = 0; iK < N_UDP_BUFFERS; iK++)
        {
            if ((udpBuff.data[iK] = (char *) malloc (UDP_BUFFER_SIZE*sizeof(char))) == NULL) error("malloc");
        }
        if((udpBuff.dataLen = (size_t *)malloc(sizeof(size_t)*N_UDP_BUFFERS)) == NULL) error("malloc");
        if((udpBuff.orderNo = (ssize_t *)malloc(sizeof(ssize_t)*N_UDP_BUFFERS)) == NULL) error("malloc");
        if((udpBuff.lastPacketNo= (unsigned int *)malloc(sizeof(unsigned int)*N_UDP_BUFFERS)) == NULL) error("malloc");
        memset(udpBuff.orderNo,0,sizeof(ssize_t)*N_UDP_BUFFERS);
        udpBuff.NReady = 0;

        if(pthread_mutex_init(&fileBuff.udpBuff.dataMutex,NULL)) error("mutexI");
        if(pthread_mutex_init(&fileBuff.udpBuff.NReadyMutex,NULL)) error("mutexI");
        if((fileBuff.udpBuff.data = (char **)malloc(sizeof(char*)*N_UDP_BUFFERS)) == NULL) error("malloc");
        for(iK = 0; iK < N_UDP_BUFFERS; iK++)
        {
            if ((fileBuff.udpBuff.data[iK] = (char *) malloc (2*UDP_BUFFER_SIZE*sizeof(char))) == NULL) error("malloc");
        }
        if((fileBuff.udpBuff.dataLen = (size_t *)malloc(sizeof(size_t)*N_UDP_BUFFERS)) == NULL) error("malloc");
        if((fileBuff.udpBuff.orderNo = (ssize_t *)malloc(sizeof(ssize_t)*N_UDP_BUFFERS)) == NULL) error("malloc");
        memset(fileBuff.udpBuff.orderNo,0,sizeof(ssize_t)*N_UDP_BUFFERS);
        fileBuff.udpBuff.NReady = 0;
        */

        udpStr.udpBuff = &udpBuff;
        procStr.udpBuff = &udpBuff;
        procStr.fileBuff = &fileBuff;
        fileStr.fileBuff = &fileBuff;

        //blocking SIGINT in all threads
        sigset_t mask;
        setsighandler(SIGINT, inthandler);
        sigemptyset(&mask);
        sigaddset(&mask, SIGINT);
        pthread_sigmask(SIG_BLOCK, &mask, NULL);

        mlockall(MCL_CURRENT);


        if (pthread_create(&udpRcv, NULL, udp_recv_work, (void *)&udpStr)) error ("recv Create");
        if (pthread_create(&fileWriter, NULL, file_writer_work, (void *)&fileStr)) error ("recv Create");
        if (pthread_create(&worker, NULL, worker_work, (void *)&procStr)) error ("recv Create");


        //unblocking in main thread
        pthread_sigmask(SIG_UNBLOCK, &mask, NULL);

        while(keep_going) {
            pause();
            printf("main thread keepgoing %d\n",keep_going);
        }

        printf("Starting main thread termination\n");
        if(pthread_join(udpRcv,NULL)) error("join");
        if(pthread_join(fileWriter,NULL)) error("join");
        if(pthread_join(worker,NULL)) error("join");

        munlockall();

        destroy_udpBuff(&udpBuff);
        destroy_udpBuff(&fileBuff.udpBuff);

/*
        if(pthread_mutex_destroy(&udpBuff.dataMutex)) error("mutexD");
        if(pthread_mutex_destroy(&udpBuff.NReadyMutex)) error("mutexD");
        free(udpBuff.orderNo);
        free(udpBuff.dataLen);
        for(iK = 0; iK < N_UDP_BUFFERS; iK++)
        {
                free(udpBuff.data[iK]);
                free(udpBuff.firstHeader[iK]);
        }
        free(udpBuff.data);
        free(udpBuff.firstHeader);

        if(pthread_mutex_destroy(&fileBuff.dataMutex)) error("mutexD");
        if(pthread_mutex_destroy(&fileBuff.NReadyMutex)) error("mutexD");
        free(fileBuff.orderNo);
        free(fileBuff.dataLen);
        for(iK = 0; iK <N_UDP_BUFFERS; iK++)
        {
                free(fileBuff.data[iK]);
                free(fileBuff.firstHeader[iK]);
        }
        free(fileBuff.data);
        free(fileBuff.firstHeader);
        */
        return 0;
}
