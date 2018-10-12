/*
	AxisGrid.h
*/

#ifndef VDB_Nuke_AxisGrid_h__
#define VDB_Nuke_AxisGrid_h__

#include "VBA.h"

namespace koala {
  class CameraController;
}

NUKEVDB_NAMESPACE_

class AxisGrid
{
	const  GLuint kNumLines, kNumVerts;
    VBA           mGrid, mAxis;

	float_t setVerts( koala::Vector3 *verts, unsigned &start, unsigned N, float_t step , float_t X );
public:
	AxisGrid( GLuint n = 10 ) : kNumLines(n), kNumVerts(n*2 - 2*(n%2)) {}
    void init( GLuint P = 0 );
    void operator () ( GLfloat size, koala::CameraController& cam) const;
};

_NUKEVDB_NAMESPACE

#endif
