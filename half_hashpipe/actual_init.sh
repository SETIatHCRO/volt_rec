#!/bin/bash

numactl --cpunodebind=1 --membind=1 \
	hashpipe -p hpguppi_daq -p ata_volt -I 1 \
	-o BINDHOST=enp134s0 -o BINDPORT=4016 \
	-o NANTS=2 -o FREQCHAN=1024 -o SCHAN=2048\
	-o MAXFLOWS=2 \
	-o IBVPKTSZ=8,8192 \
	-o DATADIR=/mnt/buf0/ \
	-c hpguppi_ibvpkt_thread -c ata_volt_outer_thread -c null_output_thread
