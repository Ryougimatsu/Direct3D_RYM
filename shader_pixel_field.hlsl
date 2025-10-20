
struct PS_IN
{
    float4 posH : SV_POSITION;
    float4 color : COLOR0;
    float2 uv : TEXCOORD0;
};

Texture2D tex0 : register(t0);
Texture2D tex1 : register(t1);

SamplerState samplerState;

float4 main(PS_IN ps_in) : SV_TARGET
{

    float2 uv;
    float angle = 3.14159f * 45 / 180.0f;
    uv.x = ps_in.uv.x * cos(angle) + ps_in.uv.y * sin(angle);
    uv.y = -ps_in.uv.x * sin(angle) + ps_in.uv.y * cos(angle);

    return tex0.Sample(samplerState, uv) * 0.5f + tex1.Sample(samplerState, ps_in.uv * 0.1f) * 0.5f;
}