/*
	ShaderProgram.cpp
*/

#include "ShaderProgram.h"
#include "VDB_GL.h"
#include "../standalone/math/Vector2.h"

#include <assert.h>

#define VDB_NUKE_SHADER_LOC     "/Volumes/Work/Developer/repos/nukevdb/rsrc/shaders/"
#define VDB_NUKE_SHADER_HEAD	"#version 120\n"

#if !defined(DEBUG)
  static const char *kConstant[] = {
  #include "../build/vdbnuke.build/constant.vert.glsl"
  ,
  #include "../build/vdbnuke.build/constant.frag.glsl"
  };

  static const char *kPointSprite[] = {
  #include "../build/vdbnuke.build/pointsprite.vert.glsl"
  ,
  #include "../build/vdbnuke.build/pointsprite.frag.glsl"
  };

  static const char *kVoxelSprite[] = {
  #include "../build/vdbnuke.build/voxelsprite.vert.glsl"
  ,
  #include "../build/vdbnuke.build/voxelsprite.frag.glsl"
  };

  static const char *kRayMarch[] = {
  #include "../build/vdbnuke.build/raymarch.vert.glsl"
  ,
  #include "../build/vdbnuke.build/raymarch.frag.glsl"
  };

  static const char *kVolSlices[] = {
  #include "../build/vdbnuke.build/volslices.vert.glsl"
  ,
  #include "../build/vdbnuke.build/volslices.frag.glsl"
  };
#endif


NUKEVDB_NAMESPACE_

static void shaderSource( const std::string &filePath, std::string &contents )
{
  std::FILE *fp = std::fopen(filePath.c_str(), "r");
  if ( fp )
  {
  	const size_t cLen = contents.length();

    std::fseek(fp, 0, SEEK_END);
    contents.resize( std::ftell(fp) + cLen );
    std::rewind(fp);

    std::fread(&contents[cLen], 1, contents.size(), fp);
    std::fclose(fp);
  }
}

class ShaderProgram::Source
{
	std::string mVertex,
                mFragment;

public:
	Source( const std::string &fileKey ) :
        mVertex(VDB_NUKE_SHADER_HEAD), mFragment(VDB_NUKE_SHADER_HEAD)
    {
      #if !defined(DEBUG)
    	const char **shaders = NULL;
    	if ( fileKey == "constant" )
        	shaders = kConstant;
    	else if ( fileKey == "pointsprite" )
        	shaders = kPointSprite;
    	else if ( fileKey == "voxelsprite" )
        	shaders = kVoxelprite;
    	else if ( fileKey == "volslices" )
        	shaders = kVolSlices;
    	else if ( fileKey == "raymarch" )
        	shaders = kRayMarch;

		if ( shaders )
        {
          mVertex += shaders[0];
          mFragment += shaders[1];
          return;
        }
      #endif

		std::string filePath( VDB_NUKE_SHADER_LOC );
      	filePath += fileKey;

        shaderSource(filePath+std::string(".vert.glsl"), mVertex);
        shaderSource(filePath+std::string(".frag.glsl"), mFragment);
    }
  
    const char* vShader() const { return mVertex.c_str(); }
    const char* fShader() const { return mFragment.c_str(); }
};

static GLuint compileShader( GLenum type, const char *fileSrc, const char *fileKey )
{
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &fileSrc, 0);
    glCompileShader(shader);

    GLint  compileSuccess;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compileSuccess);

	if ( compileSuccess==GL_FALSE )
    {
      const char *shaderType = (type == GL_VERTEX_SHADER)            ? "vertex" :
      						   (type == GL_FRAGMENT_SHADER)          ? "fragmet" :
								#if defined(GL_GEOMETRY_SHADER)
      						     (type == GL_GEOMETRY_SHADER)        ? "geometry" :
                                #endif
                                #if defined(GL_TESS_CONTROL_SHADER)
      						     (type == GL_TESS_EVALUATION_SHADER) ? "tess eval" :
                                #endif
                                #if defined(GL_TESS_CONTROL_SHADER)
      						     (type == GL_TESS_CONTROL_SHADER)    ? "tess control" :
                                #endif
                                #if defined(GL_COMPUTE_SHADER)
      						     (type == GL_COMPUTE_SHADER)         ? "compute" :
                                #endif
                                                                       "unknown";

    	GLchar compLog[1024];
        glGetShaderInfoLog(shader, sizeof(compLog), 0, compLog);
        printf("Can't compile shader %s[%s]:\n%s", fileKey, shaderType, compLog);
        glDeleteShader(shader);
        shader = 0;
    }
    return shader;
}

static void attachShader( GLenum type, const char *fileSrc, const char *fileKey, GLuint program )
{
    glAttachShader(program, compileShader(type, fileSrc, fileKey) );
}

void ShaderProgram::load( const char *fileKey, const Attribute **attrs )
{
	if ( mProgram ) glDeleteProgram(mProgram);
	mProgram = glCreateProgram();

	Source src(fileKey);
	attachShader(GL_VERTEX_SHADER,   src.vShader(), fileKey, mProgram);
	attachShader(GL_FRAGMENT_SHADER, src.fShader(), fileKey, mProgram);

	for ( const Attribute *attr = attrs ? *attrs : NULL; attr && attr->name; ++attr )
    	glBindAttribLocation(mProgram, attr->location, attr->name);

    glLinkProgram(mProgram);

    GLint linkSuccess;
    glGetProgramiv(mProgram, GL_LINK_STATUS, &linkSuccess);
    if ( linkSuccess==GL_FALSE )
    {
    	GLchar compLog[1024];
		glGetProgramInfoLog(mProgram, sizeof(&compLog), 0, compLog);
        printf("Link error %s\n", fileKey);
        printf("%s\n", compLog);
    }
}

static GLint uniformLocation( GLuint program, const char *name )
{
  GLint location = glGetUniformLocation(program, name);
  // kassert( location != -1 );
  return location;
}

template<> void ShaderProgram::setUniform<Matrix4>( GLuint program, const char *name, const Matrix4 &value )
{
    glUniformMatrix4fv(uniformLocation(program, name), 1, GL_FALSE, value[0]);
}
template<> void ShaderProgram::setUniform<Vector4>( GLuint program, const char *name, const Vector4 &value )
{
    glUniform4fv(uniformLocation(program, name), 1, &value[0]);
}
template<> void ShaderProgram::setUniform<Vector3>( GLuint program, const char *name, const Vector3 &value )
{
    glUniform3fv(uniformLocation(program, name), 1, &value[0]);
}
template<> void ShaderProgram::setUniform<koala::Vector2f>( GLuint program, const char *name, const koala::Vector2f &value )
{
    glUniform2fv(uniformLocation(program, name), 1, &value[0]);
}
template<> void ShaderProgram::setUniform<double>( GLuint program, const char *name, const double &value )
{
    glUniform1f(uniformLocation(program, name), value);
}
template<> void ShaderProgram::setUniform<float>( GLuint program, const char *name, const float &value )
{
    glUniform1f(uniformLocation(program, name), value);
}
template<> void ShaderProgram::setUniform<int>( GLuint program, const char *name, const int &value )
{
    glUniform1f(uniformLocation(program, name), value);
}

template <class T>
void ShaderProgram::setUniform( const char *name, const T &value )
{
  GLuint program;
  glGetIntegerv(GL_CURRENT_PROGRAM, (GLint*) &program);
  setUniform(program, name, value);
}

_NUKEVDB_NAMESPACE
