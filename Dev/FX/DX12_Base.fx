// generated (temp) from MTL_Base.fx

// ======================================================================= COMMON BETWEEN VERTEX AND PIXEL

struct VertexOut
{

    float4 Position : SV_POSITION;
    float3 Colour : TEXCOORD0;

TTE_SECTION_BEGIN(!EFFECT_FLAT)
    float2 UV : TEXCOORD1;
TTE_SECTION_END()

};

// =========================================================================== VERTEX SHADER

TTE_SECTION_BEGIN(TTE_VERTEX)

struct VertexIn
{

    float3 Position : TTE_VERTEX_ATTRIB(AttribPosition);

TTE_SECTION_BEGIN(FEATURE_KEY_LIGHT)
    float3 Normal   : TTE_VERTEX_ATTRIB(AttribNormal);
TTE_SECTION_END()


TTE_SECTION_BEGIN(!EFFECT_FLAT)
    float2 UV       : TTE_VERTEX_ATTRIB(AttribUVDiffuse);
TTE_SECTION_END()


TTE_SECTION_BEGIN(FEATURE_DEFORMABLE)
    uint4 BoneIndex : TTE_VERTEX_ATTRIB(AttribBlendIndex);
    float3 BoneWeight : TTE_VERTEX_ATTRIB(AttribBlendWeight);
TTE_SECTION_END()

};

cbuffer UniformCamera : TTE_UNIFORM_BUFFER(BufferCamera)
{
    float4x4 ViewProjection;
    float2 FOV_XY;
    float NearClip;
    float FarClip;
    float AspectRatio;
};

cbuffer UniformObject : TTE_UNIFORM_BUFFER(BufferObject)
{
    float4x4 WorldMatrix;
    float3 Diffuse;
    float Alpha;
};

TTE_SECTION_BEGIN(FEATURE_KEY_LIGHT)
static const float3 lightPos = float3(10.0f, 10.0f, 10.0f);
TTE_SECTION_END()

VertexOut VertexMain(VertexIn vin

TTE_SECTION_BEGIN(FEATURE_DEFORMABLE)
    , StructuredBuffer<float4x4> boneMatrix : TTE_GENERIC_BUFFER(Generic0) // BONE BUFFER INPUT
TTE_SECTION_END()

)
{

    VertexOut vout;

TTE_SECTION_BEGIN(FEATURE_DEFORMABLE)

    uint i0 = vin.BoneIndex.x;
    uint i1 = vin.BoneIndex.y;
    uint i2 = vin.BoneIndex.z;

    float w0 = vin.BoneWeight.x;
    float w1 = vin.BoneWeight.y;
    float w2 = vin.BoneWeight.z;

    float4 localPos = float4(vin.Position, 1.0f);
    float4x4 skin0 = boneMatrix[i0];
    float4x4 skin1 = boneMatrix[i1];
    float4x4 skin2 = boneMatrix[i2];

    float4 skinnedPosition =
        mul(skin0, localPos) * w0 +
        mul(skin1, localPos) * w1 +
        mul(skin2, localPos) * w2;

TTE_SECTION_ELSE()

    float4 skinnedPosition = float4(vin.Position, 1.0f);

TTE_SECTION_END() // FEATURE_DEFORMABLE

    vout.Position = mul(ViewProjection, mul(WorldMatrix, skinnedPosition));

TTE_SECTION_BEGIN(FEATURE_KEY_LIGHT)

    float3 NormalWorld = normalize(mul(float4(vin.Normal, 0.0f), WorldMatrix).xyz);
    float3 worldPos = mul(WorldMatrix, float4(vin.Position, 1.0f)).xyz;
    float3 lightDir = normalize(lightPos - worldPos);
    float diff = max(dot(NormalWorld, lightDir), 0.0f);

    float3 ambient = 0.6f * Diffuse;
    vout.Colour = (ambient + diff) * Diffuse;

TTE_SECTION_ELSE() // FEATURE_KEY_LIGHT

    vout.Colour = Diffuse;

TTE_SECTION_BEGIN(!EFFECT_FLAT)

    vout.UV = vin.UV;

TTE_SECTION_END() // !EFFECT_FLAT

TTE_SECTION_END() // FEATURE_KEY_LIGHT

    return vout;
}

TTE_SECTION_END() // TTE_VERTEX

// =========================================================================== PIXEL SHADER

TTE_SECTION_BEGIN(TTE_PIXEL)

TTE_SECTION_BEGIN(EFFECT_FLAT)

float4 PixelMain(VertexOut vin) : SV_Target
{
    return float4(vin.Colour, 1.0f);
}

TTE_SECTION_ELSE() // EFFECT_FLAT

Texture2D<float4> textureMap : TTE_TEXTURE(SamplerDiffuse);
SamplerState textureSampler : TTE_SAMPLER(SamplerDiffuse);

float4 PixelMain(VertexOut vin) : SV_Target
{
    float4 texColor = textureMap.Sample(textureSampler, vin.UV);
    return texColor * float4(vin.Colour, 1.0f);
}

TTE_SECTION_END() // !EFFECT_FLAT

TTE_SECTION_END() // TTE_PIXEL
