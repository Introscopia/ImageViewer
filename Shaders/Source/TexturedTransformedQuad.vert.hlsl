cbuffer TransformBlock : register(b0, space1) {
    float4x4 tmatrix : packoffset(c0);
    float2   window : packoffset(c4);
};

struct Input
{
    float3 pos : POSITION;   // vertex position in pixel units
    float2 uv  : TEXCOORD0;  // texture coordinates
};

struct Output
{
    float4 pos : SV_POSITION;
    float2 uv  : TEXCOORD0;
};

Output main(Input IN){
    Output OUT;

    float4 worldPos = mul(float4(IN.pos, 1.0), tmatrix);

    float2 ndc;
    ndc.x =  2 * (worldPos.x / window.x) -1;
    ndc.y = -2 * (worldPos.y / window.y) +1;

    OUT.pos = float4(ndc, 0.0, 1.0);
    OUT.uv  = IN.uv;
    return OUT;
}
