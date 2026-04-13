#!/usr/bin/env python3
"""Create a TAKEOVER.DAT with all 5 scenarios marked complete.
This enables the cracktro easter egg at the menu (press C).

DAT format:
  unsigned short magic = 0x544B ("TK")
  unsigned char version = 1
  unsigned char completed[8] = { 1,1,1,1,1, 0,0,0 }
"""
import struct, sys, os

dat_path = os.path.join(os.path.dirname(__file__), '..', 'TAKEOVER.DAT')
if len(sys.argv) > 1:
    dat_path = sys.argv[1]

# magic=0x544B, version=1, completed=[1,1,1,1,1,0,0,0]
data = struct.pack('<HB8B', 0x544B, 1, 1, 1, 1, 1, 1, 0, 0, 0)
with open(dat_path, 'wb') as f:
    f.write(data)
print(f"Wrote {dat_path} ({len(data)} bytes) - all 5 scenarios marked complete")
