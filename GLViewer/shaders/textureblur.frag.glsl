in vec2 UV;
out vec4 FrameBuffer0;
uniform sampler2D RenderTexture;
uniform sampler2D DepthTexture;

uniform uint ObjectID;
uniform usampler2D ObjectTexture;
#define BlurSize 6

uniform bool horizontal = true;
uniform bool vertical = true;
uniform float weight[5] = float[] (0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216);

float normpdf(in float x, in float sigma)
{
    return 0.39894*exp(-0.5*x*x/(sigma*sigma))/sigma;
}


void main() {
    bool found = false;
    vec4 color = vec4(0);
#if 0
        color = texture(RenderTexture, UV);
#elif 1
        vec2 texSize = textureSize(RenderTexture, 0);
        vec2 texOff = 1.0 / texSize; // gets size of single texel

        //declare stuff
        const int mSize = BlurSize;
        const int kSize = (mSize-1)/2;
        float kernel[mSize];
        vec4 final_colour = vec4(0);
        
        //create the 1-D kernel
        float sigma = 7.0;
        for (int j = 0; j <= kSize; ++j)
            kernel[kSize+j] = kernel[kSize-j] = normpdf(float(j), sigma);

        //get the normalization factor (as the gaussian has been clamped)
        float Z = 0.0;
        for (int j = 0; j < mSize; ++j)
            Z += kernel[j];
    
        //read out the texels
        for (int i=-kSize; i <= kSize; ++i) {
            for (int j=-kSize; j <= kSize; ++j) {
                found = texture(ObjectTexture, UV+vec2(i,j)*texOff).r == ObjectID  || found;
                final_colour += kernel[kSize+j] * kernel[kSize+i] * texture(RenderTexture, UV+vec2(i,j)*texOff);
            }
        }

        color = final_colour * 1.f/(Z*Z);
#else
        vec2 tex_offset = 1.0 / textureSize(RenderTexture, 0); // gets size of single texel
        if (horizontal)
        {
            color += texture(RenderTexture, UV) * weight[0]; // current fragment's contribution
            for(int i = 1; i < 5; ++i) {
                found  = texture(ObjectTexture, UV + vec2(tex_offset.x * i, 0.0)).r == ObjectID || found;
                color += texture(RenderTexture, UV + vec2(tex_offset.x * i, 0.0)) * weight[i];
                color += texture(RenderTexture, UV - vec2(tex_offset.x * i, 0.0)) * weight[i];
            }
        }
        if (vertical)
        {
            color += texture(RenderTexture, UV) * weight[0]; // current fragment's contribution
            for(int i = 1; i < 5; ++i) {
                found  = texture(ObjectTexture, UV + vec2(0.0, tex_offset.y * i)).r == ObjectID || found;
                color += texture(RenderTexture, UV + vec2(0.0, tex_offset.y * i)) * weight[i];
                color += texture(RenderTexture, UV - vec2(0.0, tex_offset.y * i)) * weight[i];
            }
        }
        if (horizontal && vertical)
            color *= 0.5f;
#endif
    if (!found) {
        discard;
        return;
    }

    //color = vec4(color.rgb, 1.f);
    FrameBuffer0 = color;
    //FrameBuffer0 = vec4(color.rgb, 1);

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
