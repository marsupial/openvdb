
attribute vec3 P;
varying   vec4 vcolor;

uniform mat4 ModelView;
uniform mat4 Projection;
uniform mat4 ModelViewProjection;
uniform vec2 WindowSize;
uniform float kMaxDist;

void main()
{
	float VoxelSize = kMaxDist / 1.73205078;
	vec4 position = vec4(P,1);
    vec4 eyePos = ModelView * position;
    vec4 projVoxel = Projection * vec4(vec2(VoxelSize), eyePos.z, eyePos.w);
    vec2 projSize = WindowSize * projVoxel.xy / projVoxel.w;
    gl_PointSize = 0.25 * (projSize.x+projSize.y);
    gl_Position = Projection * eyePos;
	vcolor = vec4(gl_Color.xyz, 1);

    gl_TexCoord[0] = gl_MultiTexCoord0; // sprite texcoord
}

