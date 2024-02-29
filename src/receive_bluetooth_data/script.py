import struct
import math

if __name__ == "__main__":

    buf = bytearray(bytes.fromhex("840C43BFFC7E963FDA221040471DA7BFA90B9D3FBB1CD33F"))

    # buf = bytearray(
    #     b"\xd6#6<\xa8\xb7V>\x06*\xd8=\xac`\x1d<C9\x8f\xbe\x9am\xfa\xbc,>\xdcZ=\xe9c(>\xe1\xb4\xfd>\xf5\\];=ZN\xbd\x18\xb4\xa3>\n"
    # )

    # buf = struct.pack("%sf" % len(values), *values)

    print(buf)
    print(len(buf))

    print([180 / math.pi * i for i in struct.unpack("%sf" % 6, buf)])
