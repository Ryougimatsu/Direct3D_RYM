cbuffer PS_CONSTANT_BUFFER : register(b0)
{
    float4 color;

}

cbuffer VS_CONSTANT_BUFFER : register(b1)
{
    float4 ambient_color;
}

cbuffer VS_CONSTANT_BUFFER : register(b2)
{
    float4 directional_vector;
    float4 directional_color;
    float3 eyeposW;
    //float Specular_power;
}

struct PS_IN
{
    float4 posH : SV_POSITION; // 変換後の座標
    float4 posW : POSITION0; // ワールド座標
    float4 normalW : NORMAL0; // ワールド法線
    float4 color : COLOR0; // 色
    float2 uv : TEXCOORD0; // uv
};

Texture2D tex;
SamplerState samplerState;

float4 main(PS_IN pi) : SV_TARGET
{
    float4 normalW = normalize(pi.normalW);
    float dl = max(0.0f,dot(-directional_vector, normalW));

    float3 toEye = normalize(eyeposW - pi.posW.xyz);
    float3 r = reflect(directional_vector.xyz, pi.normalW).xyz;
    float t = pow(max(dot(r,toEye),0.0f),1.2f);
    
    float3 L_color = pi.color.rgb * dl * directional_color.rgb + ambient_color.rgb * pi.color.rgb;
    L_color += float3(1.0f,1.0f,1.0f) * t;
    
    return tex.Sample(samplerState, pi.uv)* float4(L_color,1.0f) * color;
}