uniform vec3  eyePos;
uniform vec2  G;
uniform float Blend;

in vec4 Color;
in vec3 Vertex;
out vec4 FrameBuffer0;

void main()
{
//FrameBuffer0 = vec4(G, 1);
//return;
vec2  VG = vec2(Vertex.xy)*G;
vec3  gridNormal = vec3(0,1,0);

vec2  iboldinc = vec2(10);
vec2  iboldnextinc = vec2(1);
float floorAlpha = 1;
vec2  levelBlend = vec2(1);
ivec2 levelMultiplier = ivec2(1,1);

    vec2 line, thin_line, bold_line;
    vec2 p, w, bp, bs1, bs2, bs, wsc, lw, blw;
    vec2 line_alpha;
    float edge,sil;

    p = fract(VG);
    lw = vec2( length(vec2(dFdx(VG.x), dFdy(VG.x))),
           length(vec2(dFdx(VG.y), dFdy(VG.y))) ) * 0.5;
    
float lm = 1;
    wsc = vec2(1.75*lm);

    // Thin line value
    //
    w = lw * wsc;
    thin_line = smoothstep(w, vec2(0.0), p)
                + smoothstep(vec2(1.0)-w, vec2(1.0), p);
    // Thin lines may be fading out as the result of a blend between levels.
    // NB: We multiply line_alpha into thin_line here so that bold_line will
    //     dominate later.  We later take the square root of edge, basically
    //     the maximum of the line values, so line_alpha is squared here.
    line_alpha = vec2(1.0) - levelBlend;
    thin_line *= (line_alpha * line_alpha);

    // Bold line value
    //
    // There are two sets of bold lines.  The ones at the current level, and
    // those that will become bold at the next level.  This second set is
    // most likely going to be a subset of the first set.  The levelBlend
    // parameter causes the first set to transition from bold to thin lines,
    // and the second set to transition from thin to bold lines.  We do this
    // by blending between bs1, the additional width from the bold lines in
    // the first set, and bs2, the additional width from the bold lines in
    // the second set.
    //
    // NB: Half the width of a bold line is lw * 1.75 * 2, and here we scale
    //     our distance from a bold line into the range [0,1] multiplying by
    //     iboldinc, and so we must also scale blw for the distance check.
    //     As step() returns 1 if and only if the second argument is greater
    //     than or equal to the first, an iboldinc value of 0 will eliminate
    //     bold lines entirely (first step() call below returns 1, and the
    //     second returns 0).  The same applies for iboldnextinc.
    bp = fract(VG * iboldinc);
    blw = iboldinc * lw * 3.5 * lm;
    bs1 = (1.0-step(blw, bp)) + (step(1.0-blw, bp));
    bp = fract(VG * iboldnextinc);
    blw = iboldnextinc * lw * 3.5 * lm;
    bs2 = (1.0-step(blw, bp)) + (step(1.0-blw, bp));
    bs = mix(bs1, bs2, levelBlend);
    wsc *= (1.0 + bs);

    w = iboldinc * lw * wsc;
    bp = fract(VG * iboldinc);
    bold_line = smoothstep(w, vec2(0.0), bp)
                + smoothstep(vec2(1.0)-w, vec2(1.0), bp);

    line = max(thin_line, bold_line);
    edge = min(max(line.x, line.y), 1.0);

    if(edge < 0.05)
    discard;

    // We use an adjusted lw for the antialiasing here as VG (and thus
    // lw) drops abruptly by a factor of iboldinc when the levelMultiplier
    // is bumped.  Using the raw lw would result in popping when the level
    // is bumped.
    vec2 adjusted_lw = lw * mix(vec2(1.0), iboldinc, levelBlend);
    sil = abs(max(adjusted_lw.x, adjusted_lw.y));
    if(sil > 0.2)
    edge *= 0.2 / sil;

    edge = sqrt(edge);
    edge = 1.0 - (1.0 - edge) * (1.0 - floorAlpha);

    float fw;

    // Basing the horizon fade on the partial derivatives of the real line
    // numbers, while smooth between level transitions,  will eventually
    // cause all the lines to fade as zooming out increases these values.
    // vec2 level_mult = levelMultiplier;
    // fw = smoothstep(0.5, 2.0, length(lw*level_mult));

    // A horizon fade based on the partial derivatives of VG, which
    // we already have in lw.  As levelBlend crosses 1 from below, VG
    // will switch to the previously bold lines and lw will suddenly drop by
    // a factor of iboldinc.  We smooth this transition by interpolating
    // between the current and future value of lw based on levelBlend.
    // TODO: Unfortunately, there is still a jump, nor will this work well
    //       if level transitions are turned off.
    //fw = smoothstep(0.5, 2.0, length(lw));
    //fw = smoothstep(0.5, 2.0, length(lw*mix(vec2(1.0), iboldinc, levelBlend)));

    // Alternative horizon fade explicitly based on the angle the grid normal
    // makes with the vector from the fragment position to the eye.
    fw = abs(dot(normalize(Vertex-eyePos), gridNormal));
    fw = 1.0 - smoothstep(0.0, 0.087, fw);
    float fade = fw;

    FrameBuffer0 = vec4(Color.rgb, edge * Blend * (1.0 - fade));

    // float depth = 0;
    // if (false)
    //     depth = (0.5 * depth + 0.5) * gl_DepthRange.diff + gl_DepthRange.near;
    // gl_FragDepth = depth;
}

