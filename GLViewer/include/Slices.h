/*
	Grid.h
*/

#ifndef VDB_Nuke_Slices_h__
#define VDB_Nuke_Slices_h__

#include "VBA.h"

NUKEVDB_NAMESPACE_

class Slices : public VBA
{
	GLuint  mNumPoints;

public:

	Slices() : mNumPoints(0) {}
	void init( const koala::Box3 &volume, const koala::Vector3 &viewDir, const unsigned numSlices = 256 );
	void operator () ();
};

_NUKEVDB_NAMESPACE

#endif
