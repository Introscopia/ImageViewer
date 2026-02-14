cbuffer UBO : register(b0, space2){

    int2 input_size : packoffset(c0);
    int2 output_size : packoffset(c0.z);
    float scale : packoffset(c1);
};

Texture2D<float4> input_image : register(t0, space0);
SamplerState inSampler : register(s0, space0);
RWTexture2D<unorm float4> output_image : register(u0, space1);

float gaussian(float x, float sigma){
    return exp(-(x * x) / (2.0 * sigma * sigma));
}

[numthreads(8, 8, 1)]
void main(uint3 global_id : SV_DispatchThreadID){

    if (global_id.x >= output_size.x || global_id.y >= output_size.y)
        return;

    float2 output_coord = float2(global_id.xy);
    float scale = (float)input_size.x / (float)output_size.x;
    float2 src_center = (output_coord + 0.5) * scale - 0.5;
    
    float sigma = scale * 0.5f;
    int radius = (int)ceil(sigma * 1.25f);
    
    float4 accumulated_color = float4(0.0, 0.0, 0.0, 0.0);
    float weight_sum = 0.0;
    
    for (int ky = -radius; ky <= radius; ky++) {
        for (int kx = -radius; kx <= radius; kx++) {
            int2 sample_coord = int2(src_center + float2(kx, ky));
            
            sample_coord = clamp(sample_coord, int2(0, 0), input_size - int2(1, 1));
            
            float2 offset = float2(kx, ky);
            float dist = length(offset);
            float weight = gaussian(dist, sigma);
            
            accumulated_color += input_image.SampleLevel(inSampler, float2(sample_coord) / input_size, 0) * weight;
            weight_sum += weight;
        }
    }
    
    if (weight_sum > 0.0){
        accumulated_color /= weight_sum;
    }
    
    output_image[global_id.xy] = accumulated_color;
}