//-----------------------------------------------------------------------------
// Constant Buffers
//-----------------------------------------------------------------------------

// Slot: b0 - ������ɫ
cbuffer CB_MATERIAL : register(b0)
{
    float4 g_MaterialColor;
}

//-----------------------------------------------------------------------------
// Resources
//-----------------------------------------------------------------------------

Texture2D g_Texture : register(t0); // ��������ͼ
SamplerState g_Sampler : register(s0); // ���������

// [����] ��Ӱ��Դ
Texture2D g_ShadowMap : register(t1); // ��Ӱ���ͼ (��Ӧ C++ SetShaderResources(1, ...))
SamplerComparisonState g_ShadowSampler : register(s1); // [��Ҫ] ��Ӱ�Ƚϲ����� (��Ҫ C++ �������󶨵� slot 1)

//-----------------------------------------------------------------------------
// Input Structure
//-----------------------------------------------------------------------------
struct PS_IN
{
    float4 posH : SV_POSITION;
    float4 posW : POSITION0;
    float4 normalW : NORMAL0;
    float4 color : COLOR0;
    float2 uv : TEXCOORD0;
    float4 ShadowPos : TEXCOORD1; // [����] ������ VS ���һ��
};

//-----------------------------------------------------------------------------
// Helper Function: ������Ӱ����
// ���� 1.0 (����Ӱ) ~ 0.0 (ȫ��Ӱ)
//-----------------------------------------------------------------------------
float CalcShadowFactor(float4 shadowPos)
{
    // 1. ͸�ӳ��� (�������һ���� [-1, 1])
    float3 projCoords = shadowPos.xyz / shadowPos.w;

    // 2. �� [-1, 1] ӳ�䵽 UV �ռ� [0, 1]
    projCoords.x = projCoords.x * 0.5f + 0.5f;
    projCoords.y = -projCoords.y * 0.5f + 0.5f; // DX11 Y�����£���Ҫ��ת

    // 3. �߽��飺���������Ӱͼ��Χ����Ϊδ���ڵ�
    if (projCoords.x < 0.0f || projCoords.x > 1.0f ||
        projCoords.y < 0.0f || projCoords.y > 1.0f ||
        projCoords.z > 1.0f)
    {
        return 1.0f;
    }

    // 4. ���ƫ�� (Bias) ��ֹ��Ӱ���� (Shadow Acne)
    float bias = 0.0f;
    float currentDepth = projCoords.z - bias;

    // 5. PCF ���� (ʹ�� SampleCmpLevelZero ����Ӳ���ȽϹ���)
    // �Ƚ��߼������ ShadowMap.depth >= currentDepth���򷵻� 1�����򷵻� 0
    // ����ᱻ���Բ�ֵ��������ͱ�Ե
    return g_ShadowMap.SampleCmpLevelZero(g_ShadowSampler, projCoords.xy, currentDepth);
}

//-----------------------------------------------------------------------------
// Main Function
//-----------------------------------------------------------------------------
float CalcShadowFactorPCF(
    float4 shadowPos,
    float3 normalW,
    float3 lightDirection)
{
    float3 projCoords = shadowPos.xyz / shadowPos.w;
    projCoords.x = projCoords.x * 0.5f + 0.5f;
    projCoords.y = -projCoords.y * 0.5f + 0.5f;

    bool outsideShadowMap =
        projCoords.x < 0.0f || projCoords.x > 1.0f ||
        projCoords.y < 0.0f || projCoords.y > 1.0f ||
        projCoords.z < 0.0f || projCoords.z > 1.0f;

    float NdotL = saturate(dot(normalize(normalW), -normalize(lightDirection)));
    float bias = max(0.00008f, 0.00060f * (1.0f - NdotL));
    float currentDepth = projCoords.z - bias;

    uint shadowWidth = 1;
    uint shadowHeight = 1;
    g_ShadowMap.GetDimensions(shadowWidth, shadowHeight);
    float2 texelSize = 1.0f / float2(shadowWidth, shadowHeight);

    float2 uv = projCoords.xy;
    float shadow =
        g_ShadowMap.SampleCmpLevelZero(g_ShadowSampler, uv + float2(-1, -1) * texelSize, currentDepth) +
        g_ShadowMap.SampleCmpLevelZero(g_ShadowSampler, uv + float2( 0, -1) * texelSize, currentDepth) * 2.0f +
        g_ShadowMap.SampleCmpLevelZero(g_ShadowSampler, uv + float2( 1, -1) * texelSize, currentDepth) +
        g_ShadowMap.SampleCmpLevelZero(g_ShadowSampler, uv + float2(-1,  0) * texelSize, currentDepth) * 2.0f +
        g_ShadowMap.SampleCmpLevelZero(g_ShadowSampler, uv, currentDepth) * 4.0f +
        g_ShadowMap.SampleCmpLevelZero(g_ShadowSampler, uv + float2( 1,  0) * texelSize, currentDepth) * 2.0f +
        g_ShadowMap.SampleCmpLevelZero(g_ShadowSampler, uv + float2(-1,  1) * texelSize, currentDepth) +
        g_ShadowMap.SampleCmpLevelZero(g_ShadowSampler, uv + float2( 0,  1) * texelSize, currentDepth) * 2.0f +
        g_ShadowMap.SampleCmpLevelZero(g_ShadowSampler, uv + float2( 1,  1) * texelSize, currentDepth);

    return outsideShadowMap ? 1.0f : shadow / 16.0f;
}

float4 main(PS_IN pin) : SV_TARGET
{
    // 1. ������������
    float4 texColor = g_Texture.Sample(g_Sampler, pin.uv);
    
    // ԭʼ�߼����������͸��/��ʧ����
    if (texColor.a < 0.01f)
    {
        texColor = float4(1.0f, 1.0f, 1.0f, 1.0f);
    }
    else
    {
        clip(texColor.a - 0.1f);
    }

    // 2. ׼����������
    float3 normal = normalize(pin.normalW.xyz);
    // [ע��] �����Ӳ������շ���Ӧ����������Ӱ�Ĺ�Դ����һ�£�������Ӱ�ᡰ��λ��
    float3 lightDir = normalize(float3(-10.0f, -25.0f, 5.0f));

    // 3. ������������� (Half Lambert)
    float NdotL = saturate(dot(normal, -lightDir));
    float halfLambert = NdotL * 0.5f + 0.5f;

    // 4. [����] ������Ӱ�ڵ�
    float shadowFactor = CalcShadowFactorPCF(
        pin.ShadowPos,
        normal,
        lightDir);

    // 5. �ϳ�������ɫ
    // ��ʽ���ԣ�Ambient + (Diffuse * Shadow)
    // ��Ĵ���ʹ���� 0.2f ��Ϊ�������ɫ��halfLambert ��Ϊ������
    // ����ֻ����ӰӰ�� halfLambert ���֣����� 0.2f �Ļ������ȣ�������Ӱ������
    float3 lighting = (0.2f + halfLambert * shadowFactor);

    float4 finalColor;
    finalColor.rgb = texColor.rgb * g_MaterialColor.rgb * lighting;
    finalColor.a = texColor.a;

    return finalColor;
}
