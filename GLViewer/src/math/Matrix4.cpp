/*
	Matrix4.cpp
*/

#include "Matrix4.h"

#include <string.h> //memset

NAMESPACE_KOALA_

const Matrix4 Matrix4::kIdentity(1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1);

Matrix4
Matrix4::lookAt( const Vector3 &eyePos, const Vector3 &centerPos, const Vector3 &upDir )
{
	Vector3 forward, side, up;
	Matrix4 m;

	forward = centerPos - eyePos;
	up = upDir;

	forward.normalize();

	// Side = forward x up
	side = forward.cross(up);
	side.normalize();

	// Recompute up as: up = side x forward
	up = side.cross(forward);

	m.m[0][0] = side.x;
	m.m[1][0] = side.y;
	m.m[2][0] = side.z;

	m.m[0][1] = up.x;
	m.m[1][1] = up.y;
	m.m[2][1] = up.z;

	m.m[0][2] = -forward.x;
	m.m[1][2] = -forward.y;
	m.m[2][2] = -forward.z;

	m.m[3][0] = m.m[3][1] = m.m[3][2] = 0;
	m.m[0][3] = m.m[1][3] = m.m[2][3] = 0;
	m.m[3][3] = 1;

	m = Matrix4(-eyePos.x, -eyePos.y, -eyePos.z) * m;
	return m;
}


Matrix4
Matrix4::ortho( float_t left, float_t right, float_t bottom, float_t top, float_t zNear, float_t zFar)
{
	const float_t iWidth = 1.0  / (right  - left), iHeight = 1.0 / (top - bottom), iDepth = 1.0  / (zFar - zNear);

	return Matrix4( 2 * iWidth,  0, 0, 0,
	                0, 2 * iHeight, 0, 0,
	                0, 0, -2 * iDepth, 0,
	                -(right + left) * iWidth, -(top + bottom) * iHeight, -(zFar + zNear) * iDepth, 1);
}

void
Matrix4::alignZAxisWithTargetDir( Vector3 targetDir, Vector3 upDir, Matrix4 &result )
{
	// Ensure that the target direction is non-zero.
	if ( targetDir.length () == 0 )
		targetDir.set(0, 0, 1);

	// Ensure that the up direction is non-zero.
	if ( upDir.length () == 0 )
		upDir.set(0, 1, 0);

	// Check for degeneracies.  If the upDir and targetDir are parallel 
	// or opposite, then compute a new, arbitrary up direction that is
	// not parallel or opposite to the targetDir.

	if ( upDir.cross(targetDir).length () == 0 )
	{
		upDir = targetDir.cross(Vector3(1, 0, 0));
		if ( upDir.length() == 0 )
			upDir = targetDir.cross(Vector3(0, 0, 1));
	}

	// Compute the x-, y-, and z-axis vectors of the new coordinate system.

	Vector3 targetPerpDir = upDir.cross(targetDir),
	        targetUpDir   = targetDir.cross (targetPerpDir);

	// Rotate the x-axis into targetPerpDir (row 0),
	// rotate the y-axis into targetUpDir   (row 1),
	// rotate the z-axis into targetDir     (row 2).

	Vector3 row[3];
	row[0] = targetPerpDir.normalized();
	row[1] = targetUpDir  .normalized();
	row[2] = targetDir    .normalized();

	result.m[0][0] = row[0][0];
	result.m[0][1] = row[0][1];
	result.m[0][2] = row[0][2];
	result.m[0][3] = 0;

	result.m[1][0] = row[1][0];
	result.m[1][1] = row[1][1];
	result.m[1][2] = row[1][2];
	result.m[1][3] = 0;

	result.m[2][0] = row[2][0];
	result.m[2][1] = row[2][1];
	result.m[2][2] = row[2][2];
	result.m[2][3] = 0;

	result.m[3][0] = 0;
	result.m[3][1] = 0;
	result.m[3][2] = 0;
	result.m[3][3] = 1;
}


Matrix4
Matrix4::rotationMatrixWithUpDir( const Vector3 &fromDir, const Vector3 &toDir, const Vector3 &upDir )
{

	// The goal is to obtain a rotation matrix that takes 
	// "fromDir" to "toDir".  We do this in two steps and 
	// compose the resulting rotation matrices; 
	//    (a) rotate "fromDir" into the z-axis
	//    (b) rotate the z-axis into "toDir"
	//

	// The from direction must be non-zero; but we allow zero to and up dirs.
	if ( fromDir.length () != 0 )
	{
		Matrix4 zAxis2FromDir;
		alignZAxisWithTargetDir(fromDir, Vector3(0, 1, 0), zAxis2FromDir);

		Matrix4 fromDir2zAxis = zAxis2FromDir.transposed();
	
		Matrix4 zAxis2ToDir;
		alignZAxisWithTargetDir(toDir, upDir, zAxis2ToDir);

		return fromDir2zAxis * zAxis2ToDir;
	}
	return kIdentity;
}


Matrix4
Matrix4::concatenate( const Matrix4 &mat ) const
{
	Matrix4 r;
	r.m[0][0] = m[0][0] * mat.m[0][0] + m[1][0] * mat.m[0][1] + m[2][0] * mat.m[0][2] + m[3][0] * mat.m[0][3];
	r.m[1][0] = m[0][0] * mat.m[1][0] + m[1][0] * mat.m[1][1] + m[2][0] * mat.m[1][2] + m[3][0] * mat.m[1][3];
	r.m[2][0] = m[0][0] * mat.m[2][0] + m[1][0] * mat.m[2][1] + m[2][0] * mat.m[2][2] + m[3][0] * mat.m[2][3];
	r.m[3][0] = m[0][0] * mat.m[3][0] + m[1][0] * mat.m[3][1] + m[2][0] * mat.m[3][2] + m[3][0] * mat.m[3][3];

	r.m[0][1] = m[0][1] * mat.m[0][0] + m[1][1] * mat.m[0][1] + m[2][1] * mat.m[0][2] + m[3][1] * mat.m[0][3];
	r.m[1][1] = m[0][1] * mat.m[1][0] + m[1][1] * mat.m[1][1] + m[2][1] * mat.m[1][2] + m[3][1] * mat.m[1][3];
	r.m[2][1] = m[0][1] * mat.m[2][0] + m[1][1] * mat.m[2][1] + m[2][1] * mat.m[2][2] + m[3][1] * mat.m[2][3];
	r.m[3][1] = m[0][1] * mat.m[3][0] + m[1][1] * mat.m[3][1] + m[2][1] * mat.m[3][2] + m[3][1] * mat.m[3][3];

	r.m[0][2] = m[0][2] * mat.m[0][0] + m[1][2] * mat.m[0][1] + m[2][2] * mat.m[0][2] + m[3][2] * mat.m[0][3];
	r.m[1][2] = m[0][2] * mat.m[1][0] + m[1][2] * mat.m[1][1] + m[2][2] * mat.m[1][2] + m[3][2] * mat.m[1][3];
	r.m[2][2] = m[0][2] * mat.m[2][0] + m[1][2] * mat.m[2][1] + m[2][2] * mat.m[2][2] + m[3][2] * mat.m[2][3];
	r.m[3][2] = m[0][2] * mat.m[3][0] + m[1][2] * mat.m[3][1] + m[2][2] * mat.m[3][2] + m[3][2] * mat.m[3][3];

	r.m[0][3] = m[0][3] * mat.m[0][0] + m[1][3] * mat.m[0][1] + m[2][3] * mat.m[0][2] + m[3][3] * mat.m[0][3];
	r.m[1][3] = m[0][3] * mat.m[1][0] + m[1][3] * mat.m[1][1] + m[2][3] * mat.m[1][2] + m[3][3] * mat.m[1][3];
	r.m[2][3] = m[0][3] * mat.m[2][0] + m[1][3] * mat.m[2][1] + m[2][3] * mat.m[2][2] + m[3][3] * mat.m[2][3];
	r.m[3][3] = m[0][3] * mat.m[3][0] + m[1][3] * mat.m[3][1] + m[2][3] * mat.m[3][2] + m[3][3] * mat.m[3][3];

	return r;
}

void
Matrix4::multiply( const Matrix4 &a, const Matrix4 &b, Matrix4 &c )
{
	register const float_t *ap = &a.m[0][0];
	register const float_t *bp = &b.m[0][0];
	register       float_t *cp = &c.m[0][0];

	register float_t a0, a1, a2, a3;

	a0 = ap[0];
	a1 = ap[1];
	a2 = ap[2];
	a3 = ap[3];

	cp[0]  = a0 * bp[0]  + a1 * bp[4]  + a2 * bp[8]  + a3 * bp[12];
	cp[1]  = a0 * bp[1]  + a1 * bp[5]  + a2 * bp[9]  + a3 * bp[13];
	cp[2]  = a0 * bp[2]  + a1 * bp[6]  + a2 * bp[10] + a3 * bp[14];
	cp[3]  = a0 * bp[3]  + a1 * bp[7]  + a2 * bp[11] + a3 * bp[15];

	a0 = ap[4];
	a1 = ap[5];
	a2 = ap[6];
	a3 = ap[7];

	cp[4]  = a0 * bp[0]  + a1 * bp[4]  + a2 * bp[8]  + a3 * bp[12];
	cp[5]  = a0 * bp[1]  + a1 * bp[5]  + a2 * bp[9]  + a3 * bp[13];
	cp[6]  = a0 * bp[2]  + a1 * bp[6]  + a2 * bp[10] + a3 * bp[14];
	cp[7]  = a0 * bp[3]  + a1 * bp[7]  + a2 * bp[11] + a3 * bp[15];

	a0 = ap[8];
	a1 = ap[9];
	a2 = ap[10];
	a3 = ap[11];

	cp[8]  = a0 * bp[0]  + a1 * bp[4]  + a2 * bp[8]  + a3 * bp[12];
	cp[9]  = a0 * bp[1]  + a1 * bp[5]  + a2 * bp[9]  + a3 * bp[13];
	cp[10] = a0 * bp[2]  + a1 * bp[6]  + a2 * bp[10] + a3 * bp[14];
	cp[11] = a0 * bp[3]  + a1 * bp[7]  + a2 * bp[11] + a3 * bp[15];

	a0 = ap[12];
	a1 = ap[13];
	a2 = ap[14];
	a3 = ap[15];

	cp[12] = a0 * bp[0]  + a1 * bp[4]  + a2 * bp[8]  + a3 * bp[12];
	cp[13] = a0 * bp[1]  + a1 * bp[5]  + a2 * bp[9]  + a3 * bp[13];
	cp[14] = a0 * bp[2]  + a1 * bp[6]  + a2 * bp[10] + a3 * bp[14];
	cp[15] = a0 * bp[3]  + a1 * bp[7]  + a2 * bp[11] + a3 * bp[15];
}

static Matrix4
gjInverse( const Matrix4 &m )
{
    int i, j, k;
    Matrix4 s;
    Matrix4 t(m);

    // Forward elimination

    for ( i = 0; i < 3 ; ++i )
    {
        int pivot = i;

        float_t pivotsize = t[i][i];

        if (pivotsize < 0)
            pivotsize = -pivotsize;

        for ( j = i + 1; j < 4; ++j )
        {
            float_t tmp = t[j][i];

            if (tmp < 0)
                tmp = -tmp;

            if (tmp > pivotsize)
            {
                pivot = j;
                pivotsize = tmp;
            }
        }

        if ( pivotsize == 0 )
					return Matrix4::kIdentity;

        if ( pivot != i )
        {
            for (j = 0; j < 4; j++)
            {
                float_t tmp;

                tmp = t[i][j];
                t[i][j] = t[pivot][j];
                t[pivot][j] = tmp;

                tmp = s[i][j];
                s[i][j] = s[pivot][j];
                s[pivot][j] = tmp;
            }
        }

        for ( j = i + 1; j < 4; ++j )
        {
            float_t f = t[j][i] / t[i][i];

            for (k = 0; k < 4; k++)
            {
                t[j][k] -= f * t[i][k];
                s[j][k] -= f * s[i][k];
            }
        }
    }

    // Backward substitution
    for ( i = 3; i >= 0; --i )
    {
        float_t f;

        if ( (f = t[i][i]) == 0 )
					return Matrix4::kIdentity;

        for ( j = 0; j < 4; ++j )
        {
            t[i][j] /= f;
            s[i][j] /= f;
        }

        for ( j = 0; j < i; ++j )
        {
            f = t[j][i];

            for (k = 0; k < 4; k++)
            {
                t[j][k] -= f * t[i][k];
                s[j][k] -= f * s[i][k];
            }
        }
    }

    return s;
}

Matrix4
Matrix4::inverse() const
{
	if ( m[0][3] != 0 || m[1][3] != 0 || m[2][3] != 0 || m[3][3] != 1 )
		return gjInverse(*this);

	Matrix4 s( m[1][1] * m[2][2] - m[2][1] * m[1][2], m[2][1] * m[0][2] - m[0][1] * m[2][2], m[0][1] * m[1][2] - m[1][1] * m[0][2], 0,
	           m[2][0] * m[1][2] - m[1][0] * m[2][2], m[0][0] * m[2][2] - m[2][0] * m[0][2], m[1][0] * m[0][2] - m[0][0] * m[1][2], 0,
	           m[1][0] * m[2][1] - m[2][0] * m[1][1], m[2][0] * m[0][1] - m[0][0] * m[2][1], m[0][0] * m[1][1] - m[1][0] * m[0][1], 0,
	           0, 0, 0, 1 );

	const float_t r = m[0][0] * s[0][0] + m[0][1] * s[1][0] + m[0][2] * s[2][0];
	if ( math::abs (r) >= 1 )
	{
			for ( unsigned i = 0; i < 3; ++i )
			{
					for ( unsigned j = 0; j < 3; ++j )
							s[i][j] /= r;
			}
	}
	else
	{
			const float_t mr = math::abs (r) / math::numeric_limits<float_t>::min();
			for ( unsigned i = 0; i < 3; ++i )
			{
					for ( unsigned j = 0; j < 3; ++j )
					{
							if ( mr > math::abs (s[i][j]) )
									s[i][j] /= r;
							else
								return Matrix4::kIdentity;
					}
			}
	}

	s[3][0] = -m[3][0] * s[0][0] - m[3][1] * s[1][0] - m[3][2] * s[2][0];
	s[3][1] = -m[3][0] * s[0][1] - m[3][1] * s[1][1] - m[3][2] * s[2][1];
	s[3][2] = -m[3][0] * s[0][2] - m[3][1] * s[1][2] - m[3][2] * s[2][2];

	return s;
}

float_t
Matrix4::determinant() const
{
	return m[3][0] * m[2][1] * m[1][2] * m[0][3] - m[2][0] * m[3][1] * m[1][2] * m[0][3]
	     - m[3][0] * m[1][1] * m[2][2] * m[0][3] + m[1][0] * m[3][1] * m[2][2] * m[0][3]

	     + m[2][0] * m[1][1] * m[3][2] * m[0][3] - m[1][0] * m[2][1] * m[3][2] * m[0][3]
	     - m[3][0] * m[2][1] * m[0][2] * m[1][3] + m[2][0] * m[3][1] * m[0][2] * m[1][3]

	     + m[3][0] * m[0][1] * m[2][2] * m[1][3] - m[0][0] * m[3][1] * m[2][2] * m[1][3]
	     - m[2][0] * m[0][1] * m[3][2] * m[1][3] + m[0][0] * m[2][1] * m[3][2] * m[1][3]

	     + m[3][0] * m[1][1] * m[0][2] * m[2][3] - m[1][0] * m[3][1] * m[0][2] * m[2][3]
	     - m[3][0] * m[0][1] * m[1][2] * m[2][3] + m[0][0] * m[3][1] * m[1][2] * m[2][3]

	     + m[1][0] * m[0][1] * m[3][2] * m[2][3] - m[0][0] * m[1][1] * m[3][2] * m[2][3]
	     - m[2][0] * m[1][1] * m[0][2] * m[3][3] + m[1][0] * m[2][1] * m[0][2] * m[3][3]

	     + m[2][0] * m[0][1] * m[1][2] * m[3][3] - m[0][0] * m[2][1] * m[1][2] * m[3][3]
	     - m[1][0] * m[0][1] * m[2][2] * m[3][3] + m[0][0] * m[1][1] * m[2][2] * m[3][3];
}

Matrix4
Matrix4::operator + ( const Matrix4 &mat ) const
{
	Matrix4 rmat;
	for ( unsigned r = 0; r < 4; ++r )
		for ( unsigned c = 0; c < 4; ++c )
			rmat.m[r][c] = m[r][c] + mat.m[r][c];
	return rmat;
}

Matrix4
Matrix4::operator - ( const Matrix4 &mat ) const
{
	Matrix4 rmat;
	for ( unsigned r = 0; r < 4; ++r )
		for ( unsigned c = 0; c < 4; ++c )
			rmat.m[r][c] = m[r][c] - mat.m[r][c];
	return rmat;
}

bool
Matrix4::operator == ( const Matrix4& m2 ) const
{
	return ( m[0][0] != m2.m[0][0] || m[0][1] != m2.m[0][1] || m[0][2] != m2.m[0][2] || m[0][3] != m2.m[0][3] ||
	         m[1][0] != m2.m[1][0] || m[1][1] != m2.m[1][1] || m[1][2] != m2.m[1][2] || m[1][3] != m2.m[1][3] ||
	         m[2][0] != m2.m[2][0] || m[2][1] != m2.m[2][1] || m[2][2] != m2.m[2][2] || m[2][3] != m2.m[2][3] ||
	         m[3][0] != m2.m[3][0] || m[3][1] != m2.m[3][1] || m[3][2] != m2.m[3][2] || m[3][3] != m2.m[3][3] ) ? false : true;
}

bool
Matrix4::operator != ( const Matrix4& m2 ) const
{
	return ( m[0][0] != m2.m[0][0] || m[0][1] != m2.m[0][1] || m[0][2] != m2.m[0][2] || m[0][3] != m2.m[0][3] ||
	         m[1][0] != m2.m[1][0] || m[1][1] != m2.m[1][1] || m[1][2] != m2.m[1][2] || m[1][3] != m2.m[1][3] ||
	         m[2][0] != m2.m[2][0] || m[2][1] != m2.m[2][1] || m[2][2] != m2.m[2][2] || m[2][3] != m2.m[2][3] ||
	         m[3][0] != m2.m[3][0] || m[3][1] != m2.m[3][1] || m[3][2] != m2.m[3][2] || m[3][3] != m2.m[3][3] ) ? true : false;
}

void
Matrix4::setTranslation( const Vector3 &t )
{
/*
	m[0][0] = 1;
	m[0][1] = 0;
	m[0][2] = 0;
	m[0][3] = 0;

	m[1][0] = 0;
	m[1][1] = 1;
	m[1][2] = 0;
	m[1][3] = 0;

	m[2][0] = 0;
	m[2][1] = 0;
	m[2][2] = 1;
	m[2][3] = 0;
*/
	m[3][0] = t[0];
	m[3][1] = t[1];
	m[3][2] = t[2];
	m[3][3] = 1;
}

void
Matrix4::translate( float_t x, float_t y, float_t z )
{
	m[3][0] += x * m[0][0] + y * m[1][0] + z * m[2][0];
	m[3][1] += x * m[0][1] + y * m[1][1] + z * m[2][1];
	m[3][2] += x * m[0][2] + y * m[1][2] + z * m[2][2];
	m[3][3] += x * m[0][3] + y * m[1][3] + z * m[2][3];
}

void
Matrix4::setScale( const Vector3 &s )
{
	::memset(m, 0, sizeof(m));
	m[0][0] = s[0];
	m[1][1] = s[1];
	m[2][2] = s[2];
	m[3][3] = 1;
}

void
Matrix4::scale( const Vector3 &s )
{
	m[0][0] *= s[0];
	m[0][1] *= s[0];
	m[0][2] *= s[0];
	m[0][3] *= s[0];

	m[1][0] *= s[1];
	m[1][1] *= s[1];
	m[1][2] *= s[1];
	m[1][3] *= s[1];

	m[2][0] *= s[2];
	m[2][1] *= s[2];
	m[2][2] *= s[2];
	m[2][3] *= s[2];
}

void
Matrix4::rotate( const Vector3 &r )
{
	float_t cos_rz, sin_rz, cos_ry, sin_ry, cos_rx, sin_rx,
	        m00, m01, m02,
	        m10, m11, m12,
	        m20, m21, m22;

	cos_rz = cos(r[2]);
	cos_ry = cos(r[1]);
	cos_rx = cos(r[0]);

	sin_rz = sin(r[2]);
	sin_ry = sin(r[1]);
	sin_rx = sin(r[0]);

	m00 =  cos_rz *  cos_ry;
	m01 =  sin_rz *  cos_ry;
	m02 = -sin_ry;
	m10 = -sin_rz *  cos_rx + cos_rz * sin_ry * sin_rx;
	m11 =  cos_rz *  cos_rx + sin_rz * sin_ry * sin_rx;
	m12 =  cos_ry *  sin_rx;
	m20 = -sin_rz * -sin_rx + cos_rz * sin_ry * cos_rx;
	m21 =  cos_rz * -sin_rx + sin_rz * sin_ry * cos_rx;
	m22 =  cos_ry *  cos_rx;

	Matrix4 P (*this);

	m[0][0] = P[0][0] * m00 + P[1][0] * m01 + P[2][0] * m02;
	m[0][1] = P[0][1] * m00 + P[1][1] * m01 + P[2][1] * m02;
	m[0][2] = P[0][2] * m00 + P[1][2] * m01 + P[2][2] * m02;
	m[0][3] = P[0][3] * m00 + P[1][3] * m01 + P[2][3] * m02;

	m[1][0] = P[0][0] * m10 + P[1][0] * m11 + P[2][0] * m12;
	m[1][1] = P[0][1] * m10 + P[1][1] * m11 + P[2][1] * m12;
	m[1][2] = P[0][2] * m10 + P[1][2] * m11 + P[2][2] * m12;
	m[1][3] = P[0][3] * m10 + P[1][3] * m11 + P[2][3] * m12;

	m[2][0] = P[0][0] * m20 + P[1][0] * m21 + P[2][0] * m22;
	m[2][1] = P[0][1] * m20 + P[1][1] * m21 + P[2][1] * m22;
	m[2][2] = P[0][2] * m20 + P[1][2] * m21 + P[2][2] * m22;
	m[2][3] = P[0][3] * m20 + P[1][3] * m21 + P[2][3] * m22;
}

void
Matrix4::setAxisAngle( const Vector3& axis, float_t angle )
{
	const Vector3 unit = axis.normalized();
	const float_t sine   = math::sin(angle),
	              cosine = math::cos(angle);

	m[0][0] = unit[0] * unit[0] * (1 - cosine) + cosine;
	m[0][1] = unit[0] * unit[1] * (1 - cosine) + unit[2] * sine;
	m[0][2] = unit[0] * unit[2] * (1 - cosine) - unit[1] * sine;
	m[0][3] = 0;

	m[1][0] = unit[0] * unit[1] * (1 - cosine) - unit[2] * sine;
	m[1][1] = unit[1] * unit[1] * (1 - cosine) + cosine;
	m[1][2] = unit[1] * unit[2] * (1 - cosine) + unit[0] * sine;
	m[1][3] = 0;

	m[2][0] = unit[0] * unit[2] * (1 - cosine) + unit[1] * sine;
	m[2][1] = unit[1] * unit[2] * (1 - cosine) - unit[0] * sine;
	m[2][2] = unit[2] * unit[2] * (1 - cosine) + cosine;
	m[2][3] = 0;

	m[3][0] = 0;
	m[3][1] = 0;
	m[3][2] = 0;
	m[3][3] = 1;
}

void
Matrix4::multDirMatrix( const Vector3 &src, Vector3 &dst) const
{
	float_t a, b, c;

	a = src[0] * m[0][0] + src[1] * m[1][0] + src[2] * m[2][0];
	b = src[0] * m[0][1] + src[1] * m[1][1] + src[2] * m[2][1];
	c = src[0] * m[0][2] + src[1] * m[1][2] + src[2] * m[2][2];

	dst.x = a;
	dst.y = b;
	dst.z = c;
}

#if 0

bool		
checkForZeroScaleInRow( const float_t& scl, const Vector3 &row, bool exc = true )
{
	for ( unsigned i=0; i < 3; ++i )
	{
		if ( ( math::abs(scl) < 1 && math::abs(row[i]) >= math::numeric_limits<float_t>::max() * math::abs(scl)) )
		{
			/*if ( exc ) throw IMATH_INTERNAL_NAMESPACE::ZeroScaleExc ("Cannot remove zero scaling from matrix.");
	    else
			*/
			return false;
		}
	}
	return true;
}

bool
extractAndRemoveScalingAndShear( Matrix4 &mat, Vector3 &scl, Vector3 &shr, bool exc )
{
	//
	// This implementation follows the technique described in the paper by
	// Spencer W. Thomas in the Graphics Gems II article: "Decomposing a 
	// Matrix into Simple Transformations", p. 320.
	//

	Vector3 row[3];

	row[0] = Vector3 (mat[0][0], mat[0][1], mat[0][2]);
	row[1] = Vector3 (mat[1][0], mat[1][1], mat[1][2]);
	row[2] = Vector3 (mat[2][0], mat[2][1], mat[2][2]);

	float_t maxVal = 0;
	for ( unsigned i=0; i < 3; ++i )
		for ( unsigned j=0; j < 3; ++j )
			if ( math::abs (row[i][j]) > maxVal )
				maxVal = math::abs(row[i][j]);

	//
	// We normalize the 3x3 matrix here.
	// It was noticed that this can improve numerical stability significantly,
	// especially when many of the upper 3x3 matrix's coefficients are very
	// close to zero; we correct for this step at the end by multiplying the 
	// scaling factors by maxVal at the end (shear and rotation are not 
	// affected by the normalization).

	if ( maxVal != 0 )
	{
		for ( unsigned i=0; i < 3; ++i )
			if ( !checkForZeroScaleInRow(maxVal, row[i], exc) )
				return false;
			else
				row[i] /= maxVal;
	}

	// Compute X scale factor. 
	scl.x = row[0].length ();
	if ( !checkForZeroScaleInRow (scl.x, row[0], exc) )
		return false;

	// Normalize first row.
	row[0] /= scl.x;

	// An XY shear factor will shear the X coord. as the Y coord. changes.
	// There are 6 combinations (XY, XZ, YZ, YX, ZX, ZY), although we only
	// extract the first 3 because we can effect the last 3 by shearing in
	// XY, XZ, YZ combined rotations and scales.
	//
	// shear matrix <   1,  YX,  ZX,  0,
	//                 XY,   1,  ZY,  0,
	//                 XZ,  YZ,   1,  0,
	//                  0,   0,   0,  1 >

	// Compute XY shear factor and make 2nd row orthogonal to 1st.
	shr[0]  = row[0].dot (row[1]);
	row[1] -= row[0] * shr[0];

	// Now, compute Y scale.
	scl.y = row[1].length ();
	if ( !checkForZeroScaleInRow (scl.y, row[1], exc) )
		return false;

	// Normalize 2nd row and correct the XY shear factor for Y scaling.
	row[1] /= scl.y; 
	shr[0] /= scl.y;

	// Compute XZ and YZ shears, orthogonalize 3rd row.
	shr[1]  = row[0].dot (row[2]);
	row[2] -= row[0] * shr[1];
	shr[2]  = row[1].dot (row[2]);
	row[2] -= row[1] * shr[2];

	// Next, get Z scale.
	scl.z = row[2].length ();
	if ( !checkForZeroScaleInRow(scl.z, row[2], exc) )
		return false;

	// Normalize 3rd row and correct the XZ and YZ shear factors for Z scaling.
	row[2] /= scl.z;
	shr[1] /= scl.z;
	shr[2] /= scl.z;

	// At this point, the upper 3x3 matrix in mat is orthonormal.
	// Check for a coordinate system flip. If the determinant
	// is less than zero, then negate the matrix and the scaling factors.
	if ( row[0].dot (row[1].cross (row[2])) < 0 )
	for ( unsigned i=0; i < 3; ++i )
	{
		scl[i] *= -1;
		row[i] *= -1;
	}

	// Copy over the orthonormal rows into the returned matrix.
	// The upper 3x3 matrix in mat is now a rotation matrix.
	for ( unsigned i=0; i < 3; ++i )
	{
		mat[i][0] = row[i][0]; 
		mat[i][1] = row[i][1]; 
		mat[i][2] = row[i][2];
	}

	// Correct the scaling factors for the normalization step that we 
	// performed above; shear and rotation are not affected by the 
	// normalization.
	scl *= maxVal;

	return true;
}

void
extractEulerXYZ (const Matrix4 &mat, Vector3 &rot)
{
	//
	// Normalize the local x, y and z axes to remove scaling.
	//

	Vector3 i (mat[0][0], mat[0][1], mat[0][2]);
	Vector3 j (mat[1][0], mat[1][1], mat[1][2]);
	Vector3 k (mat[2][0], mat[2][1], mat[2][2]);

	i.normalize();
	j.normalize();
	k.normalize();

	Matrix4 M( i[0], i[1], i[2], 0, 
	           j[0], j[1], j[2], 0, 
	           k[0], k[1], k[2], 0, 
	           0,    0,    0,    1);

	//
	// Extract the first angle, rot.x.
	// 

	rot.x = math::atan2(M[1][2], M[2][2]);

	//
	// Remove the rot.x rotation from M, so that the remaining
	// rotation, N, is only around two axes, and gimbal lock
	// cannot occur.
	//

	Matrix4 N;
	N.rotate(Vector3 (-rot.x, 0, 0));
	N = N * M;

	//
	// Extract the other two angles, rot.y and rot.z, from N.
	//

	float_t cy = math::sqrt(N[0][0]*N[0][0] + N[0][1]*N[0][1]);
	rot.y = math::atan2(-N[0][2], cy);
	rot.z = math::atan2(-N[1][0], N[1][1]);
}

void
extractEulerZYX( const Matrix4 &mat, Vector3 &rot )
{
	//
	// Normalize the local x, y and z axes to remove scaling.
	//

	Vector3 i (mat[0][0], mat[0][1], mat[0][2]);
	Vector3 j (mat[1][0], mat[1][1], mat[1][2]);
	Vector3 k (mat[2][0], mat[2][1], mat[2][2]);

	i.normalize();
	j.normalize();
	k.normalize();

	Matrix4 M( i[0], i[1], i[2], 0, 
	           j[0], j[1], j[2], 0, 
	           k[0], k[1], k[2], 0, 
	           0,    0,    0,    1);

	//
	// Extract the first angle, rot.x.
	// 

	rot.x = -math::atan2(M[1][0], M[0][0]);

	//
	// Remove the x rotation from M, so that the remaining
	// rotation, N, is only around two axes, and gimbal lock
	// cannot occur.
	//

	Matrix4 N;
	N.rotate (Vector3 (0, 0, -rot.x));
	N = N * M;

	//
	// Extract the other two angles, rot.y and rot.z, from N.
	//

	float_t cy = math::sqrt(N[2][2]*N[2][2] + N[2][1]*N[2][1]);
	rot.y = -math::atan2(-N[2][0], cy);
	rot.z = -math::atan2(-N[1][2], N[1][1]);
}

#endif

_NAMESPACE_KOALA
