// See LICENSE file for details
// Copyright 2016 Florian Link (at) gmx.de
#ifndef LASER_H
#define LASER_H

#include "Arduino.h"
#include "Basics.h"

// -- The following flags can be used to fine tune the laser timing

// defines the granularity of the line interpolation. 64 means that each line is split into steps of 64 pixels in the longer direction.
// setting smaller values will slow down the rendering but will cause more linearity in the galvo movement,
// setting bigger values will cause faster rendering, but lines will not be straight anymore.
#define LASER_QUALITY 16

// Defines how long the galvos wait for the on/off toggling of the laser pointer (in microseconds), this will depend on your laser pointer.
#define LASER_TOGGLE_DELAY 500
// Defines how long the galvos wait at the end of a line (currently only used for the 3D cube rendering, in microseconds).
#define LASER_LINE_END_DELAY 200
// Defines the delay the laser waits after reaching a given position (in microseconds).
#define LASER_END_DELAY 5
// Defines the delay after each laser movement (used when interpolating lines, in microseconds), if not defines, 0 is used
#define LASER_MOVE_DELAY 5

// -- The following flags can be used to rotate/flip the output without changing the DAC wiring, just uncomment the desired swap/flip
// define this to swap X and Y on the DAC
#define LASER_SWAP_XY
// define this to flip along the x axis
#define LASER_FLIP_X
// define this to flip along the y axis
#define LASER_FLIP_Y

//! Encapsulates the laser movement and on/off state.
class Laser
{
public:
  //! The laser is initialized with the laserPin,
  //! which selects the digital pin that turns the laser pointer on/off.
  Laser(int laserPin);

  void init();

  //! send the laser to the given position, scaled and translated and line clipped.
  void sendto(long x, long y);
  //! sends the laser to the raw position (the movement is always linearly interpolated depending on the quality,
  //! to avoid too fast movements.
  void sendtoRaw(long x, long y);

  //! draws a line by turning the laser off, going to x1,y1, turning it on and going to x2,y2.
  void drawline(long x1, long y1, long x2, long y2);

  void wait(long length);

  void on();
  void off();
  
  void setScale(float scale);
  void setOffset(long offsetX, long offsetY);
  
  void resetClipArea(); 
  void setClipArea(long x, long y, long x1, long y1);

  void resetMaxMove() { _maxMove = -1; _laserForceOff = false; }
  void setMaxMove(long length) { _moved = 0; _maxMove = length; _laserForceOff = false; }
  bool maxMoveReached() { return _laserForceOff; }
  void getMaxMoveFinalPosition(long &x, long &y) { x = _maxMoveX; y = _maxMoveY; }

  void setEnable3D(bool flag) { _enable3D = flag; }
  void setMatrix(const Matrix3& matrix) { _matrix = matrix; }
  void setZDist(long dist) { _zDist = dist; }

private:
  //! send X/Y to DAC
  void sendToDAC(int x, int y);

  //! computes the out code for line clipping
  int computeOutCode(long x, long y);
  //! returns if the line should be drawn, clips line to clip area
  bool clipLine(long& x0, long& y0, long& x1, long& y1);

  int _laserPin;

  FIXPT _quality;

  long _x;
  long _y;
  int  _state;

  FIXPT _scale;
  long _offsetX;
  long _offsetY;

  long _moved;
  long _maxMove;
  bool _laserForceOff;
  long _maxMoveX;
  long _maxMoveY;

  long _oldX;
  long _oldY;

  long _clipXMin;
  long _clipYMin;
  long _clipXMax;
  long _clipYMax;

  bool _enable3D;
  Matrix3 _matrix;
  long _zDist;
};

#endif
