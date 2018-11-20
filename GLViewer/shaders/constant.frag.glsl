in vec4 Color;
out vec4 FrameBuffer0;

void main() {
#if 1
  FrameBuffer0 = Color;
#else
  float depth = gl_FragCoord.z; //gl_FragDepth; //gl_FragCoord.z;
  float near = gl_DepthRange.near;
  float diff = gl_DepthRange.diff;
  //depth = (0.5 * depth + 0.5) * diff + near;
  //depth = (0.5 * depth + 0.5);
  FrameBuffer0 = vec4(depth, depth, depth, 1);
#endif
}

