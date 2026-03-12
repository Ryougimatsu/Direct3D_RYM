cbuffer ConstantBuffer : register(b0)
{
    matrix WorldLightViewProj;
}

// ���������ͨ��Ƥ��ɫ���еĹ�������
cbuffer BoneBuffer : register(b1)
{
    matrix BoneTransforms[256];
}

struct VS_INPUT
{
    float4 Pos : POSITION;
    float3 Normal : NORMAL;
    float4 Color : COLOR; // ���� Color
    float2 TexCoord : TEXCOORD;
    uint4 BoneIndices : BLENDINDICES; // ע����������� uint4 �� int4
    float4 BoneWeights : BLENDWEIGHT;
};

float4 main(VS_INPUT input) : SV_POSITION
{
    // 1. ������Ƥ�������
    matrix boneTransform = BoneTransforms[input.BoneIndices[0]] * input.BoneWeights[0];
    boneTransform += BoneTransforms[input.BoneIndices[1]] * input.BoneWeights[1];
    boneTransform += BoneTransforms[input.BoneIndices[2]] * input.BoneWeights[2];
    boneTransform += BoneTransforms[input.BoneIndices[3]] * input.BoneWeights[3];

    // 2. ������Ƥ��ı�������
    float4 skinnedPos = mul(input.Pos, boneTransform);

    // 3. ת�������ղü�ռ�
    return mul(skinnedPos, WorldLightViewProj);
}