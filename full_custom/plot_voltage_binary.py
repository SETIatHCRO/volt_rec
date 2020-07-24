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

    args = parser.parse_args()

    # found this
    # https://stackoverflow.com/questions/26369520/how-to-load-4-bit-data-into-numpy-array
    data = np.fromfile(args.in_file, dtype=np.int8, 
            count=args.nchans*args.nsamps*NPOL, offset=args.offset*args.nchans*NPOL)
    data = (data << 4 >> 4)  +\
            1j*(data >> 4)

    data = data.reshape(-1, args.nchans, NPOL)

    fig, (ax1, ax2) = plt.subplots(1, sharex=True, sharey=True)
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
