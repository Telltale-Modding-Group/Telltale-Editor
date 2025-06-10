// generated (temp) from MTL_Base.fx

// ======================================================================= STRUCTURES

struct VertexIn
{
    float3 position : TTE_VERTEX_ATTRIB(AttribPosition);

#ifdef FEATURE_KEY_LIGHT
    float3 normal   : TTE_VERTEX_ATTRIB(AttribNormal);
#endif

#ifndef EFFECT_FLAT
    float2 uv       : TTE_VERTEX_ATTRIB(AttribUVDiffuse);
#endif

#ifdef FEATURE_DEFORMABLE
    uint4 bone_index : TTE_VERTEX_ATTRIB(AttribBlendIndex);
    float3 bone_weight : TTE_VERTEX_ATTRIB(AttribBlendWeight);
#endif
};

struct VertexOut
{
    float4 position : SV_POSITION;
    float3 colour   : TEXCOORD1;

#ifndef EFFECT_FLAT
    float2 uv       : TEXCOORD2;
#endif
};

// ======================================================================= BUFFERS

cbuffer UniformCamera : register(bTTE_UNIFORM_BUFFER(BufferCamera))
{
    float4x4 viewProj;
    float2 fov;
    float cam_near;
    float cam_far;
    float aspect;
};

cbuffer UniformObject : register(bTTE_UNIFORM_BUFFER(BufferObject))
{
    float4x4 worldMatrix;
    float3 diffuse;
    float alpha;
};

#ifdef FEATURE_KEY_LIGHT
static const float3 lightPos = float3(10.0f, 10.0f, 10.0f);
#endif

#ifdef TTE_VERTEX

VertexOut VertexMain(VertexIn vin
#ifdef FEATURE_DEFORMABLE
    , StructuredBuffer<float4x4> boneMatrix : register(tTTE_GENERIC_BUFFER(Generic0))
#endif
)
{
    VertexOut vout;

#ifdef FEATURE_DEFORMABLE

    uint i0 = vin.bone_index.x;
    uint i1 = vin.bone_index.y;
    uint i2 = vin.bone_index.z;

    float w0 = vin.bone_weight.x;
    float w1 = vin.bone_weight.y;
    float w2 = vin.bone_weight.z;

    float4 localPos = float4(vin.position, 1.0f);
    float4x4 skin0 = boneMatrix[i0];
    float4x4 skin1 = boneMatrix[i1];
    float4x4 skin2 = boneMatrix[i2];

    float4 skinnedPosition = ((skin0 * localPos) * w0) + ((skin1 * localPos) * w1) + ((skin2 * localPos) * w2);
#else
    float4 skinnedPosition = float4(vin.position, 1.0f);
#endif

    vout.position = mul(viewProj, mul(worldMatrix, skinnedPosition));

#ifdef FEATURE_KEY_LIGHT

    float3 normalWorld = normalize(mul(float4(vin.normal, 0.0f), worldMatrix).xyz);
    float3 worldPos = mul(worldMatrix, float4(vin.position, 1.0f)).xyz;
    float3 lightDir = normalize(lightPos - worldPos);
    float diff = max(dot(normalWorld, lightDir), 0.0f);

    float3 ambient = 0.6f * diffuse;
    vout.colour = (ambient + diff) * diffuse;

#else

    vout.colour = diffuse;

#ifndef EFFECT_FLAT
    vout.uv = vin.uv;
#endif

#endif

    return vout;
}

#endif // TTE_VERTEX

#ifdef TTE_PIXEL

#ifdef EFFECT_FLAT

float4 PixelMain(VertexOut vin) : SV_Target
{
    return float4(vin.colour, 1.0f);
}

#else

Texture2D<float4> textureMap : register(tTTE_TEXTURE(SamplerDiffuse));
SamplerState textureSampler : register(sTTE_SAMPLER(SamplerDiffuse));

float4 PixelMain(VertexOut vin) : SV_Target
{
    float4 texColor = textureMap.Sample(textureSampler, vin.uv);
    return texColor * float4(vin.colour, 1.0f);
}

#endif // EFFECT_FLAT

#endif // TTE_PIXEL
