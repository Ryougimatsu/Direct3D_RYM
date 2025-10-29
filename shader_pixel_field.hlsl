cbuffer PS_CONSTANT_BUFFER : register(b0)
{
    float4 diffuse_color;

}

cbuffer PS_CONSTANT_BUFFER : register(b1)
{
    float4 ambient_color;
}

cbuffer PS_CONSTANT_BUFFER : register(b2)
{
    float4 directional_vector;
    float4 directional_color = { 1.0f, 1.0f, 1.0f, 1.0f };
}

cbuffer PS_CONSTANT_BUFFER : register(b3)
{
    float3 eye_posW;
    float specular_power = 30.0f;
    float4 specular_color = { 0.1f, 0.01f, 0.1f, 1.0f };
    
}

struct PS_IN
{
    float4 posH : SV_POSITION; // 変換後の座標
    float4 posW : POSITION0; // ワールド座標
    float4 normalW : NORMAL0; // ワールド法線
    float4 blend : COLOR0; // 色
    float2 uv : TEXCOORD0; // uv
};
Texture2D tex0 : register(t0);
Texture2D tex1 : register(t1);

SamplerState samp;

float4 main(PS_IN pi) : SV_TARGET
{
    float2 uv;
    float angle = 3.14159f * 45 / 180.0f;
    uv.x = pi.uv.x * cos(angle) + pi.uv.y * sin(angle);
    uv.y = -pi.uv.x * sin(angle) + pi.uv.y * cos(angle);

    float4 tex_color = tex0.Sample(samp, pi.uv) * pi.blend.g 
                     + tex1.Sample(samp, pi.uv) * pi.blend.r;

    // 材質の色
    float3 material_color = tex_color.rgb * diffuse_color.rgb;

    // 並行光源 (ディフューズライト)
    float4 normalW = normalize(pi.normalW);
    float dl = (dot(-directional_vector, normalW+1.0f)*0.5f);
    float3 diffuse = material_color * directional_color.rgb * dl;

    // 環境光 (アンビエントライト)
    float3 ambient = material_color * ambient_color.rgb;

    // スペキュラ
    float3 toEye = normalize(eye_posW - pi.posW.xyz);
    float3 r = reflect(directional_vector, normalW).xyz;
    float t = pow(max(dot(r, toEye), 0.0f), specular_power);
    float3 specular = specular_color * t;
    //float3 specular = diffuse_color.rgb * t;
   

    float3 color = ambient + diffuse + specular; // 最終的な我々の目に届く色
    return float4(color,1.0f);
}