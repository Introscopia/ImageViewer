cbuffer Uniforms : register(b0, space3) {
    float4 dimensions : packoffset(c0);
}

Texture2D input_texture : register(t0, space2);
SamplerState smp : register(s0, space2);

struct PSInput {
    float4 position : SV_POSITION;
    float2 tex_coord : TEXCOORD0;
};

float4 main(PSInput input) : SV_TARGET {

    float2 img_rct = dimensions.xy;
    float2 dst_rct = dimensions.zw;
    float source_texels_per_dest_pixel = img_rct.x / dst_rct.x;
    float kernel_radius = max(3.0, source_texels_per_dest_pixel) * 1;
    float texel_step = 1.0 / img_rct.x;
    float normalized_radius = kernel_radius * texel_step;
    int sample_count = int(ceil(kernel_radius)) * 2;
    sample_count = clamp(sample_count, 3, 64);
    float4 accumulated_color = float4(0.0, 0.0, 0.0, 0.0);
    float total_weight = 0.0;
    
    for (int i = 0; i < sample_count; i++) {
        
        float offset = (float(i) / float(sample_count - 1) - 0.5) * 2.0 * normalized_radius;
        float2 sample_coord = float2(input.tex_coord.x + offset, input.tex_coord.y);
        float4 sample_color = input_texture.Sample(smp, sample_coord);
        float weight = 1.0 - (abs(offset) / normalized_radius) * 0.333;
        weight = max(0.0, weight); 
        
        accumulated_color += sample_color * weight;
        total_weight += weight;
    }
    
    if (total_weight > 0.0) {
        accumulated_color /= total_weight;
    } else {
        accumulated_color = input_texture.Sample(smp, input.tex_coord);
    }
    
    return accumulated_color;
}