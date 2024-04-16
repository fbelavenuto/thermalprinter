#!/usr/bin/python2.7
import sys
import png				# pip install pypng
#import curses.ascii

if len(sys.argv) < 2:
	sys.stderr.write("Please specify the input PNG file\n")
	sys.exit(1)

reader = png.Reader(filename=sys.argv[1])
data = reader.asRGB()
width = data[0]
height = data[1]
bitmap = list(data[2]) # get image RGB values
#bitdepth = data[3]["bitdepth"]
char_width = int(width / 18)
char_height = int(height / 17)

sys.stdout.write("""unsigned char console_font_{}x{}[] = {{
""".format(char_width, char_height))

raster = []
for line in bitmap:
	raster.append([c == 1 and 1 or 0 for c in [line[k+1] for k in range(0, width * 3, 3)]])

# array of character bitmaps; each bitmap is an array of lines, each line
# consists of 1 - bit is set and 0 - bit is not set
char_bitmaps = [] 
for c in range(256): # for each character
	char_bitmap = []
	raster_row = int(c / 16) * (char_height+1)
	offset = (c % 16) * (char_width+1) + 1
	for y in range(char_height): # for each scan line of the character
		char_bitmap.append(raster[raster_row + y][offset : offset + char_width])
	char_bitmaps.append(char_bitmap)
raster = None # no longer required


# how many bytes a single character scan line should be
num_bytes_per_scanline = int((char_width + 7) / 8)

# convert the whole bitmap into an array of character bitmaps
char_bitmaps_processed = []
for c in range(len(char_bitmaps)):
	bitmap = char_bitmaps[c]
	encoded_lines = []
	for line in bitmap:
		encoded_line = []
		for b in range(num_bytes_per_scanline):
			offset = b * 8
			char_byte = 0
			mask = 0x80
			for x in range(8):
				if b * 8 + x >= char_width:
					break
				if line[offset + x]:
					char_byte += mask
				mask >>= 1
			encoded_line.append(char_byte)
		encoded_lines.append([encoded_line, line])
	char_bitmaps_processed.append([c, encoded_lines])
char_bitmaps = None

for c in char_bitmaps_processed:
	sys.stdout.write("""
	/*
	 * code=%d, hex=0x%02X
	 */
""" % (c[0], c[0]))
	for line in c[1]:
		sys.stdout.write("    ")
		for char_byte in line[0]:
			sys.stdout.write(("0x%02X," % char_byte))
		sys.stdout.write("  /* %s */" % ''.join([s == 0 and ' ' or '#' for s in line[1]]))
		sys.stdout.write("\n")

sys.stdout.write("""};

""")
