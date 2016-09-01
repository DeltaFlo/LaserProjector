// See LICENSE file for details
// Copyright 2016 Florian Link (at) gmx.de
#include "Laser.h"

// these values can be adapted to fine tune sendto:

// if this is enabled, pins need to be 10 and 7 in dac init below, but it is a big speedup!
#define MCP4X_PORT_WRITE 1

#include "DAC_MCP4X.h"

MCP4X dac;

Laser::Laser(int laserPin)
{
  _laserPin = laserPin;
  _quality = FROM_FLOAT(1./(LASER_QUALITY));

  _x = 0;
  _y = 0;
  _oldX = 0;
  _oldY = 0;

  _state = 0;
  
  _scale = 1;
  _offsetX = 0;
  _offsetY = 0;

  _moved = 0;
  _maxMove = -1;
  _laserForceOff = false;
  resetClipArea();

  _enable3D = false;
  _zDist = 1000;
}

void Laser::init()
{
  dac.init(MCP4X_4822, 5000, 5000,
      10, 7, 1);
  dac.setGain2x(MCP4X_CHAN_A, 0);
  dac.setGain2x(MCP4X_CHAN_B, 0);
  dac.begin(1);
 
  pinMode(_laserPin, OUTPUT);
}

void Laser::sendToDAC(int x, int y)
{
  #ifdef LASER_SWAP_XY
  int x1 = y;
  int y1 = x;
  #else
  int x1 = x;
  int y1 = y;
  #endif
  #ifdef LASER_FLIP_X
  x1 = 4095 - x1;
  #endif
  #ifdef LASER_FLIP_Y
  y1 = 4095 - y1;
  #endif
  dac.output2(x1, y1);
}

void Laser::resetClipArea()
{
  _clipXMin = 0;
  _clipYMin = 0;
  _clipXMax = 4095;
  _clipYMax = 4095;
}

void Laser::setClipArea(long x, long y, long x1, long y1)
{
  _clipXMin = x;
  _clipYMin = y;
  _clipXMax = x1;
  _clipYMax = y1;
}

const int INSIDE = 0; // 0000
const int LEFT = 1;   // 0001
const int RIGHT = 2;  // 0010
const int BOTTOM = 4; // 0100
const int TOP = 8;    // 1000

int Laser::computeOutCode(long x, long y)
{
  int code = INSIDE;          // initialised as being inside of [[clip window]]

  if (x < _clipXMin)           // to the left of clip window
    code |= LEFT;
  else if (x > _clipXMax)      // to the right of clip window
    code |= RIGHT;
  if (y < _clipYMin)           // below the clip window
    code |= BOTTOM;
  else if (y > _clipYMax)      // above the clip window
    code |= TOP;

  return code;
}

// Cohen–Sutherland clipping algorithm clips a line from
// P0 = (x0, y0) to P1 = (x1, y1) against a rectangle with 
// diagonal from (_clipXMin, _clipYMin) to (_clipXMax, _clipYMax).
bool Laser::clipLine(long& x0, long& y0, long& x1, long& y1)
{
  // compute outcodes for P0, P1, and whatever point lies outside the clip rectangle
  int outcode0 = computeOutCode(x0, y0);
  int outcode1 = computeOutCode(x1, y1);
  bool accept = false;
  
  while (true) {
    if (!(outcode0 | outcode1)) { // Bitwise OR is 0. Trivially accept and get out of loop
      accept = true;
      break;
    } else if (outcode0 & outcode1) { // Bitwise AND is not 0. Trivially reject and get out of loop
      break;
    } else {
      // failed both tests, so calculate the line segment to clip
      // from an outside point to an intersection with clip edge
      long x, y;

      // At least one endpoint is outside the clip rectangle; pick it.
      int outcodeOut = outcode0 ? outcode0 : outcode1;

      // Now find the intersection point;
      // use formulas y = y0 + slope * (x - x0), x = x0 + (1 / slope) * (y - y0)
      if (outcodeOut & TOP) {           // point is above the clip rectangle
        x = x0 + (x1 - x0) * float(_clipYMax - y0) / float(y1 - y0);
        y = _clipYMax;
      } else if (outcodeOut & BOTTOM) { // point is below the clip rectangle
        x = x0 + (x1 - x0) * float(_clipYMin - y0) / float(y1 - y0);
        y = _clipYMin;
      } else if (outcodeOut & RIGHT) {  // point is to the right of clip rectangle
        y = y0 + (y1 - y0) * float(_clipXMax - x0) / float(x1 - x0);
        x = _clipXMax;
      } else if (outcodeOut & LEFT) {   // point is to the left of clip rectangle
        y = y0 + (y1 - y0) * float(_clipXMin - x0) / float(x1 - x0);
        x = _clipXMin;
      }

      // Now we move outside point to intersection point to clip
      // and get ready for next pass.
      if (outcodeOut == outcode0) {
        x0 = x;
        y0 = y;
        outcode0 = computeOutCode(x0, y0);
      } else {
        x1 = x;
        y1 = y;
        outcode1 = computeOutCode(x1, y1);
      }
    }
  }
  return accept;
}

void Laser::sendto (long xpos, long ypos)
{
  if (_enable3D) {
    Vector3i p1;
    Vector3i p;
    p1.x = xpos;
    p1.y = ypos;
    p1.z = 0;
    Matrix3::applyMatrix(_matrix, p1, p);
    xpos = ((_zDist*(long)p.x) / (_zDist + (long)p.z)) + 2048;
    ypos = ((_zDist*(long)p.y) / (_zDist + (long)p.z)) + 2048;
  }
  // Float was too slow on Arduino, so I used
  // fixed point precision here:
  long xNew = TO_INT(xpos * _scale) + _offsetX;
  long yNew = TO_INT(ypos * _scale) + _offsetY;
  long clipX = xNew;
  long clipY = yNew;
  long oldX = _oldX;
  long oldY = _oldY;
  if (clipLine(oldX,oldY, clipX,clipY)) {
    if (oldX != _oldX || oldY != _oldY) {
      sendtoRaw(oldX, oldY);
    }
    sendtoRaw(clipX, clipY);
  }
  _oldX = xNew;
  _oldY = yNew;
}

void Laser::sendtoRaw (long xNew, long yNew)
{
  // devide into equal parts, using _quality
  long fdiffx = xNew - _x;
  long fdiffy = yNew - _y;
  long diffx = TO_INT(abs(fdiffx) * _quality); 
  long diffy = TO_INT(abs(fdiffy) * _quality);

  // store movement for max move
  long moved = _moved;
  _moved += abs(fdiffx) + abs(fdiffy);

  // use the bigger direction
  if (diffx < diffy) 
  {
    diffx = diffy;     
  }
  fdiffx = FROM_INT(fdiffx) / diffx;
  fdiffy = FROM_INT(fdiffy) / diffx;
  // interpolate in FIXPT
  FIXPT tmpx = 0;
  FIXPT tmpy = 0;
  for (int i = 0; i<diffx-1;i++) 
  {
    // for max move, stop inside of line if required...
    if (_maxMove != -1) {
      long moved2 = moved + abs(TO_INT(tmpx)) + abs(TO_INT(tmpy));
      if (!_laserForceOff && moved2 > _maxMove) {
        off();
        _laserForceOff = true;
        _maxMoveX = _x + TO_INT(tmpx);
        _maxMoveY = _y + TO_INT(tmpy);
      }
    } 
    tmpx += fdiffx;
    tmpy += fdiffy;
    sendToDAC(_x + TO_INT(tmpx), _y + TO_INT(tmpy));
    #ifdef LASER_MOVE_DELAY
    wait(LASER_MOVE_DELAY);
    #endif
  }
  
  // for max move, stop if required...
  if (!_laserForceOff && _maxMove != -1 && _moved > _maxMove) {
    off();
    _laserForceOff = true;
    _maxMoveX = xNew;
    _maxMoveY = yNew;
  }

  _x = xNew;
  _y = yNew;
  sendToDAC(_x, _y);
  wait(LASER_END_DELAY);
}

void Laser::drawline(long x1, long y1, long x2, long y2)
{
  if (_x != x1 or _y != y1) 
  {
    off();
    sendto(x1,y1);
  }
  on();
  sendto(x2,y2);
  wait(LASER_LINE_END_DELAY);
}

void Laser::on()
{
  if (!_state && !_laserForceOff) 
  {
    wait(LASER_TOGGLE_DELAY);
    _state = 1;
    digitalWrite(_laserPin, HIGH);
  }
}

void Laser::off()
{
  if (_state) 
  {
    wait(LASER_TOGGLE_DELAY);
    _state = 0;
    digitalWrite(_laserPin, LOW);
  }
}

void Laser::wait(long length)
{
  delayMicroseconds(length);
}

void Laser::setScale(float scale)
{ 
  _scale = FROM_FLOAT(scale);
}

void Laser::setOffset(long offsetX, long offsetY)
{ 
  _offsetX = offsetX;
  _offsetY = offsetY;
}

