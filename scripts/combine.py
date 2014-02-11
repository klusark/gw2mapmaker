#!/usr/bin/python3

from PIL import Image
import os

#tolayer = 6
def processImage(newimage, x, y, pos):
	try:
		im = Image.open(basepath + "/" + x + "/" + y + filetype)
		newimage.paste(im, pos);
	except:
		None

for tolayer in (5,4,3,2,1):

	layermax = 2**tolayer

	basepath = "tiles/1/1/"+str(tolayer+1);
	filetype = ".png"


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
			newimage = Image.new("RGBA", (512,512));
			processImage(newimage, x1, y1, (0,0));
			processImage(newimage, x2, y1, (256,0));
			processImage(newimage, x1, y2, (0,256));
			processImage(newimage, x2, y2, (256,256));

			newimage.thumbnail((256,256), Image.ANTIALIAS);

			dir = "tiles/1/1/"+str(tolayer)+"/"+str(x);
			if not os.path.exists(dir):
				os.makedirs(dir);
			out = dir + "/" + str(y) + filetype
			newimage.save(out);
			
