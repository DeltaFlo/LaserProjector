# Simple GCode to Arduino hex format converter.
# It only understands G00 and G01 codes, nothing fancy!
#
# It will automatically scale the object to the full 12 bit
# range for my Arduino laser project, to change that
# you have to modify the scale in createObject().
#
# Typical files I worked with have been generated with
# http://ncplot.com/stickfont/stickfont.htm (StickFont 1.1)
#
# Usage: python convertGCode.py inputfile.nc outputfile.cpp

import math
import sys

def createObject(name, cmds):
  minx = miny = 10000000
  maxx = maxy = 0
  string = ""
  for cmd in cmds:
    if cmd[0] == 2:
      minx = min(minx,cmd[1])
      miny = min(miny,cmd[2])
      maxx = max(maxx,cmd[1])
      maxy = max(maxy,cmd[2])

  string += "const unsigned short draw_" + name + "[] PROGMEM = {\n";
  laserState = False
  
  biggestSide = max(maxx-minx, maxy-miny)
  # scale to the laser range
  scale = 4095. / biggestSide;
  print "bounding box x: ", minx, maxx
  print "bounding box y: ", miny, maxy
  print "scale: ", scale
  for cmd in cmds:
    if cmd[0] == 0:laserState = False
    if cmd[0] == 1:laserState = True
    if cmd[0] == 2:
      x = int(math.floor((cmd[1]-minx) * scale))
      y = int(math.floor((cmd[2]-miny) * scale))
      if laserState:
        x += 0x8000
      string += hex(x) + "," + hex(y) + ",\n"
  string += "};\n"
  return string

def run(input, output):
  result = ""
  f = open(input);
  lines = f.readlines()
  drawing = False
  posx = posy = 0.
    
  cmds = []
  for l in lines:
    if l.startswith("G00"):
      if drawing:
        cmds.append((0,))
      drawing = False
    elif l.startswith("G01"):
      drawing = True
      cmds.append((1,))
    elif l.startswith("X"):
      parts = l.split("Y")
      newposx = float(parts[0][1:])
      newposy = float(parts[1])
      cmds.append((2,newposx,newposy))
      posx = newposx
      posy = newposy

  result = createObject("object", cmds)
  
  o = open(output,"w")
  o.write(result)

if __name__ == "__main__":
  if len(sys.argv) < 3:
    print "Usage: convertGCode.py inputfile.nc outputfile.cpp"
  else:
    run(sys.argv[1], sys.argv[2])
