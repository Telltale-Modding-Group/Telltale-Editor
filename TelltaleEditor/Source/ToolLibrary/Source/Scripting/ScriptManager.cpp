#include <Scripting/ScriptManager.hpp>
#include <Scheduler/JobScheduler.hpp>
#include <Meta/Meta.hpp>
#include <Core/Context.hpp>

#include <cctype>

LuaManager& GetThreadLVM()
{
    if(IsCallingFromMain())
    {
        ToolContext* pContext = GetToolContext();
        TTE_ASSERT(pContext, "No global tool context!");
        return pContext->GetLibraryLVM();
    }
    if(JobScheduler::IsRunningFromWorker())
    {
        return JobScheduler::GetCurrentThread().L;
    }
    TTE_ASSERT(false, "GetThreadLVM(): No lua environment was found! Being called from an invalid thread.");
    static LuaManager _{};
    return _;
}

namespace ScriptManager {
    
    DataStreamRef DecryptScript(DataStreamRef& src)
    {
        if(src->GetSize() < 4)
            return src;
        U8* Buffer = TTE_ALLOC(src->GetSize(), MEMORY_TAG_SCRIPTING);
        TTE_ATTACH_DBG_STR(Buffer, "Decrypted Lua Script");
        TTE_ASSERT(src->Read(Buffer, src->GetSize()), "Could not read in bytes while decrypting lua script");
        
        Bool bCompiled = src->GetSize() > 4 && Buffer[0] == 0x1B && Buffer[1] == 'L' && Buffer[2] == 'u' && Buffer[3] == 'a';
        Bool bLEn = src->GetSize() > 4 && Buffer[0] == 0x1B && Buffer[1] == 'L' && Buffer[2] == 'E' && Buffer[3] == 'n';
        Bool bLEo = src->GetSize() > 4 && Buffer[0] == 0x1B && Buffer[1] == 'L' && Buffer[2] == 'E' && Buffer[3] == 'o';
        
        if (bCompiled)
        {
            // COMPILED. DONE.
            return DataStreamManager::GetInstance()->CreateBufferStream({}, src->GetSize(), Buffer, true);
        }
        
        if(bLEn)
        {
            Blowfish::GetInstance()->Decrypt(Buffer + 4, (U32)src->GetSize() - 4);
            memcpy(Buffer, "\x1BLua", 4);
            return DataStreamManager::GetInstance()->CreateBufferStream({}, src->GetSize(), Buffer, true);
        }
        
        if(bLEo) // no compilation magic, because its text
        {
            Blowfish::GetInstance()->Decrypt(Buffer + 4, (U32)src->GetSize() - 4);
            memmove(Buffer, Buffer + 4, src->GetSize() - 4);
            return DataStreamManager::GetInstance()->CreateBufferStream({}, src->GetSize() - 4, Buffer, true);
        }
        
        Bool bText = true;
        for (U64 i = 0; i < src->GetSize(); ++i)
        {
            if(!std::isprint(Buffer[i]))
            {
                bText = false;
                break;
            }
        }
        
        if(bText)
        {
            // Return the script its already OK.
            return DataStreamManager::GetInstance()->CreateBufferStream({}, src->GetSize(), Buffer, true);
        }
        // Assume a whole encrypted source script
        Blowfish::GetInstance()->Decrypt(Buffer, (U32)src->GetSize());
        return DataStreamManager::GetInstance()->CreateBufferStream({}, src->GetSize(), Buffer, true);
    }
    
    
    DataStreamRef EncryptScript(DataStreamRef& src, Bool bLenc)
    {
        if(src->GetSize() < 4)
            return src;
        U8* Buffer = TTE_ALLOC(src->GetSize() + 4, MEMORY_TAG_SCRIPTING);
        TTE_ATTACH_DBG_STR(Buffer, "Encrypted Lua Script");
        TTE_ASSERT(src->Read(Buffer, src->GetSize()), "Could not read in bytes while decrypting lua script");
        
        if(bLenc)
        {
            // simple full encrypt
            Blowfish::GetInstance()->Encrypt(Buffer, (U32)src->GetSize());
            return DataStreamManager::GetInstance()->CreateBufferStream({}, src->GetSize(), Buffer, true);
        }
        
        Bool bCompiled = src->GetSize() > 4 && Buffer[0] == 0x1B && Buffer[1] == 'L' && Buffer[2] == 'u' && Buffer[3] == 'a';
        
        if(bCompiled)
        {
            Blowfish::GetInstance()->Encrypt(Buffer + 4, (U32)src->GetSize() - 4);
            memcpy(Buffer, "\x1BLEn", 4);
            return DataStreamManager::GetInstance()->CreateBufferStream({}, src->GetSize(), Buffer, true);
        }
        
        // Raw text script
        Blowfish::GetInstance()->Encrypt(Buffer, (U32)src->GetSize());
        memmove(Buffer + 4, Buffer, src->GetSize());
        memcpy(Buffer, "\x1BLEo", 4);
        return DataStreamManager::GetInstance()->CreateBufferStream({}, src->GetSize() + 4, Buffer, true);
    }
    
    void RegisterCollection(LuaManager& man, const LuaFunctionCollection& collection)
    {
        
        if(collection.Name.length() > 0)
            man.PushTable(); // new table
        else
            man.PushEnv(); // no container name, pushing everything to _ENV/_G
        
        for(auto& regObj : collection.Functions){
            
            // Register each function
            man.PushLString(regObj.Name);
            man.PushFn(regObj.Function);
            man.SetTable(-3, true);
            
        }
        
        for(auto& st: collection.StringGlobals)
        {
            man.PushLString(st.first.c_str());
            man.PushLString(st.second.c_str());
            man.SetTable(-3, true);
        }
        
        for(auto& st: collection.IntegerGlobals)
        {
            man.PushLString(st.first.c_str());
            man.PushUnsignedInteger(st.second);
            man.SetTable(-3, true);
        }
        
        // set global
        if(collection.Name.length() > 0)
            SetGlobal(man, collection.Name, true);
        
        man.Pop(1); // pop globals/new table
        
        for(auto fn: collection.GenerateMetaTables)
            fn(man);
        
    }
    
    Symbol ToSymbol(LuaManager& man, I32 i)
    {
        String v = man.ToString(i);
        Symbol sym = SymbolFromHexString(v, true);
        return sym.GetCRC64() == 0 ? Symbol(v) : sym;
    }
    
    void PushSymbol(LuaManager& man, Symbol value)
    {
        String val = SymbolTable::Find(value);
        man.PushLString(val.length() == 0 ? SymbolToHexString(value) : val);
    }
    
}
