/*
	noise.h
*/

#include "VDB_Nuke.h"
#include "VDBMath.h"

NUKEVDB_NAMESPACE_

namespace noise
{
  template <class T>
  struct noise
  {
  	static double gen( const T& );
  	static double perlin( const T&, double alpha, double beta, int n );
  };

  double noise1(double);
  double noise2(double*);
  double noise3(double*);

  double perlin1D(double,double,double,int);
  double perlin2D(double,double,double,double,int);
  double perlin3D(double,double,double,double,double,int);

  void pyroclastic( int n, float r, void *ref, void* (*writeFunc) (void*, bool, const Vector3 &) );
}

_NUKEVDB_NAMESPACE
