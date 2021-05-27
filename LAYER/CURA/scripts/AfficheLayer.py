# AfficheLayer script - Show current layer on the controller
#
# This script is a quick migration of the AfficheLayer plugin for legacy Cura.
# 

from ..Script import Script
class AfficheLayer(Script):
    def __init__(self):
        super().__init__()
        
    def getSettingDataString(self):
        return """{ 
            "name":"Affiche Layer",
            "key": "AfficheLayer",
            "metadata": {},
            "version": 2,
            "settings": {}
        }"""
    
    def execute(self, data):
        count = "-"
        for index, layer in enumerate(data):
            new_layer = ""
            lines = layer.split("\n")
            for line in lines:
                if line:
                    new_layer += line + "\n"
                    if line.startswith(";LAYER_COUNT:"):
                        count = line[13:].rstrip()
                    if line.startswith(";LAYER:"):
                        this_layer = int(line[7:].rstrip()) + 1
                        new_layer += "M117 Layer {0}/{1}...\n".format(this_layer, count)
            data[index] = new_layer
        return data