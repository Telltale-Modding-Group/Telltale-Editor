#pragma once

// Render types

#include <Core/Config.hpp>
#include <Core/BitSet.hpp>
#include <Core/LinearHeap.hpp>
#include <Renderer/Camera.hpp>
#include <Renderer/RenderParameters.hpp>
#include <Scheduler/JobScheduler.hpp>
#include <Meta/Meta.hpp>

#include <SDL3/SDL.h>
#include <SDL3/SDL_gpu.h>

#include <stack>
#include <vector>
#include <unordered_map>
#include <mutex>

class RenderContext;

// ============================================= ENUMS =============================================

enum class RenderSurfaceFormat
{
    UNKNOWN,
    RGBA8,
    BGRA8,
    DXT1,
    DXT3,
    DXT5,
    DEPTH32FSTENCIL8,
};

enum class RenderTextureAddressMode
{
    CLAMP = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
    WRAP = SDL_GPU_SAMPLERADDRESSMODE_REPEAT,
    //BORDER. Border colors arent supported by SDL3 :( (if outside UV range)
};

enum class RenderTextureMipMapMode
{
    NEAREST = SDL_GPU_SAMPLERMIPMAPMODE_NEAREST,
    LINEAR = SDL_GPU_SAMPLERMIPMAPMODE_LINEAR,
};

enum class RenderShaderType
{
    FRAGMENT,
    VERTEX,
};

enum class RenderBufferAttributeFormat : U32
{
    UNKNOWN,
    
    I32x1,
    I32x2,
    I32x3,
    I32x4,
    
    F32x1,
    F32x2,
    F32x3,
    F32x4,
    
    U32x1,
    U32x2,
    U32x3,
    U32x4,
    
    U8x2,
    U8x4,
    
    I8x2,
    I8x4,
    
    U8x2_NORM,
    U8x4_NORM,
    
    I8x2_NORM,
    I8x4_NORM,
    
};

// ============================================= RENDER VERTEX ATTRIBS =============================================

enum class RenderAttributeType : U32
{
    POSITION = 0,
    NORMAL = 1,
    BINORMAL = 2,
    TANGENT = 3,
    BLEND_WEIGHT = 4,
    BLEND_INDEX = 5,
    COLOUR = 6,
    UV_DIFFUSE = 7,
    UV_LIGHTMAP = 8,
    UNKNOWN,
    COUNT = UNKNOWN,
};

static struct AttribInfo {
    RenderAttributeType Type;
    CString ConstantName;
} constexpr AttribInfoMap[]
{
    {RenderAttributeType::POSITION, "kCommonMeshAttributePosition"},        // 0
    {RenderAttributeType::NORMAL, "kCommonMeshAttributeNormal"},            // 1
    {RenderAttributeType::BINORMAL, "kCommonMeshAttributeBinormal"},        // 2
    {RenderAttributeType::TANGENT, "kCommonMeshAttributeTangent"},          // 3
    {RenderAttributeType::BLEND_WEIGHT, "kCommonMeshAttributeBlendWeight"}, // 4
    {RenderAttributeType::BLEND_INDEX, "kCommonMeshAttributeBlendIndex"},   // 5
    {RenderAttributeType::COLOUR, "kCommonMeshAttributeColour"},            // 6
    {RenderAttributeType::UV_DIFFUSE, "kCommonMeshAttributeUVDiffuse"},     // 7
    {RenderAttributeType::UV_LIGHTMAP, "kCommonMeshAttributeUVLightMap"},   // 8
    {RenderAttributeType::UNKNOWN, "kCommonMeshAttributeUnknown"},          // ~
};

using VertexAttributesBitset = BitSet<RenderAttributeType, (U32)RenderAttributeType::COUNT, RenderAttributeType::POSITION>;

class RenderContext;

struct RenderScene;
class RenderTexture;
class RenderFrame;

// ============================================= UTIL ENUMS AND TYPES =============================================

enum class RenderBufferUsage : U32
{
    VERTEX = SDL_GPU_BUFFERUSAGE_VERTEX,
    INDICES = SDL_GPU_BUFFERUSAGE_INDEX,
    UNIFORM = SDL_GPU_BUFFERUSAGE_GRAPHICS_STORAGE_READ,
};

enum class RenderPrimitiveType : U32
{
    UNKNOWN,
    TRIANGLE_LIST,
    LINE_LIST,
};

// NDC (Normalised device coords) scissor rect. From -1 to 1 in both. By default is whole screen
struct RenderNDCScissorRect
{
    
    Vector2 Min;
    Vector2 Max;
    
    inline RenderNDCScissorRect() : Min(-1.0f, -1.0f), Max(1.0f, 1.0f) {}
    
    // Convert to screen space (0-width/height in x and y)
    inline void GetScreenSpaceRect(U32& x, U32& y, U32& width, U32& height, U32 screenWidth, U32 screenHeight) const
    {
        U32 minX = static_cast<U32>((Min.x + 1.0f) * 0.5f * screenWidth);
        U32 minY = static_cast<U32>((Min.y + 1.0f) * 0.5f * screenHeight);
        U32 maxX = static_cast<U32>((Max.x + 1.0f) * 0.5f * screenWidth);
        U32 maxY = static_cast<U32>((Max.y + 1.0f) * 0.5f * screenHeight);
        x = minX;
        y = minY;
        width = maxX > minX ? (maxX - minX) : 0;
        height = maxY > minY ? (maxY - minY) : 0;
    }
    
    // sub rect. scales child inside parent one
    static inline RenderNDCScissorRect SubRect(const RenderNDCScissorRect& parent, const RenderNDCScissorRect& child)
    {
        RenderNDCScissorRect result;
        Float scaleX = (parent.Max.x - parent.Min.x) * 0.5f;
        Float scaleY = (parent.Max.y - parent.Min.y) * 0.5f;
        Vector2 center = Vector2{(parent.Max.x + parent.Min.x) * 0.5f, (parent.Max.y + parent.Min.y) * 0.5f };
        
        result.Min.x = center.x + (child.Min.x * scaleX);
        result.Min.y = center.y + (child.Min.y * scaleY);
        result.Max.x = center.x + (child.Max.x * scaleX);
        result.Max.y = center.y + (child.Max.y * scaleY);
        
        if (result.Min.x > result.Max.x || result.Min.y > result.Max.y)
        {
            result = RenderNDCScissorRect();
        }
        return result;
    }
    
};

// Returns number of expected vertices for a given primitive type above.
inline U32 GetNumVertsForPrimitiveType(RenderPrimitiveType type, U32 numPrims)
{
    if(type == RenderPrimitiveType::TRIANGLE_LIST)
        return 3 * numPrims; // a triangle is defiend by three points
    else if(type == RenderPrimitiveType::LINE_LIST)
        return 2 * numPrims; // a line is defined by two points
    return 0;
}

// ============================================= RENDER CORE TYPES =============================================

enum class RenderStateType
{
    NONE = 0,
    
    // RASTERISER STATE
    
    CULL_MODE = 1, // SDL_GPUCullMode
    FILL_MODE = 2, // SDL_GPUFillMode
    WINDING = 3, // SDL_GPUFrontFace
    Z_OFFSET = 4, // Bool. True = add 16 (telltale uses this exact number), False = 0
    Z_INVERSE = 5, // Bool. True to invert Z values by scale of -1.0. Only if depth bias is enabled. Affects depth state too.
    Z_CLIPPING = 6, // Bool. Enable depth clipping
    Z_BIAS = 7, // Bool. If true, add depth slope. Add negative depth slope if Z_INVERSE
    
    // DEPTH STENCIL STATE
    Z_ENABLE = 8, // Bool. Enable depth testing
    Z_WRITE_ENABLE = 9, // Bool. Enable writing to depth (mask with 1s, else 0s)
    Z_COMPARE_FUNC = 10, // SDL_GPUCompareOp. Depth comparison operation
    STENCIL_ENABLE = 11, // Bool. Enable stencil testing
    STENCIL_READ_MASK = 12, // U8. Read mask for stencil test
    STENCIL_WRITE_MASK = 13, // U8. Write mask for stencil test
    STENCIL_COMPARE_FUNC = 14, // SDL_GPUCompareOp. Stencil comparison operation
    STENCIL_DEPTH_FAIL_OP = 15, // SDL_GPUStencilOp. Depth test fail operation
    STENCIL_PASS_OP = 16, // SDL_GPUStencilOp. Stencil test pass operation
    STENCIL_FAIL_OP = 17, // SDL_GPUStencilOp. Stencil test fail operation
    
    // RENDER TARGET INDEX 0 BLEND DESC
    BLEND_ENABLE = 18, // Bool
    COLOUR_WRITE_ENABLE_MASK = 19, // Nibble mask. Bit 0 red, through to 3 being alpha. Default all 1s
    SOURCE_BLEND = 20, // SDL_GPUBlendFactor
    DEST_BLEND = 21, // SDL_GPUBlendFactor
    BLEND_OP = 22, // SDL_GPUBlendOp
    ALPHA_SOURCE_BLEND = 23, // SDL_GPUBlendFactor
    ALPHA_DEST_BLEND = 24, // SDL_GPUBlendFactor
    ALPHA_BLEND_OP = 25, // SDL_GPUBlendOp
    SEPARATE_ALPHA_BLEND = 26, // Bool, default true. Separate blend for alpha. If false, uses color blends for alpha and alpha_xxx are ignored. (color converted to alpha enum dw)
    
    NUM,
};

// varying unit size bitset, packed, render state information which typically is used attached to a pipeline state.
class RenderStateBlob
{
    
    struct Entry
    {
        RenderStateType Type;
        CString Name;
        U32 BitWidth;
        U32 Default;
        mutable U32 WordIndex;
        mutable U32 WordShift;
    };
    
    // Simpler to store this way. Telltale do this as well and only need to store the bit widths so can calculate the
    static constexpr Entry _Entries[(U32)RenderStateType::NUM] =
    {
        {RenderStateType::NONE, "kRenderStateNone", 0, 0},
        {RenderStateType::CULL_MODE, "kRenderStateCullMode", 2, SDL_GPU_CULLMODE_BACK},
        {RenderStateType::FILL_MODE, "kRenderStateFillMode", 1, SDL_GPU_FILLMODE_FILL},
        {RenderStateType::WINDING, "kRenderStateWinding", 1, SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE},  // TAKE INTO ACCOUNT GRAPHICS API!!! D3D11 USES CLOCKWISE NORMALLY
        {RenderStateType::Z_OFFSET, "kRenderStateZOffset", 1, 0},
        {RenderStateType::Z_INVERSE, "kRenderStateZInverse", 1, 0},
        {RenderStateType::Z_CLIPPING, "kRenderStateZClipping", 1, 1}, // true, enable clipping by default
        {RenderStateType::Z_BIAS, "kRenderStateZBias", 1, 0},
        {RenderStateType::Z_ENABLE, "kRenderStateZEnable", 1, 0},
        {RenderStateType::Z_WRITE_ENABLE, "kRenderStateZWriteEnable", 1, 0},
        {RenderStateType::Z_COMPARE_FUNC, "kRenderStateZCompareFunc", 4, SDL_GPU_COMPAREOP_LESS_OR_EQUAL},
        {RenderStateType::STENCIL_ENABLE, "kRenderStateStencilEnable", 1, 0},
        {RenderStateType::STENCIL_READ_MASK, "kRenderStateStencilReadMask", 8, 0},
        {RenderStateType::STENCIL_WRITE_MASK, "kRenderStateStencilWriteMask", 8, 0},
        {RenderStateType::STENCIL_COMPARE_FUNC, "kRenderStateStencilCompareFunc", 4, SDL_GPU_COMPAREOP_NEVER},
        {RenderStateType::STENCIL_DEPTH_FAIL_OP, "kRenderStateStencilDepthFailOp", 4, SDL_GPU_STENCILOP_KEEP},
        {RenderStateType::STENCIL_PASS_OP, "kRenderStateStencilPassOp", 4, SDL_GPU_STENCILOP_KEEP},
        {RenderStateType::STENCIL_FAIL_OP, "kRenderStateStencilFailOp", 4, SDL_GPU_STENCILOP_KEEP},
        {RenderStateType::BLEND_ENABLE, "kRenderStateBlendEnable", 1, 0},
        {RenderStateType::COLOUR_WRITE_ENABLE_MASK, "kRenderStateColourWriteMask", 4, 0xF}, // all channels
        {RenderStateType::SOURCE_BLEND, "kRenderStateSourceBlend", 4, SDL_GPU_BLENDFACTOR_ONE},
        {RenderStateType::DEST_BLEND, "kRenderStateDestBlend", 4, SDL_GPU_BLENDFACTOR_ZERO},
        {RenderStateType::BLEND_OP, "kRenderStateBlendOp", 3, SDL_BLENDOPERATION_ADD},
        {RenderStateType::ALPHA_SOURCE_BLEND, "kRenderStateAlphaSourceBlend", 4, SDL_GPU_BLENDFACTOR_ZERO},
        {RenderStateType::ALPHA_DEST_BLEND, "kRenderStateAlphaDestBlend", 4, SDL_GPU_BLENDFACTOR_ZERO},
        {RenderStateType::ALPHA_BLEND_OP, "kRenderStateAlphaBlendOp", 3, SDL_BLENDOPERATION_ADD},
        {RenderStateType::SEPARATE_ALPHA_BLEND, "kRenderStateSeparateAlphaBlendEnable", 1, 0},
    };
    
    static Entry _GetEntry(RenderStateType type);
    
    U32 _Words[3];
    
public:
    
    static RenderStateBlob Default;
    
    static void Initialise();
    
    U32 GetValue(RenderStateType) const;
    
    void SetValue(RenderStateType, U32 value);
    
    RenderStateBlob(); // sets defaults
    
};

/// A buffer in the renderer holding data. You own these and can create them below.
struct RenderBuffer
{
    
    // Last frame number which buffer was used to a binding slot in.
    U64 LastUsedFrame = 0;
    // Last frame number which this buffer was uploaded from the CPU (updated), that the data is on GPU for.
    U64 LastUpdatedFrame = 0;
    
    RenderContext* _Context = nullptr;
    SDL_GPUBuffer* _Handle = nullptr;
    
    RenderBufferUsage Usage = RenderBufferUsage::VERTEX;
    U64 SizeBytes = 0; // size of the bytes of the buffer
    
    ~RenderBuffer();
    
};

/// Internal transfer buffer for uploads.
struct _RenderTransferBuffer
{
    
    U64 LastUsedFrame = 0;
    SDL_GPUTransferBuffer* Handle = nullptr;
    U32 Capacity = 0; // total number available
    U32 Size = 0; // total used at the moment if acquired
    
    inline bool operator<(const _RenderTransferBuffer& rhs) const
    {
        return Capacity < rhs.Capacity;
    }
    
};

/// Render sampler which is bindable
struct RenderSampler
{
    
    U64 LastUsedFrame = 0;
    
    RenderContext* _Context = nullptr;
    SDL_GPUSampler* _Handle = nullptr;
    
    Float MipBias = 0.0f;
    RenderTextureAddressMode WrapU = RenderTextureAddressMode::WRAP;
    RenderTextureAddressMode WrapV = RenderTextureAddressMode::WRAP;
    RenderTextureMipMapMode MipMode = RenderTextureMipMapMode::NEAREST;
    
    // Test description, not the handle.
    inline Bool operator==(const RenderSampler& rhs)
    {
        return MipBias == rhs.MipBias && MipMode == rhs.MipMode && WrapU == rhs.WrapU && WrapV == rhs.WrapV;
    }
    
    ~RenderSampler();
    
};

/// Used in pipeline state. Represents the format of a vertex data input array and input attributes. No ownership and they are part of a pipeline state descriptor.
struct RenderVertexState
{
    
    RenderContext* _Context = nullptr;
    
    struct VertexAttrib
    {
        RenderAttributeType Attrib = RenderAttributeType::UNKNOWN; //  ie binding slot.
        RenderBufferAttributeFormat Format = RenderBufferAttributeFormat::UNKNOWN;
        U32 VertexBufferIndex = 0; // 0 to 31 (which vertex buffer to use)
    };
    
    // === INFO FORMAT
    
    VertexAttrib Attribs[32];
    U32 BufferPitches[32] = {0,0,0,0}; // byte pitch between consecutive elements in each vertex buffer
    
    U32 NumVertexBuffers = 0; // between 0 and 32
    U32 NumVertexAttribs = 0; // between 0 and 32
    
    Bool IsHalf = true; // true means U16 indices, false meaning U32
    
    void Release(); // Releases this vertex state. Called automatically. Find() in context will return a new one after this.
    
};

// ============================================= RENDER TARGETS =============================================

// Render target IDs. These are constant render targets such as backbuffers and depth targets. Dynamic ones surpass this range.
enum class RenderTargetConstantID
{
    NONE = -1,
    BACKBUFFER = 0,
    DEPTH,
    NUM,
};

// Info about constant target
struct RenderTargetDesc
{
    RenderTargetConstantID ID;
    CString Name;
    RenderSurfaceFormat Format;
    U32 NumMips = 1;
    U32 NumSlices = 1;
    U32 ArraySize = 1;
    Bool Extrinsic = false; // true for backbuffer handled separate
    Bool DepthTarget = false;
};

constexpr RenderTargetDesc TargetDescs [] =
{
    {RenderTargetConstantID::BACKBUFFER, "BackBuffer", RenderSurfaceFormat::UNKNOWN, 1, 1, 1, true},
    {RenderTargetConstantID::DEPTH, "Depth Stencil", RenderSurfaceFormat::DEPTH32FSTENCIL8, 1, 1, 1, false, true},
    {RenderTargetConstantID::NONE, "None"},
};

const RenderTargetDesc& GetRenderTargetDesc(RenderTargetConstantID id);

struct RenderTargetID
{
    
    friend class RenderContext;
    
private:
    U32 _Value;
public:
    
    inline RenderTargetID() : RenderTargetID(RenderTargetConstantID::NONE) {}
    
    inline RenderTargetID(RenderTargetConstantID targetConst) : _Value((U32)targetConst) {}
    
    inline static RenderTargetID CreateDynamicID(U32 zeroBasedDynamicIndex) // create dynamic id
    {
        return RenderTargetID((RenderTargetConstantID)(zeroBasedDynamicIndex + (U32)RenderTargetConstantID::NUM));
    }
    
    inline Bool IsConstantTarget() const { return _Value < (U32)RenderTargetConstantID::NUM; }
    
    inline Bool IsDynamicTarget() const { return IsValid() && _Value >= (U32)RenderTargetConstantID::NUM; }
    
    inline Bool IsValid() const { return _Value != (U32)-1; }
    
    inline U32 GetRawID() { return _Value; }
    
};

/// Render target surface descriptor
struct RenderTargetIDSurface
{
    RenderTargetID ID;
    U32 Mip;
    U32 Slice;
};

struct RenderTargetResolvedSurface
{
    RenderTexture* Target = nullptr;
    U32 Mip = 0, Slice = 0;
};

// High level render target IDs which will translate to actual render target sets in render
class RenderTargetIDSet
{
    
    friend class RenderContext;
    friend class RenderViewPass;
    
    RenderTargetIDSurface Target[8];
    RenderTargetIDSurface Depth;
    
public:
    
    RenderTargetIDSet() = default;
    
};

// Physical (internal) render targets which are to be bound.
class RenderTargetSet
{
    
    friend class RenderContext;
    friend struct RenderCommandBuffer;
    
    RenderTargetResolvedSurface Target[8];
    RenderTargetResolvedSurface Depth;
    
public:
    
    RenderTargetSet() = default;
    
};

// ============================================= PIPELINE STATES AND PASSES =============================================

/// Bindable pipeline state. Create lots at initialisation and then bind each and render, this is the modern typical best approach to rendering. Internal use. Represents a state of the rasterizer.
struct RenderPipelineState
{
    
    struct
    {
        
        SDL_GPUGraphicsPipeline* _Handle = nullptr; // SDL handle
        RenderContext* _Context; // handle to context
        
    } _Internal;
    
    U64 Hash = 0; // calculated pipeline hash. draw calls etc specify this.
    
    // SET BY INTERNAL USER
    
    String ShaderProgram = "";
    
    RenderPrimitiveType PrimitiveType = RenderPrimitiveType::TRIANGLE_LIST; // primitive type
    
    RenderSurfaceFormat TargetFormat[8];
    RenderSurfaceFormat DepthFormat = RenderSurfaceFormat::UNKNOWN;
    U32 NumColourTargets = 0;
    
    RenderStateBlob RState; // render state
    
    // FUNCTIONALITY
    
    RenderVertexState VertexState; // bindable buffer information (format) - no actual binds
    
    void Create(); // creates and sets hash.
    
    ~RenderPipelineState(); // destroy if needed
    
};

/// A render pass
struct RenderPass
{
    
    SDL_GPURenderPass* _Handle = nullptr;
    SDL_GPUCopyPass* _CopyHandle = nullptr;
    
    CString Name = nullptr;
    Colour ClearColour = Colour::Black;
    Float ClearDepth = 0.0f;
    U8 ClearStencil = 0;
    Bool DoClearColour = false;
    Bool DoClearDepth = false;
    Bool DoClearStencil = false;
    
    RenderTargetSet Targets;

};

// ============================================= RENDER SHADERS =============================================

/// Internal shader, lightweight object.
struct RenderShader
{
    
    String Name; // no extension
    RenderContext* Context = nullptr;
    SDL_GPUShader* Handle = nullptr;
    
    U8 ParameterSlots[PARAMETER_COUNT]; // parameter type => slot index
    VertexAttributesBitset Attribs; // for vertex shaders
    
    inline RenderShader()
    {
        memset(ParameterSlots, 0xFF, PARAMETER_COUNT);
    }
    
    ~RenderShader();
    
};

// ============================================= RENDER COMMAND BUFFER =============================================

/// A set of draw commands which are appended to and then finally submitted in one batch.
struct RenderCommandBuffer
{
    
    RenderContext* _Context = nullptr;
    SDL_GPUCommandBuffer* _Handle = nullptr;
    SDL_GPUFence* _SubmittedFence = nullptr; // wait object for command buffer in render thread
    RenderPass* _CurrentPass = nullptr; // pass stack for command buffer
    Bool _Submitted = false;
    
    std::vector<_RenderTransferBuffer> _AcquiredTransferBuffers; // cleared given to context once done.
    
    Ptr<RenderPipelineState> _BoundPipeline; // currently bound pipeline
    
    // bind a pipeline to this command list
    void BindPipeline(Ptr<RenderPipelineState>& state);
    
    // bind vertex buffers, (array). First is the first slot to bind to. Offsets can be NULL to mean all from start. Offsets is each offset.
    void BindVertexBuffers(Ptr<RenderBuffer>* buffers, U32* offsets, U32 first, U32 num);
    
    // bind an index buffer for a draw call. isHalf = true means U16 indices in the buffer, false = U32 indices. Pass in offset into buffer.
    void BindIndexBuffer(Ptr<RenderBuffer> indexBuffer, U32 startIndex, Bool isHalf);
    
    // Binds num textures along with samplers, starting at the given slot, to the pipeline fragment shader. num is 0 to 32.
    void BindTextures(U32 slot, U32 num, RenderShaderType shader,
                      Ptr<RenderTexture>* pTextures, Ptr<RenderSampler>* pSamplers);
    
    // Bind a default texture
    void BindDefaultTexture(U32 slot, RenderShaderType, Ptr<RenderSampler> sampler, DefaultRenderTextureType type);
    
    // After calling bind default mesh and any other textures or generic buffers have been bound, draw a default mesh using this
    void DrawDefaultMesh(DefaultRenderMeshType type);
    
    // Binds num generic storage buffers (eg camera) to a specific shader in the pipeline.
    void BindGenericBuffers(U32 slot, U32 num, Ptr<RenderBuffer>* pBuffers, RenderShaderType shaderSlot);
    
    // Bind uniform data. Does not need to be within any pass.
    void BindUniformData(U32 slot, RenderShaderType shaderStage, const void* data, U32 size);
    
    // Perform a buffer upload. Prefer to use render frame update list! This is OK for use in defaults.
    void UploadBufferDataSlow(Ptr<RenderBuffer>& buffer, DataStreamRef srcStream, U64 srcOffset, U32 destOffset, U32 numBytes);
    
    // Perform a texture sub-image upload. Prefer to use render frame update list! This is OK for use in defaults.
    void UploadTextureDataSlow(Ptr<RenderTexture>& texture, DataStreamRef srcStream,
                               U64 srcOffset, U32 mip, U32 slice, U32 face, U32 dataSize);
    
    // Draws indexed.
    void DrawIndexed(U32 numIndices, U32 numInstances, U32 indexStart, I32 vertexIndexOffset, U32 firstInstanceIndex);
    
    // Sets as many parameters as possible going down the given stack until all have been set that are needed in currently bound shaders
    void BindParameters(RenderFrame& frame, ShaderParametersStack* stack);
    
    // push a render pass to the stack. Pop the pass when needed. BindXXX calls must have a pass pushed.
    void StartPass(RenderPass&& pass);
    
    RenderPass EndPass(); // end pass, its invalid but its information is in the return value.
    
    // By default all memory uploads and other related operations create their own copy pass if this isn't called. Batch them ideally together
    void StartCopyPass();
    
    void Submit(); // submit. will not wait.
    
    void Wait(); // wait for commands to finish executing, if submitted. use in render thread.
    
    Bool Finished(); // Returns true if finished, or was never submitted.
    
};

// ============================================= HIGH LEVEL DRAW COMMAND STRUCTS =============================================

// A draw instance. These can be created by the render context only on the populater thread to issue high level draw commands.
// These boil down to in the render execution of these finding a pipeline state for this render instance
class RenderInst
{
    
    friend class RenderContext;
    
    RenderInst* _Next = nullptr; // next in unordered array. this is sorted at execution.
    
    /*Used to sort for render layers. BIT FORMAT:
     Bits 0-25: unused (all 0s for now)
     Bits 26-36: sub layer (0-1023)
     Bits 37-45: unused (all 1s for now)
     Bits 46-62: layer + 0x8000 (layer is signed short range, from -32767 to 32767)
     Bits 63-64: transparent mode (MSB, meaning most significant in determining order). not used yet.
     */
    U64 _SortKey;
    
    RenderVertexState _VertexStateInfo; // info (not actual) about vertex state
    
    RenderStateBlob _RenderState;
    
    RenderPrimitiveType _PrimitiveType = RenderPrimitiveType::TRIANGLE_LIST; // default triangles
    
    ShaderParametersGroup* _Parameters = nullptr; // shader inputs
    
    //U32 _ObjectIndex = (U32)-1; // object arbitrary index. can be used to hide specific draws if the view says they arent visible.
    //U32 _BaseIndex = 0; ??
    //U32 _MinIndex = 0;
    //U32 _MaxIndex = 0; // vertex range
    
    U32 _StartIndex = 0; // start index in index buffer
    U32 _IndexCount = 0; // number of indices to draw
    U32 _InstanceCount = 0; // number of instances to draw
    I32 _BaseIndex = 0; // offset added to each index buffer value
    U32 _IndexBufferIndex = 0; // into vertex state
    
    DefaultRenderMeshType _DrawDefault = DefaultRenderMeshType::NONE;
    
    String Program;
    
public:

    /**
     * Sets the render layer. This determines the draw order. layer is from -0x8000 to 0x7FFF and sublayer is from 0 to 0x3FF.
     */
    inline void SetRenderLayer(I32 layer, U32 sublayer){
        U32 transparencyMode = 0; // no need rn
        layer = MIN(0x7FFF, MAX(layer, -0x8000));
        sublayer = MIN(0x3FF, MAX(sublayer, 0));
        _SortKey = ((U64)transparencyMode << 62) | ((U64)sublayer << 26) | ((U64)(layer + 0x8000) << 46) | 0x3FF000000000llu;
    }
    
    /**
     * Gets the current render layer of this draw instance.
     */
    inline I32 GetRenderLayer() {
        return (I32)((U32)(_SortKey >> 46)) - 0x8000;
    }
    
    /**
     Gets the current sub-layer of this draw instance
     */
    inline U32 GetRenderSubLayer(){
        return (U32)((_SortKey >> 26) & 0x3FF);
    }
    
    /**
     * Sets the range of indices to draw.
     
     inline void SetIndexRange(U32 minIndex, U32 maxIndex){
     _MinIndex = minIndex;
     _MaxIndex = maxIndex;
     } */
    
    /**
     Draws a primitive array.
     */
    inline void DrawPrimitives(RenderPrimitiveType type, U32 start_index, U32 prim_count, U32 instance_count, I32 baseIndex)
    {
        _StartIndex = start_index;
        _PrimitiveType = type;
        _InstanceCount = instance_count;
        _IndexCount = GetNumVertsForPrimitiveType(type, prim_count);
        _BaseIndex = baseIndex;
    }
    
    /**
     Draws vertices for a given primitive.
     */
    inline void DrawVertices(RenderPrimitiveType type, U32 start_index, U32 vertex_count, U32 instance_count, I32 baseIndex){
        _InstanceCount = instance_count;
        _PrimitiveType = type;
        _StartIndex = start_index;
        _IndexCount = vertex_count;
        _BaseIndex = baseIndex;
    }
    
    /**
     Sets the vertex and fragment shaders. In the future this will be made obsolete by having a system where shaders are referenced by enums and variants.
     */
    inline void SetShaderProgram(String name)
    {
        Program = std::move(name);
    }
    
    /**
     Required if drawing a non-default mesh. Sets the vertex information layout.
     */
    inline void SetVertexState(RenderVertexState& info)
    {
        _VertexStateInfo = info;
    }
    
    /**
     Draws a default mesh. Ensure you bind the correct parameters required in the parameter stack somewhere for this default mesh type.
     Nothing else needs to be specified. Shaders, blending and everything else is part of the default mesh pipeline so is not customisable.
     */
    inline void DrawDefaultMesh(DefaultRenderMeshType type)
    {
        _DrawDefault = type;
        _InstanceCount = 1;
        _IndexCount = _IndexBufferIndex = 0; // set later
    }
    
    // Returns reference to blob which can be modified.
    inline RenderStateBlob& GetRenderState()
    {
        return _RenderState;
    }
    
    inline Bool operator<(const RenderInst& rhs) const
    {
        return _SortKey < rhs._SortKey;
    }
    
    friend struct RenderInstSorter;
    friend struct RenderViewPass;
    
};

// Sorter for above
struct RenderInstSorter {
    
    inline Bool operator()(const RenderInst* lhs, const RenderInst* rhs) const {
        return lhs->_SortKey < rhs->_SortKey;
    }
    
};

class RenderViewPass;

// registers common mesh constants
void RegisterRenderConstants(LuaFunctionCollection& Col);

// Utilities for rendering. Only to be used in scene::asyncrender and its calls
namespace RenderUtility
{
    
    /**
     Sets the object uniform buffer parameters, setting the world matrix and diffuse colour (includes alpha value).
     */
    void SetObjectParameters(RenderContext& context, ShaderParameter_Object* obj, Matrix4 objectWorldMatrix, Colour colour);
    
    /**
     Sets the camera uniform buffer parameters from a given camera.
     */
    void SetCameraParameters(RenderContext& context, ShaderParameter_Camera* cam, Camera* pCamera);
    
    /**
     Creates a scaling and transforming matrix for the given bounding box so its the same
     */
    inline Matrix4 CreateBoundingBoxModelMatrix(BoundingBox box) {
        // Calculate the center of the bounding box from _Min and _Max
        Float centerX = (box._Min.x + box._Max.x) / 2.0f;
        Float centerY = (box._Min.y + box._Max.y) / 2.0f;
        Float centerZ = (box._Min.z + box._Max.z) / 2.0f;
        
        // Calculate the size (dimensions) of the bounding box
        Float width = box._Max.x - box._Min.x;
        Float height = box._Max.y - box._Min.y;
        Float depth = box._Max.z - box._Min.z;
        
        // Scale factors to transform the bounding box into a unit box from -1 to 1
        Float scaleX = 0.5f * width;
        Float scaleY = 0.5f * height;
        Float scaleZ = 0.5f * depth;
        
        // Translation to move the center of the bounding box to the origin
        Float translateX = -centerX;
        Float translateY = -centerY;
        Float translateZ = -centerZ;
        
        // Create the transformation matrix using scaling and translation
        Matrix4 modelMatrix{};
        
        modelMatrix = MatrixScaling(scaleX, scaleY, scaleZ) * MatrixTranslation(Vector3(translateX, translateY, translateZ));
        
        return modelMatrix;
    }
    
    /**
     Creates a scaling and transforming matrix for the given sphere such that it can be drawn as default sphere or wireframe sphere
     */
    inline Matrix4 CreateSphereModelMatrix(Sphere sphere)
    {
        return MatrixTransformation(Vector3(sphere._Radius), Quaternion::kIdentity, sphere._Center);
    }
    
    // internal draw. null camera means a higher level camera will be searched for in the base parameters stack
    void _DrawInternal(RenderContext& context, Camera* cam, Matrix4 world, Colour col, DefaultRenderMeshType primitive, RenderViewPass* pBaseParams);
    
    /**
     Draws a wire sphere with the given camera and model matrix specifying its world transformation. Vertices from -1 to 1. Camera can be null. Pass in base parameters.
     */
    inline void DrawWireSphere(RenderContext& context, Camera* cam, Matrix4 model, Colour col, RenderViewPass* pBaseParams)
    {
        _DrawInternal(context, cam, model, col, DefaultRenderMeshType::WIREFRAME_SPHERE, pBaseParams);
    }
    
    /**
     Draws a wireframe unit capsule with the given camera and model matrix specifying its world transformation. Vertices are from -1 to 1! Camera can be null. Pass in base parameters.
     */
    inline void DrawWireCapsule(RenderContext& context, Camera* cam, Matrix4 model, Colour col, RenderViewPass* pBaseParams)
    {
        _DrawInternal(context, cam, model, col, DefaultRenderMeshType::WIREFRAME_CAPSULE, pBaseParams);
    }
    
    /**
     Draws a wireframe box with the given camera and model matrix specifying its world transformation.  Vertices from -1 to 1 Camera can be null. Pass in base parameters.
     */
    inline void DrawWireBox(RenderContext& context, Camera* cam, Matrix4 model, Colour col, RenderViewPass* pBaseParams)
    {
        _DrawInternal(context, cam, model, col, DefaultRenderMeshType::WIREFRAME_BOX, pBaseParams);
    }
    
    /**
     Draws a filled coloured box with the given camera and model matrix specifying its world transformation.  Vertices from -1 to 1 Camera can be null. Pass in base parameters.
     */
    inline void DrawFilledBox(RenderContext& context, Camera* cam, Matrix4 model, Colour col, RenderViewPass* pBaseParams)
    {
        _DrawInternal(context, cam, model, col, DefaultRenderMeshType::FILLED_BOX, pBaseParams);
    }
    
    /**
     Draws a filled in sphere with the given camera and model matrix specifying its world transformation.  Vertices from -1 to 1 Camera can be null. Pass in base parameters.
     */
    inline void DrawFilledSphere(RenderContext& context, Camera* cam, Matrix4 model, Colour col, RenderViewPass* pBaseParams)
    {
        _DrawInternal(context, cam, model, col, DefaultRenderMeshType::FILLED_SPHERE, pBaseParams);
    }
    
    /**
     Draws a filled in cone with the given camera and model matrix specifying its world transformation.  Vertices from -1 to 1 in height and width. Camera can be null. Pass in base parameters.
     */
    inline void DrawFilledCone(RenderContext& context, Camera* cam, Matrix4 model, Colour col, RenderViewPass* pBaseParams)
    {
        _DrawInternal(context, cam, model, col, DefaultRenderMeshType::FILLED_CONE, pBaseParams);
    }
    
    /**
     Draws a wireframe cone with the given camera and model matrix specifying its world transformation.  Vertices from -1 to 1 Camera can be null. Pass in base parameters.
     */
    inline void DrawWireCone(RenderContext& context, Camera* cam, Matrix4 model, Colour col, RenderViewPass* pBaseParams)
    {
        _DrawInternal(context, cam, model, col, DefaultRenderMeshType::WIREFRAME_CONE, pBaseParams);
    }
    
    /**
     Draws a filled in cylinder with the given camera and model matrix specifying its world transformation.  Vertices from -1 to 1 in height and width. Camera can be null. Pass in base parameters.
     */
    inline void DrawFilledCylinder(RenderContext& context, Camera* cam, Matrix4 model, Colour col, RenderViewPass* pBaseParams)
    {
        _DrawInternal(context, cam, model, col, DefaultRenderMeshType::FILLED_CYLINDER, pBaseParams);
    }
    
    /**
     Draws a wireframe cylinder with the given camera and model matrix specifying its world transformation.  Vertices from -1 to 1 Camera can be null. Pass in base parameters.
     */
    inline void DrawWireCylinder(RenderContext& context, Camera* cam, Matrix4 model, Colour col, RenderViewPass* pBaseParams)
    {
        _DrawInternal(context, cam, model, col, DefaultRenderMeshType::WIREFRAME_CYLINDER, pBaseParams);
    }
    
}
