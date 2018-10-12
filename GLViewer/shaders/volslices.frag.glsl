
varying vec4 vcolor;
varying vec3 coords;
uniform sampler3D Density;
uniform float     Absorption;
uniform float     kMaxDist;
uniform float     DensityFactor = 5.0;
uniform vec2      Samples       = vec2(128, 32);

uniform int	  Transparency = 1;

void main()
{
#if 0
	float alpha = texture3D(Density, coords).x;

	vec3  color;
	if ( alpha != 0 )
		color = vcolor.rgb / alpha;
	else
		color = vec3(0, 0, 0);

	float density = DensityFactor * alpha;

  
	// compute transmittance and alpha
    float stepSize = 0.015; // sqrt(kMaxDist)/float(Samples[0]);
	//float transmittance = pow(2, -density * stepSize);
	float transmittance = pow(2.71828182846, -density * stepSize);
	alpha = 1.0 - transmittance;

    gl_FragColor = vec4(color, alpha);
#else
  
  	vec4 color = vcolor;
    color.a = texture3D(Density, coords).x;
    color.a *= kMaxDist/float(Samples[0]);

    float cutoff = (Transparency==0) ? 0.01 : 0.001;
    if ( color.a < cutoff )
    {
      if ( any(greaterThan(color.rgb, vec3(cutoff))) )
	    color.a = cutoff;
      else
	    discard;
    }

    if( Transparency == 0 )
    {
      color.rgb /= color.a;
      color.a = 1.0;
    }

    gl_FragColor = color;

#endif
}
