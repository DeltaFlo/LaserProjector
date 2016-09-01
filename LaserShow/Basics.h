#ifndef BASICS_H
#define BASICS_H

// typedef for fix point numbers
typedef long FIXPT;
#define PRES             16384
#define PSHIFT           14
#define PROUNDBIT        (1 << (PSHIFT-1))
#define FROM_FLOAT(a) (long(a*PRES))
#define FROM_INT(a) (a << PSHIFT)
#define TO_INT(a) ((a + PROUNDBIT)>> PSHIFT)

typedef struct {
  int x, y, z;
} Vector3i;

// fixed point identity matrix
struct Matrix3 {
  long m[3][3] = {
      {PRES,    0,    0},
      {   0, PRES,    0},
      {   0,    0, PRES}
  };
   static void applyMatrix(const Matrix3& matrix, const Vector3i& in, Vector3i& out);
   static Matrix3 multiply(const Matrix3 &mat1, const Matrix3 &mat2);
   static Matrix3 rotateX(const unsigned int angle); 
   static Matrix3 rotateY(const unsigned int angle);
   static Matrix3 rotateZ(const unsigned int angle);
};


long SIN(unsigned int angle);
long COS(unsigned int angle);

#endif

