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
};

Texture2D tex;
SamplerState samplerState;

float4 main(PS_IN pi) : SV_TARGET
{
    // 材質の色
    float3 material_color = tex.Sample(samplerState, pi.uv).rgb * pi.color.rgb * diffuse_color.rgb;

    // 並行光源 (ディフューズライト)
    float4 normalW = normalize(pi.normalW);
    //float dl = max(0.0f, dot(-direcional_vector, normalW));
    float dl = (dot(-directional_vector, normalW+1.0f)*0.5f);
    float3 diffuse = material_color * directional_color.rgb * dl;

    // 環境光 (アンビエントライト)
    float3 ambient = material_color * ambient_color.rgb;

    // スペキュラ
    float3 toEye = normalize(eye_posW - pi.posW.xyz);
    float3 r = reflect(directional_vector, normalW).xyz;
    float t = pow(max(dot(r, toEye), 0.0f), specular_power);
    float3 specular = specular_color * t;

    float alpha = tex.Sample(samplerState, pi.uv).a * pi.color.a * diffuse_color.a;
    float3 color = ambient + diffuse + specular; // 最終的な我々の目に届く色
    
    //边缘光
    float lim = 1.0f - max(dot(normalW.xyz, toEye), 0.0f);
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
