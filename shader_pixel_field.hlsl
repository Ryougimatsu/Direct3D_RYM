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
    float4 posLight : POSITION1;
};
Texture2D tex0 : register(t0);
Texture2D tex1 : register(t1);
SamplerState samp : register(s0);;
Texture2D shadowMap : register(t5);
SamplerComparisonState shadowSampler : register(s5);
float CalculateShadow(float4 posLight)
{
    float3 projCoords = posLight.xyz / posLight.w;
    projCoords.x = projCoords.x * 0.5f + 0.5f;
    projCoords.y = -projCoords.y * 0.5f + 0.5f;

    // 边界检查：超出光源范围视为无阴影
    if (projCoords.z > 1.0f || projCoords.x < 0.0f || projCoords.x > 1.0f || projCoords.y < 0.0f || projCoords.y > 1.0f)
        return 1.0f;

    // 旧值 0.005 在当前 1~200 的光源深度范围内接近 1 个世界单位，
    // 会把角色脚下的阴影明显推开。生成阴影时已经施加了小幅光栅化偏移，
    // 接收端只保留极小的数值误差补偿。
    static const float RECEIVER_BIAS = 0.00002f;
    float bias = RECEIVER_BIAS;
    float currentDepth = projCoords.z - bias;
    
    // PCF 采样
    float shadow = shadowMap.SampleCmpLevelZero(shadowSampler, projCoords.xy, currentDepth);
    return shadow;
}


float CalculateShadowPCF(
    float4 posLight,
    float3 normalW,
    float3 lightDirection)
{
    float3 projCoords = posLight.xyz / posLight.w;
    projCoords.x = projCoords.x * 0.5f + 0.5f;
    projCoords.y = -projCoords.y * 0.5f + 0.5f;

    bool outsideShadowMap =
        projCoords.z < 0.0f || projCoords.z > 1.0f ||
        projCoords.x < 0.0f || projCoords.x > 1.0f ||
        projCoords.y < 0.0f || projCoords.y > 1.0f;

    float NdotL = saturate(dot(normalize(normalW), -normalize(lightDirection)));
    float bias = max(0.00008f, 0.00060f * (1.0f - NdotL));
    float currentDepth = projCoords.z - bias;

    uint shadowWidth = 1;
    uint shadowHeight = 1;
    shadowMap.GetDimensions(shadowWidth, shadowHeight);
    float2 texelSize = 1.0f / float2(shadowWidth, shadowHeight);

    float2 uv = projCoords.xy;
    float shadow =
        shadowMap.SampleCmpLevelZero(shadowSampler, uv + float2(-1, -1) * texelSize, currentDepth) +
        shadowMap.SampleCmpLevelZero(shadowSampler, uv + float2( 0, -1) * texelSize, currentDepth) * 2.0f +
        shadowMap.SampleCmpLevelZero(shadowSampler, uv + float2( 1, -1) * texelSize, currentDepth) +
        shadowMap.SampleCmpLevelZero(shadowSampler, uv + float2(-1,  0) * texelSize, currentDepth) * 2.0f +
        shadowMap.SampleCmpLevelZero(shadowSampler, uv, currentDepth) * 4.0f +
        shadowMap.SampleCmpLevelZero(shadowSampler, uv + float2( 1,  0) * texelSize, currentDepth) * 2.0f +
        shadowMap.SampleCmpLevelZero(shadowSampler, uv + float2(-1,  1) * texelSize, currentDepth) +
        shadowMap.SampleCmpLevelZero(shadowSampler, uv + float2( 0,  1) * texelSize, currentDepth) * 2.0f +
        shadowMap.SampleCmpLevelZero(shadowSampler, uv + float2( 1,  1) * texelSize, currentDepth);

    return outsideShadowMap ? 1.0f : shadow / 16.0f;
}

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
    float shadowFactor = CalculateShadowPCF(
        pi.posLight,
        normalize(pi.normalW.xyz),
        directional_vector.xyz);

    // 並行光源 (ディフューズライト)
    float4 normalW = normalize(pi.normalW);
    float dl = (dot(-directional_vector, normalW+1.0f)*0.5f);
    float3 diffuse = material_color * directional_color.rgb * dl * shadowFactor;

    // 環境光 (アンビエントライト)
    float3 ambient = material_color * ambient_color.rgb;

    // スペキュラ
    float3 toEye = normalize(eye_posW - pi.posW.xyz);
    float3 r = reflect(directional_vector, normalW).xyz;
    float t = pow(max(dot(r, toEye), 0.0f), specular_power);
    float3 specular = specular_color.rgb * t * shadowFactor;
    //float3 specular = diffuse_color.rgb * t;
   

    float3 color = ambient + diffuse + specular; // 最終的な我々の目に届く色
    return float4(color,1.0f);
}
