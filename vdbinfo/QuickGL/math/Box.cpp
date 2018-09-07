/*
	Box.cpp
*/

#include "Box.h"
#include "Matrix4.h"

NAMESPACE_KOALA_

static void
affineTransform( const Matrix4 &m, Box3f &box )
{	
	Vector3 nmin, nmax;
	for ( unsigned i = 0; i < 3; ++i )
	{
		nmin[i] = nmax[i] = m[3][i];

		for ( unsigned j = 0; j < 3; ++j )
		{
			float_t a, b;

			a = m[j][i] * box.min[j];
			b = m[j][i] * box.max[j];

			if ( a < b ) 
			{
				nmin[i] += a;
				nmax[i] += b;
			}
			else 
			{
				nmin[i] += b;
				nmax[i] += a;
			}
		}
	}
	box.min = nmin;
	box.max = nmax;
}

template <> void
Box3f::affine( const Matrix4 &m )
{
	//
	// Transform a 3D box by a matrix whose rightmost column
	// is (0 0 0 1), and compute a new box that tightly encloses
	// the transformed box.
	//
	// As in the transform() function, above, we use James Arvo's
	// fast method.
	//

	// A transformed empty or infinite box is still empty or infinite
	if ( !isEmpty() && !isInfinite() )
		affineTransform(m, *this);
}

template <> void
Box3f::transform( const Matrix4 &m )
{
	// Transform a 3D box by a matrix, and compute a new box that
	// tightly encloses the transformed box.
	//
	// If m is an affine transform, then we use James Arvo's fast
	// method as described in "Graphics Gems", Academic Press, 1990,
	// pp. 548-550.

	//
	// A transformed empty box is still empty, and a transformed infinite box
	// is still infinite

	if ( isEmpty() || isInfinite() )
		return;


	// If the last column of m is (0 0 0 1) then m is an affine
	// transform, and we use the fast Graphics Gems trick.

	if (m[0][3] == 0 && m[1][3] == 0 && m[2][3] == 0 && m[3][3] == 1)
		return affineTransform(m, *this);

	//
	// M is a projection matrix.  Do things the naive way:
	// Transform the eight corners of the box, and find an
	// axis-parallel box that encloses the transformed corners.
	//

	Vector3 points[8];

	points[0][0] = points[1][0] = points[2][0] = points[3][0] = min[0];
	points[4][0] = points[5][0] = points[6][0] = points[7][0] = max[0];

	points[0][1] = points[1][1] = points[4][1] = points[5][1] = min[1];
	points[2][1] = points[3][1] = points[6][1] = points[7][1] = max[1];

	points[0][2] = points[2][2] = points[4][2] = points[6][2] = min[2];
	points[1][2] = points[3][2] = points[5][2] = points[7][2] = max[2];

	for ( unsigned i = 0; i < 8; ++i )
		extendBy( m * points[i] );
}

_NAMESPACE_KOALA
