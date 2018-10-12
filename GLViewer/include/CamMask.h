/*
	Grid.h
*/

#ifndef VDB_Nuke_CamMask_h__
#define VDB_Nuke_CamMask_h__

#include "VBA.h"

namespace koala { class CameraController; }

NUKEVDB_NAMESPACE_

class CamMask : private VBA
{
	GLuint mVBO;
public:

	void init( koala::CameraController &controller );
	void operator () ( koala::CameraController &controller );
	void operator () () const;
};

_NUKEVDB_NAMESPACE

#endif
