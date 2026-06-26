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

struct PointLight
{
    float3 posW;
    float range;
    float4 color;
};

cbuffer PS_CONSTANT_BUFFER : register(b4)
{
   PointLight Point_light[4];
    int Point_light_count;
    float3 point_light_dummy;
}

struct PS_IN
{
    float4 posH : SV_POSITION; // 変換後の座標
    float4 posW : POSITION0; // ワールド座標
    float4 normalW : NORMAL0; // ワールド法線
    float4 color : COLOR0; // 色
    float2 uv : TEXCOORD0; // uv
	float4 posLight : POSITION1;
};

Texture2D tex : register(t0);
SamplerState samplerState : register(s0);

Texture2D shadowMap : register(t5);
SamplerComparisonState shadowSampler : register(s5);

float CalculateShadow(float4 posLight)
{
    // 1. 透视除法
    float3 projCoords = posLight.xyz / posLight.w;

    // 2. 将坐标从 [-1, 1] 变换到 [0, 1] 纹理空间
    projCoords.x = projCoords.x * 0.5f + 0.5f;
    projCoords.y = -projCoords.y * 0.5f + 0.5f;

    // 3. 边界检查：如果超出视锥体范围，则不计算阴影（视为被照亮）
    // 【修复1】建议把这里的注释解开，这能防止阴影贴图视野之外的地面变成死黑色
    if (projCoords.z > 1.0f || projCoords.x < 0.0f || projCoords.x > 1.0f || projCoords.y < 0.0f || projCoords.y > 1.0f)
    {
        return 1.0f;
    }

    // 4. 计算 Shadow Bias (防止阴影波纹/Shadow Acne)
    float bias = 0.0f;
    float currentDepth = projCoords.z - bias;

    // 5. 采样并比较深度 (使用 SampleCmpLevelZero 进行硬件 PCF)
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

    // More bias on surfaces viewed at a grazing angle from the light.
    float NdotL = saturate(dot(normalize(normalW), -normalize(lightDirection)));
    float bias = max(0.00008f, 0.00060f * (1.0f - NdotL));
    float currentDepth = projCoords.z - bias;

    uint shadowWidth = 1;
    uint shadowHeight = 1;
    shadowMap.GetDimensions(shadowWidth, shadowHeight);
    float2 texelSize = 1.0f / float2(shadowWidth, shadowHeight);

    // 3x3 tent PCF, weights [1 2 1] x [1 2 1].
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
    // 材質の色
    float3 material_color = tex.Sample(samplerState, pi.uv).rgb * pi.color.rgb * diffuse_color.rgb;

    // 並行光源 (ディフューズライト)
    float4 normalW = normalize(pi.normalW);
    
	float shadowFactor = CalculateShadowPCF(
        pi.posLight,
        normalW.xyz,
        directional_vector.xyz);
    //float dl = max(0.0f, dot(-direcional_vector, normalW));
	float NdotL = dot(-directional_vector, normalW);
	float dl = saturate(NdotL); // 标准做法
	float3 diffuse = material_color * directional_color.rgb * dl * shadowFactor;

    // 環境光 (アンビエントライト)
    float3 ambient = material_color * ambient_color.rgb;

    // スペキュラ
    float3 toEye = normalize(eye_posW - pi.posW.xyz);
    float3 r = reflect(directional_vector, normalW).xyz;
    float t = pow(max(dot(r, toEye), 0.0f), specular_power);
	float3 specular = specular_color.rgb * t * shadowFactor;

    float alpha = tex.Sample(samplerState, pi.uv).a * pi.color.a * diffuse_color.a;
    float3 color = ambient + diffuse + specular; // 最終的な我々の目に届く色
    
    //边缘光
    float lim = saturate(1.0f - max(dot(normalW.xyz, toEye), 0.0f));
    lim = pow(lim,3.2f);
    //color += float3(lim,lim,lim);
    for (int i = 0; i < Point_light_count; i++)
    {

        //面（ピクセル）から点光源へのベクトルを求める
        float3 lightToPixel = pi.posW.xyz - Point_light[i].posW;

        //面（ピクセル）とライトの距離を測る
        float distance = length(lightToPixel);

        //点光源の減衰を求める
        float A = pow(max(1.0f - 1.0f / Point_light[i].range * distance, 0.0f), 2.0f);

        //range = 400 length = 0   -> A =    1 A * A = 1
        //range = 400 length = 100 -> A = 0.75 A * A = 0.5625
         //range = 400 length = 200 -> A =  0.5 A * A = 0.25
        //range = 400 length = 300 -> A = 0.25 A * A = 0.0625
        //range = 400 length = 400 -> A =    0 A * A = 0

        //color += float3(A,A,A);

        //点光源の方向と面（ピクセル）の法線の内積を求める
        float point_light_dl = max(0.0f, dot(-normalize(lightToPixel), normalW.xyz));

        //color += point_light[i].color.rgb * A ;
        color += material_color * Point_light[i].color.rgb * A * point_light_dl;

        //スペキュラ
        float3 point_light_r = reflect(normalize(lightToPixel), normalW.xyz).xyz; //反射ベクトル
        float point_light_t = pow(max(dot(point_light_r, toEye), 0.0f), specular_power); //スペキュラ強度

        //
        color += Point_light[i].color.rgb * point_light_t;

    }
    return float4(color, alpha);
}
