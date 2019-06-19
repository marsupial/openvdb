
attribute vec3 P;
attribute vec4 color;

varying vec4   vcolor;
varying vec3   coords;

uniform mat4 ModelViewProjection;
uniform vec3 kMaxSize;
// 3 : &value	const float	10.3923044	10.3923044

void main()
{
    gl_Position = ModelViewProjection * vec4(P,1);
	vcolor = gl_Color; //color;
    coords = gl_Position.xyz;
    coords = (P + kMaxSize) / (kMaxSize*2);
}
