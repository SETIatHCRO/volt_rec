#ifndef __FILE_THREAD_HEADER__
#define __FILE_THREAD_HEADER__

#include "data_formats.h"

void * file_writer_work(void * pointer);

int writeHeader(int fd, fileBuffStruct * structure);

#endif
