in vec4 Color;
out vec4 FrameBuffer0;
out uint FrameBuffer1;
uniform uint ObjectID;

void main() {
    vec4 color = Color;
    FrameBuffer0 = vec4(max(color.rgb,0.15), color.a);

    FrameBuffer1 = ObjectID;

#if 0
    float z = gl_FragCoord.z;
    // z = (z + 1) * 0.5;
    // gl_FragDepth = (.2*z) - 0.5;
    gl_FragDepth = gl_FragCoord.z - 0.75f;
#endif
}

