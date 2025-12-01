#include <Core/Config.hpp>
#include <Meta/Meta.hpp>
#include <Core/Callbacks.hpp>

#include <sstream>
#include <stdio.h>
#include <fstream>
#include <filesystem>
#include <vector>

String MakeTypeName(String fullName)
{
    if(Meta::GetInternalState().GameIndex == -1 || Meta::GetInternalState().GetActiveGame().Caps[GameCapability::RAW_TYPE_NAMES])
    {
        StringReplace(fullName, "class ", "");
        StringReplace(fullName, "struct ", "");
        StringReplace(fullName, "enum ", "");
        StringReplace(fullName, "std::", "");
        StringReplace(fullName, " ", "");
    }
    return fullName;
}

static std::stringstream _ConsoleCache{};
static Callbacks _ConsoleCallbacks{};
static Bool _ConsoleCaching = false;
static char _ConsoleLogTemp[2048]{};
static std::mutex _ConsoleGuard{};

void ToggleLoggerCache(Bool bOnOff)
{
    _ConsoleGuard.lock();
    _ConsoleCaching = bOnOff;
    _ConsoleGuard.unlock();
}

void DetachLoggerCallback(Symbol tag)
{
    _ConsoleGuard.lock();
    _ConsoleCallbacks.RemoveCallbacks(tag);
    _ConsoleGuard.unlock();
}

void AttachLoggerCallback(Ptr<FunctionBase> pCallback)
{
    _ConsoleGuard.lock();
    _ConsoleCallbacks.PushCallback(std::move(pCallback));
    _ConsoleGuard.unlock();
}

void DumpLoggerCache(CString fileStr)
{
	_ConsoleGuard.lock();

	std::filesystem::path p{ fileStr };
	p = std::filesystem::absolute(p);

	std::ofstream outFile(p);
	if (outFile.is_open())
	{
		outFile << _ConsoleCache.str();
		outFile.close();
	}

	_ConsoleGuard.unlock();
}

void LogConsole(CString Msg, ...)
{
	_ConsoleGuard.lock();
    {
	    va_list va{};
	    va_start(va, Msg);
	    int len = vsnprintf(_ConsoleLogTemp, 2048, Msg, va);
	    va_end(va);

	    // Check if we need a new line
        printf("%s", _ConsoleLogTemp);
	    if (len > 0 && _ConsoleLogTemp[len - 1] != '\n')
		    printf("\n");

        if(_ConsoleCaching)
        {
			_ConsoleCache << _ConsoleLogTemp;
			if (_ConsoleLogTemp[len - 1] != '\n')
				_ConsoleCache << '\n';
        }
        
        CString pArgument = _ConsoleLogTemp;
        _ConsoleCallbacks.CallErased(&pArgument, 0, 0, 0, 0, 0, 0, 0);
        
    }
	_ConsoleGuard.unlock();
}

String StringFromInteger(I64 original_value,U32 radix, Bool is_negative)
{
    U8 Buf[64]{};
    U8* out = Buf;
    if (radix < 2 || radix > 36)
    {
        TTE_LOG("Invalid radix");
        return "";
    }
    
    uint64_t value = static_cast<uint64_t>(original_value);
    uint64_t written = 0;
    
    if (is_negative) {
        *out++ = '-';
        ++written;
        value = static_cast<uint64_t>(-original_value);
    }
    
    U8* digits_start = out;
    
    // Write digits in reverse order
    do {
        if (written >= 63)
        {
            TTE_LOG("Integer too large for buffer");
            return "";
        }
        
        uint64_t remainder = value % radix;
        value /= radix;
        *out++ = (remainder < 10) ? ('0' + remainder) : ('a' + remainder - 10);
        ++written;
        
    } while (value != 0);
    
    *out = '\0';
    
    // Reverse the digits (excluding minus sign if present)
    U8* left = digits_start;
    U8* right = out - 1;
    while (left < right)
    {
        std::swap(*left++, *right--);
    }
    
    return String((CString)Buf);
}

static std::vector<CommonClassInfo> CommonClassInfoDescs{};

void CommonClassInfo::Shutdown()
{
    CommonClassInfoDescs.clear();
}

void CommonClassInfo::RegisterConstants(LuaFunctionCollection& Col)
{
    for(const auto& desc: CommonClassInfoDescs)
    {
        PUSH_GLOBAL_I(Col, desc.ConstantName, (I32)desc.Class, "Common class types");
    }
}

void CommonClassInfo::RegisterCommonClass(CommonClassInfo info)
{
    CommonClassInfoDescs.push_back(std::move(info));
}

CommonClassInfo CommonClassInfo::GetCommonClassInfoByExtension(const String& extension)
{
    for (const auto& info : CommonClassInfoDescs)
    {
        if (CompareCaseInsensitive(info.Extension, extension))
            return info;
    }
    return {};
}

CommonClassInfo CommonClassInfo::GetCommonClassInfo(CommonClass cls)
{
    for(const auto& info: CommonClassInfoDescs)
    {
        if(info.Class == cls)
            return info;
    }
    return {};
}
