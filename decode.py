#! /usr/bin/env python3
import struct
import sys
with open(sys.argv[1], 'rb') as f:
    line = f.read(16)
    while line:
        out = struct.unpack('<qhhhxx', line)
        print(out)
        line = f.read(16)
