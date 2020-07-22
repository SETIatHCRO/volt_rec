## recorder code

This is reposisory containing the voltage data recording code. The code captures
voltage data from the snaps. 3 part of the repository exist (or will exist)

## UDP data

Each snap is sending part of the spectrum to given ip/port address. the packets are 8200 bytes each (8 header + 8192 data)

Header is a big endian int 64 with the following information, starting with LSB
[0-5] - antenna information - the id of the antenna programmed into snap
[6-17] - channel number - number of the lowest channel in the package
[18-55] - packet number - number of packet (in time). Uses the internal snap counter that
is zeroed at the sync process
[56-63] - version number. [observed number is 193]. and with 0x0080 to determine if packet is a voltage packet

the data section stores information from 16 time samples (s) x 256 freq channels (c) x 2 pols (p). Each sample is 4+4 bit signed integer(real/imag)
the samples are in following order [bytes]:
[s0c0p0][s0c0p1][s0c1p0][s0c1p1]...[s0c255p1][s1c1p0]...[s15c255p1]

## full custom

the fully custom C based data capture program. 4 threads: 1 network, 1 "processing/sorting" 1 disc 1 supervisor. File format of the output
described in detail in separate README 

## half-hashpipe

Software written in hashpipe, designed to use ibverbs packet capture (ibverbs_pkt_thread from hpguppi) and hashpipe transfer. The software is still doing sorting/processing and file writing in the format as full custom. It is intended as an intermediate step to rawspec file format writer/converter

requires properly compiled and installed hpguppi_daq, branch meerkat-multi-qp-branch (as of end July 2020)
