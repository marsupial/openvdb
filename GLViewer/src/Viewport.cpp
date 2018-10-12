
#include <vector>
#include "Camera.h"
#include "CameraController.h"
#include "ShaderProgram.h"
#include "UnitCube.h"
#include "Points.h"
#include "Slices.h"
#include "ShaderProgram.h"
#include "CamMask.h"


#include "AxisGrid.h"
#include "CamMask.h"
#include "../include/Viewport.h"

using namespace koala;

struct ProgramHandles {
	ShaderProgram constant, grid, gradient;
};
static ShaderProgram::Attribute kAttributes[] = { {"P", 0}, {"color", 3}, {0,0} };
static ProgramHandles Programs;
static Vector3 kMaxSize(1,1,1);


NUKEVDB_NAMESPACE_

VBA::~VBA()
{
    if ( mVBA )
        glDeleteVertexArrays(1, &mVBA);
    else
        printf("mVBA was ZERO!\n");
}

void VBA::update( const GLfloat *vertices, const size_t Nv, GLint vE, GLuint buffer, GLuint loc )
{
  glBindVertexArray(mVBA);
  glBindBuffer(GL_ARRAY_BUFFER, buffer);
  glBufferData(GL_ARRAY_BUFFER, Nv, vertices, GL_STATIC_DRAW);
  glEnableVertexAttribArray(loc);
  glVertexAttribPointer(loc, vE, GL_FLOAT, 0, 0, 0);
}

void VBA::add( const GLfloat *vertices, const size_t Nv, GLint vE, GLuint loc )
{
  GLuint buffer;
  glGenBuffers(1, &buffer);

  glBindVertexArray(mVBA);
  glBindBuffer(GL_ARRAY_BUFFER, buffer);
  glBufferData(GL_ARRAY_BUFFER, Nv, vertices, GL_STATIC_DRAW);
  update(vertices, Nv, vE, buffer, loc);
}

void VBA::create( const GLfloat *vertices, const size_t Nv, GLint vE, const GLuint *elements, const size_t Ne, GLuint *buffOUT )
{
  glGenVertexArrays(1, &mVBA);
  glBindVertexArray(mVBA);

  const GLuint nBuf = Ne ? 2 : 1;
  GLuint buffers[nBuf];
  glGenBuffers(nBuf, buffers);
  update(vertices, Nv, vE, buffers[0]);
  if ( Ne )
  {
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffers[1]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, Ne, elements, GL_STATIC_DRAW);
  }
  if ( buffOUT )
    ::memcpy(buffOUT, buffers, sizeof(GLuint)*nBuf);
}

void VBA::bind()  const
{
  glBindVertexArray(mVBA);
}

void VBA::operator() (GLenum mode, GLsizei count, GLenum type, const GLvoid *indices ) const
{
    bind();
    glDrawElements(mode, count, type, indices);
}

float_t AxisGrid::setVerts( Vector3 *verts, unsigned &start, unsigned N, float_t step , float_t X )
{
    unsigned i = start;
    for ( ; i < N; ++i )
    {
      const unsigned a = i*2, b = a+1;
      verts[a].set(X, 0, -1);
      verts[b].set(X, 0,  1);
      X += step;
    }
    start = i;
    return X;
}

void AxisGrid::init( GLuint P )
{
  //P = glGetAttribLocation(Programs.constant, "P");

  std::vector<Vector3> vertices(kNumVerts);

  unsigned i = 0;
  float_t  step = 2.f / float_t(kNumLines - (kNumLines % 2));

  // Skip center axis'
  float_t  X = setVerts(&vertices[0], i, kNumLines/2, step, -1) + step;
  setVerts(&vertices[0], i, kNumVerts/2, step, X);

  GLuint buffers[2];
  mGrid.create(&vertices[0].x, sizeof(Vector3)*kNumVerts, 3);

  vertices.resize(4);
  vertices[0].set(-1, 0, 0);
  vertices[1].set( 1, 0, 0);
  vertices[2].set(0, 0, -1);
  vertices[3].set(0, 0,  1);
  mAxis.create(&vertices[0].x, sizeof(Vector3)*4, 3);
}

static void UseProgram( const ShaderProgram &program , const CameraController &camera,
                        const Matrix4 &model = koala::Matrix4::kIdentity)
{
    glUseProgram(program);

    Vector3 eyePos;
    Matrix4 view, projection, modelView, modelViewProj;

    camera.getTransformations(projection, &view, &model, &modelView,
                              &modelViewProj, &eyePos);

    program.uniform("ModelViewProjection", modelViewProj);
    program.uniform("ProjectMatrix", projection);
    program.uniform("ViewMatrix", view);
    program.uniform("eyePos", eyePos);
}

void AxisGrid::operator () ( GLfloat size, CameraController& cam) const
{
    Matrix4 scale(size, 0, 0, 0,  0, size, 0, 0,  0, 0, size, 0 );
    Matrix4 rotY(0, 0, -size, 0,  0, size, 0, 0,  size, 0, 0, 0 );

    //glPushAttrib(GL_LINE_BIT);
    glLineWidth(1);
    mGrid();
    UseProgram(Programs.constant, cam, scale);
    glDrawArrays(GL_LINES, 0, kNumVerts);

    UseProgram(Programs.constant, cam, rotY);
    glDrawArrays(GL_LINES, 0, kNumVerts);

    mAxis();
    glLineWidth(4);
    UseProgram(Programs.constant, cam, scale);
    glDrawArrays(GL_LINES, 0, 4);
  
    //glPopAttrib();
}

void CamMask::init( CameraController &controller )
{
  GLfloat verts[16];
  GLuint  elem[12];
  controller.viewportMask(verts, elem);

  GLuint buffers[2];
  create(verts, sizeof(verts), 2, elem, sizeof(elem), buffers);
  mVBO = buffers[0];

  GLfloat colors[16*4];
  for (unsigned i = 0; i < 16*4; ) {
      colors[i++] = .1f;
      colors[i++] = .1f;
      colors[i++] = .1f;
      colors[i++] = 0.3;
  }
  add(colors, sizeof(colors), 4, kAttributes[1].location);
}

void CamMask::operator () ( CameraController &controller )
{
  GLfloat verts[16];
  GLuint  elem[12];
  controller.viewportMask(verts, NULL);
  update(verts, sizeof(verts), 2, mVBO);
}

void CamMask::operator () () const
{
  VBA::operator()(GL_TRIANGLES, 12, GL_UNSIGNED_INT);
}

void UnitCube::init()
{
  GLfloat vertices[] = {
       kMaxSize.x,   kMaxSize.y,   kMaxSize.z,
      -kMaxSize.x,   kMaxSize.y,   kMaxSize.z,
       kMaxSize.x,   kMaxSize.y,  -kMaxSize.z,
      -kMaxSize.x,   kMaxSize.y,  -kMaxSize.z,
       kMaxSize.x,  -kMaxSize.y,   kMaxSize.z,
      -kMaxSize.x,  -kMaxSize.y,   kMaxSize.z,
      -kMaxSize.x,  -kMaxSize.y,  -kMaxSize.z,
       kMaxSize.x,  -kMaxSize.y,  -kMaxSize.z,
  };

  // Declares the Elements Array, where the indexs to be drawn are stored
  GLuint elements [] = {
      3, 2, 6, 7, 4, 2, 0,
      3, 1, 6, 5, 4, 1, 0
  };

  create(vertices, sizeof(vertices), 3, elements, 14 * sizeof(GLuint));


  GLfloat colors[] = {
      0, 0, 1.0, 1.f,
      0, 0, 1.0, 1.f,
      1.0f, 0, 0, 1.f,
      1.0f, 0, 0, 1.f,
      0, 0, 1.0, 1.f,
      0, 0, 1.0, 1.f,
      1.0f, 0, 0, 1.f,
      1.0f, 0, 0, 1.f,
  };
  add(colors, sizeof(colors), 3, kAttributes[1].location);
}

void UnitCube::operator() ()
{
    VBA::operator()(GL_TRIANGLE_STRIP, 14, GL_UNSIGNED_INT);
}


class Quad : public VBA
{
public:
    Quad() {}
    void init();
};

void Quad::init() {
  GLfloat vertices[] = { -1, -1, 0,
                          1, -1, 0,
                          1,  1, 0,
                         -1,  1, 0 };

  GLuint elements [] = { 0, 1, 2, 0, 3, 2 };

  create(vertices, sizeof(vertices), 3, elements, 6 * sizeof(GLuint));
}

static UnitCube sUnitCube;
static Quad sQuad;

void Viewport::init(int width, int height) {
	sUnitCube.init();
	sQuad.init();
    mCameraController.viewPort(width, height);

    Programs.constant.load("constant", kAttributes);
    Programs.grid.load("grid", kAttributes);
    Programs.gradient.load("gradient", kAttributes);

    glEnable(GL_DEPTH_TEST);
    //glEnable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glClearColor(0.2f, 0.2f, 0.2f, 0);
}

void Viewport::resize(int width, int height) {
	mCameraController.viewPort(width, height);
	glViewport(0, 0, width, height);
}

void Viewport::mouse(int x, int y, Action action)
{
    Vector2i mouseLoc(x,y);

    if (action & kMouseDown)
    {
      if ( action & kMMouseModifier || action & kCmdKeyModifier  )
          mCameraController.motionStart(CameraController::Track, mouseLoc);
      else if ( action & kRMouseModifier || action & kCtlKeyModifier )
          mCameraController.motionStart(CameraController::Dolly, mouseLoc);
      else if ( action & kAltKeyModifier )
          mCameraController.motionStart(CameraController::Tumble, mouseLoc);
      else
      {
          Vector3 near, far;
          mCameraController.unproject(mouseLoc, near, far);
      }
    }
    else if (action & kMouseUp)
        mCameraController.motionEnd(mouseLoc);
    else if (action & kMouseMove)
        mCameraController.motionUpdate(mouseLoc);
    else if (action & kScrollWheel)
    {
      mouseLoc += Vector2i(x,y);
      mCameraController.motionStart(CameraController::Dolly, mouseLoc);
      mouseLoc -= Vector2i(x,y);
      mCameraController.motionUpdate(mouseLoc);
      mCameraController.motionEnd(mouseLoc);
    }
}

static float fit01(float x, float omin, float omax) {
  return (x-omin) / (omax-omin);
}

void Viewport::grid( float size ) const {
    float dist;
    if (mCameraController.perspective()) {
      size *= mCameraController.coi();
      dist = 2.0f * mCameraController.coi() * tan(mCameraController.camera()->fov() * 0.5f * math::kDegreesToRadians);
    } else {
      size = mCameraController.screenWindow().size().x;
      dist = mCameraController.screenWindow().size().x;
    }
    Matrix4 xform(size, 0, 0, 0, 0, 0, size, 0, 0, -size, 0, 0 );
    UseProgram(Programs.grid, mCameraController, xform);

    float frac = modff(log10(dist), &dist);
    float level = pow(10, dist);

    Programs.grid.uniform("color", Vector4(1,1,1,1));
    Programs.grid.uniform("lineWidth", 1.f);

    float A = 0.1;
    glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );

    Programs.grid.uniform("G", (Vector2(size/level)));
    Programs.grid.uniform("Blend", A);
    sQuad(GL_TRIANGLE_STRIP, 6, GL_UNSIGNED_INT);

    const float vpcut[2] = { 0.2f, 0.6f };
    if (frac < vpcut[1]) {
      level *= 0.1f;
      Programs.grid.uniform("G", (Vector2(size/level)));
      if (frac > vpcut[0])
        A *= (1.f -  fit01(frac, vpcut[0], vpcut[1]));
      Programs.grid.uniform("Blend", A);
      sQuad(GL_TRIANGLE_STRIP, 6, GL_UNSIGNED_INT);
    }
}

void Viewport::render()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Background
    glDisable(GL_DEPTH_TEST);
    UseProgram(Programs.gradient, mCameraController);
    Programs.gradient.uniform("TopColor", Vector3(0.3384,0.376,0.423));
    Programs.gradient.uniform("BottomColor", Vector3(0.1328, 0.147556, 0.166));
    Programs.gradient.uniform("InvViewport", Vector2(1.f)/Vector2(mCameraController.viewPort()));
    Programs.gradient.uniform("Mode", 1);
    Programs.gradient.uniform("Alpha", 1.f);
    sQuad(GL_TRIANGLE_STRIP, 6, GL_UNSIGNED_INT);

    glEnable(GL_DEPTH_TEST);

    UseProgram(Programs.constant, mCameraController);
    Programs.constant.uniform("color", Vector4(.4,.8,.1,1));
	glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
    sUnitCube();

    grid();

  	if ( mMask )
    {
        const float ratios[2] = { mCamera.resolution().aspectRatio(), mCameraController.aspectRatio() };
        if (ratios[0] != ratios[1]) {
            glDisable(GL_DEPTH_TEST);

            UseProgram(Programs.gradient, mCameraController);
            Vector3 vratio(1);
            bool select = ratios[0] > ratios[1];
            vratio[select] = (1.f - ratios[select] / ratios[!select]) * 0.5f;
            Programs.gradient.uniform("TopColor", vratio);
            Programs.gradient.uniform("BottomColor", Vector3(0.1f));
            Programs.gradient.uniform("Mode", 2);
            Programs.gradient.uniform("Alpha", 0.1f);
            sQuad(GL_TRIANGLE_STRIP, 6, GL_UNSIGNED_INT);
        }
    }

	// OUT
    glBindVertexArray(0);
	glUseProgram(0);
}

_NUKEVDB_NAMESPACE

