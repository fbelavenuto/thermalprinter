#!/usr/bin/python

import sys
from PIL import Image, ImageDraw	# pip install pillow

if len(sys.argv) != 4:
	raise Exception('Usage: bin2bmp.py <file> <width in bits> <height in bits>')

filename = sys.argv[1]
width = int(sys.argv[2]) 
height = int(sys.argv[3])
with open(filename, "rb") as f:
	raw = f.read()

expected_len = 256*height*round(width/8)
if len(raw) != expected_len:
	raise Exception("File must be {} bytes in size.".format(expected_len))

chars = []
offset = 0
for c in range(256):
	onechar = []
	for y in range(height):
		byte = 0
		for x in range(round(width/8)):
			byte = byte << 8 | raw[offset]
			offset = offset + 1
		onechar.append(byte)
	chars.append(onechar)

img = Image.new('RGB', (16*width, 16*height), color=(255, 0, 255))
draw = ImageDraw.Draw(img)
for c in range(256):
	x = (c % 16) * width
	y = int(c / 16) * height
	for cy in range(height):
		byte = chars[c][cy]
		mask = 1 << 8*round(width/8)-1
		for l in range(width):
			bit = byte & mask
			mask >>= 1
			if bit != 0:
				draw.rectangle([x+l, y+cy, x+l, y+cy], fill=(255,255,255))

img.save('chars.png')
