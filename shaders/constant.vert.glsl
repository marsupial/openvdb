attribute vec3 P;
varying   vec4 vcolor;

uniform mat4 ModelViewProjection;

void main()
{
    gl_Position = ModelViewProjection * vec4(P,1);
	vcolor = gl_Color;
}
