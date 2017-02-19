#include <Arduino.h>
#include "Cube.h"

// Copied from "Arduino - Tiny 3D Engine" by Themistokle "mrt-prodz" Benetatos.
// https://github.com/mrt-prodz/ATmega328-Tiny-3D-Engine
// (and adapted for laser/quad rendering)

// ----------------------------------------------
// functions
// ----------------------------------------------

#define NODECOUNT 8
#define TRICOUNT 6

#define NODE(a, b) (long)(pgm_read_dword(&nodes[a][b]))
#define EDGE(a, b) pgm_read_byte(&faces[a][b])
#define EDGECODE(a, b) pgm_read_byte(&edgecodes[a][b])

const long nodes[NODECOUNT][3] PROGMEM = {
  { 1500,  1500, -1500},
  { 1500, -1500, -1500},
  {-1500, -1500, -1500},
  {-1500,  1500, -1500},
  { 1500,  1500, 1500},
  {-1500,  1500, 1500},
  {-1500, -1500, 1500},
  { 1500, -1500, 1500},
};

const unsigned char faces[TRICOUNT][4] PROGMEM = {
  {0, 1, 2,3},
  {4, 5, 6,7},
  {0, 4, 7,1},
  {1, 7, 6,2},
  {2, 6, 5,3},
  {4, 0, 3, 5},
};

const unsigned char edgecodes[TRICOUNT][4] PROGMEM = {
  {0, 1, 2,3},
  {7, 6, 5,4},
  {8,4,9,0},
  {9, 5, 10,1},
  {10, 6, 11,2},
  {8, 7, 11,3},
};

// ----------------------------------------------
// global variables
// ----------------------------------------------
Matrix3 m_world;
Vector3i mesh_rotation = {0, 0, 0};
Vector3i mesh_position = {0, 0, 0};


static long proj_nodes[NODECOUNT][2];         // projected nodes (x,y)
static unsigned char i;


// ----------------------------------------------
// Shoelace algorithm to get the surface
// ----------------------------------------------
int shoelace(const int (*n)[2], const unsigned char index) {
  unsigned char t = 0;
  int surface = 0;
  for (; t<3; t++) {
    // (x1y2 - y1x2) + (x2y3 - y2x3) ...
    surface += (n[EDGE(index,t)][0]           * n[EDGE(index,(t<2?t+1:0))][1]) -
               (n[EDGE(index,(t<2?t+1:0))][0] * n[EDGE(index,t)][1]);
  }
  return surface * 0.5;
}

// ----------------------------------------------
// Shoelace algorithm for triangle visibility
// ----------------------------------------------
bool is_hidden(const long (*n)[2], const unsigned char index) {
  // (x1y2 - y1x2) + (x2y3 - y2x3) ...
  return ( ( (n[EDGE(index,0)][0] * n[EDGE(index,1)][1]) -
             (n[EDGE(index,1)][0] * n[EDGE(index,0)][1])   ) +
           ( (n[EDGE(index,1)][0] * n[EDGE(index,2)][1]) -
             (n[EDGE(index,2)][0] * n[EDGE(index,1)][1])   ) +
           ( (n[EDGE(index,2)][0] * n[EDGE(index,0)][1]) -
             (n[EDGE(index,0)][0] * n[EDGE(index,2)][1])   ) ) < 0 ? false : true;
}

void draw_wireframe_quads(const long (*n)[2]) {
  i = TRICOUNT-1;
  int edges[12];
  for (int i = 0; i<12;i++) edges[i]=0;
  do {
    // don't draw triangle with negative surface value
    if (!is_hidden(n, i)) {
      // draw triangle edges - 0 -> 1 -> 2 -> 0
      int code = EDGECODE(i,0);
      if (edges[code] == 0)
      {
        edges[code] = 1;
        laser.drawline(n[EDGE(i,0)][0], n[EDGE(i,0)][1], n[EDGE(i,1)][0], n[EDGE(i,1)][1]);
      }
      code = EDGECODE(i,1);
      if (edges[code] == 0){
        edges[code] = 1;
        laser.drawline(n[EDGE(i,1)][0], n[EDGE(i,1)][1], n[EDGE(i,2)][0], n[EDGE(i,2)][1]);
      }
      code = EDGECODE(i,2);
      if (edges[code] == 0){
        edges[code] = 1;
        laser.drawline(n[EDGE(i,2)][0], n[EDGE(i,2)][1], n[EDGE(i,3)][0], n[EDGE(i,3)][1]);
      }
      code = EDGECODE(i,3);
      if (edges[code] == 0){
        edges[code] = 1;
        laser.drawline(n[EDGE(i,3)][0], n[EDGE(i,3)][1], n[EDGE(i,0)][0], n[EDGE(i,0)][1]);
      }
    }
  } while(i--);
}

void rotateCube(int count) {
  laser.setScale(1);
  laser.setOffset(2048,2048);
  float scale = 1.;
  for (int lo = 0;lo<count;lo++) 
  {    
    long time = micros();
    // rotation
    Matrix3 tmp;
    m_world = Matrix3::rotateX(mesh_rotation.x);
    Matrix3::multiply(Matrix3::rotateY(mesh_rotation.y), m_world, tmp);
    Matrix3::multiply(Matrix3::rotateZ(mesh_rotation.z), tmp, m_world);

    // project nodes with world matrix
    Vector3i p1;
    Vector3i p;
    for (i=0; i<NODECOUNT; i++) {
      p1.x = NODE(i,0);
      p1.y = NODE(i,1);
      p1.z = NODE(i,2);
      Matrix3::applyMatrix(m_world, p1, p);

      // store projected node
      proj_nodes[i][0] = ((1000*(long)p.x) / (3000 + (long)p.z/2));
      proj_nodes[i][1] = ((1000*(long)p.y) / (3000 + (long)p.z/2));
    }

      // default auto-rotation mode
      mesh_rotation.x+=3;
      mesh_rotation.y+=2;
//      mesh_rotation.z++;
    
    if (mesh_rotation.x > 360) mesh_rotation.x = 0;
    if (mesh_rotation.y > 360) mesh_rotation.y = 0;
    if (mesh_rotation.z > 360) mesh_rotation.z = 0;
    draw_wireframe_quads(proj_nodes);
    laser.off();
    // keep rotation locked to maximum time the cube takes, which is 20000 micros
    long elapsed = micros() - time;
    if (elapsed < 20000) { delayMicroseconds(20000-elapsed); }
    // do an intro/extro animation
    if (lo < 120) {
      laser.setOffset(2048 - (120-lo)*20,2048);
    }
    if (lo > count - 60) {
      laser.setScale(scale);
      scale -= 0.02;
      if (scale <0) { scale = 0; }
    }
  }
}

