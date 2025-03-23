#pragma once

#include <Renderer/Camera.hpp>
#include <Core/BitSet.hpp>

// DEFAULT BINDABLE TEXTURES AND BUFFERS

enum class DefaultRenderTextureType
{
    NONE,
    BLACK,
    WHITE,
};

enum class DefaultRenderMeshType
{
    NONE,
    QUAD, // requires one texture bound to slot 0
    FILLED_SPHERE, // filled sphere
    WIREFRAME_SPHERE, // wireframe sphere https://www.youtube.com/watch?v=2ycBWnH1-HI 0:54 left screen big.
    WIREFRAME_BOX,
    WIREFRAME_CAPSULE,
    FILLED_BOX,
    FILLED_CONE,
    WIREFRAME_CONE,
    FILLED_CYLINDER,
    WIREFRAME_CYLINDER,
};


// Render parameter buffer types. These MUST match, with align, shader structs. STD140 (16 byte align) should be followed

struct alignas(16) ShaderParameter_Object
{
    Vector4 WorldMatrix[4];
    Vector3 Diffuse; // diffuse colour RGBA
    Float Alpha = 0.f;
};

struct alignas(16) ShaderParameter_Camera
{
    Vector4 ViewProj[4]; // view projection matrix
    Float HFOVParam; // H FOV
    Float VFOVParam; // V FOV
    Float CameraNear; // near clip
    Float CameraFar; // far clip
    Float Aspect; // aspect ratio
};

enum class ShaderParameterTypeClass : U32
{
    // dont change the order of this enum.
    
    UNIFORM_BUFFER, // uniform
    GENERIC_BUFFER, // generic buffer (like uniform but larger and modifyable)
    SAMPLER, // sampler for texture
    VERTEX_BUFFER, // input vertex buffer
    INDEX_BUFFER, // input index bufer
    
};

struct ShaderParameterTypeInfo
{
    
    CString Name; // name
    U32 Size; // size
    ShaderParameterTypeClass Class; // type of parameter (eg uniform/generic/sampler)
    
    DefaultRenderTextureType DefaultTex;
    
    inline constexpr ShaderParameterTypeInfo(CString name, U64 sz, ShaderParameterTypeClass cl, DefaultRenderTextureType texDef)
    : Name(name), Size((U32)sz), Class(cl), DefaultTex(texDef)
    {
    }
    
    inline constexpr ShaderParameterTypeInfo(CString name, U64 sz, ShaderParameterTypeClass cl)
    : Name(name), Size((U32)sz), Class(cl), DefaultTex(DefaultRenderTextureType::BLACK)
    {
    }
    
};

enum ShaderParameterType : U32
{
    
    // UNIFORMS
    
    PARAMETER_FIRST_UNIFORM = 0,
    
    ///
    PARAMETER_OBJECT = 0, // see ShaderParameter_Object
    PARAMETER_CAMERA = 1,
    ///
    
    PARAMETER_LAST_UNIFORM = 1,
    
    // SAMPLERS
    
    PARAMETER_FIRST_SAMPLER = 2,
    
    ///
    PARAMETER_SAMPLER_DIFFUSE = 2,
    ///
    
    PARAMETER_LAST_SAMPELR = 2,
    
    // VERTEX AND INDEX BUFFERS
    
    PARAMETER_FIRST_VERTEX_BUFFER = 3,
    
    ///
    PARAMETER_VERTEX0IN = 3, // vertex buffer inputs
    PARAMETER_VERTEX1IN = 4,
    PARAMETER_VERTEX2IN = 5,
    PARAMETER_VERTEX3IN = 6,
    PARAMETER_VERTEX4IN = 7,
    PARAMETER_VERTEX5IN = 8,
    PARAMETER_VERTEX6IN = 9,
    PARAMETER_VERTEX7IN = 10,
    PARAMETER_VERTEX8IN = 11,
    ///
    
    PARAMETER_LAST_VERTEX_BUFFER = 11,
    
    ///
    PARAMETER_INDEX0IN = 12, // index buffer input (only one)
    ///
    
    // GENERIC BUFFERS
    
    PARAMETER_FIRST_GENERIC = 13,
    
    ///
    PARAMETER_GENERIC0 = 13,
    ///
    
    PARAMETER_LAST_GENERIC = 13,
    
    PARAMETER_COUNT = 14, // increase by one each time another added. AND UPDATE NAMES BELOW!
};

// Info mappings by type enum index
constexpr ShaderParameterTypeInfo ShaderParametersInfo[] =
{
    ShaderParameterTypeInfo("BufferObject", sizeof(ShaderParameter_Object), ShaderParameterTypeClass::UNIFORM_BUFFER),
    ShaderParameterTypeInfo("BufferCamera", sizeof(ShaderParameter_Camera), ShaderParameterTypeClass::UNIFORM_BUFFER),
    ShaderParameterTypeInfo("SamplerDiffuse", 0, ShaderParameterTypeClass::SAMPLER, DefaultRenderTextureType::BLACK),
    ShaderParameterTypeInfo("Vertex0Input", 0, ShaderParameterTypeClass::VERTEX_BUFFER),
    ShaderParameterTypeInfo("Vertex1Input", 0, ShaderParameterTypeClass::VERTEX_BUFFER),
    ShaderParameterTypeInfo("Vertex2Input", 0, ShaderParameterTypeClass::VERTEX_BUFFER),
    ShaderParameterTypeInfo("Vertex3Input", 0, ShaderParameterTypeClass::VERTEX_BUFFER),
    ShaderParameterTypeInfo("Vertex4Input", 0, ShaderParameterTypeClass::VERTEX_BUFFER),
    ShaderParameterTypeInfo("Vertex5Input", 0, ShaderParameterTypeClass::VERTEX_BUFFER),
    ShaderParameterTypeInfo("Vertex6Input", 0, ShaderParameterTypeClass::VERTEX_BUFFER),
    ShaderParameterTypeInfo("Vertex7Input", 0, ShaderParameterTypeClass::VERTEX_BUFFER),
    ShaderParameterTypeInfo("Vertex8Input", 0, ShaderParameterTypeClass::VERTEX_BUFFER),
    ShaderParameterTypeInfo("Indices0Input", 0, ShaderParameterTypeClass::INDEX_BUFFER),
    ShaderParameterTypeInfo("Generic0", 0, ShaderParameterTypeClass::GENERIC_BUFFER),
};

using ShaderParameterTypes = BitSet<ShaderParameterType, PARAMETER_COUNT, PARAMETER_OBJECT>;

// BIND STRUCTS. SEE BELOW.

struct RenderBuffer;
struct RenderSampler;
struct RenderTexture;

struct UniformParameterBinding
{
    const void* Data; // uniform data to upload
    U32 Size; // size of data to upload
};

struct GenericBufferParameterBinding // these cover all generics and also vertex + index buffers
{
    RenderBuffer* Buffer;
    U32 Offset; // offset in buffer
};

struct SamplerParameterBinding
{
    RenderTexture* Texture;
    RenderSampler* SamplerDesc; // not created, just a descriptor.
};

/**
 For all functionality see the creation and modifying functions inside of RenderContext.
 Represents a group of parameters, from 0 to all of the bitset parameter types. You set the sampler/buffer pointers when you want and them bind the parameter stack its part of.
 This is only the header in actual memory, the rest of it contains the buffer information and shared pointers.
 The idea is that you have some parameters which need to be set and they could be bound from anywhere in a stack of parameters. When binding, the parameters which
 need to be set are iterated over and starting with the top of the stack and going down (see stack below) each group is gone through until all needed parameters have been set.
 See BindParameters in the command list.
 */
struct ShaderParametersGroup
{
    
    struct ParameterHeader
    {
        U8 Type; // parameter type enum
        U8 Class; // parameter class enum
    };
    
    union Parameter
    {
        UniformParameterBinding UniformValue; // if a uniform parameter, this field contains the info to bind when needed
        GenericBufferParameterBinding GenericValue; // if a generic parameter, this field contains the info to bind when needed
        SamplerParameterBinding SamplerValue; // if a sampler parameter, this field contains the info to bind when needed
    };
    
    U32 NumParameters = 0; // number of parameters in this group
    
    // get parameter header by index
    inline ParameterHeader& GetHeader(U32 index)
    {
        return *(((ParameterHeader*)((U8*)this + sizeof(ShaderParametersGroup) + (index * sizeof(ParameterHeader)))));
    }
    
    // Gets a parameter by index
    inline Parameter& GetParameter(U32 index)
    {
        Parameter& p = *(((Parameter*)((U8*)this + sizeof(ShaderParametersGroup) +
                                       (NumParameters * sizeof(ParameterHeader)) + (index*sizeof(Parameter)))));
        return p;
    }
    
};

/**
 For all functionality see the creation and modifying functions inside of RenderContext.
 Represents an element in a shader parameter stack. This can be chained with others.
 */
struct ShaderParametersStack
{
    
    ShaderParametersStack* Parent = nullptr; // parent group to search if the parameter looking for isn't in this group.
    
    ShaderParametersGroup* Group = nullptr; // associated group of parameters
    
    ShaderParameterTypes ParameterTypes; // parameter types inside the associated group.
    
};
