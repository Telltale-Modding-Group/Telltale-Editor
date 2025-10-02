#include <Common/Texture.hpp>
#include <TelltaleEditor.hpp>

class TextureAPI
{
public:
    
    static inline RenderTexture* Task(LuaManager& man)
    {
        return (RenderTexture*)man.ToPointer(1);
    }
    
    static U32 luaTextureSetName(LuaManager& man)
    {
        TTE_ASSERT(man.GetTop() == 2, "Incorrect usage");
        
        RenderTexture* task = Task(man);
        String name = man.ToString(2);
        
        task->_Name = std::move(name);
        
        return 0;
    }
    
    static U32 luaTextureSetFormat(LuaManager& man)
    {
        TTE_ASSERT(man.GetTop() == 2, "Incorrect usage");
        
        RenderTexture* task = Task(man);
        task->_Format = (RenderSurfaceFormat)man.ToInteger(2);
        
        return 0;
    }
    
    static U32 luaTextureSetDimensions(LuaManager& man)
    {
        TTE_ASSERT(man.GetTop() == 6, "Incorrect usage");
        
        RenderTexture* task = Task(man);
        task->_Width = (U32)man.ToInteger(2);
        task->_Height = (U32)man.ToInteger(3);
        task->_NumMips = (U32)man.ToInteger(4);
        task->_Depth = (U32)man.ToInteger(5);
        task->_ArraySize = (U32)man.ToInteger(6);
        
        return 0;
    }
    
    static U32 luaTexturePushImage(LuaManager& man)
    {
        TTE_ASSERT(man.GetTop() == 6, "Incorrect usage");
        
        RenderTexture* task = Task(man);
        
        RenderTexture::Image img{};
        img.Width = (U32)man.ToInteger(2);
        img.Height = (U32)man.ToInteger(3);
        img.RowPitch = (U32)man.ToInteger(4);
        img.SlicePitch = (U32)man.ToInteger(5);
        
        Meta::ClassInstance buf = Meta::AcquireScriptInstance(man, 6);
        TTE_ASSERT(buf, "Texture data buffer invalid");
        
        Meta::BinaryBuffer* pBuffer = (Meta::BinaryBuffer*)buf._GetInternal();
        img.Data = *pBuffer;
        
        task->_Images.push_back(std::move(img));
        
        return 0;
    }
    
    // resolve(state, buffer, resolvableFormat, width, height) with the width and height of this buffer. resolve to rgba.
    static U32 luaTextureResolve(LuaManager& man)
    {
        TTE_ASSERT(man.GetTop() == 5, "Incorrect usage");
        
        RenderTexture* task = Task(man);
        
        Meta::ClassInstance buf = Meta::AcquireScriptInstance(man, 2);
        TTE_ASSERT(buf, "Texture data buffer invalid");
        
        Meta::BinaryBuffer* pBuffer = (Meta::BinaryBuffer*)buf._GetInternal();
        
        U32 fmt = (U32)man.ToInteger(3);
        U32 width = (U32)man.ToInteger(4);
        U32 height = (U32)man.ToInteger(5);
        
        U8* data = pBuffer->BufferData.get();
        TTE_ASSERT(data, "Buffer data empty");
        
        if(fmt == 0) // BGRX => RGBA
        {
            // set alpha to 0xff
            for(U32 i = 0; i < (width*height); i++) // RGBA: 0=r, 1=g, 2=b, 3=a. BGRX 0=b, 1=g 2=r 3=n/a
            {
                data[(i << 2) + 3] = 0xFF; // make opaque
                
                U8 b = data[(i << 2) + 0]; // swap red and blue channels
                data[(i << 2) + 0] = data[(i << 2) + 2];
                data[(i << 2) + 2] = b;
            }
        }
        else
        {
            TTE_ASSERT(false, "Unknown resolvable format!");
        }
        
        return 0;
    }
    
    static U32 luaTextureCalculatePitch(LuaManager& man)
    {
        TTE_ASSERT(man.GetTop() == 3, "Incorrect usage");
        RenderSurfaceFormat format = (RenderSurfaceFormat)man.ToInteger(1);
        U32 w = (U32)man.ToInteger(2);
        U32 h = (U32)man.ToInteger(3);
        man.PushUnsignedInteger(RenderTexture::CalculatePitch(format, w, h));
        return 1;
    }
    
    
    static U32 luaTextureCalculateSlicePitch(LuaManager& man)
    {
        TTE_ASSERT(man.GetTop() == 3, "Incorrect usage");
        RenderSurfaceFormat format = (RenderSurfaceFormat)man.ToInteger(1);
        U32 w = (U32)man.ToInteger(2);
        U32 h = (U32)man.ToInteger(3);
        man.PushUnsignedInteger(RenderTexture::CalculateSlicePitch(format, w, h));
        return 1;
    }
    
};

U32 RenderTexture::CalculatePitch(RenderSurfaceFormat format, U32 mipWidth, U32) {
    U32 rowPitch = 0;
    U32 blockWidth = 0;
    
    switch (format) {
        case RenderSurfaceFormat::RGBA8:
        case RenderSurfaceFormat::BGRA8:
        {
            rowPitch = (4 * mipWidth + 7) / 8;
            break;
        }
        case RenderSurfaceFormat::DXT1:
        {
            blockWidth = MAX(1, (mipWidth + 3) / 4);
            rowPitch = 8 * blockWidth;
            break;
        }
        case RenderSurfaceFormat::DXT3:
        case RenderSurfaceFormat::DXT5:
        {
            blockWidth = MAX(1, (mipWidth + 3) / 4);
            rowPitch = 16 * blockWidth;
            break;
        }
        default:
            TTE_ASSERT(false, "Unknown or invalid format");
            return 0;
    }
    
    return rowPitch;
}

U32 RenderTexture::CalculateSlicePitch(RenderSurfaceFormat format, U32 mipWidth, U32 mipHeight)
{
    U32 slicePitch = 0;
    
    switch (format) {
        case RenderSurfaceFormat::RGBA8:
        case RenderSurfaceFormat::BGRA8:
        {
            slicePitch = CalculatePitch(format, mipWidth, 0) * mipHeight;
            break;
        }
        case RenderSurfaceFormat::DXT1:
        case RenderSurfaceFormat::DXT3:
        case RenderSurfaceFormat::DXT5:
        {
            U32 blockHeight = MAX(1, (mipHeight + 3) / 4);
            slicePitch = RenderTexture::CalculatePitch(format, mipWidth, 0) * blockHeight;
            break;
        }
        default:
            TTE_ASSERT(false, "Unknown or invalid format");
            return 0;
    }
    
    return slicePitch;
}

void RenderTexture::RegisterScriptAPI(LuaFunctionCollection &Col)
{
    
    PUSH_FUNC(Col, "CommonTextureSetName", &TextureAPI::luaTextureSetName, "nil CommonTextureSetName(state, name)", "Sets the name of the common texture");
    PUSH_FUNC(Col, "CommonTextureSetFormat", &TextureAPI::luaTextureSetFormat, "nil CommonTextureSetFormat(state, texFormat)", "Sets the format of the texture data");
    PUSH_FUNC(Col, "CommonTextureSetDimensions", &TextureAPI::luaTextureSetDimensions,
              "nil CommonTextureSetDimensions(state, width, height, mipMapCount, depth, arraySize)", "Sets the texture dimensions");
    PUSH_FUNC(Col, "CommonTexturePushOrderedImage", &TextureAPI::luaTexturePushImage,
              "nil CommonTexturePushOrderedImage(state, width, height, rowPitch, slicePitch, binaryBuffer)",
              "Pushes an ordered image inside the texture. This must be done in an ordered fashion! See the DDS file format on how to push.");
    PUSH_FUNC(Col, "CommonTextureResolveRGBA", &TextureAPI::luaTextureResolve,
              "nil CommonTextureResolveRGBA(state, binaryBuffer, resolvableFormat, width, height)",
              "Resolve/decompress a texture format into a standard RGBA image such that it fits as a standard texture format."); // resolve image to RGBA
    PUSH_FUNC(Col, "CommonTextureCalculatePitch", &TextureAPI::luaTextureCalculatePitch,
              "int CommonTextureCalculatePitch(texFormat, width, height)", "Calculates the pitch of the texture given the texture format, in bytes.");
    PUSH_FUNC(Col, "CommonTextureCalculateSlicePitch", &TextureAPI::luaTextureCalculateSlicePitch,
              "int CommonTextureCalculateSlicePitch(texFormat, width, height)", "Calculates the slice pitch of the texture given the texture format, in bytes.");
    PUSH_GLOBAL_I(Col, "kCommonTextureResolvableFormatBGRX", 0, "BGR'X' resolvable texture format. 'X' is unused and set to opaque.");
    
}

void RenderTexture::FinaliseNormalisationAsync()
{
    // Any final checks?
}
