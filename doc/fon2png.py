#!/usr/bin/python

import sys
from PIL import Image, ImageDraw	# pip install pillow

if len(sys.argv) != 2:
	raise Exception('Usage: fon2bmp.py <file>')

filename = sys.argv[1]
with open(filename, "rb") as f:
	raw = f.read()

width = raw[442]
height = raw[444]
char_bytes_len = raw[446]

chars = []
offset = 1642
for c in range(256):
	onechar = [0 for i in range(height)]
	for x in range(round(width/8)):
		for y in range(height):
			onechar[y] = onechar[y] << 8 | raw[offset]
			offset = offset + 1
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
