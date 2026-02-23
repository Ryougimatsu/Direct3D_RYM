cbuffer ConstantBuffer : register(b0)
{
    matrix WorldLightViewProj;
}

// 必须包含普通蒙皮着色器中的骨骼矩阵
cbuffer BoneBuffer : register(b1)
{
    matrix BoneTransforms[256];
}

struct VS_INPUT
{
    float4 Pos : POSITION;
    float3 Normal : NORMAL;
    float4 Color : COLOR; // 加上 Color
    float2 TexCoord : TEXCOORD;
    uint4 BoneIndices : BLENDINDICES; // 注意这里最好用 uint4 或 int4
    float4 BoneWeights : BLENDWEIGHT;
};

float4 main(VS_INPUT input) : SV_POSITION
{
    // 1. 进行蒙皮矩阵计算
    matrix boneTransform = BoneTransforms[input.BoneIndices[0]] * input.BoneWeights[0];
    boneTransform += BoneTransforms[input.BoneIndices[1]] * input.BoneWeights[1];
    boneTransform += BoneTransforms[input.BoneIndices[2]] * input.BoneWeights[2];
    boneTransform += BoneTransforms[input.BoneIndices[3]] * input.BoneWeights[3];

    // 2. 计算蒙皮后的本地坐标
    float4 skinnedPos = mul(input.Pos, boneTransform);

    // 3. 转换到光照裁剪空间
    return mul(skinnedPos, WorldLightViewProj);
}