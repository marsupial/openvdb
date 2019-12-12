in vec2 UV;
out vec4 FrameBuffer0;
uniform sampler2D RenderTexture;
uniform sampler2D DepthTexture;

void main() {
    vec4 color = vec4(0);
    color = texture(RenderTexture, UV);
    FrameBuffer0 = color;

#if 1
    float depth = texture(DepthTexture, UV).r;
    gl_FragDepth = depth;
    if (true)
         depth = (0.5 * depth + 0.5) * gl_DepthRange.diff + gl_DepthRange.near;
    //gl_FragDepth = 0.9999f;
    gl_FragDepth = color.a <= 0.1 ? 1.f : -1.f;
#endif
    //FrameBuffer0 = vec4(depth, depth, depth, 1);
}
