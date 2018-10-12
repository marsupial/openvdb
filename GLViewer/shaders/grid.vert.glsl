
in vec3 P;
uniform vec4 color;
uniform mat4 ModelViewProjection;

out vec3 Vertex;
out vec4 Color;

void main() {
    Color = color;
    Vertex = P;
    gl_Position = ModelViewProjection * vec4(P,1);
}
