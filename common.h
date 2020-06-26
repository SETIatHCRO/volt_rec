#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include "data_fromats.h"

void error(char *message);
ssize_t findEmpty(ssize_t * no, size_t buffLen);
ssize_t findLowest(ssize_t * no, size_t buffLen);

int fetchEmptyBuffer(udpBuffStruct* structure,char ** buff, ssize_t * index);
int returnBuffer(udpBuffStruct* structure, ssize_t index, ssize_t order, size_t dataLen);
int returnEmptyBuffer(udpBuffStruct* structure, ssize_t index);
int fetchBuffer(udpBuffStruct* structure, char ** buff, ssize_t * index, ssize_t * order, size_t * dataLen);
int init_udpBuff(udpBuffStruct* structure, int number_of_buffers, ssize_t bufsize);
int destroy_udpBuff(udpBuffStruct* structure);
int anyBufFull(udpBuffStruct structure);

#define ERR(source) {perror(source),\
                     fprintf(stderr,"%s:%d\n",__FILE__,__LINE__);\
                     printf("%s:%d\n",__FILE__,__LINE__);\
                     return EXIT_FAILURE;}
