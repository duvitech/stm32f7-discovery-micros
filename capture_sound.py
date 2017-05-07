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
    parser.add_argument(
        "--length",
        "-l",
        help="Buffer length (44.1k by default",
        type=int,
        default=44100)
    parser.add_argument("--plot", "-p")
    parser.add_argument(
        "-b",
        "--baudrate",
        type=int,
        default=921600,
        help="Baudrate, default=921600")

    return parser.parse_args()


def main():
    args = parse_args()
    conn = serial.Serial(args.port, args.baudrate)

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

    data = struct.unpack('<' + 'i' * args.length, buf)

    with wave.open(args.output, 'wb') as f:
        f.setnchannels(1)
        f.setsampwidth(4)
        f.setframerate(44.1e3)
        for d in data:
            # We have to apply some gain to make it audible at reasonable
            # levels. Another option would be to put the file in 24 bit mode
            # and divide it instead.
            f.writeframes(struct.pack('i', d * 1000))

    data = data[1000:2000]

    print(len(buf))
    plt.plot(data)

    print(max(abs(s) for s in data))

    print(args)
    if args.plot:
        plt.savefig(args.plot)
    else:
        plt.show()


if __name__ == '__main__':
    main()
