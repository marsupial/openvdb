/*
	ShaderProgram.h
*/

#ifndef VDB_Nuke_ShaderProgram_h__
#define VDB_Nuke_ShaderProgram_h__

#include <string>


class ShaderProgram
{
	unsigned mProgram;

	class Source;
public:
	struct Attribute {
    	const char *name;
        unsigned   location;
    };
    static const Attribute kColorAttributes[];
    static const Attribute kTexAttributes[];

	ShaderProgram() : mProgram(0) {}
    void load(const char *vert, const char *frag, const Attribute * = nullptr );
    void load(const char *vertFrag, const Attribute *a ) { return load(vertFrag, vertFrag, a); }
    operator unsigned () const { return mProgram; }

	template <class T> static void setUniform( unsigned program, const char *name, const T &value );
	template <class T> static void setUniform( const char *name, const T &value );
	template <class T>        void uniform( const char *name, const T &value ) const { setUniform(mProgram, name, value); }

};


#endif
