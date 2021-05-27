#Name: AfficheLayer v1
#Info: Show current layer
#Depend: GCode
#Type: postprocess

## Written by L.Christophe (Tititopher68-dev/Tititopher)
## This script is licensed under the Creative Commons - Attribution - Share Alike (CC BY-SA) terms

# Uses -
# M117 <message> - Set staus message

#history / changelog:
#v1

version = '1'
count = 0
layer = 0

import re
import os
import sys

# Read all code...
with open(os.path.join(sys.path[0], sys.argv[1]), "r") as f:
	lines = f.readlines()

# ... and write it back!
with open(os.path.join(sys.path[0], sys.argv[1]), "r") as f:
	for line in lines:         
            if line.startswith(';LAYER_CHANGE'):
                    count += 1 
            if line.startswith(';LAYER:'):
                    count += 1     
        
with open(os.path.join(sys.path[0], sys.argv[1]), "w") as f:
    for line in lines:
            f.write(line)
            if line.startswith(';LAYER_CHANGE'):
			            layer += 1
			            f.write("M117 Layer %d/%s...\n" % (layer, count))
            if line.startswith(';LAYER:'):
			            layer += 1
			            f.write("M117 Layer %d/%s...\n" % (layer, count))
