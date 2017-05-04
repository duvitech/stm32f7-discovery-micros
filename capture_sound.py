#!/usr/bin/env python3
"""
Reads sounds from the microphone
"""

import argparse
import progressbar
import serial
import wave
import struct
import matplotlib.pyplot as plt



def parse_args():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("port", help="Serial port")
    parser.add_argument("output", help="WAV file for output")
    parser.add_argument("--length", "-l", help="Buffer length (44.1k by default", type=int, default=44100)
    parser.add_argument("--plot", "-p")

    return parser.parse_args()

def main():
    args = parse_args()
    conn = serial.Serial(args.port, 9600)

    conn.write("dfsdm\r\n".encode())

    buf = bytes()

    print("Acquiring...")
    while not buf.decode().endswith("Done !\r\n"):
        buf = buf + conn.read(1)

    print("done")

    buf = bytes()
    pbar = progressbar.ProgressBar(maxval=args.length).start()

    while len(buf) < 4 * args.length:
        pbar.update(len(buf) / 4)
        buf += conn.read(100)

    pbar.finish()

    with wave.open(args.output, 'wb') as f:
        f.setnchannels(1)
        f.setsampwidth(4)
        f.setframerate(44.1e3)
        f.writeframes(buf)

    data = struct.unpack('>' + 'i'*args.length, buf)

    data = data[1000:2000]

    print(len(buf))
    plt.plot(data)
    plt.plot([0, len(data)], [2**31, 2**31])
    plt.plot([0, len(data)], [-2**31, -2**31])

    print(max(data))

    print(args)
    if args.plot:
        plt.savefig(args.plot)
    else:
        plt.show()

if __name__ == '__main__':
    main()
