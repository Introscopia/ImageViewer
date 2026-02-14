cbuffer UniformBlock : register(b0, space1) {
    float3 transform : packoffset(c0);
};
struct Input{
    float3 position : TEXCOORD0;
    float2 texpos : TEXCOORD1;
};

struct Output{
    float2 texpos : TEXCOORD0;
    float4 position : SV_Position;
};

Output main(Input input){
    Output output;
    output.texpos = input.texpos;
    output.position = float4( transform.x + (transform.z * input.position.x), 
                              transform.y + (transform.z * input.position.y), 0, 1 );
    return output;
}
