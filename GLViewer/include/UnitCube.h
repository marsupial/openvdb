/*
	Grid.h
*/

#ifndef VDB_Nuke_UnitCube_h__
#define VDB_Nuke_UnitCube_h__

#include "VBA.h"

NUKEVDB_NAMESPACE_

class UnitCube : public VBA
{
public:

	void init();
	void operator () ();
};

_NUKEVDB_NAMESPACE

#endif
