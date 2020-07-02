#ifndef __UDP_THREAD_HEADER__
#define __UDP_THREAD_HEADER__

void * udp_recv_work(void * pointer);

//initializes socket and sets various socket parameters
int initSocket(int debugPrint,const short portNum);

#endif
