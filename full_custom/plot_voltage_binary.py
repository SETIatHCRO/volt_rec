import numpy as np
import matplotlib.pyplot as plt
import os,sys

import argparse

NPOL = 2

# make plots prettier
RCPARAMS_SCRIPT = "/home/obsuser/utils/rcparams.py"
if os.path.exists(RCPARAMS_SCRIPT):
    exec(open(RCPARAMS_SCRIPT, "r").read())

def main():
    parser = argparse.ArgumentParser(
            description='Plot voltage binary files')
    parser.add_argument('in_file', type=str,
            help = 'input filterbank files')
    parser.add_argument('-n', dest='nchans', required =True,
            help = 'number of channels in file', type=int)
    parser.add_argument('-i', dest='nsamps', default = 10000,
            help = 'number of samples to read', type=int)
    parser.add_argument('-o', dest='offset', default = 0,
            help = 'sampleOffset', type=int)
    parser.add_argument('-b', dest='nbit', default = 4,
            help = 'nbits in file', type=int)
    parser.add_argument('-s', dest='hdr_sze', default=16384,
            help = 'number of bytes to skip as header')

    args = parser.parse_args()

    # found this
    # https://stackoverflow.com/questions/26369520/how-to-load-4-bit-data-into-numpy-array
    if args.nbit == 4:
        data = np.fromfile(args.in_file, dtype=np.int8, 
                count=args.nchans*args.nsamps*NPOL, 
                offset=args.offset*args.nchans*NPOL + args.hdr_sze)
        data = (data << 4 >> 4)  +\
                1j*(data >> 4)

    elif args.nbit == 8:
        data = np.fromfile(args.in_file, dtype=np.int8,
                count=args.nchans*args.nsamps*NPOL*2,
                offset=args.offset*args.nchans*NPOL*2 + args.hdr_sze)
        data = data[::2] + 1j*data[1::2]

    else:
        raise RuntimeError("Can only support 4 or 8 bit input")

    data = data.reshape(-1, args.nchans, NPOL)

    fig, (ax1, ax2) = plt.subplots(2, sharex=True, sharey=True)
    #fig, (ax1) = plt.subplots(1, sharex=True, sharey=True)
    ax1.set_title("X-pol")
    ax1.set_ylabel("Freq chan")
    ax1.imshow(np.abs(data[:,:,0]).T, interpolation='nearest', aspect='auto')

    ax2.set_title("Y-pol")
    ax2.imshow(np.abs(data[:,:,1]).T, interpolation='nearest', aspect='auto')
    ax2.set_ylabel("Freq chan")
    ax2.set_xlabel("Time (samples)")

    plt.figure()
    plt.title("Bandpass")
    plt.plot((np.abs(data[:,:,0]).sum(axis=0)),
            label="X-pol")
    plt.plot((np.abs(data[:,:,1]).sum(axis=0)),
            label="Y-pol")
    plt.legend()
    plt.xlabel("Frequency channel")
    plt.ylabel("power (dB)")

    plt.show()


if __name__ == "__main__":
    main()
