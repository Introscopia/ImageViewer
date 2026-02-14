struct VSOutput {
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD;
};

VSOutput main(uint vertexID : SV_VertexID) {
    VSOutput output;
    
    // Generate a fullscreen triangle without vertex data
    float2 uv = float2((vertexID << 1) & 2, vertexID & 2);
    output.position = float4(uv * 2.0 - 1.0, 0.0, 1.0);
    output.uv = uv;
    
    // Flip Y if needed (depends on your coordinate system)
    output.uv.y = 1.0 - output.uv.y;
    
    return output;
}