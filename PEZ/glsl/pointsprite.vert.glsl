
attribute vec3 P;
varying   vec4 vcolor;

uniform mat4 ModelView;
uniform mat4 Projection;
uniform mat4 ModelViewProjection;
uniform vec2 WindowSize;
//uniform float SpriteSize = 0.01299038106;
uniform float kMaxDist = 3.46410155;

//float rand(vec2 co){ return fract(sin(dot(co.xy ,vec2(12.9898,78.233))) * 43758.5453); }

void main()
{
	//float ssize = SpriteSize * kMaxDist;
	float ssize = (kMaxDist / 1.73205078) * 1.5; // * 6.2;

	vec4 position = vec4(P,1); // + vec4( vec3(rand(P.xy), rand(P.yz), rand(P.zx))/10.f, 0);
    vec4 eyePos = ModelView * position;
    vec4 projVoxel = Projection * vec4(vec2(ssize), eyePos.z, eyePos.w);
    vec2 projSize = WindowSize * projVoxel.xy / projVoxel.w;
    gl_PointSize = 0.25 * (projSize.x+projSize.y);
    gl_Position = Projection * eyePos;
	vcolor = vec4(gl_Color.xyz, 1);

    gl_TexCoord[0] = gl_MultiTexCoord0; // sprite texcoord
    //gl_TexCoord[1] = eyePos;
}

