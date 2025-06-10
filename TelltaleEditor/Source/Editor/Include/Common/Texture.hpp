#pragma once

#include <Core/Config.hpp>
#include <Core/BitSet.hpp>
#include <Scripting/ScriptManager.hpp>
#include <Meta/Meta.hpp>
#include <Resource/ResourceRegistry.hpp>
#include <Renderer/RenderAPI.hpp>

#include <vector>
#include <memory>
#include <SDL3/SDL_gpu.h>

class RenderContext;

// =========================== COMMON SURFACE FORMATS

enum class RenderTextureType
{
    DIFFUSE,
};

static struct TextureFormatInfo {
    RenderSurfaceFormat Format;
    SDL_GPUTextureFormat SDLFormat;
    CString ConstantName;
    Float BytesPerPixel;
}
constexpr SDL_FormatMappings[]
{
    {RenderSurfaceFormat::RGBA8, SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM, "kCommonTextureFormatRGBA8", 4.0f},
    {RenderSurfaceFormat::BGRA8, SDL_GPU_TEXTUREFORMAT_B8G8R8A8_UNORM, "kCommonTextureFormatBGRA8", 4.0f},
    {RenderSurfaceFormat::DXT1, SDL_GPU_TEXTUREFORMAT_BC1_RGBA_UNORM, "kCommonTextureFormatDXT1", 0.5f},
    {RenderSurfaceFormat::DXT3, SDL_GPU_TEXTUREFORMAT_BC2_RGBA_UNORM, "kCommonTextureFormatDXT3", 1.0f},
    {RenderSurfaceFormat::DXT5, SDL_GPU_TEXTUREFORMAT_BC3_RGBA_UNORM, "kCommonTextureFormatDXT5", 1.0f},
    {RenderSurfaceFormat::DEPTH32FSTENCIL8, SDL_GPU_TEXTUREFORMAT_D32_FLOAT_S8_UINT, "kCommonTextureFormatDepth32FStencil8", 8.0f},
    {RenderSurfaceFormat::UNKNOWN, SDL_GPU_TEXTUREFORMAT_INVALID}, // do not add below this, add above
};

TextureFormatInfo GetSDLFormatInfo(RenderSurfaceFormat format);
RenderSurfaceFormat FromSDLFormat(SDL_GPUTextureFormat format);

// ==================================================

/// A texture.
class RenderTexture : public HandleableRegistered<RenderTexture>
{
private:
    
    friend struct RenderCommandBuffer;
    friend class RenderContext;
    friend class TextureNormalisationTask;
    friend class RenderFrameUpdateList;
    friend class TextureAPI;
    
    struct Image
    {
        
        U32 Width = 0;
        U32 Height = 0;
        U32 RowPitch = 0;
        U32 SlicePitch = 0;
        
        Meta::BinaryBuffer Data;
        
    };
    
    enum TextureFlag
    {
        TEXTURE_FLAG_DELEGATED = 1, // dont release texture.
    };
    
    // ======= RENDER CONTEXT INTERNAL (NO LOCK NEEDED, NO RESOURCE SHARING WITH OTHER CONTEXTS)
    
    // Last frame number which buffer was used to a binding slot in.
    mutable U64 _LastUsedFrame = 0;
    // Last frame number which this buffer was uploaded from the CPU (updated), that the data is on GPU for.
    mutable U64 _LastUpdatedFrame = 0;
    
    // ===============================
    
    mutable RenderContext* _Context = nullptr;
    SDL_GPUTexture* _Handle = nullptr;
    
    String _Name = "";
    
    Flags _TextureFlags;
    
    Flags _UploadedMips; // uploaded mip indices
    
    U32 _Width = 0, _Height = 0, _Depth = 0, _ArraySize = 0, _NumMips = 0; // array size is 6 for cubemaps, else number in array
    
    RenderSurfaceFormat _Format = RenderSurfaceFormat::RGBA8;
    
    std::vector<Image> _Images; // sub images
    
    void _Release();
    
public:
    
    static constexpr CString ClassHandle = "Handle<D3DTexture>;Handle<T3Texture>";
    static constexpr CString Class = "D3DTexture;T3Texture";
    static constexpr CString Extension = "d3dtx";
    
    inline RenderTexture(Ptr<ResourceRegistry> reg) : HandleableRegistered<RenderTexture>(std::move(reg)) {}
    
    static U32 CalculatePitch(RenderSurfaceFormat format, U32 mipWidth, U32 mipHeight);
    static U32 CalculateSlicePitch(RenderSurfaceFormat format, U32 mipWidth, U32 mipHeight);
    
    // ================================
    
    // If not created, creates the texture to a 2D texture all black.
    void Create2D(RenderContext*, U32 width, U32 height, RenderSurfaceFormat format, U32 nMips);
    
    // Create render target optionally pre-allocated
    void CreateTarget(RenderContext& context, Flags flags, RenderSurfaceFormat format, U32 width,
                      U32 height, U32 nMips, U32 nSlices, U32 array, Bool bDepthTarget, SDL_GPUTexture* handle = nullptr);
    
    SDL_GPUTexture* GetHandle(RenderContext* context); // creates if needed
    
    // ensure the given mip is on the GPU when drawing
    void EnsureMip(RenderContext*, U32 mipIndex);
    
    void SetName(CString name);

    ~RenderTexture(); // RenderContext specific.
    
    static void RegisterScriptAPI(LuaFunctionCollection& Col); // registry normalisation api
    
    virtual void FinaliseNormalisationAsync() override;

    inline virtual CommonClass GetCommonClassType() override
    {
        return CommonClass::TEXTURE;
    }
    
    inline U32 GetImageIndex(U32 mip, U32 slice, U32 face)
    {
        TTE_ASSERT(face >= 0 && face < _ArraySize, "Invalid face index");
        TTE_ASSERT(mip >= 0 && mip < _NumMips, "Invalid mip");
        TTE_ASSERT(slice >= 0 && slice < _Depth, "Invalid slice index");
        return (mip * _Depth * _ArraySize) + (slice * _ArraySize) + face;
    }
    
    inline void GetImageInfo(U32 mip, U32 slice, U32 face, U32& outWidth, U32& outHeight, U32& outPitch, U32& outSlicePitch)
    {
        const Image& img = _Images[GetImageIndex(mip, slice, face)];
        outWidth = img.Width;
        outHeight = img.Height;
        outPitch = img.RowPitch;
        outSlicePitch = img.SlicePitch;
    }
    
    inline void GetDimensions(U32& outWidth, U32& outHeight, U32& outDepth, U32& outArraySize)
    {
        outWidth = _Width;
        outHeight = _Height;
        outDepth = _Depth;
        outArraySize = _ArraySize;
    }
    
};


