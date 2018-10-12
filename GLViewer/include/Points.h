/*
	Grid.h
*/

#ifndef VDB_Nuke_Points_h__
#define VDB_Nuke_Points_h__

#include "VBA.h"
#include <vector>

NUKEVDB_NAMESPACE_

class Points : public VBA
{
	GLsizei mNumPts;

public:
    static void glSetup();

	void init( std::vector<koala::Vector3>& );
    void operator () ();
};

_NUKEVDB_NAMESPACE

#endif
