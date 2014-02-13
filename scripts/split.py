#!/usr/bin/python3

from PIL import Image
from PIL import PngImagePlugin
import os
import glob
import json
import sys

red = Image.new("RGBA", (256,256), "red")

pimg2tile = {}
tile2pimg = {}

def allBlack(im):
	c = im.getcolors();
	if c == None:
		return False;
	return len(c) == 1;

def ensureDir(dir):
	if not os.path.exists(dir):
		os.makedirs(dir);

def pngSave(im, x, y, rect, layer, pimg):
	meta = PngImagePlugin.PngInfo();
	meta.add_text("Comment", im.info["Comment"]);
	im1 = im.crop(rect);
	if (allBlack(im1) == False):
		ensureDir("out/tiles/"+str(layer)+"/7/"+str(x));
		filename = "out/tiles/" + str(layer) + "/7/" + str(x) + "/" + str(y)+".png";
		retval = 0;
		if (os.path.exists(filename)):
			print(filename + " already exists. Fix it.");
			red.save(filename, "PNG", pnginfo = meta);
		else:
			im1.save(filename, "PNG", pnginfo = meta);
			retval = 1
		dir2 = "out/pimgs/" + pimg + "/" + str(x) + "/";
		filename2 = dir2 + str(y)+".png";
		ensureDir(dir2);
		im1.save(filename2, "PNG", pnginfo = meta);
		return retval
	return 2

def outputImage(name):
	s = name.split("/");
	pimg = s[0];
	layer = 1
	if (pimg in layers):
		layer = layers[pimg]
	if (layer == -1):
		return;

	if not pimg in pimg2tile:
		pimg2tile[pimg] = []
	y = int(s[1]);
	x = int(s[2].split(".")[0]);
	x2 = x*2;
	y2 = y*2;
	im = Image.open(name);

	size = im.size;
	if (size[0] == 1024):
		im.thumbnail((512,512), Image.ANTIALIAS);

	numsub = 2;

	for i in range(numsub):
		for j in range(numsub):
			val = pngSave(im, x2+i, y2+j, (i*256,j*256,(i+1)*256,(j+1)*256), layer, pimg)
			if val == 0:
				print("Fix: " + pimg);
			if val != 2:
				pos = str(x2+i)+","+str(y2+j);
				pimg2tile[pimg].insert(0, pos);
				if not pos in tile2pimg:
					tile2pimg[pos] = []
				tile2pimg[pos].insert(0, pimg)

layers = json.load(open(sys.argv[1]))

for file in glob.glob("*/*/*.png"):
	#print("processing " + file);
	outputImage(file)

with open("out/tile2pimg.json", "w") as outfile:
	json.dump(tile2pimg, outfile)
with open("out/pimg2tile.json", "w") as outfile:
	json.dump(pimg2tile, outfile)
