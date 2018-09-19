
varying  vec3 vcolor;

uniform mat4 ModelView;
uniform sampler3D Density;
uniform float FocalLength;
uniform vec2 WindowSize;
uniform vec3 CamCenter;
uniform vec4 ScreenWindow;
uniform bool Orthographic;
uniform vec2 ScreenCenter;

uniform vec3  kMaxSize      = vec3(1);
uniform float kMaxDist      = 3.464101615; // diaganol from kMaxSize -> -kMaxSize
uniform float Absorption    = 1.0;
uniform float DensityFactor = 5.0;
uniform vec2  Samples       = vec2(128, 32);

uniform vec3 LightPosition = vec3(1, 1.0, 3.0);
uniform vec3 LightIntensity = vec3(15.0, 5, 5);

vec3 safeDenominator( vec3 a )
{
	return vec3(
    	a.x!=0.0 ? a.x : 0.0000000001,
        a.y!=0.0 ? a.y : 0.0000000001,
        a.z!=0.0 ? a.z : 0.0000000001 );
}

bool intersectBox(vec3 Origin, vec3 Dir, vec3 Min, vec3 Max, out float t0, out float t1)
{
    vec3 invR = 1.0 / safeDenominator(Dir);
    vec3 tbot = invR * (Min-Origin);
    vec3 ttop = invR * (Max-Origin);
    vec3 tmin = min(ttop, tbot);
    vec3 tmax = max(ttop, tbot);
#if 0
    vec2 t = max(tmin.xx, tmin.yz);
    t0 = max(t.x, t.y);
    t = min(tmax.xx, tmax.yz);
    t1 = min(t.x, t.y);
    return t0 <= t1;
#else
	t0 = max(max(tmin.x, tmin.y), tmin.z);
	t1 = min(min(tmax.x, tmax.y), tmax.z);
#endif
    return t0 <= t1;
}

void main()
{
  	vec3 rayOrigin;
    vec3 rayDirection;
    vec2 winUV = gl_FragCoord.xy / WindowSize.xy;

	if( !Orthographic )
    {
    	float aspectRatio = WindowSize.x/WindowSize.y;
    	vec2  vpRatio = WindowSize[0] >= WindowSize[1] ? vec2(aspectRatio, 1.0) : vec2(1.0, 1.f/aspectRatio);

    	rayOrigin = CamCenter;
        rayDirection.xy = (winUV*2.f - 1.f ) * vpRatio;
    	rayDirection.z = -FocalLength;
    }
    else
    {
    	rayOrigin = CamCenter + (
        	vec4( ScreenWindow[0] + (ScreenWindow[1]-ScreenWindow[0]) * winUV.x,
    	          ScreenWindow[2] + (ScreenWindow[3]-ScreenWindow[2]) * winUV.y, 0, 1) * ModelView).xyz;

        rayDirection = vec3(0.00001f,0.00001f,-1);
    }
	rayDirection = normalize( (vec4(rayDirection, 0) * ModelView).xyz );


	vec3  aabMin = -kMaxSize;
	vec3  aabMax = kMaxSize;
    float tnear, tfar;

#if 0
    if (intersectBox(rayOrigin, rayDirection, aabMin, aabMax, tnear, tfar))
		gl_FragColor = vec4(0,0,1,1);
    else
    	gl_FragColor = vec4(1,0,0,1);
    return;
#endif

    intersectBox(rayOrigin, rayDirection, aabMin, aabMax, tnear, tfar);
    if (tnear < 0.0) tnear = 0.0;

    vec3 rayStart = rayOrigin + rayDirection * tnear;
    vec3 rayStop  = rayOrigin + rayDirection * tfar;
  
    //gl_FragColor = vec4(vcolor,1); return;

    rayStart = (0.5 * (rayStart + aabMax));
    rayStop = (0.5 * (rayStop + aabMax));

    vec3  pos = rayStart / aabMax;
    float travel = distance(rayStop, rayStart);
    float T = 1.0;
    vec3  Lo = vec3(0.0);

    int   numSamples      = int(Samples[0]);
    float stepSize        = kMaxDist/float(numSamples);
    int   numLightSamples = int(Samples[1]);
    float lscale          = kMaxDist / float(numLightSamples);
    vec3  step            = normalize(rayStop-rayStart) * stepSize;

    for ( int i = 0; i < numSamples && travel > 0.0; ++i, pos += step, travel -= stepSize)
    {
        float density = texture3D(Density, pos).x * DensityFactor;
        if (density <= 0.0)
            continue;

        T *= 1.0-density*stepSize*Absorption;
        if (T <= 0.01)
            break;

        vec3 lightDir = normalize(LightPosition-pos)*lscale;
        float Tl = 1.0;
        vec3 lpos = pos + lightDir;

        for (int s=0; s < numLightSamples; ++s) {
            float ld = texture3D(Density, lpos).x;
            Tl *= 1.0-Absorption*stepSize*ld;
            if (Tl <= 0.01) 
            lpos += lightDir;
        }

        vec3 Li = LightIntensity*Tl;
        Lo += Li*T*density*stepSize;
    }

	//gl_FragColor = vec4(vcolor, 1-T);
    gl_FragColor = vec4(Lo, 1-T);
}
