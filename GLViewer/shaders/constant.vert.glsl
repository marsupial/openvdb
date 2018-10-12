
uniform mat4 ModelViewProjection;
uniform vec4 color;
in vec3 P;

out vec4 Color;

void main() {
    Color = color;
    gl_Position = ModelViewProjection * vec4(P,1);
}
