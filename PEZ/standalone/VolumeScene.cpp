
#include <vector>
#include "Camera.h"
#include "CameraController.h"
#include "ShaderProgram.h"
#include "noise.h"

#include "opengl/UnitCube.h"
#include "opengl/Points.h"
#include "opengl/Slices.h"

#include "opengl/AxisGrid.h"
#include "opengl/CamMask.h"

#include "pez.h"

#if 1
 #define glGenVertexArrays      glGenVertexArraysAPPLE
 #define glBindVertexArray      glBindVertexArrayAPPLE
 #define glDeleteVertexArrays   glDeleteVertexArraysAPPLE
#endif

using namespace nukevdb;

struct ProgramHandles {
	ShaderProgram constant, points, voxels, slices, rmarch;
};
static ShaderProgram::Attribute kAttributes[] = { {"P", 0}, {"color", 3}, {0,0} };
static ProgramHandles Programs;
static GLuint CloudTexture, PointTexture;

static Camera sCamera(Vector3(0,0,3.87));
static CameraController sCamController(&sCamera);
static Matrix4 sModelMatrix(0,0,0);

static const Vector3 kMaxSize(1, 1, 1);
static int   sSample = 7;
static bool  sAbsorptionDir = 1;
static float sAbsorption = 1.0;
static bool  sDrawGate = true;

static void printVector( const char *msg, const Vector3 &v, const char *token = "()\n")
{
	printf("%s%c%f,%f,%f%s", msg, token[0], v.x, v.y, v.z, &token[1]);
}

static void UseProgram( const ShaderProgram &program , const Matrix4 &model, int flags = 0 );
static GLuint PointSprite();

NUKEVDB_NAMESPACE_



static void drawVDB( ShaderProgram& );
static void loadVDB( const char *filename );

VBA::~VBA()
{
    if ( mVBA )
        glDeleteVertexArrays(1, &mVBA);
    else printf("mVBA was ZERO!\n");
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
  //P = glGetAttribLocation(Programs.Constant,"Position");

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

void AxisGrid::operator () ( GLfloat size ) const
{
    Matrix4 scale(size, 0, 0, 0,  0, size, 0, 0,  0, 0, size, 0 );
    Matrix4 rotY(0, 0, -size, 0,  0, size, 0, 0,  size, 0, 0, 0 );

    glPushAttrib(GL_LINE_BIT);
    glLineWidth(1);
    mGrid();
    UseProgram(Programs.constant, scale);
    glDrawArrays(GL_LINES, 0, kNumVerts);

    UseProgram(Programs.constant, rotY);
    glDrawArrays(GL_LINES, 0, kNumVerts);

    mAxis();
    glLineWidth(4);
    UseProgram(Programs.constant, scale);
    glDrawArrays(GL_LINES, 0, 4);
  
    glPopAttrib();
}

void CamMask::init( CameraController &controller )
{
  GLfloat verts[16];
  GLuint  elem[12];
  controller.viewportMask(verts, elem);

  GLuint buffers[2];
  create(verts, sizeof(verts), 2, elem, sizeof(elem), buffers);
  mVBO = buffers[0];
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
  add(colors, sizeof(colors), 3, glGetAttribLocation(Programs.rmarch, "color"));
}

void UnitCube::wireframe()
{
  #if 1
    glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
    VBA::operator()(GL_TRIANGLE_STRIP, 14, GL_UNSIGNED_INT);
    glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
  #else
    GLuint indices[] { 0, 1, 5, 4 };
    GLVBA::operator()(GL_LINES, 4, GL_UNSIGNED_INT, indices);
  #endif
}


void Points::init( std::vector<Vector3> &pts )
{
    mNumPts = pts.size();
    create(&pts[0].x, sizeof(Vector3)*mNumPts, 3);
}

void Points::glSetup()
{
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_POINT_SPRITE);
    glEnable(GL_PROGRAM_POINT_SIZE_EXT);
    glTexEnvi(GL_POINT_SPRITE_ARB, GL_COORD_REPLACE_ARB, GL_TRUE);
}

void Points::operator () ()
{
	glSetup();
    bind();
    glDrawArrays(GL_POINTS, 0, mNumPts);
}

void Slices::init( const Box3 &volume, const Vector3 &viewDir, const unsigned numSlices )
{
  const int indices[] = { 0, 1, 2, 0, 2, 3, 0, 3, 4, 0, 4, 5 };

  const Vector3 vertices[8] = {
      Vector3( volume.min[0], volume.min[1], volume.min[2] ),
      Vector3( volume.max[0], volume.min[1], volume.min[2] ),
      Vector3( volume.max[0], volume.max[1], volume.min[2] ),
      Vector3( volume.min[0], volume.max[1], volume.min[2] ),
      Vector3( volume.min[0], volume.min[1], volume.max[2] ),
      Vector3( volume.max[0], volume.min[1], volume.max[2] ),
      Vector3( volume.max[0], volume.max[1], volume.max[2] ),
      Vector3( volume.min[0], volume.max[1], volume.max[2] )
  };

  const int edges[12][2] = {
      { 0, 1 }, { 1, 2 }, { 2, 3 },
      { 3, 0 }, { 0, 4 }, { 1, 5 },
      { 2, 6 }, { 3, 7 }, { 4, 5 },
      { 5, 6 }, { 6, 7 }, { 7, 4 }
  };

  const int edge_list[8][12] = {
      { 0, 1, 5, 6, 4, 8, 11, 9, 3, 7, 2, 10 },
      { 0, 4, 3, 11, 1, 2, 6, 7, 5, 9, 8, 10 },
      { 1, 5, 0, 8, 2, 3, 7, 4, 6, 10, 9, 11 },
      { 7, 11, 10, 8, 2, 6, 1, 9, 3, 0, 4, 5 },
      { 8, 5, 9, 1, 11, 10, 7, 6, 4, 3, 0, 2 },
      { 9, 6, 10, 2, 8, 11, 4, 7, 5, 0, 1, 3 },
      { 9, 8, 5, 4, 6, 1, 2, 0, 10, 7, 11, 3 },
      { 10, 9, 6, 5, 7, 2, 3, 1, 11, 4, 8, 0 }
  };

  // find vertex that is the furthest from the view plane
  int max_index = 0;
  float max_dist, min_dist;
  min_dist = max_dist = viewDir.dot(vertices[0]);

  for (int i = 1; i < 8; i++) {
      float dist = viewDir.dot(vertices[i]);

      if (dist > max_dist) {
          max_dist = dist;
          max_index = i;
      }

      if (dist < min_dist) {
          min_dist = dist;
      }
  }

  max_dist -= std::numeric_limits<float>::epsilon();
  min_dist += std::numeric_limits<float>::epsilon();;

  Vector3 vecStart[12], vecDir[12];
  float lambda[12], lambadInc[12];
  float denom = 0.0f;

  float plane_dist = min_dist;
  float plane_dist_inc = (max_dist - min_dist) / float(numSlices);

  /* for all egdes */
  for (int i = 0; i < 12; i++) {
      vecStart[i] = vertices[edges[edge_list[max_index][i]][0]];
      vecDir[i]   = vertices[edges[edge_list[max_index][i]][1]];
      vecDir[i]   -= vecStart[i];

      denom = vecDir[i].dot(viewDir);

      if (1.0f + denom != 1.0f) {
          lambadInc[i] = plane_dist_inc / denom;
          lambda[i] = (plane_dist - vecStart[i].dot(viewDir)) / denom;
      }
      else {
          lambda[i] = -1.0f;
          lambadInc[i] = 0.0f;
      }
  }

  std::vector<Vector3> verts;
  verts.resize(numSlices * 12);
  
  // find intersections for each slice, process them in back to front order
  float   dL[12];
  Vector3 intersections[6];
  mNumPoints = 0;

  for (int i = 0; i < numSlices; i++) {
      for (int e = 0; e < 12; e++)
          dL[e] = lambda[e] + i * lambadInc[e];

      if ((dL[0] >= 0.0f) && (dL[0] < 1.0f))
          intersections[0] = vecStart[0] + vecDir[0] * dL[0];
      else if ((dL[1] >= 0.0f) && (dL[1] < 1.0f))
          intersections[0] = vecStart[1] + vecDir[1] * dL[1];
      else if ((dL[3] >= 0.0f) && (dL[3] < 1.0f))
          intersections[0] = vecStart[3] + vecDir[3] * dL[3];
      else continue;

      if ((dL[2] >= 0.0f) && (dL[2] < 1.0f))
          intersections[1] = vecStart[2] + vecDir[2] * dL[2];
      else if ((dL[0] >= 0.0f) && (dL[0] < 1.0f))
          intersections[1] = vecStart[0] + vecDir[0] * dL[0];
      else if ((dL[1] >= 0.0f) && (dL[1] < 1.0f))
          intersections[1] = vecStart[1] + vecDir[1] * dL[1];
      else
          intersections[1] = vecStart[3] + vecDir[3] * dL[3];

      if ((dL[4] >= 0.0f) && (dL[4] < 1.0f))
          intersections[2] = vecStart[4] + vecDir[4] * dL[4];
      else if ((dL[5] >= 0.0f) && (dL[5] < 1.0f))
          intersections[2] = vecStart[5] + vecDir[5] * dL[5];
      else
          intersections[2] = vecStart[7] + vecDir[7] * dL[7];

      if ((dL[6] >= 0.0f) && (dL[6] < 1.0f))
          intersections[3] = vecStart[6] + vecDir[6] * dL[6];
      else if ((dL[4] >= 0.0f) && (dL[4] < 1.0f))
          intersections[3] = vecStart[4] + vecDir[4] * dL[4];
      else if ((dL[5] >= 0.0f) && (dL[5] < 1.0f))
          intersections[3] = vecStart[5] + vecDir[5] * dL[5];
      else
          intersections[3] = vecStart[7] + vecDir[7] * dL[7];

      if ((dL[8] >= 0.0f) && (dL[8] < 1.0f))
          intersections[4] = vecStart[8] + vecDir[8] * dL[8];
      else if ((dL[9] >= 0.0f) && (dL[9] < 1.0f))
          intersections[4] = vecStart[9] + vecDir[9] * dL[9];
      else
          intersections[4] = vecStart[11] + vecDir[11] * dL[11];

      if ((dL[10] >= 0.0f) && (dL[10] < 1.0f))
          intersections[5] = vecStart[10] + vecDir[10] * dL[10];
      else if ((dL[8] >= 0.0f) && (dL[8] < 1.0f))
          intersections[5] = vecStart[8] + vecDir[8] * dL[8];
      else if ((dL[9] >= 0.0f) && (dL[9] < 1.0f))
          intersections[5] = vecStart[9] + vecDir[9] * dL[9];
      else
          intersections[5] = vecStart[11] + vecDir[11] * dL[11];

      for (int e = 0; e < 12; e++) {
          verts[mNumPoints++] = intersections[indices[e]];
      }
  }

  create(&verts[0].x, sizeof(Vector3)*mNumPoints, 3);
}

void Slices::operator () ()
{
  bind();
  glDrawArrays(GL_TRIANGLES, 0, mNumPoints);
}

_NUKEVDB_NAMESPACE

static AxisGrid sGrid;
static CamMask  sCamMask;
static UnitCube sUnitCube;
static Points   sVolumePoints;


PezConfig PezGetConfig()
{
    PezConfig config;
    config.Title = "Raycast";
    config.Width = 640;
    config.Height = 480;
    config.Multisampling = 0;
    config.VerticalSync = 0;
    return config;
}

struct VoxelFill
{
  std::vector<Vector3> pts;
  std::vector<GLubyte> tex;
  const Vector3        oneHalf, volSize, maxSize;

  VoxelFill( unsigned vs ) : oneHalf(0.5f,0.5f,0.5f), volSize(vs, vs, vs), maxSize(kMaxSize*2.f)
  {
  	pts.reserve(vs);
    tex.reserve(vs*vs*vs);
  }

  void addVoxel( bool isFilled, const Vector3 &P )
  {
    if ( isFilled )
    {
      Vector3 rnd(drand48(), drand48(), drand48());
      rnd /= 100 / kMaxSize.length();
      pts.push_back( (((P/volSize) - oneHalf)) * maxSize + rnd);
    }
    tex.push_back(isFilled ? 255 : 0);
  }
  
  GLuint texture()
  {
    GLuint handle;
    glGenTextures(1, &handle);
    glBindTexture(GL_TEXTURE_3D, handle);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_BORDER);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    glTexImage3D(GL_TEXTURE_3D, 0,
                 GL_RED,
                 volSize.x, volSize.y, volSize.z, 0,
                 GL_RED,
                 GL_UNSIGNED_BYTE,
                 &tex[0]);

    return handle;
  }

  static void* addVoxel( void *ref, bool isFilled, const Vector3 &P )
  {
  	static_cast<VoxelFill*>(ref)->addVoxel(isFilled, P);
    return ref;
  }
};

void PezInitialize( int width, int height )
{
	//loadVDB("/Users/jermainedupris/repos/openvdb/sphere.vdb");
	loadVDB("/Users/jermainedupris/repos/openvdb/spher.surf.vdb");
	//loadVDB("/Users/jermainedupris/repos/openvdb/cpart.vdb");
	//loadVDB("/Users/jermainedupris/repos/openvdb/ccube.vdb");

	Programs.constant.load("constant", kAttributes);
	Programs.points.load("pointsprite", kAttributes);
	Programs.voxels.load("voxelsprite", kAttributes);
	Programs.slices.load("volslices", kAttributes);
	Programs.rmarch.load("raymarch", kAttributes);

	sGrid.init();
    sUnitCube.init();
    sCamController.viewPort(width, height);
	sCamMask.init(sCamController);

	const unsigned kVolumeSize = 128;
	VoxelFill vfill(kVolumeSize);
	noise::pyroclastic(kVolumeSize, 0.025f, &vfill, &VoxelFill::addVoxel);

	sVolumePoints.init(vfill.pts);
    CloudTexture = vfill.texture();
	PointTexture = PointSprite();

    glDisable(GL_DEPTH_TEST);
    //glEnable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glClearColor(0.2f, 0.2f, 0.2f, 0);
}

static void UseProgram( const ShaderProgram &program , const Matrix4 &model, int flags )
{
    glUseProgram(program);

    Vector3 eyePos;
	Matrix4 view, projection, modelView, modelViewProj;

	sCamController.getTransformations(&projection, &view, &model,
                                      &modelView, &modelViewProj, &eyePos);

    program.uniform("ModelViewProjection", modelViewProj);
    if ( flags==0 )
      return;

    program.uniform("kMaxDist", kMaxSize.distanceBetween(-kMaxSize));  // cube diagnoal
    program.uniform("ModelView", modelView);
    program.uniform("WindowSize", Vector2f(sCamController.viewPort()));
    if ( flags==1 )
    {
      program.uniform("Projection", projection);
      return;
	}
  
    program.uniform("CamCenter", sModelMatrix.inverse() * eyePos);

    if ( sCamera.perspective() )
    {
        program.uniform("Orthographic", 0);
		const float_t vpAspect = sCamController.aspectRatio();
		program.uniform("FocalLength", sCamController.focalLength() * (vpAspect > 1.0 ? vpAspect : 1.f));
		//program.uniform("ScreenWindow", Vector4(0,0,0,0));
    }
    else
    {
      program.uniform("Orthographic", 1);
      program.uniform("ScreenWindow", sCamController.screenWindow());
      program.uniform("FocalLength", 0.f);
	}

    program.uniform("Absorption", sAbsorption);
    program.uniform("Samples", Vector2(pow(2, sSample), 32));
    program.uniform("kMaxSize", kMaxSize);  // cube size
}

void PezResize(int w, int h)
{
  sCamController.viewPort(w, h);
  sCamMask(sCamController);
  glViewport(0, 0, w, h);
}

void PezRender()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Grid
    glColor4f(0.8, 1.0, 0.2, 0.6f);
    sGrid();

    //glDisable(GL_DEPTH_TEST);
    //glEnable(GL_CULL_FACE);

  #if 0
    UseProgram(Programs.RayMarch, sModelMatrix, 2);
    glUniform1i(glGetUniformLocation(Programs.RayMarch, "Density"), 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_3D, CloudTexture);
  
    sUnitCube(GL_TRIANGLE_STRIP, 14, GL_UNSIGNED_INT);

	glColor4f(1, 0.1, 0.2, 0.8f);
    UseProgram(Programs.Constant, sModelMatrix);
    sUnitCube.wireframe();
  #endif

  #if 1
    UseProgram(Programs.slices, sModelMatrix, 2);
    glUniform1i(glGetUniformLocation(Programs.rmarch, "Density"), 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_3D, CloudTexture);


    //glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
  	Slices vSlice;
    vSlice.init( Box3(-kMaxSize, kMaxSize), sCamera.viewMatrix().forward());
    vSlice();
    //glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
  #endif

  #if 1

    //glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    //glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_COLOR);

  	glColor4f(1, 1, 1, 1);
    UseProgram(Programs.points, sModelMatrix * Matrix4(kMaxSize.x*2.f, 0, 0), 1);
    Programs.points.uniform("kMaxDist", Vector3(0.0078125, 0.0078125, 0.0078125).length()); // 1/128
    glUniform1i(glGetUniformLocation(Programs.points, "input0"), 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, PointTexture);
  	sVolumePoints();
    glDisable(GL_DEPTH_TEST);

    //glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  #endif

#if 1
    glColor4f(1, 1, 1, 1);
    UseProgram(Programs.points, Matrix4(-2), 1);
    Points::glSetup();
    //glEnable(GL_DEPTH_TEST);
    drawVDB(Programs.points);
    glDisable(GL_DEPTH_TEST);
#endif

	glColor4f(0.2, 0.2, 0.5, 0.3f);
	// Camera gate
  	if ( sDrawGate )
    {
      glUseProgram(Programs.constant);
      Programs.constant.uniform("ModelViewProjection", sCamController.pixelProjection());
      sCamMask();
    }

	// OUT
    glBindVertexArray(0);
	glUseProgram(0);
}

int PezUpdate(unsigned int microseconds)
{
	const float motion = microseconds * 0.00065f;
#if 0
    sAbsorption = sAbsorption + (sAbsorptionDir ? motion : -motion);
  	if ( sAbsorption < 0 )
    {
    	sAbsorption = 0;
        sAbsorptionDir = 1;
    }
    else if ( sAbsorption > 5 )
        sAbsorptionDir = 0;
#endif

#if 1
	//sModelMatrix.rotate(Vector3(motion, motion, motion));
    //sModelMatrix.translate(Vector3(motion, motion, motion));
    //sModelMatrix.translate(Vector3(motion,0,0));
    //sModelMatrix.scale(Vector3(1+motion, 1+motion, 1+motion));
#endif

	return true;
}

void PezHandleMouse(int x, int y, int action)
{
	Vector2i mouseLoc(x,y);
    if (action & PEZ_DOWN)
    {
      if ( action & PEZ_M_MOUSE || action & PEZ_CMD_KEY  )
      	sCamController.motionStart(CameraController::Track, mouseLoc);
      else if ( action & PEZ_R_MOUSE || action & PEZ_CTL_KEY )
      	sCamController.motionStart(CameraController::Dolly, mouseLoc);
      else if ( action & PEZ_ALT_KEY )
        sCamController.motionStart(CameraController::Tumble, mouseLoc);
      else
      {
      	Vector3 near, far;
        sCamController.unproject(mouseLoc, near, far);
      }
    }
    else if (action & PEZ_UP)
        sCamController.motionEnd(mouseLoc);
    else if (action & PEZ_MOVE)
        sCamController.motionUpdate(mouseLoc);
    else if (action & PEZ_SCROLL)
    {
      sCamController.motionStart(CameraController::Dolly, mouseLoc);
      if ( action & PEZ_M_MOUSE )
      	mouseLoc.y -= 20;
      else
      	mouseLoc.y += 20;
      sCamController.motionUpdate(mouseLoc);
      sCamController.motionEnd(mouseLoc);
    }
    // else if (action & PEZ_DOUBLECLICK)
}


void PezHandleKey(char c)
{
	if ( c == 'f' )
      sCamController.frame(Box3(sModelMatrix * Vector3(-1,-1,-1), sModelMatrix *Vector3(1,1,1)));
	else if (c == 'p') sCamera.perspective() = !sCamera.perspective();
    else if (c == '+') sCamera.fov() += 1.f;
    else if (c == '-') sCamera.fov() -= 1.f;
    else if (c == 'm') sDrawGate = !sDrawGate;
    else if (c == 's') sSample += 1;
    else if (c == 'a') sSample -= 1;
    else if (c == 'c') glEnable( !glIsEnabled(GL_CULL_FACE) );
    else if (c == 'r')
    {
    	const Vector2i &res = sCamera.resolution();
    	sCamera.resolution(res.y, res.x);
        sCamMask(sCamController);
    }
    else if (c == 'w')
    {
    	printf("-----------------------------------------\n");
    	printf("Window: %d %d\n", sCamController.viewPort().x, sCamController.viewPort().y);
        printf("perspective: %d\n", sCamera.perspective());

		const Vector3 eye = sCamera.transform().translation();
      	const Vector3 N   = sCamera.viewMatrix().forward();
		printVector("eyePos: ", eye);
		printVector("forwrd: ", N);
        printf("pln d: %f\n", eye.distanceFromPlane(N.x, N.y, N.z, 0));
        printf("obj d: %f\n", eye.distanceBetween(sModelMatrix.translation()+Vector3(0,0,0.5)));
#if 0
      	const float vAspect = sCamController.viewPort().aspectRatio();
        float a =
    return Matrix4::ortho( -vpAspect, vpAspect, -1, 1, near, far);
Matrix4::ortho( float_t left, float_t right, float_t bottom, float_t top, float_t zNear, float_t zFar)

      	float
        P = (i/widthScreen)*(b - a) + (j/HeightScreen)*(c - a)
#endif
      
    	printf("-----------------------------------------\n");
    }
}

static const unsigned char kRamp[512*512*2] = {
#include "ramp.h"
};

static GLuint PointSprite()
{
  GLuint mTexture;
  glGenTextures(1, &mTexture);
  glBindTexture(GL_TEXTURE_2D, mTexture);

  glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE_ALPHA, 512, 512, 0, GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE, kRamp);

  // Poor filtering
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); 
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

  return mTexture;
}

#include <openvdb/openvdb.h>
#include "VDBPrimitive.h"
#include "VDBConvert.h"

NUKEVDB_NAMESPACE_

struct VDBGrid
{
  openvdb::GridBase::Ptr grid;
  BufferObject           buffer;
};

typedef std::vector< VDBGrid* > VDBGrids;
static VDBGrids sVDBGrids;

static void loadVDB( const char *filename )
{
  NUKEVDB_INIT
  std::vector< VDBGrid* > &grids = sVDBGrids;

  openvdb::io::File file(filename);
  file.open();
  for ( openvdb::io::File::NameIterator nameIter = file.beginName(); nameIter != file.endName(); ++nameIter)
  {
  	grids.push_back( new VDBGrid );
    grids.back()->grid = file.readGrid(nameIter.gridName());
    #if 0
    {
    	SurfaceGeo    srf(grids.back()->buffer, grids.back()->grid->getGridClass() == openvdb::GRID_LEVEL_SET ? 0.0 : 0.01);
        processTypedScalarGrid(grids.back()->grid, srf);
    }
    #endif
    #if 0
    {
    	ActiveVoxelGeo    srf(grids.back()->buffer);
        processTypedScalarGrid(grids.back()->grid, srf);
    }
    #endif
    #if 1
    {
        ActiveScalarValuesOp srf(grids.back()->buffer, &grids.back()->buffer);
        processTypedScalarGrid(grids.back()->grid, srf);
    }
	#endif
    #if 0
    {
        ActiveVectorValuesOp srf(grids.back()->buffer);
        processTypedVectorGrid(grids.back()->grid, srf);
    }
	#endif
  	//grids.push_back( new VDBPrimitive(file.readGrid(nameIter.gridName()), new BufferObject) );
  }
  file.close();
}

static void drawVDB( ShaderProgram &prog )
{
  std::vector< VDBGrid* > &grids = sVDBGrids;
  for ( VDBGrids::iterator it = grids.begin(), e = grids.end(); it != e; ++it )
  {
    prog.uniform("kMaxDist", (*it)->grid->voxelSize().length());
	(*it)->buffer.render();

  }
}

_NUKEVDB_NAMESPACE
