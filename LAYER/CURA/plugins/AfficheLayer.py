#Name: AfficheLayer v1
#Info: Affiche la couche en coours
#Help: AfficheLayer
#Depend: GCode
#Type: postprocess

# Uses -
# M117 <message> - Set staus message

#history / changelog: v1
#v1

version = '1'
count = '-'

import re

# Read all code...
with open(filename, "r") as f:
	lines = f.readlines()

# ... and write it back!
with open(filename, "w") as f:
	for line in lines:
		f.write(line)
		if line.startswith(';Layer count:'):
			count = line[14:].rstrip()
		if line.startswith(';LAYER:'):
			# New layer -- issue message
			layer = int(line[7:].rstrip()) + 1
			f.write("M117 Layer %d/%s...\n" % (layer, count))
