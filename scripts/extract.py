#!/usr/bin/python3

import json
import subprocess
import os
import sys

file = open(sys.argv[1])
arr = json.load(file)

def ensureDir(dir):
        if not os.path.exists(dir):
                os.makedirs(dir);

for i in arr:
	ensureDir(str(i))
	subprocess.call(["/home/joel/code/gw2mapmaker/build/gw2mapmaker", "/home/joel/Gw2.dat", str(i)], cwd=str(i))
