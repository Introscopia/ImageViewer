cbuffer Uniforms : register(b0, space3) {
    float4 dimensions : packoffset(c0);
}

Texture2D input_texture : register(t0, space2);
SamplerState smp : register(s0, space2);

static const float PI = 3.14159265359;

float gaussian(float x, float sigma) {
    return exp(-(x * x) / (2.0 * sigma * sigma)) / (sqrt(2.0 * PI) * sigma);
}

struct PSInput {
    float4 position : SV_POSITION;
    float2 tex_coord : TEXCOORD0;
};

float4 main(PSInput input) : SV_TARGET {

    float2 img_rct = dimensions.xy;
    float2 dst_rct = dimensions.zw;
    // Calculate scale factor (matches your CPU code)
    float scale = img_rct.y / dst_rct.y;
    float sigma = scale * 0.5;
    
    // Calculate radius (matches your CPU code)
    float radius = ceil(sigma * 2); // Using 3*sigma for practical cutoff
    
    // Convert radius to normalized texture coordinates
    float texel_step = 1.0 / img_rct.y;
    float normalized_radius = radius * texel_step;
    
    float4 accumulated_color = float4(0.0, 0.0, 0.0, 0.0);
    float total_weight = 0.0;
    
    // Gaussian blur sampling
    for (int i = 0; i <= int(radius * 2); i++) {
        // Calculate offset from -radius to +radius
        float offset = (float(i) - radius) * texel_step;
        
        // Sample the texture at offset position
        float2 sample_coord = float2(input.tex_coord.x, input.tex_coord.y + offset);
        float4 sample_color = input_texture.Sample(smp, sample_coord);
        
        // Calculate Gaussian weight
        float weight = gaussian(offset / texel_step, sigma);
        
        accumulated_color += sample_color * weight;
        total_weight += weight;
    }
    
    // Normalize by total weight
    if (total_weight > 0.0) {
        accumulated_color /= total_weight;
    } else {
        accumulated_color = input_texture.Sample(smp, input.tex_coord);
    }
    
    return accumulated_color;
}