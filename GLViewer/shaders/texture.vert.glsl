
uniform mat4 ModelViewProjection;
in vec3 P;

out vec2 UV;

void main() {
    UV = vec2(0.5, 0.5) * (P.xy + vec2(1,1));
    gl_Position = vec4(P,1);
}
