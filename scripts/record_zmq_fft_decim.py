#!/usr/bin/env python

# Subscribe to fft data topic. Output separate metadata and data files.

import json
import zmq

import numpy as np

NUM_RECORDS = 10000
TIME_DECIM = 1
FFT_SIZE_OUT = 4096
URI = 'ipc:///tmp/gqrx_data'
TOPIC = b'data.fft.linear'
META_FILE = '/tmp/fft.meta'
DATA_FILE = '/tmp/fft.data'
STD_FILE = '/tmp/fft.std'


def rx_loop(socket, outf=None, metaf=None, stdf=None):
    win = None
    i = 0
    count = NUM_RECORDS

    prevFftDecimFactor = 0

    # Time decimation circular buffer
    win = np.zeros((TIME_DECIM, FFT_SIZE_OUT))

    while True:
        if NUM_RECORDS:
            if count == 0:
                break
            else:
                count -= 1
        message = socket.recv_multipart()
        if (message[0] == TOPIC):
            meta = json.loads(message[1].decode())
            fftsize = meta['fftsize']
            fftDecimFactor = fftsize // FFT_SIZE_OUT

            # Check that data length matches fftsize, bail if not
            if len(message[2]) != fftsize * 4:
                raise ValueError('Received incorrect data size')

            # fftsize shorter than output size, ignore data
            if (fftDecimFactor < 1):
                print(f'Received length {fftsize}, require at least {FFT_SIZE_OUT}')
                continue

            a = np.frombuffer(message[2], np.float32)

            # fftsize larger than output size, decimate using rectangular
            # window and stride
            if (fftDecimFactor > 1):
                if (fftDecimFactor != prevFftDecimFactor):
                    fftDecimWindow = np.ones(fftDecimFactor) / (fftDecimFactor * fftDecimFactor)
                    prevFftDecimFactor = fftDecimFactor
                a = np.convolve(a, fftDecimWindow, 'valid')[::fftDecimFactor]

            win[i] = a

            # Every TIME_DECIM vectors, write out decimated data and metadata.
            if i == (TIME_DECIM - 1):

                # Use maximum values from window. This is what Gqrx does.
                if outf or metaf:
                    xmax = np.max(win, 0)

                # Copy out data vector first, so metadata always refers to available data.
                if outf:
                    x = xmax.astype(np.float32).tobytes()
                    outf.write(x)
                    outf.flush()

                if stdf or metaf:
                    xstd = np.std(win, 0)

                # Write std dev vector
                if stdf:
                    x = xstd.astype(np.float32).tobytes()
                    stdf.write(x)
                    stdf.flush()

                # Add more fields that readers would otherwise need to get from
                # the data file. Write out metadata.
                if metaf:
                    xavg = np.average(win, 0)
                    meta['avg'] = np.average(xavg)
                    meta['maxmax'] = max(xmax)
                    xmin = np.min(win, 0)
                    meta['minmin'] = min(xmin)
                    meta['minstd'] = min(xstd)
                    meta['maxstd'] = max(xstd)
                    meta['fftdecim'] = fftDecimFactor

                    metaf.write(json.dumps(meta))
                    metaf.write('\n')
                    metaf.flush()

                i = 0

            else:
                i += 1


context = zmq.Context()
socket: zmq.Socket = context.socket(zmq.SUB)
socket.connect(URI)

# Subscribe to single topic.
socket.setsockopt(zmq.SUBSCRIBE, TOPIC)

with open(DATA_FILE, 'wb') as dataf, open(META_FILE, 'w') as metaf, open(STD_FILE, 'wb') as stdf:
    try:
        rx_loop(socket, dataf, metaf, stdf)
    except KeyboardInterrupt:
        print()
    # except Exception as e:
    #     print(str(e))
