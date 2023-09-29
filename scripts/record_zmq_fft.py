#!/usr/bin/env python

# Subscribe to fft data topic. Output separate metadata and data files.

import json
import zmq

import numpy as np

URI = 'ipc:///tmp/gqrx_data'
TOPIC = b'data.fft.linear'
META_FILE = '/tmp/fft.meta'
DATA_FILE = '/tmp/fft.data'


def rx_loop(socket, outf=None, metaf=None):
    fftsize = None
    while True:
        message = socket.recv_multipart()
        if (message[0] == TOPIC):
            meta = json.loads(message[1].decode())
            if fftsize is None:
                fftsize = meta['fftsize']

            if len(message[2]) != fftsize * 4:
                raise ValueError('Data length wrong or changed.')

            # Copy out data.
            if outf:
                x = np.frombuffer(message[2], np.float32).tobytes()
                outf.write(x)

            # Copy out metadata. Rewrite json in case we want to add fields.
            if metaf:
                metaf.write(json.dumps(meta))
                metaf.write('\n')


context = zmq.Context()
socket: zmq.Socket = context.socket(zmq.SUB)
socket.connect(URI)

# Subscribe to single topic.
socket.setsockopt(zmq.SUBSCRIBE, TOPIC)

with open(DATA_FILE, 'wb') as dataf, open(META_FILE, 'w') as metaf:
    try:
        rx_loop(socket, dataf, metaf)
    except KeyboardInterrupt:
        print()
    except Exception as e:
        print(str(e))