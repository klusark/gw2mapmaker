#!/usr/bin/python3

from PIL import Image
from PIL import PngImagePlugin
import os
import glob

def allBlack(im):
	c = im.getcolors();
	if c == None:
		return False;
	return len(c) == 1;

def ensureDir(dir):
	if not os.path.exists(dir):
		os.makedirs(dir);

def pngSave(im, x, y, rect):
	meta = PngImagePlugin.PngInfo();
	meta.add_text("Comment", im.info["Comment"]);
	im1 = im.crop(rect);
	if (allBlack(im1) == False):
		im1.save("out/"+str(x)+"/"+str(y)+".png", "PNG", pnginfo = meta);

def outputImage(name):
	s = name.split("/");
	y = int(s[1]);
	x = int(s[2].split(".")[0]);
	x2 = x*2;
	y2 = y*2;
	im = Image.open(name);

	ensureDir("out/"+str(x2));
	ensureDir("out/"+str(x2+1));

	pngSave(im, x2, y2, (0,0,256,256));

	pngSave(im, x2+1, y2, (256,0,512,256));

	pngSave(im, x2, y2+1, (0,256,256,512));

	pngSave(im, x2+1, y2+1, (256,256,512,512));

for file in glob.glob("*/*/*.png"):
	print("processing " + file);
	outputImage(file)
