// ======================================================================= INCLUDES AND INFO

#include <metal_stdlib>
using namespace metal;

// ======================================================================= STRUCTURES

struct VertexIn
{
    float3 position TTE_VERTEX_ATTRIB(AttribPosition);

TTE_SECTION_BEGIN(FEATURE_KEY_LIGHT)
    float3 normal TTE_VERTEX_ATTRIB(AttribNormal);
TTE_SECTION_END()

TTE_SECTION_BEGIN(!EFFECT_FLAT)
    float2 uv TTE_VERTEX_ATTRIB(AttribUVDiffuse);
TTE_SECTION_END()

TTE_SECTION_BEGIN(FEATURE_DEFORMABLE)
    uchar4 bone_index TTE_VERTEX_ATTRIB(AttribBlendIndex);
    float3 bone_weight TTE_VERTEX_ATTRIB(AttribBlendWeight);
TTE_SECTION_END()

};

struct VertexOut
{
    float4 position [[position]];
    float3 colour [[user(locn1)]];

TTE_SECTION_BEGIN(!EFFECT_FLAT)
    float2 uv [[user(locn2)]];
TTE_SECTION_END()

};

struct UniformCamera
{
    float4x4 viewProj;
    float2 fov;
    float cam_near;
    float cam_far;
    float aspect;
};

struct UniformObject
{
    float4x4 worldMatrix;
    float3 diffuse;
    float alpha;
};

// ======================================================================= VERTEX SHADER

TTE_SECTION_BEGIN(TTE_VERTEX)

TTE_SECTION_BEGIN(FEATURE_KEY_LIGHT)

// Constants for the point light (hardcoded for now)
constant float3 lightPos = float3(10.0f, 10.0f, 10.0f); // Light position in world space

TTE_SECTION_END()

vertex VertexOut VertexMain(VertexIn in [[stage_in]], 
    constant UniformCamera& cam TTE_UNIFORM_BUFFER(BufferCamera),
    constant UniformObject& obj TTE_UNIFORM_BUFFER(BufferObject)
TTE_SECTION_BEGIN(FEATURE_DEFORMABLE)
    , constant float4x4* boneMatrix TTE_GENERIC_BUFFER(Generic0)
TTE_SECTION_END()
)
{
    VertexOut out;

TTE_SECTION_BEGIN(FEATURE_DEFORMABLE)

    // SKINNING

    // Read bone indices
    uint i0 = (uint)in.bone_index.x;
    uint i1 = (uint)in.bone_index.y;
    uint i2 = (uint)in.bone_index.z;

    // Read bone weights
    float w0 = in.bone_weight.x;
    float w1 = in.bone_weight.y;
    float w2 = in.bone_weight.z;

    // Compute local model space skinned positions.
    float4 localPos = float4(in.position, 1.0);
    float4x4 skin0 = boneMatrix[i0];
    float4x4 skin1 = boneMatrix[i1];
    float4x4 skin2 = boneMatrix[i2];
    float4 skinnedPosition = ((skin0 * localPos) * w0) + ((skin1 * localPos) * w1) + ((skin2 * localPos) * w2);

TTE_SECTION_ELSE()

    float4 skinnedPosition = float4(in.position, 1.0);

TTE_SECTION_END()

    // Apply world and view-projection transform
    out.position = cam.viewProj * obj.worldMatrix * skinnedPosition;

TTE_SECTION_BEGIN(FEATURE_KEY_LIGHT)

    // Calculate normal in world space (applying world matrix)
    float3 normalWorldSpace = normalize((obj.worldMatrix * float4(in.normal, 0.0f)).xyz);
    
    // Convert input position to world space
    float3 worldPos = (obj.worldMatrix * float4(in.position, 1.0f)).xyz;

    // Compute light direction in world space
    float3 lightDir = normalize(lightPos - worldPos);
    
    // Calculate diffuse lighting based on the normal and light direction (Lambertian model)
    float diff = max(dot(normalWorldSpace, lightDir), 0.0f);

    float3 ambient = 0.6 * obj.diffuse;
    out.colour = (ambient + diff) * obj.diffuse;

TTE_SECTION_ELSE()

    out.colour = obj.diffuse;

TTE_SECTION_BEGIN(!EFFECT_FLAT)

    out.uv = in.uv;

TTE_SECTION_END()

TTE_SECTION_END()

    return out;
}

TTE_SECTION_END()

// ======================================================================= PIXEL SHADER

TTE_SECTION_BEGIN(TTE_PIXEL)

TTE_SECTION_BEGIN(EFFECT_FLAT)

fragment float4 PixelMain(VertexOut in [[stage_in]])
{
    return float4(in.colour, 1.0);
}

TTE_SECTION_ELSE()

fragment float4 PixelMain(VertexOut in [[stage_in]], 
                          texture2d<float> texture TTE_TEXTURE(SamplerDiffuse), 
                          sampler samplr TTE_SAMPLER(SamplerDiffuse))
{
    float4 texColor = texture.sample(samplr, in.uv);
    return texColor * float4(in.colour, 1.0f);
}

TTE_SECTION_END()

TTE_SECTION_END()
