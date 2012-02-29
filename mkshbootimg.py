#!/usr/bin/env python
import sys, os

def copydata(outfile, infile):
    while 1:
        data = infile.read(512)
        if (data):
            outfile.write(data)
        else:
            break

def alignoffset(outfile):
    offset = outfile.tell()
    outfile.seek((offset + 511) & ~511)
    return outfile.tell()

def appendimage(outfile, infile):
    offset = alignoffset(outfile)
    copydata(outfile, infile)
    length = alignoffset(outfile) - offset
    assert (offset % 512 == 0)
    assert (length % 512 == 0)
    return (offset/512, length/512)

if len(sys.argv) < 4:
    print "Usage:", sys.argv[0], "output kernel boot [recovery]"
    sys.exit(1)

outfile = open(sys.argv[1], 'wb')
kernel = open(sys.argv[2], 'r')
boot = open(sys.argv[3], 'r')
recovery = None
if (len(sys.argv) == 5):
    recovery = open(sys.argv[4], 'r')
offset_table = "\n\nBOOT_IMAGE_OFFSETS\n"
copydata(outfile, kernel)
table_loc = alignoffset(outfile)
outfile.write('\x00' * 512)
offset_table += "boot_offset=%d;boot_len=%d;" % appendimage(outfile, boot)
if recovery:
    offset_table += "recovery_offset=%d;recovery_len=%d;" % appendimage(outfile, recovery)
offset_table += "\n\n"
outfile.seek(table_loc)
outfile.write(offset_table)
outfile.flush()
os.fsync(outfile.fileno())
outfile.close()
