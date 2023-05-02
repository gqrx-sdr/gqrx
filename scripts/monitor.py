#!/usr/bin/env python

# Subscribe to all topics, print topic and metadata

import zmq


def rx_loop(socket):
    try:
        while True:
            message = socket.recv_multipart()
            if len(message) == 2:
                print(f"{message[0].decode()}: {message[1].decode()}")
            elif len(message) == 3:
                print(f"{message[0].decode()}: {message[1].decode()} data({len(message[2])})")
    except KeyboardInterrupt:
        print()


context = zmq.Context()
socket: zmq.Socket = context.socket(zmq.SUB)
socket.connect('ipc:///tmp/gqrx_data')

# Subscribe to all control and data topics.
socket.setsockopt(zmq.SUBSCRIBE, b'control.')
socket.setsockopt(zmq.SUBSCRIBE, b'data.')

rx_loop(socket)
