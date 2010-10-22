#include "math.h"

//! Perlin noise class
/*!
    Based on Java reference implementation of:
        Improved Noise - Copyright 2002 Ken Perlin.

    (http://mrl.nyu.edu/~perlin/noise/)
*/
class PerlinNoise
{
public:
   static float noise(float x, float y, float z)
   {
      int varX = int(floor(x)) & 255,
          varY = int(floor(y)) & 255,
          varZ = int(floor(z)) & 255;

      x -= floor(x);
      y -= floor(y);
      z -= floor(z);
      float u = fade(x),
            v = fade(y),
            w = fade(z);

      int A = p[varX  ]+varY, AA = p[A]+varZ, AB = p[A+1]+varZ,
          B = p[varX+1]+varY, BA = p[B]+varZ, BB = p[B+1]+varZ;

      return lerp(w, lerp(v, lerp(u, grad(p[AA  ], x  , y  , z   ),
                                     grad(p[BA  ], x-1, y  , z   )),
                             lerp(u, grad(p[AB  ], x  , y-1, z   ),
                                     grad(p[BB  ], x-1, y-1, z   ))),
                     lerp(v, lerp(u, grad(p[AA+1], x  , y  , z-1 ),
                                     grad(p[BA+1], x-1, y  , z-1 )),
                             lerp(u, grad(p[AB+1], x  , y-1, z-1 ),
                                     grad(p[BB+1], x-1, y-1, z-1 ))));
   }

private:
   static float fade(float t) {return t * t * t * (t * (t * 6 - 15) + 10);}
   static float lerp(float t, float a, float b) {return a + t * (b - a);}
   static float grad(int hash, float x, float y, float z)
   {
      int h = hash & 15;
      float u = h<8 ? x : y,
            v = h<4 ? y : h==12||h==14 ? x : z;
      return ((h&1) == 0 ? u : -u) + ((h&2) == 0 ? v : -v);
   }

   static const unsigned char p[512];
};