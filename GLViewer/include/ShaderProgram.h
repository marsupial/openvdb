/*
	ShaderProgram.h
*/

#ifndef VDB_Nuke_ShaderProgram_h__
#define VDB_Nuke_ShaderProgram_h__

#include <string>

NUKEVDB_NAMESPACE_

class ShaderProgram
{
	unsigned mProgram;

	class Source;
public:
	struct Attribute
    {
    	const char *name;
        unsigned   location;
    };

	ShaderProgram() : mProgram(0) {}
    void load( const char *fileKey, const Attribute ** = NULL );
    void load( const char *fileKey, const Attribute *a = NULL ) { return load(fileKey, a ? &a : NULL); }
    operator unsigned () const { return mProgram; }

	template <class T> static void setUniform( unsigned program, const char *name, const T &value );
	template <class T> static void setUniform( const char *name, const T &value );
	template <class T>        void uniform( const char *name, const T &value ) const { setUniform(mProgram, name, value); }

};

_NUKEVDB_NAMESPACE

#endif
