
varying  vec4 vcolor;
uniform sampler2D input0;

void main()
{
#if 0
    float alpha = texture2D(input0, gl_PointCoord).a;
    if (alpha == 0.0) discard;
    gl_FragColor = vec4(vcolor.rgb, vcolor.a*alpha);
#else
    vec3 N;
    N.xy = gl_TexCoord[0].xy*vec2(2.0, -2.0) + vec2(-1.0, 1.0);    
    float r2 = dot(N.xy, N.xy);                                    

    if (r2 > 1.0) discard;   // kill pixels outside circle         
    N.z = sqrt(1.0-r2);                                            

    //  float alpha = saturate(1.0 - r2);                          
    float alpha = clamp((1.0 - r2), 0.0, 1.0);
    alpha *= vcolor.a;

    gl_FragColor = vec4(vcolor.rgb, alpha*0.035);
#endif
}
