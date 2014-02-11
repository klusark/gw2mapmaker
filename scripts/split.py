#!/usr/bin/python3

from PIL import Image
from PIL import PngImagePlugin
import os
import glob
import json

def allBlack(im):
	c = im.getcolors();
	if c == None:
		return False;
	return len(c) == 1;

def ensureDir(dir):
	if not os.path.exists(dir):
		os.makedirs(dir);

def pngSave(im, x, y, rect, layer):
	meta = PngImagePlugin.PngInfo();
	meta.add_text("Comment", im.info["Comment"]);
	im1 = im.crop(rect);
	if (allBlack(im1) == False):
		ensureDir("out/"+str(layer)+"/7/"+str(x));
		im1.save("out/" + str(layer) + "/7/" + str(x) + "/" + str(y)+".png", "PNG", pnginfo = meta);

def outputImage(name):
	s = name.split("/");
	pimg = s[0];
	layer = 1
	if (pimg in layers):
		layer = layers[pimg]
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
			pngSave(im, x2+i, y2+j, (i*256,j*256,(i+1)*256,(j+1)*256), layer);


layers = json.load(open("/home/joel/layers.json"))

for file in glob.glob("*/*/*.png"):
	print("processing " + file);
	outputImage(file)
