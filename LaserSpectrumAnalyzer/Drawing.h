// See LICENSE file for details
// Copyright 2016 Florian Link (at) gmx.de
#ifndef DRAWING_H
#define DRAWING_H

#include "Laser.h"

extern Laser laser;

//! Allows to draw text and objects from PROGMEM.
class Drawing
{
public:
  //! Draws the given string at x,y position. Count indicates how often the drawing is repeated.
  static void drawString(String text, int x, int y, int repeat = 1);

  //! Draws the given letter (A-Z, 0-9, !? are currently supported in the font), returns the x advance...
  static long drawLetter(byte letter, long translateX = 0, long translateY = 0);

  //! Get X advance for given char
  static long advance(byte letter);

  //! Get X advance for string
  static long stringAdvance(String text);

  //! Draws the given data (which needs to be in PROGMEM). Size indicates the number
  //! of draw commands (so it is sizeof(data)/4).
  static void drawObject(const unsigned short* data, int size, long translateX = 0, long translateY = 0);

  //! Draws the given data (which needs to be in PROGMEM). Size indicates the number
  //! of draw commands (so it is sizeof(data)/4).
  static void drawObjectRotated(const unsigned short* data, int size, long centerX, long centerY, int angle);

  //! Draws the object and rotates in 3D. 
  static void drawObjectRotated3D(const unsigned short* data, int size, long centerX, long centerY, int angleX, int angleY, int fov);

  //! Returns the center of the object (center of bounding box)
  static void calcObjectBox(const unsigned short* data, int size, long& centerX, long& centerY, long& width, long& height);
};

#endif
