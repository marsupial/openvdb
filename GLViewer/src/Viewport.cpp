
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

#include <openvdb/openvdb.h>
#include <openvdb/math/Maps.h>

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


class BoxCube : public VBA
{
    bool mValid = false;
public:

    void init(const openvdb::GridBase& grid,
              const openvdb::math::NonlinearFrustumMap& frust,
              const openvdb::math::AffineMap& xform) {
      const auto& box = frust.getBBox();
      auto min = box.min(), max = box.max();

      if (1) {
        min = frust.applyMap(min);
        max = frust.applyMap(max);
      } else if (1) {
        min = xform.applyMap(min);
        max = xform.applyMap(max);
      }

      GLfloat vertices[] = {
          float(max[0]), float(max[1]), float(max[2]),
          float(min[0]), float(max[1]), float(max[2]),
          float(max[0]), float(max[1]), float(min[2]),
          float(min[0]), float(max[1]), float(min[2]),
          float(max[0]), float(min[1]), float(max[2]),
          float(min[0]), float(min[1]), float(max[2]),
          float(min[0]), float(min[1]), float(min[2]),
          float(max[0]), float(min[1]), float(min[2]),
      };

      // Declares the Elements Array, where the indexs to be drawn are stored
      GLuint elements [] = {
#if 1
          3, 2, 6, 7, 4, 2, 0,
          3, 1, 6, 5, 4, 1, 0
#else
          0, 8,
#endif
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
    
      mValid = true;
    }

    bool valid() const { return mValid; }
    void operator () () { VBA::operator()(GL_TRIANGLE_STRIP, 14, GL_UNSIGNED_INT); }
};

static BoxCube sVDBBox;

void processGrid(openvdb::GridBase& grid) {
    using namespace openvdb;
    using namespace openvdb::math;

    const Transform &transform =  grid.transform();
    NonlinearFrustumMap::ConstPtr frustMap = transform.constMap<NonlinearFrustumMap>();
    if (!frustMap)
        return;

    std::cout << grid.voxelSize() << "\n";

    AffineMap secondMap = frustMap->secondMap();
    if (frustMap->getDepth() != 1.0) {
        secondMap.accumPreScale(Vec3d(1, 1, frustMap->getDepth()));

        frustMap = NonlinearFrustumMap::ConstPtr(new NonlinearFrustumMap(frustMap->getBBox(),
                                                 frustMap->getTaper(), /*depth*/1.0, secondMap.copy()));
    }

    sVDBBox.init(grid, *frustMap, secondMap);
}

void Viewport::init(int width, int height) {
    if (mFilePath) {
        using namespace openvdb;
        static struct VDB {
            VDB() { openvdb::initialize(); }
            ~VDB() { openvdb::uninitialize(); }
        } sVDBLib;

        io::File f(mFilePath);
        if (f.open()) {
            if (GridPtrVecPtr grids = f.getGrids()) {
                for (auto&& g : *grids) {
                    processGrid(*g);
                    //if (sVDBBox.valid())
                    //    break;
                }
            }
            f.close();
        }
    }

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


#if 0

const char kVertexShader[] =
    "#version 150 core"
    ""                                                                           "\n"
    "in vec2 pos;"                                                               "\n"
    "in vec3 color;"                                                             "\n"
    "in float sides;"                                                            "\n"
    ""                                                                           "\n"
    "out vec3 vColor;"                                                           "\n"
    "out float vSides;"                                                          "\n"
    ""                                                                           "\n"
    "void main() {"                                                              "\n"
    "    gl_Position = vec4(pos, 0.0, 1.0);"                                     "\n"
    "    vColor = color;"                                                        "\n"
    "    vSides = sides;"                                                        "\n"
    "}"                                                                          "\n";

// Geometry shader
const char kGeometryShader[] =
   "#version 150 core"                                                           "\n"
   ""                                                                            "\n"
   "layout(points) in;"                                                          "\n"
   "layout(line_strip, max_vertices = 64) out;"                                  "\n"
     ""                                                                          "\n"
   "in vec3 vColor[];"                                                           "\n"
   "in float vSides[];"                                                          "\n"
   "out vec3 fColor;"                                                            "\n"
   ""                                                                            "\n"
   "const float PI = 3.1415926;"                                                 "\n"
   ""                                                                            "\n"
   "void main() {"                                                               "\n"
   "    fColor = vColor[0];"                                                     "\n"
   "    // Safe, GLfloats can represent small integers exactly"                  "\n"
   "    for (int i = 0; i <= vSides[0]; i++) {"                                  "\n"
   "        // Angle between each side in radians"                               "\n"
   "        float ang = PI * 2.0 / vSides[0] * i;"                               "\n"
   ""                                                                            "\n"
   "        // Offset from center of point (0.3 to accomodate for aspect ratio)" "\n"
   "        vec4 offset = vec4(cos(ang) * 0.3, -sin(ang) * 0.4, 0.0, 0.0);"      "\n"
   "        gl_Position = gl_in[0].gl_Position + offset;"                        "\n"
   ""                                                                            "\n"
   "        EmitVertex();"                                                       "\n"
   "    }"                                                                       "\n"
   ""                                                                            "\n"
   "    EndPrimitive();"                                                         "\n"
   "}"                                                                           "\n";

// Fragment shader
const char kFragmentShader[] =                                                   "\n"
   "#version 150 core"                                                           "\n"
   "in vec3 fColor;"                                                             "\n"
   "out vec4 outColor;"                                                          "\n"
   "void main() {"                                                               "\n"
   "    outColor = vec4(fColor, 1.0);"                                           "\n"
   "}"                                                                           "\n";

class BucketDrawer {
    // Shader creation helper
    GLuint createShader(GLenum type, const GLchar* src) {
        GLuint shader = glCreateShader(type);
        glShaderSource(shader, 1, &src, nullptr);
        glCompileShader(shader);
        return shader;
    }

    GLuint mProgram;
public:

    BucketDrawer() {
        // Compile and activate shaders
        GLuint vertexShader = createShader(GL_VERTEX_SHADER, kVertexShader);
        GLuint geometryShader = createShader(GL_GEOMETRY_SHADER, kGeometryShader);
        GLuint fragmentShader = createShader(GL_FRAGMENT_SHADER, kFragmentShader);

        mProgram = glCreateProgram();
        glAttachShader(mProgram, vertexShader);
        glAttachShader(mProgram, geometryShader);
        glAttachShader(mProgram, fragmentShader);
        glLinkProgram(mProgram);

        glDeleteShader(fragmentShader);
        glDeleteShader(geometryShader);
        glDeleteShader(vertexShader);
    }

    ~BucketDrawer() {
        glDeleteProgram(mProgram);
    }

    void draw() {

        // Create VBO with point coordinates
        GLuint vbo;
        glGenBuffers(1, &vbo);

        GLfloat points[] = {
        //  Coordinates     Color             Sides
        -0.45f,  0.45f, 1.0f, 0.0f, 0.0f,  4.0f,
         0.45f,  0.45f, 0.0f, 1.0f, 0.0f,  8.0f,
         0.45f, -0.45f, 0.0f, 0.0f, 1.0f, 16.0f,
        -0.45f, -0.45f, 1.0f, 1.0f, 0.0f, 32.0f
        };

        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(points), points, GL_STATIC_DRAW);

        // Create VAO
        GLuint vao;
        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);

        // Specify the layout of the vertex data
        GLint posAttrib = glGetAttribLocation(mProgram, "pos");
        glEnableVertexAttribArray(posAttrib);
        glVertexAttribPointer(posAttrib, 2, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), 0);

        GLint colAttrib = glGetAttribLocation(mProgram, "color");
        glEnableVertexAttribArray(colAttrib);
        glVertexAttribPointer(colAttrib, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (void*) (2 * sizeof(GLfloat)));

        GLint sidesAttrib = glGetAttribLocation(mProgram, "sides");
        glEnableVertexAttribArray(sidesAttrib);
        glVertexAttribPointer(sidesAttrib, 1, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (void*) (5 * sizeof(GLfloat)));


        glUseProgram(mProgram);
        glDrawArrays(GL_POINTS, 0, 4);
        glUseProgram(0);
        
        glDeleteBuffers(1, &vbo);
        glDeleteVertexArrays(1, &vao);
    }
};

#else


const char kVertexShader[] =
    "#version 150 core"
    ""                                                                                "\n"
    "uniform vec2 tiles = vec2(16,16);"                                              "\n"
    ""
    "void main() {"                                                                   "\n"
    "    float vid = float(gl_VertexID);"                                             "\n"
    "    vec2 offset = vec2(0) + 1.0 / tiles;"                                               "\n"
    "    gl_Position = vec4(2.0 * mod(vid,tiles.x)/tiles.x - 1.0 + offset.x,"   "\n"
    "                       (1.0 - offset.y) - 2.0 * floor(vid/tiles.x)/tiles.y, 0.0, 1.0)"
    "                  * vec4(vec2(0.9999), 0, 1);" "\n"
    "}"                                                                               "\n";

// Geometry shader
const char kGeometryShader[] =
   "#version 150 core"                                                           "\n"
   ""                                                                            "\n"
   "layout(points) in;"                                                          "\n"
#if 0
   "layout(line_strip, max_vertices = 5) out;"                                  "\n"
   "const int kIndex[5] = int[](0,1,2,3,0);"                                                 "\n"
#else
   "layout(triangle_strip, max_vertices = 4) out;"                                  "\n"
   "const int kIndex[4] = int[](0,1,3,2);"                                                 "\n"

#endif
   ""
   "const vec2 kData[4] = vec2[]"                                                 "\n"
   "("                                                                           "\n"
   "  vec2(-1.0,  1.0),"                                                     "\n"
   "  vec2( 1.0,  1.0),"                                                     "\n"
   "  vec2( 1.0, -1.0),"                                                     "\n"
   "  vec2(-1.0, -1.0)"                                                      "\n"
   ");"
   ""                                                                            "\n"
   "uniform vec2 tiles = vec2(16,16);"                                          "\n"
   "out vec2 uv;"                                                            "\n"
                                                                         "\n"
   ""                                                                            "\n"
   "void drawIt(vec2 center, vec2 size) {"                                       "\n"
   "    for (int i = 0, n = kIndex.length(); i < n; ++i) {"                      "\n"
   "        vec2 P = kData[kIndex[i]];"                                          "\n"
   "        gl_Position = vec4(center + size * P, 0, 1);"                        "\n"
   "        gl_PrimitiveID = gl_PrimitiveIDIn;"                                  "\n"
   "        uv = 0.5 * P + 0.5;"                                                 "\n"
   ""                                                                            "\n"
   "        EmitVertex();"                                                       "\n"
   "    }"                                                                       "\n"
   ""                                                                            "\n"
   "    EndPrimitive();"                                                         "\n"
   "}"                                                                           "\n"
   ""
   "void main() {"                                                               "\n"
//    "    if (mod(gl_PrimitiveIDIn,3)==0)"
   "    drawIt(gl_in[0].gl_Position.xy, 1.0/tiles);"                             "\n"
   "}"                                                                           "\n";

// Fragment shader
const char kFragmentShader[] =                                                   "\n"
   "#version 150 core"                                                           "\n"
   "in vec2 uv;"                                                             "\n"
   "out vec4 outColor;"                                                          "\n"
   "void main() {"                                                               "\n"
   "    outColor = vec4(gl_PrimitiveID/100.0,0.0,0.0, 1.0);"                                      "\n"
   "    outColor = vec4(uv,0,0.25);"                                      "\n"
   "}"                                                                           "\n";

class BucketDrawer {
    // Shader creation helper
    void check(const char* name, GLuint shader) {
        GLint  compileSuccess;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &compileSuccess);
        if (compileSuccess==GL_FALSE) {

            GLchar compLog[1024];
            glGetShaderInfoLog(shader, sizeof(compLog), 0, compLog);
            printf("Can't compile shader %s:\n%s", name, compLog);
            glDeleteShader(shader);
            shader = 0;
        }
    }
    GLuint createShader(GLenum type, const GLchar* src) {
        GLuint shader = glCreateShader(type);
        glShaderSource(shader, 1, &src, nullptr);
        glCompileShader(shader);
        check("?", shader);
        return shader;
    }

    GLuint mProgram;
public:

    BucketDrawer() {
        // Compile and activate shaders
        GLuint vertexShader = createShader(GL_VERTEX_SHADER, kVertexShader);
        GLuint geometryShader = createShader(GL_GEOMETRY_SHADER, kGeometryShader);
        GLuint fragmentShader = createShader(GL_FRAGMENT_SHADER, kFragmentShader);

        mProgram = glCreateProgram();
        glAttachShader(mProgram, vertexShader);
        glAttachShader(mProgram, geometryShader);
        glAttachShader(mProgram, fragmentShader);
        glLinkProgram(mProgram);

        glDeleteShader(fragmentShader);
        glDeleteShader(geometryShader);
        glDeleteShader(vertexShader);
    }

    ~BucketDrawer() {
        glDeleteProgram(mProgram);
    }

    void draw() {

        // Create VAO
        GLuint vao;
        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);

        glUseProgram(mProgram);
        glDrawArrays(GL_POINTS, 0, 16 * 6 - 5);
        glUseProgram(0);
        
        glDeleteVertexArrays(1, &vao);
    }
};


#endif


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

    if (!sVDBBox.valid()) {
    #if 0
        auto M = koala::Matrix4::kIdentity;
        M.scale(Vector3(5,5,5));
        UseProgram(Programs.constant, mCameraController, M);
    #else
        UseProgram(Programs.constant, mCameraController);
    #endif
        Programs.constant.uniform("color", Vector4(.4,.8,.1,1));
        glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
        //glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
        sUnitCube();
    } else {
        UseProgram(Programs.constant, mCameraController);
        Programs.constant.uniform("color", Vector4(.4,.8,.1,1));
        glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
        sVDBBox();
    }

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


    glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
    static BucketDrawer sBucketDraw;
    sBucketDraw.draw();
}

_NUKEVDB_NAMESPACE

