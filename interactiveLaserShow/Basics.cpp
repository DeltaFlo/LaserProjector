#include "Basics.h"
#include "Arduino.h"

// Copied from "Arduino - Tiny 3D Engine" by Themistokle "mrt-prodz" Benetatos.
// https://github.com/mrt-prodz/ATmega328-Tiny-3D-Engine
// (and adapted for laser/quad rendering)

#define LUT(a) (long)(pgm_read_word(&lut[a]))// return value from LUT in PROGMEM

const unsigned int lut[] PROGMEM = {         // 0 to 90 degrees fixed point COSINE look up table
  16384, 16381, 16374, 16361, 16344, 16321, 16294, 16261, 16224, 16182, 16135, 16082, 16025, 15964, 15897, 15825, 15749, 15668, 15582, 15491, 15395, 15295, 15190, 15081, 14967, 14848, 14725, 14598, 14466, 14329, 14188, 14043, 13894, 13740, 13582, 13420, 13254, 13084, 12910, 12732, 12550, 12365, 12175, 11982, 11785, 11585, 11381, 11173, 10963, 10748, 10531, 10310, 10086, 9860, 9630, 9397, 9161, 8923, 8682, 8438, 8191, 7943, 7691, 7438, 7182, 6924, 6663, 6401, 6137, 5871, 5603, 5334, 5062, 4790, 4516, 4240, 3963, 3685, 3406, 3126, 2845, 2563, 2280, 1996, 1712, 1427, 1142, 857, 571, 285, 0
};

// ----------------------------------------------
// SIN/COS from 90 degrees LUT
// ----------------------------------------------
long SIN(unsigned int angle) {
  angle += 90;
  if (angle > 450) return LUT(0);
  if (angle > 360 && angle < 451) return -LUT(angle-360);
  if (angle > 270 && angle < 361) return -LUT(360-angle);
  if (angle > 180 && angle < 271) return  LUT(angle-180);
  return LUT(180-angle);
}

long COS(unsigned int angle) {
  if (angle > 360) return LUT(0);
  if (angle > 270 && angle < 361) return  LUT(360-angle);
  if (angle > 180 && angle < 271) return -LUT(angle-180);
  if (angle > 90  && angle < 181) return -LUT(180-angle);
  return LUT(angle);
}

// fixed point multiplication
static long pMultiply(long x, long y) {
  return ( (x * y) + PROUNDBIT) >> PSHIFT;
}

// ----------------------------------------------
// Matrix operation
// ----------------------------------------------
void Matrix3::multiply(const Matrix3 &mat1, const Matrix3 &mat2, Matrix3& mat ) {
  unsigned char r,c;
  for (c=0; c<3; c++)
    for (r=0; r<3; r++)
      mat.m[c][r] = pMultiply(mat1.m[0][r], mat2.m[c][0]) +
                    pMultiply(mat1.m[1][r], mat2.m[c][1]) +
                    pMultiply(mat1.m[2][r], mat2.m[c][2]);
}

Matrix3 Matrix3::rotateX(const unsigned int angle) {
  Matrix3 mat;
  mat.m[1][1] =  COS(angle);
  mat.m[1][2] =  SIN(angle);
  mat.m[2][1] = -SIN(angle);
  mat.m[2][2] =  COS(angle);
  return mat;
}

Matrix3 Matrix3::rotateY(const unsigned int angle) {
  Matrix3 mat;
  mat.m[0][0] =  COS(angle);
  mat.m[0][2] = -SIN(angle);
  mat.m[2][0] =  SIN(angle);
  mat.m[2][2] =  COS(angle);
  return mat;
}

Matrix3 Matrix3::rotateZ(const unsigned int angle) {
  Matrix3 mat;
  mat.m[0][0] =  COS(angle);
  mat.m[0][1] =  SIN(angle);
  mat.m[1][0] = -SIN(angle);
  mat.m[1][1] =  COS(angle);
  return mat;
}

void Matrix3::applyMatrix(const Matrix3& matrix, const Vector3i& in, Vector3i& out)
{
      out.x = (matrix.m[0][0] * in.x +
             matrix.m[1][0] * in.y +
             matrix.m[2][0] * in.z) >> PSHIFT;
      
      out.y = (matrix.m[0][1] * in.x +
             matrix.m[1][1] * in.y +
             matrix.m[2][1] * in.z) >> PSHIFT;
            
      out.z = (matrix.m[0][2] * in.x +
             matrix.m[1][2] * in.y +
             matrix.m[2][2] * in.z) >> PSHIFT;
  
}

