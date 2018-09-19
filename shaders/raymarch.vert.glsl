
attribute vec3 P;
attribute vec3 color;

varying vec3 vcolor;

uniform mat4 ModelViewProjection;

void main()
{
    gl_Position = ModelViewProjection * vec4(P,1);
	vcolor = color;
}
