## Test recorder code

this is test recorder code for snap voltage mode testing

Recorder should be only used for debugging/testing the snaps/packets, not 
for scientific puproses. The scientific verion will be avaliable as hashpipe module

header:
```
uint32 number of antennas
between 1 and 4 (previous field) uint32 - antenna ids
uint32 number of channels
uint32 first channel number
uint64 first packet number
```

data:
```
uint8 point (4 + 4 bits re/im), each "packet" has 16 time x 256 channel data points
```
