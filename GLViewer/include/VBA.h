/*
	VBA.h
*/

#ifndef VDB_Nuke_VBA_h__
#define VDB_Nuke_VBA_h__

#include <OpenGL/gl3.h>

NUKEVDB_NAMESPACE_

class VBA
{
protected:
	GLuint mVBA;

public:
	VBA() : mVBA(0) {}
    ~VBA();

    void create( const GLfloat *vertices, const size_t Nv, GLint vE,
                 const GLuint *elements = 0, const size_t Ne = 0, GLuint *buffOUT = NULL );

    void update( const GLfloat *vertices, const size_t Nv, GLint vE, GLuint buffer, GLuint loc = 0 );
    void add( const GLfloat *vertices, const size_t Nv, GLint vE, GLuint loc );

    void bind()  const;
    void operator() () const { bind(); }
    void operator() (GLenum mode, GLsizei count, GLenum type, const GLvoid *indices = NULL) const;
};

_NUKEVDB_NAMESPACE

#endif
