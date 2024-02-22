import struct

if __name__ == "__main__":
    values = [12.5, 156.3, -95, -78, 15.2, 0.0]

    buf = struct.pack("%sf" % len(values), *values)

    print(buf)

    print(struct.unpack("%sf" % len(values), buf))
