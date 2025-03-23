#pragma once

#include <Core/Config.hpp>
#include <Core/BitSet.hpp>
#include <Scripting/ScriptManager.hpp>
#include <Meta/Meta.hpp>

#include <vector>
#include <SDL3/SDL_gpu.h>

class RenderContext;

// =========================== COMMON SURFACE FORMATS

enum class RenderSurfaceFormat
{
    UNKNOWN,
    RGBA8,
    BGRA8,
    DXT1,
    DXT3,
    DXT5,
};

enum class RenderTextureType
{
    DIFFUSE,
    
};

static struct TextureFormatInfo {
    RenderSurfaceFormat Format;
    SDL_GPUTextureFormat SDLFormat;
    CString ConstantName;
}
constexpr SDL_FormatMappings[]
{
    {RenderSurfaceFormat::RGBA8, SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM, "kCommonTextureFormatRGBA8"},
    {RenderSurfaceFormat::BGRA8, SDL_GPU_TEXTUREFORMAT_B8G8R8A8_UNORM, "kCommonTextureFormatBGRA8"},
    {RenderSurfaceFormat::DXT1, SDL_GPU_TEXTUREFORMAT_BC1_RGBA_UNORM, "kCommonTextureFormatDXT1"},
    {RenderSurfaceFormat::DXT3, SDL_GPU_TEXTUREFORMAT_BC2_RGBA_UNORM, "kCommonTextureFormatDXT3"},
    {RenderSurfaceFormat::DXT5, SDL_GPU_TEXTUREFORMAT_BC3_RGBA_UNORM, "kCommonTextureFormatDXT5"},
    {RenderSurfaceFormat::UNKNOWN, SDL_GPU_TEXTUREFORMAT_INVALID}, // do not add below this, add above
};

TextureFormatInfo GetSDLFormatInfo(RenderSurfaceFormat format);
RenderSurfaceFormat FromSDLFormat(SDL_GPUTextureFormat format);

// ==================================================

/// A texture.
struct RenderTexture
{
    
    struct Image
    {
        
        U32 Width = 0;
        U32 Height = 0;
        U32 RowPitch = 0;
        U32 SlicePitch = 0;
        RenderSurfaceFormat Format;
        
        Meta::BinaryBuffer Data;
        
    };
    
    enum TextureFlag
    {
        TEXTURE_FLAG_DELEGATED = 1, // dont release texture.
    };
    
    // ======= RENDER CONTEXT INTERNAL
    
    // Last frame number which buffer was used to a binding slot in.
    U64 LastUsedFrame = 0;
    // Last frame number which this buffer was uploaded from the CPU (updated), that the data is on GPU for.
    U64 LastUpdatedFrame = 0;
    
    RenderContext* _Context = nullptr;
    SDL_GPUTexture* _Handle = nullptr;
    
    // ================================
    
    String Name = "";
    
    Flags TextureFlags;
    
    U32 Width = 0, Height = 0, Depth = 0;
    
    RenderSurfaceFormat Format = RenderSurfaceFormat::RGBA8;
    
    std::vector<Image> Images; // sub images
    
    // If not created, creates the texture to a 2D texture all black.
    void Create2D(RenderContext*, U32 width, U32 height, RenderSurfaceFormat format, U32 nMips);
    
    ~RenderTexture(); // RenderContext specific.
    
    static void RegisterScriptAPI(LuaFunctionCollection& Col); // registry normalisation api
    
    void FinishNormalisationAsync();
    
};


