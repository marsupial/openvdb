/*
	VBA.h
*/

#ifndef VDB_Nuke_VBA_h__
#define VDB_Nuke_VBA_h__

#include <OpenGL/gl3.h>
#include <cstddef>
#include <vector>


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

class UnitQuad : public VBA
{
public:
    UnitQuad() {}
    void init();
    void operator() () const;
};

class UnitCube : public VBA
{
public:
    
    void init();
    void operator () ();
};

class UnitSphere : public VBA
{
    unsigned mIndLen = 0;
public:
    
    void init(unsigned stacks = 24, unsigned slices = 24);
    void operator () () const;
};

class UnitCylinder : public VBA
{
    size_t mCapLen = 0;
    bool   mCapped;
public:
    
    void init(bool capped = true, unsigned nDiv = 16);
    void operator () ();
};

class UnitCone : public VBA
{
    size_t mIndLen = 0;
public:
    
    void init(unsigned nDiv = 12);
    void operator () ();
};

class UnitTorus : public VBA
{
    std::vector<unsigned> mStripLen;
public:
    
    void init(float sweepRadius       = 0.5f - 0.01f*0.5f,
              float tubeRadius        = 0.01f,
              unsigned sweepDivisions = 30,
              unsigned tubedivisions  = 15);

    void init(std::pair<float,float> outerInner = {0.5f, 0.49f},
              unsigned sweepDivisions           = 30,
              unsigned tubedivisions            = 15) {
        const float tubeRadius = (outerInner.first - outerInner.second) * 0.5f;
        return init(outerInner.second + tubeRadius, tubeRadius, sweepDivisions, tubedivisions);
    }
    void operator () ();
};

class UnitDisk: public VBA
{
    unsigned mIndLen;
public:
    
    void init(float outer = 0.5f, float inner = 0.4f, unsigned stacks = 30);
    void operator () ();
};



#endif
