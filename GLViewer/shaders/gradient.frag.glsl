
uniform vec2 InvViewport;
uniform vec3 TopColor;
uniform vec3 BottomColor;
uniform int Mode;
uniform float Alpha;

out vec4 FrameBuffer0;

void main() {
    vec3 c;

    switch (Mode) {
      case 2:
        float val =  gl_FragCoord.y * InvViewport.y;
        if (val > TopColor.y && val < 1.0-TopColor.y) discard;
        val =  gl_FragCoord.x * InvViewport.x;
        if (val > TopColor.x && val < 1.0-TopColor.x) discard;
        c = BottomColor;
        break;
      case 1:
        //vec3 dither = texture(randTex, gl_FragCoord.xy * coordScale).rgb;
        c = mix(BottomColor, TopColor, vec3(gl_FragCoord.y * InvViewport.y));
        //c += (dither - 0.5) *0.0039216;
        break;
      case 0:
        c = BottomColor;
    }

    FrameBuffer0 = vec4(c, Alpha);
}
