in vec4 Color;
out vec4 FrameBuffer0;

void main() {
#if 1
  FrameBuffer0 = Color;
  //FrameBuffer0 = Color * dot(L,N);
  //gl_FragDepth = -1.f;
#else
  float depth = gl_FragCoord.z; //gl_FragDepth; //gl_FragCoord.z;
  float near = gl_DepthRange.near;
  float diff = gl_DepthRange.diff;
  //depth = (0.5 * depth + 0.5) * diff + near;
  //depth = (0.5 * depth + 0.5);
  FrameBuffer0 = vec4(depth, depth, depth, 1);
#endif

    /*
    vec3 ambient = Color.rgb;
    diffuse = max( dot(L,N), 0. ) * Color.rgb;
    vec3 R = normalize( reflect( -L, N ) );
    vec3 spec = LightColor * pow( max( dot( R, E), 0. ), Shininess );
    gl_FragColor.rgb = Ka*ambient + Kd*diffuse + Ks*spec;
*/
}

