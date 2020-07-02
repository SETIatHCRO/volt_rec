CFLAGS= -Wall -g -pthread

all: recorder 
default: recorder

common.o: common.c common.h
	gcc $(CFLAGS) -c $< -o $@

udp_thread.o: udp_thread.c udp_thread.h common.h
	gcc $(CFLAGS) -c $< -o $@

processing_thread.o: processing_thread.c processing_thread.h common.h
	gcc $(CFLAGS) -c $< -o $@

file_thread.o: file_thread.c file_thread.h common.h
	gcc $(CFLAGS) -c $< -o $@

recorder: recorder.c file_thread.o processing_thread.o udp_thread.o common.o
	gcc  $(CFLAGS) recorder.c file_thread.o processing_thread.o udp_thread.o common.o -o $@

clean:
	rm *.o recorder

.PHONY: clean
