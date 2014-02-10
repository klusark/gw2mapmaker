#!/usr/bin/python3

from PIL import Image
import os

tolayer = 1

layermax = 2**tolayer

for x in range(0,layermax):
	for y in range(0,layermax):
		print(x, y);
		x1 = x*2;
		x2 = x1 + 1;
		y1 = y*2;
		y2 = y1 + 1;
		x1 = str(x1);
		x2 = str(x2);
		y1 = str(y1);
		y2 = str(y2);
		basepath = "tiles/1/1/"+str(tolayer+1);
		im1 = Image.open(basepath + "/" + x1 + "/" + y1 + ".jpg")
		im2 = Image.open(basepath + "/" + x2 + "/" + y1 + ".jpg")
		im3 = Image.open(basepath + "/" + x1 + "/" + y2 + ".jpg")
		im4 = Image.open(basepath + "/" + x2 + "/" + y2 + ".jpg")

		newimage = Image.new("RGB", (512,512));
		newimage.paste(im1, (0,0));
		newimage.paste(im2, (256,0));
		newimage.paste(im3, (0,256));
		newimage.paste(im4, (256,256));
		newimage.thumbnail((256,256), Image.ANTIALIAS);
			
		dir = "tiles/1/1/"+str(tolayer)+"/"+str(x);
		if not os.path.exists(dir):
			os.makedirs(dir);
		out = dir + "/" + str(y) + ".jpg"
		newimage.save(out);
		
