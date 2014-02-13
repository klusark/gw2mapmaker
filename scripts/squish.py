#!/usr/bin/python3

from PIL import Image
import glob
import os

visited = {}

def ensureDir(dir):
	if not os.path.exists(dir):
		os.makedirs(dir);

layers = glob.glob("out/tiles/*");
layers.sort();
for level in layers:
	currlayer = level.split("/")[2];
	print(currlayer)
	for tile in glob.glob("out/tiles/"+currlayer+"/7/*/*.png"):
		s = tile.split("/");
		xstr = s[4];
		ystr = s[5].split(".")[0];
		pos = xstr+","+ystr;
		if pos not in visited:
			print(pos)
			visited[pos] = True;
			im = Image.open(tile);
			for level2 in layers:
				l2 = level2.split("/")[2];
				if (int(l2) > int(currlayer)):
					filename = level2+"/7/"+xstr+"/"+ystr+".png";
					if os.path.exists(filename):
						im2 = Image.open(filename)
						im = Image.alpha_composite(im, im2);
			directory = "out/squish/7/"+xstr+"/";
			ensureDir(directory)
			im.save(directory+ystr+".png")

