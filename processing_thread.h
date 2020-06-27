#ifndef __PROC_THREAD_HEADER__
#define __PROC_THREAD_HEADER__
#include <unistd.h>
#include <stdint.h>

typedef struct __work_local_struct
{
    uint64_t packetNo;
    char * data;
    ssize_t index;
    ssize_t order;
    ssize_t nToFull;
} work_local_struct;

void * worker_work(void * pointer);



#endif
