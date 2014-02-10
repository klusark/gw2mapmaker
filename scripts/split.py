#!/usr/bin/python3

from PIL import Image
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

def outputImage(name):
	s = name.split("/");
	y = int(s[1]);
	x = int(s[2].split(".")[0]);
	x2 = x*2;
	y2 = y*2;
	im = Image.open(name);
	ensureDir("out/"+str(x2));
	ensureDir("out/"+str(x2+1));
	im1 = im.crop((0,0,256,256));
	im1.load();
	if (allBlack(im1) == False):
		im1.save("out/"+str(x2)+"/"+str(y2)+".png");

	im2 = im.crop((256,0,512,256));
	if (allBlack(im2) == False):
		im2.save("out/"+str(x2+1)+"/"+str(y2)+".png");

	im3 = im.crop((0,256,256,512));
	if (allBlack(im3) == False):
		im3.save("out/"+str(x2)+"/"+str(y2+1)+".png");

	im4 = im.crop((256,256,512,512));
	if (allBlack(im4) == False):
		im4.save("out/"+str(x2+1)+"/"+str(y2+1)+".png");

for file in glob.glob("*/*/*.png"):
	print("processing " + file);
	outputImage(file)
