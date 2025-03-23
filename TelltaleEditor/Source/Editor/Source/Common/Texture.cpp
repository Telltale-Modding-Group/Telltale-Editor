#include <Common/Texture.hpp>
#include <TelltaleEditor.hpp>

namespace TextureAPI
{
    
    static inline TextureNormalisationTask* Task(LuaManager& man)
    {
        return (TextureNormalisationTask*)man.ToPointer(1);
    }
    
    static U32 luaTextureSetName(LuaManager& man)
    {
        TTE_ASSERT(man.GetTop() == 2, "Incorrect usage");
        
        TextureNormalisationTask* task = Task(man);
        String name = man.ToString(2);
        
        task->Local.Name = std::move(name);
        
        return 0;
    }
    
    static U32 luaTextureSetFormat(LuaManager& man)
    {
        TTE_ASSERT(man.GetTop() == 2, "Incorrect usage");
        
        TextureNormalisationTask* task = Task(man);
        task->Local.Format = (RenderSurfaceFormat)man.ToInteger(2);
        
        return 0;
    }
    
    static U32 luaTextureSetDimensions(LuaManager& man)
    {
        TTE_ASSERT(man.GetTop() == 4, "Incorrect usage");
        
        TextureNormalisationTask* task = Task(man);
        task->Local.Width = (U32)man.ToInteger(2);
        task->Local.Height = (U32)man.ToInteger(3);
        task->Local.Depth = (U32)man.ToInteger(4);
        
        return 0;
    }
    
    static U32 luaTexturePushImage(LuaManager& man)
    {
        TTE_ASSERT(man.GetTop() == 7, "Incorrect usage");
        
        TextureNormalisationTask* task = Task(man);
        
        RenderTexture::Image img{};
        img.Width = (U32)man.ToInteger(2);
        img.Height = (U32)man.ToInteger(3);
        img.RowPitch = (U32)man.ToInteger(4);
        img.SlicePitch = (U32)man.ToInteger(5);
        img.Format = (RenderSurfaceFormat)man.ToInteger(6);
        
        Meta::ClassInstance buf = Meta::AcquireScriptInstance(man, 7);
        TTE_ASSERT(buf, "Texture data buffer invalid");
        
        Meta::BinaryBuffer* pBuffer = (Meta::BinaryBuffer*)buf._GetInternal();
        img.Data = *pBuffer;
        
        task->Local.Images.push_back(std::move(img));
        
        return 0;
    }
    
    // resolve(state, buffer, resolvableFormat, width, height) with the width and height of this buffer. resolve to rgba.
    static U32 luaTextureResolve(LuaManager& man)
    {
        TTE_ASSERT(man.GetTop() == 5, "Incorrect usage");
        
        TextureNormalisationTask* task = Task(man);
        
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
    
}

void RenderTexture::RegisterScriptAPI(LuaFunctionCollection &Col)
{
    
    PUSH_FUNC(Col, "CommonTextureSetName", &TextureAPI::luaTextureSetName);
    PUSH_FUNC(Col, "CommonTextureSetFormat", &TextureAPI::luaTextureSetFormat);
    PUSH_FUNC(Col, "CommonTextureSetDimensions", &TextureAPI::luaTextureSetDimensions);
    PUSH_FUNC(Col, "CommonTexturePushOrderedImage", &TextureAPI::luaTexturePushImage);
    PUSH_FUNC(Col, "CommonTextureResolveRGBA", &TextureAPI::luaTextureResolve); // resolve image to RGBA
    
    PUSH_GLOBAL_I(Col, "kCommonTextureResolvableFormatBGRX", 0);
    PUSH_GLOBAL_I(Col, "kCommonTextureDDSHeaderSize", 0x7C);
    
}

void RenderTexture::FinishNormalisationAsync()
{
    // Any final checks?
}
