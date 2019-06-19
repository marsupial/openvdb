/* Coherent noise function over 1, 2 or 3 dimensions */
/* (copyright Ken Perlin) */

#include "noise.h"

NUKEVDB_NAMESPACE_

namespace noise
{

template <class T, unsigned N>
class VectorData;

template <class T> struct VectorData<T,1> { union { T data[1]; T x; }; };
template <class T> struct VectorData<T,2> { union { T data[2]; struct { T x, y; }; }; };
template <class T> struct VectorData<T,3> { union { T data[3]; struct { T x, y, z; }; }; };
template <class T> struct VectorData<T,4> { union { T data[4]; struct { T x, y, z, w; }; }; };

template <class T, unsigned N>
class Vector : public VectorData<T,N>
{
	template <class V> void copy( const V *vals ) { for ( unsigned i = 0; i < N; ++i ) this->data[i] = T(vals[i]); }
	void set( const T val) { for ( unsigned i = 0; i < N; ++i ) this->data[i] = T(val); }

public:
	enum { kNumElements = N, kSize = sizeof(T)*N };

	Vector() {};
	template <class V> Vector( const V &obj )   { copy(&obj.x); }
	template <class V> Vector( V * const vals ) { copy(vals); }
	Vector( T x );
	Vector( T x, T y );
	Vector( T x, T y, T z );
	Vector( T x, T y, T z, T w );
  
    T& operator [] ( int i )             { return this->data[i]; }
    const T& operator [] ( int i ) const { return this->data[i]; }

	void        operator *= ( T value ) { for ( unsigned i = 0; i < N; ++i ) this->data[i] *= value;   }
	Vector<T,N> operator *  ( T value ) { Vector<T,N> local(this->data); local *= value; return local; }

    void normalize()
    {
    	T sum = 0;
        for ( int i = 0; i < N; ++i )
          sum += (this->data[i] * this->data[i]);

        (*this) *= (1.f / sqrt(sum));
    }
  
    operator T* () { return this->data; }
    operator const T* () const { return this->data; }
    operator T () const;
};

typedef Vector<double, 1> Vector1d;
typedef Vector<double, 2> Vector2d;
typedef Vector<double, 3> Vector3d;

template<> Vector1d::Vector( double x_ )                       { this->x = x_; }
template<> Vector2d::Vector( double x_, double y_ )            { this->x = x_; this->y = y_; }
template<> Vector3d::Vector( double x_, double y_, double z_ ) { this->x = x_; this->y = y_; this->z = z_; }
template<> Vector1d::operator double () const                  { return this->x; }


static const int kSize = 256;

static int      kNoiseID[kSize + kSize + 2];
static Vector1d kNoise1D[kSize + kSize + 2];
static Vector2d kNoise2D[kSize + kSize + 2];
static Vector3d kNoise3D[kSize + kSize + 2];

inline double s_curve( double t )                  { return t * t * (3. - 2. * t); }
inline double lerp( double t, double a, double b ) { return a + t * (b - a); }

inline void setup( int &b0, int &b1, double &r0, double &r1, double &t, const double val )
{
  const int BM = 0xff,
            NP = 12,
            NM = 0xfff;

  t = val + pow(2, NP);
  b0 = int(t) & BM;
  b1 = (b0+1) & BM;
  r0 = t - int(t);
  r1 = r0 - 1.;
}

static double randf() {
  return (double)((rand() % (kSize + kSize)) - kSize) / kSize;
}

static void init()
{
  static bool sInited = false;
  if ( sInited ) return;
  sInited = true;

   int i, j, k;

   for (i = 0 ; i < kSize ; i++) {
      double rand[3] = { randf() }; //, randf(), randf() };
      kNoiseID[i] = i;
      kNoise1D[i] = Vector1d(rand);

      rand[0] = randf(); rand[1] = randf();
      kNoise2D[i] = Vector2d(rand);
      kNoise2D[i].normalize();

      rand[0] = randf(); rand[1] = randf(); rand[2] = randf();
      kNoise3D[i] = Vector3d(rand);
      kNoise3D[i].normalize();
   }

   while (--i) {
      k = kNoiseID[i];
      kNoiseID[i] = kNoiseID[j = rand() % kSize];
      kNoiseID[j] = k;
   }

   for (i = 0 ; i < kSize + 2 ; i++) {
      kNoiseID[kSize + i] = kNoiseID[i];
      kNoise1D[kSize + i] = kNoise1D[i];
      for (j = 0 ; j < 2 ; j++)
         kNoise2D[kSize + i][j] = kNoise2D[i][j];
      for (j = 0 ; j < 3 ; j++)
         kNoise3D[kSize + i][j] = kNoise3D[i][j];
   }
}

/*
   In what follows "alpha" is the weight when the sum is formed.
   Typically it is 2, As this approaches 1 the function is noisier.
   "beta" is the harmonic scaling/spacing, typically 2.
*/

template <>
double noise<Vector1d>::gen( const Vector1d &vec )
{
   init();

   int bx0, bx1;
   double rx0, rx1, sx, t, u, v;

   setup(bx0, bx1, rx0, rx1, t, vec);

   sx = s_curve(rx0);
   u = kNoise1D[ kNoiseID[ bx0 ] ] * rx0;
   v = kNoise1D[ kNoiseID[ bx1 ] ] * rx1;

   return(lerp(sx, u, v));
}

template <>
double noise<Vector2d>::gen( const Vector2d &vec )
{
   init();

   int bx0, bx1, by0, by1, b00, b10, b01, b11;
   double rx0, rx1, ry0, ry1, *q, sx, sy, a, b, t, u, v;

   setup(bx0,bx1, rx0,rx1, t,vec[0]);
   setup(by0,by1, ry0,ry1, t,vec[1]);

   int i = kNoiseID[ bx0 ],
       j = kNoiseID[ bx1 ];

   b00 = kNoiseID[ i + by0 ];
   b10 = kNoiseID[ j + by0 ];
   b01 = kNoiseID[ i + by1 ];
   b11 = kNoiseID[ j + by1 ];

   sx = s_curve(rx0);
   sy = s_curve(ry0);

#define at2(rx,ry)    ( rx * q[0] + ry * q[1] )

   q = kNoise2D[ b00 ] ; u = at2(rx0,ry0);
   q = kNoise2D[ b10 ] ; v = at2(rx1,ry0);
   a = lerp(sx, u, v);

   q = kNoise2D[ b01 ] ; u = at2(rx0,ry1);
   q = kNoise2D[ b11 ] ; v = at2(rx1,ry1);
   b = lerp(sx, u, v);

#undef at2

   return lerp(sy, a, b);
}

template <>
double noise<Vector3d>::gen( const Vector3d &vec )
{
   init();

   int bx0, bx1, by0, by1, bz0, bz1, b00, b10, b01, b11;
   double rx0, rx1, ry0, ry1, rz0, rz1, *q, sy, sz, a, b, c, d, t, u, v;

   setup(bx0,bx1, rx0,rx1, t,vec[0]);
   setup(by0,by1, ry0,ry1, t,vec[1] );
   setup(bz0,bz1, rz0,rz1, t,vec[2]);

   int i = kNoiseID[ bx0 ],
       j = kNoiseID[ bx1 ];

   b00 = kNoiseID[ i + by0 ];
   b10 = kNoiseID[ j + by0 ];
   b01 = kNoiseID[ i + by1 ];
   b11 = kNoiseID[ j + by1 ];

   t  = s_curve(rx0);
   sy = s_curve(ry0);
   sz = s_curve(rz0);

#define at3(rx,ry,rz) ( rx * q[0] + ry * q[1] + rz * q[2] )
   q = kNoise3D[ b00 + bz0 ] ; u = at3(rx0,ry0,rz0);
   q = kNoise3D[ b10 + bz0 ] ; v = at3(rx1,ry0,rz0);
   a = lerp(t, u, v);

   q = kNoise3D[ b01 + bz0 ] ; u = at3(rx0,ry1,rz0);
   q = kNoise3D[ b11 + bz0 ] ; v = at3(rx1,ry1,rz0);
   b = lerp(t, u, v);

   c = lerp(sy, a, b);

   q = kNoise3D[ b00 + bz1 ] ; u = at3(rx0,ry0,rz1);
   q = kNoise3D[ b10 + bz1 ] ; v = at3(rx1,ry0,rz1);
   a = lerp(t, u, v);

   q = kNoise3D[ b01 + bz1 ] ; u = at3(rx0,ry1,rz1);
   q = kNoise3D[ b11 + bz1 ] ; v = at3(rx1,ry1,rz1);
   b = lerp(t, u, v);

   d = lerp(sy, a, b);
#undef at3

   return lerp(sz, c, d);
}

template <class T>
double noise<T>::perlin( const T& p, double alpha, double beta, int n )
{
   double   sum   = 0,
            invAlpha = 1.f/alpha,
            invScale = 1.f;

   Vector<double, T::kNumElements> P(p);
   for ( int i = 0; i < n; ++i )
   {
      sum += noise< Vector<double, T::kNumElements> >::gen(P) * invScale;
      invScale *= invAlpha;
      P *= beta;
   }
   return sum;
}

double noise1( double arg )
{
  return noise<Vector1d>::gen( Vector1d(&arg) );
}

double noise2( double vec[2] )
{
  return noise<Vector2d>::gen( Vector2d(vec) );
}

double noise3( double vec[3] )
{
  return noise<Vector3d>::gen( Vector3d(vec) );
}

double perlin1D( double x, double alpha, double beta, int n )
{
  return noise<Vector1d>::perlin(Vector1d(&x), alpha, beta, n);
}

double perlin2D( double x, double y, double alpha, double beta, int n)
{
  return noise<Vector2d>::perlin(Vector2d(x, y), alpha, beta, n);
}

double perlin3D( double x, double y, double z, double alpha, double beta, int n )
{
  return noise<Vector3d>::perlin(Vector3d(x, y, z), alpha, beta, n);
}

void pyroclastic( int n, float r, void *ref, void* (*writeFunc) (void*, bool, const Vector3 &) )
{
    float frequency = 5.0f / n;
    float center = n / 2.0f + 0.5f;

	srand(300);
    for(int z=0; z < n; ++z) {
        for (int y=0; y < n; ++y) {
            for (int x=0; x < n; ++x) {
                float dx = center-x;
                float dy = center-y;
                float dz = center-z;

                float off = fabsf( perlin3D( x*frequency, y*frequency, z*frequency, 5, 6, 3) );

                float d = sqrtf(dx*dx+dy*dy+dz*dz)/(n);
                ref = writeFunc(ref, (d-off) < r, Vector3(x, y, z));
                //ref = writeFunc(ref, off > 0.25f, Vector3(x, y, z));
                //ref = writeFunc(ref, x==y==z, Vector3(x, y, z));
            }
        }
    }
}

} // namespace noise

_NUKEVDB_NAMESPACE
