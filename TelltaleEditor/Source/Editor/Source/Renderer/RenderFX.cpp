#include <Renderer/RenderFX.hpp>
#include <Renderer/RenderContext.hpp>

#include <filesystem>
#include <algorithm>
#include <climits>
#include <fstream>
#include <unordered_map>
#include <iostream>
#include <regex>
#include <stack>

#ifdef PLATFORM_WINDOWS
#include <Windows.h>
#include <d3dcompiler.h>
#else
//#error "Imp" // impl for mac and linux next
#endif

static void _Translator_HLSL(String& out, U32 index, ShaderParameterTypeClass clz)
{
    if (clz == ShaderParameterTypeClass::GENERIC_BUFFER || clz == ShaderParameterTypeClass::INDEX_BUFFER) // structured buffer + index buffer (meaning texture)
        out = "register(t" + std::to_string(index) + ")";
    else if (clz == ShaderParameterTypeClass::UNIFORM_BUFFER)
        out = "register(b" + std::to_string(index) + ")";
    else if (clz == ShaderParameterTypeClass::SAMPLER)
        out = "register(s" + std::to_string(index) + ")";
    else
        out = "TEXCOORD" + std::to_string(index);
}

static void _Translator_MetalSL(String& out, U32 index, ShaderParameterTypeClass clz)
{
    if (clz == ShaderParameterTypeClass::GENERIC_BUFFER || clz == ShaderParameterTypeClass::UNIFORM_BUFFER)
        out = "[[buffer(" + std::to_string(index) + ")]]";
    else if (clz == ShaderParameterTypeClass::SAMPLER)
        out = "[[sampler(" + std::to_string(index) + ")]]";
    else if (clz == ShaderParameterTypeClass::INDEX_BUFFER) // texture
        out = "[[texture(" + std::to_string(index) + ")]]";
    else
        out = "[[attribute(" + std::to_string(index) + ")]]";
}

// static void _Translator_GLSL(String& out, U32 index, ShaderParameterTypeClass clz)

const RenderEffectDesc& RenderEffectCache::GetEffectDesc(RenderEffect effect)
{
    const RenderEffectDesc* pDesc = RenderEffectDescriptors;
    for(;; pDesc++)
    {
        if(pDesc->Effect == effect || pDesc->Effect == RenderEffect::COUNT)
            return *pDesc;
    }
    return RenderEffectDescriptors[0]; // will never happen
}

const RenderEffectFeatureDesc& RenderEffectCache::GetEffectFeatureDesc(RenderEffectFeature effectFeat)
{
    const RenderEffectFeatureDesc* pDesc = RenderEffectFeatureDescriptors;
    for (;; pDesc++)
    {
        if (pDesc->Feature == effectFeat || pDesc->Feature == RenderEffectFeature::COUNT)
            return *pDesc;
    }
    return RenderEffectFeatureDescriptors[0]; // will never happen
}

void RenderEffectCache::Initialise()
{
    if (!_Flags.Test(RenderEffectCacheFlag::INIT))
    {
        _Flags.Add(RenderEffectCacheFlag::INIT);
        using namespace std::filesystem;
        _ProgramsResourceLocation = "<RenderFX>/";
        if (!_Registry->ResourceLocationExists(_ProgramsResourceLocation))
        {
            path devPath = absolute(path("../../Dev/FX/")); // try use dev directories from default cmake development structure
            if (is_directory(devPath))
            {
                _Registry->MountSystem(_ProgramsResourceLocation, devPath.string(), true);
            }
            else
            {
                // user release shader pack gen in bigger releases, else we can default to other folder in el futuro
                TTE_ASSERT(false, "The render FX shader resource location (%s) could not be found.", _ProgramsResourceLocation.c_str());
            }
        }
        memset(_Program, 0, sizeof(RenderEffectProgram) * RENDER_FX_MAX_RUNTIME_PROGRAMS);
        RenderDeviceType dev = _Context->GetDeviceType();
        if (dev == RenderDeviceType::D3D12)
        {
            _PsuedoMacroTranslate = &_Translator_HLSL;
            _FXFileNamePrefix = "DX12_";
        }
        else if (dev == RenderDeviceType::METAL)
        {
            _PsuedoMacroTranslate = &_Translator_MetalSL;
            _FXFileNamePrefix = "MTL_";
        }
        else if (dev == RenderDeviceType::VULKAN)
        {
            TTE_ASSERT(false, "Impl GLSL translator");
            _FXFileNamePrefix = "VK_";
        }
        else
        {
            TTE_LOG("Error while initialising render effect cache: render device is unknown");
        }
    }
}

RenderEffectCache::RenderEffectCache(Ptr<ResourceRegistry> reg, RenderContext* pContext) : _Registry(reg), _ProgramCount(0), _Context(pContext)
{
}

void RenderEffectCache::Release()
{
    // Destruct does everthing here anyway (who cares about memory clear)
    ToggleHotReloading(false);
    _ProgramCount = 0;
    _StrongShaderRefs.clear();
}

void RenderEffectCache::ToggleHotReloading(Bool bOnOff)
{
    _Flags.Set(RenderEffectCacheFlag::HOT_RELOADING, bOnOff);
    if(!bOnOff)
    {
        // Clear
        _FXHash.clear();
    }
}

RenderEffectRef RenderEffectCache::GetEffectRef(RenderEffect effect, RenderEffectFeaturesBitSet features)
{
    RenderEffectRef ref = RenderEffectRef{_ComputeEffectHash({effect, features})};
    U32 existingRef = _ResolveExistingRef(ref);
    if (existingRef == UINT32_MAX)
    {
        // Add ref
        _InsertProgramRef(RenderEffectParams{effect, features}, 0);
    }
    return ref;
}

Bool RenderEffectCache::ResolveEffectRef(RenderEffectRef ref, Ptr<RenderShader>& programVert, Ptr<RenderShader>& programPixel)
{
    TTE_ASSERT(IsCallingFromMain(), "Must be called from the main thread.");
    U32 existingRef = _ResolveExistingRef(ref);
    if(existingRef != UINT32_MAX)
    {
        RenderEffectProgram& program = _Program[existingRef];
        if(!program.ProgramFlags.Test(RenderEffectProgramFlag::LOADED))
        {
            // Compile and set shaders
            FXHashIterator srcFileIterator = _ResolveFX(program.Params.Effect);
            if(srcFileIterator == _FXHash.end())
                return false;
            program.FXFileHash = srcFileIterator->first;
            Bool bResult = _CreateProgram(srcFileIterator->second, program.Params, programVert, programPixel);
            if(!bResult)
                return false;
            _InsertInternalShader(programVert);
            _InsertInternalShader(programPixel);
            program.Vert = programVert.get();
            program.Pixel = programPixel.get();
            program.ProgramFlags.Add(RenderEffectProgramFlag::LOADED);
        }
        programVert =  _ResolveInternalShader(program.Vert);
        programPixel = _ResolveInternalShader(program.Pixel);
        return true;
    }
    return false; // not found
}

String RenderEffectCache::LocateEffectName(RenderEffectRef ref)
{
    U32 index = _ResolveExistingRef(ref);
    if(index == UINT32_MAX)
        return "";
    return BuildName(_Program[index].Params, "");
}

void RenderEffectCache::ReloadPipelines(const std::vector<Ptr<RenderPipelineState>>& pipelines)
{
    TTE_ASSERT(IsCallingFromMain(), "Must be called from the main thread.");
    // Do hot reload of any file hash changes
    if (_ProgramCount == 0)
        return;
    if(_Flags.Test(RenderEffectCacheFlag::HOT_RELOADING))
    {
        // 1st it, gather deltas
        struct Delta{String Src = "";  U32 SwapOK = 0; U64 Hash = 0;};
        std::vector<String> erroredFXVars{};
        std::unordered_map<String, Delta> changedFXNames{}; // file name => new source. (if empty, no change)
        for(U32 i = 0; i < _ProgramCount; i++)
        {
            String fileName = GetEffectDesc(_Program[i].Params.Effect).FXFileName;
            if(_Program[i].ProgramFlags.Test(RenderEffectProgramFlag::LOADED) && changedFXNames.find(fileName) == changedFXNames.end())
            {
                String src = _ResolveFXSource(_Program[i].Params.Effect);
                if (src.length() == 0)
                    continue;
                U64 hash = CRC64((const U8*)src.c_str(), (U32)src.length(), 0);
                changedFXNames[fileName] = Delta{ hash != _Program[i].FXFileHash ? std::move(src) : "", 0 ,hash};
            }
        }
        // 2nd it. reloads
        for(U32 i = 0; i < _ProgramCount; i++)
        {
            RenderEffectProgram& pgm = _Program[i];
            auto deltaIt = changedFXNames.find(GetEffectDesc(pgm.Params.Effect).FXFileName);
            if(deltaIt != changedFXNames.end() && deltaIt->second.Src.length() > 0)
            {
                pgm.FXFileHash = deltaIt->second.Hash;
                Ptr<RenderShader> programVert{}, programPixel{};
                if(_CreateProgram(deltaIt->second.Src, pgm.Params, programVert, programPixel))
                {
                    deltaIt->second.SwapOK = 1;
                    _SwapInternalShader(TTE_PROXY_PTR(pgm.Vert, RenderShader), std::move(programVert));
                    _SwapInternalShader(TTE_PROXY_PTR(pgm.Pixel, RenderShader), std::move(programPixel));
                }
                else
                {
                    erroredFXVars.push_back(BuildName(pgm.Params, ""));
                }
            }
        }
        // 3rd it. reset pipelines
        for(auto& pPipeline: pipelines)
        {
            U32 index = _ResolveExistingRef(RenderEffectRef{pPipeline->EffectHash});
            if(index != UINT32_MAX)
            {
                RenderEffectProgram& pgm = _Program[index];
                auto deltaIt = changedFXNames.find(GetEffectDesc(pgm.Params.Effect).Name);
                if (deltaIt != changedFXNames.end() && deltaIt->second.SwapOK)
                {
                    SDL_ReleaseGPUGraphicsPipeline(_Context->_Device, pPipeline->_Internal._Handle);
                    pPipeline->Create(); // recreate it
                }
            }
        }
        if(erroredFXVars.size())
        {
            TTE_LOG("ERROR: When reloading render pipelines, the following render effect variants could not be created:");
            // error report
            for (const auto& err : erroredFXVars)
            {
                TTE_LOG("Variant %s", err.c_str());
            }
        }
    }
}

String RenderEffectCache::BuildName(RenderEffectParams params, String extension)
{
    String name = GetEffectDesc(params.Effect).Name;
    for(auto it = params.Features.begin(); it != params.Features.end(); it++)
    {
        name += "_";
        String n = GetEffectFeatureDesc(*it).Name;
        std::transform(n.begin(), n.end(), n.begin(),
        [](unsigned char c)
        {
            return std::toupper(c);
        });
        name += n;
    }
    return name + extension;
}

void RenderEffectCache::_SwapInternalShader(Ptr<RenderShader> pShader, Ptr<RenderShader> pNewShader)
{
    for (U32 i = 0; i < _ProgramCount; i++)
    {
        if(_Program[i].Vert == pShader.get())
            _Program[i].Vert = pNewShader.get();
        else if (_Program[i].Pixel == pShader.get())
            _Program[i].Pixel = pNewShader.get();
    }
    auto it = _StrongShaderRefs.find(pShader);
    if(it != _StrongShaderRefs.end())
    {
        _StrongShaderRefs.erase(pShader);
        if(pNewShader)
            _StrongShaderRefs.insert(std::move(pNewShader));
    }
}

RenderEffectCache::FXHashIterator RenderEffectCache::_ResolveFX(RenderEffect effect)
{
    String src = _ResolveFXSource(effect);
    if(src.length() == 0)
        return _FXHash.end();
    U64 hash = CRC64((const U8*)src.c_str(), (U32)src.length(), 0);
    return _FXHash.insert_or_assign(hash, std::move(src)).first;
}

String RenderEffectCache::_ResolveFXSource(RenderEffect effect)
{
    Memory::FastBufferAllocator allocator{};
    String fxFilename = _FXFileNamePrefix;
    fxFilename += GetEffectDesc(effect).FXFileName;
    DataStreamRef fxStream = _Registry->FindResourceFrom(_ProgramsResourceLocation, fxFilename);
    if(!fxStream)
    {
        TTE_LOG("Could not locate or open FX shader source file '%s' in '%s'!", fxFilename.c_str(), _ProgramsResourceLocation.c_str());
        return "";
    }
    U32 size = (U32)fxStream->GetSize();
    U8* Temp = allocator.Alloc(size + 1, 1);
    if(!Temp || !fxStream->Read(Temp, size))
    {
        TTE_LOG("Could not load from data effect shader %s!", fxFilename.c_str());
        return "";
    }
    Temp[size] = 0;
    String fx = (CString)Temp;
    return fx;
}

U32 RenderEffectCache::_InsertProgramRef(RenderEffectParams params, U64 srcHash)
{
    RenderEffectProgram proxy{};
    U64 effectHash = _ComputeEffectHash(params);
    proxy.EffectHash = effectHash;

    RenderEffectProgram* pProgram = std::upper_bound(_Program, _Program + _ProgramCount, proxy);
    U32 index = (U32)(pProgram - _Program);

    if (index >= RENDER_FX_MAX_RUNTIME_PROGRAMS)
    {
        TTE_ASSERT(false, "Too many loaded effect programs. Please increase maximum runtime programs (permutations of variants)");
        return UINT32_MAX;
    }
    else if(_Program[index].EffectHash != effectHash)
    {
        // shift
        for (U32 i = _ProgramCount; i > index; --i)
        {
            _Program[i] = _Program[i - 1];
        }

        _Program[index].Params = params;
        _Program[index].EffectHash = effectHash;
        _Program[index].ProgramFlags = 0;
        _Program[index].Pixel = _Program[index].Vert = nullptr;

        _ProgramCount++;
    }
    _Program[index].FXFileHash = srcHash;
    return index;
}

U64 RenderEffectCache::_ComputeEffectHash(RenderEffectParams params)
{
    U32 effectU32 = (U32)params.Effect ^ 0x298FEED3u;
    U64 hash = CRC64(reinterpret_cast<const U8*>(&effectU32), 4);
    return CRC64(reinterpret_cast<const U8*>(params.Features.Words()), RenderEffectFeaturesBitSet::NumWords << 2, hash);
}

void RenderEffectCache::_InsertInternalShader(Ptr<RenderShader> pShader)
{
    _StrongShaderRefs.insert(std::move(pShader));
}

Ptr<RenderShader> RenderEffectCache::_ResolveInternalShader(RenderShader* pShader)
{
    Ptr<RenderShader> proxyNoDelete = TTE_PROXY_PTR(pShader, RenderShader);
    auto it = _StrongShaderRefs.find(proxyNoDelete);
    return it == _StrongShaderRefs.end() ? nullptr : *it;
}

U32 RenderEffectCache::_ResolveExistingRef(RenderEffectRef ref)
{
    RenderEffectProgram* start = _Program;
    RenderEffectProgram* end = _Program + _ProgramCount;

    U32 left = 0;
    U32 right = _ProgramCount;

    while (left < right)
    {
        U32 mid = left + ((right - left) >> 1);
        if (_Program[mid].EffectHash < ref.EffectHash)
        {
            left = mid + 1;
        }
        else
        {
            right = mid;
        }
    }

    if (left == _ProgramCount || _Program[left].EffectHash != ref.EffectHash)
    {
        return UINT32_MAX;
    }

    return left;
}

// CREATE SDL SHADER
Ptr<RenderShader> RenderEffectCache::_CreateShader(const ShaderParameterTypes& parameters, RenderShaderType shtype,
                                                   const String& shaderName, U8* pParameterSlots,
                                                   const VertexAttributesBitset& vertexStateAttribs,
                                                   const void* pShaderBinary,
                                                   U32 binarySize, SDL_GPUShaderFormat expectedFormat)
{
    if((SDL_GetGPUShaderFormats(_Context->_Device) & expectedFormat) == 0)
    {
        TTE_LOG("Could not create shader %s for %s device: expected shader format is wrong. Ensure the render device is the correct type (eg vulkan, D3D, Metal) for this platform.",
                shaderName.c_str(), SDL_GetGPUDeviceDriver(_Context->_Device));
        return nullptr;
    }
    Ptr<RenderShader> pShader = TTE_NEW_PTR(RenderShader, MEMORY_TAG_RENDERER);
    SDL_GPUShaderCreateInfo info{};
    info.entrypoint = shtype == RenderShaderType::VERTEX ? RENDER_FX_ENTRY_POINT_VERT : RENDER_FX_ENTRY_POINT_FRAG;
    info.code = (const Uint8*)pShaderBinary;
    info.code_size = (size_t)binarySize;
    info.format = expectedFormat;
    info.stage = shtype == RenderShaderType::VERTEX ? SDL_GPU_SHADERSTAGE_VERTEX : SDL_GPU_SHADERSTAGE_FRAGMENT;
    info.num_samplers = info.num_storage_buffers = info.num_storage_textures = info.num_uniform_buffers = 0;
    for(auto it = parameters.begin(); it != parameters.end(); it++)
    {
        if(ShaderParametersInfo[(U32)*it].Class == ShaderParameterTypeClass::GENERIC_BUFFER)
            info.num_storage_buffers++;
        else if (ShaderParametersInfo[(U32)*it].Class == ShaderParameterTypeClass::SAMPLER)
            info.num_samplers++;
        else if (ShaderParametersInfo[(U32)*it].Class == ShaderParameterTypeClass::UNIFORM_BUFFER)
            info.num_uniform_buffers++;
        // maybe storage textures (ie render targets etc) in future
    }
    info.props = SDL_CreateProperties();
    SDL_SetStringProperty(info.props, SDL_PROP_GPU_SHADER_CREATE_NAME_STRING, shaderName.c_str());
    SDL_GPUShader* gpuShader = SDL_CreateGPUShader(_Context->_Device, &info);
    SDL_DestroyProperties(info.props);
    if(!gpuShader)
    {
        TTE_LOG("Could not create shader %s: %s", shaderName.c_str(), SDL_GetError());
        DebugBreakpoint();
        return nullptr;
    }
    pShader->Context = _Context;
    pShader->Handle = gpuShader;
    pShader->Attribs = vertexStateAttribs;
    memcpy(pShader->ParameterSlots, pParameterSlots, (U8)ShaderParameterType::PARAMETER_COUNT);
    return pShader;
}

String RenderEffectCache::_ResolveFXBindings(const String& source, const String& macroName, ShaderParameterTypeClass clz,
                                             U32 (*resolver)(const String&, U32 line, const FXResolveState& state), const FXResolveState& resolverState)
{
    std::string patternStr = macroName + R"(\((\w+)\))";
    std::regex pattern(patternStr);

    std::string result;
    std::sregex_iterator begin(source.begin(), source.end(), pattern);
    std::sregex_iterator end;

    size_t lastPos = 0;

    for (auto it = begin; it != end; ++it)
    {
        std::smatch match = *it;
        size_t matchStart = match.position();
        size_t matchEnd = matchStart + match.length();
        std::string argument = match[1].str();
        U32 line = static_cast<U32>(std::count(source.begin(), source.begin() + matchStart, '\n')) + 1;
        U32 index = resolver(argument, line, resolverState);
        if(index == UINT32_MAX)
            return "";

        result.append(source, lastPos, matchStart - lastPos);

        String repl{};
        _PsuedoMacroTranslate(repl, index, clz);
        result += repl;

        lastPos = matchEnd;
    }

    result.append(source, lastPos, std::string::npos);

    return result;
}

U32 RenderEffectCache::FXResolver_VertexAttribute(const String& arg, U32 line, const RenderEffectCache::FXResolveState& state)
{
    for(U32 i = 0; i < (U32)RenderAttributeType::COUNT; i++)
    {
        const AttribInfo& attrib = RenderContext::GetVertexAttributeDesc((RenderAttributeType)i);
        if(CompareCaseInsensitive(attrib.FXName, arg))
        {
            return i;
        }
    }
    TTE_LOG("At %s:%u: at TTE_VERTEX_ATTRIB specifier: the argument %s is not a valid vertex attribute!", state.FXName.c_str(), line, arg.c_str());
    return UINT32_MAX;
}

U32 RenderEffectCache::FXResolver_Uniform(const String& arg, U32 line, const RenderEffectCache::FXResolveState& state)
{
    for(U32 i = (U32)ShaderParameterType::PARAMETER_FIRST_UNIFORM; i <= (U32)ShaderParameterType::PARAMETER_LAST_UNIFORM; i++)
    {
        if(CompareCaseInsensitive(arg, ShaderParametersInfo[i].Name))
        {
            if (!state.EffectParameters[(ShaderParameterType)i])
            {
                TTE_LOG("At %s[%s]: at TTE_UNIFORM_BUFFER specifier: the parameter %s is not an input of this variant", state.FXName.c_str(), state.BuiltName.c_str(), arg.c_str());
                return UINT32_MAX;
            }
            if (state.ParameterSlotsOut[i] != 0xFF)
            {
                TTE_LOG("At %s[%s]: at TTE_UNIFORM_BUFFER specifier: the parameter %s has already been specified", state.FXName.c_str(), state.BuiltName.c_str(), arg.c_str());
                return UINT32_MAX;
            }
            U32 slot = state.UniformBuffersIndex++;
            state.ParameterSlotsOut[i] = slot;
            return slot;
        }
    }
    TTE_LOG("At %s: at TTE_UNIFORM_BUFFER specifier: the argument %s is not a valid uniform buffer!", state.FXName.c_str(), arg.c_str());
    return UINT32_MAX;
}

U32 RenderEffectCache::FXResolver_Generic(const String& arg, U32 line, const RenderEffectCache::FXResolveState& state)
{
    for (U32 i = (U32)ShaderParameterType::PARAMETER_FIRST_GENERIC; i <= (U32)ShaderParameterType::PARAMETER_LAST_GENERIC; i++)
    {
        if (CompareCaseInsensitive(arg, ShaderParametersInfo[i].Name))
        {
            if (!state.EffectParameters[(ShaderParameterType)i])
            {
                TTE_LOG("At %s[%s]: at TTE_GENERIC_BUFFER specifier: the parameter %s is not an input of this variant", state.FXName.c_str(), state.BuiltName.c_str(), arg.c_str());
                return UINT32_MAX;
            }
            if (state.ParameterSlotsOut[i] != 0xFF)
            {
                TTE_LOG("At %s[%s]: at TTE_GENERIC_BUFFER specifier: the parameter %s has already been specified", state.FXName.c_str(), state.BuiltName.c_str(), arg.c_str());
                return UINT32_MAX;
            }
            U32 slot = state.GenericBuffersIndex++;
            state.ParameterSlotsOut[i] = slot;
            return slot;
        }
    }
    TTE_LOG("At %s: at TTE_GENERIC_BUFFER specifier: the argument %s is not a valid generic buffer!", state.FXName.c_str(), arg.c_str());
    return UINT32_MAX;
}

U32 RenderEffectCache::FXResolver_TextureSampler(const String& arg, U32 line, const RenderEffectCache::FXResolveState& state)
{
    for (U32 i = (U32)ShaderParameterType::PARAMETER_FIRST_SAMPLER; i <= (U32)ShaderParameterType::PARAMETER_LAST_SAMPELR; i++)
    {
        if (CompareCaseInsensitive(arg, ShaderParametersInfo[i].Name))
        {
            if (!state.EffectParameters[(ShaderParameterType)i])
            {
                TTE_LOG("At %s[%s]: at TTE_TEXTURE/TTE_SAMPLER specifier: the parameter %s is not an input of this variant", state.FXName.c_str(),state.BuiltName.c_str(), arg.c_str());
                return UINT32_MAX;
            }
            if (state.ParameterSlotsOut[i] != 0xFF)
            {
                return state.ParameterSlotsOut[i]; // quiet ignore. most devices have sampler + texture sharing common indices
            }
            U32 slot = state.TextureSamplersIndex++;
            state.ParameterSlotsOut[i] = slot;
            return slot;
        }
    }
    TTE_LOG("At %s: at TTE_TEXTURE/TTE_SAMPLER specifier: the argument %s is not a valid sampler type!", state.FXName.c_str(), arg.c_str());
    return UINT32_MAX;
}

void RenderEffectCache::_FXSectionSkipWhitespace(SectionResolverState& state)
{
    while (state.Position < state.Expression.size() && std::isspace(state.Expression[state.Position]))
        ++state.Position;
}

Bool RenderEffectCache::_FXSectionParseIdentifier(SectionResolverState& state)
{
    _FXSectionSkipWhitespace(state);

    Bool neg = false;
    if (state.Position < state.Expression.size() && state.Expression[state.Position] == '!')
    {
        neg = true;
        ++state.Position;
    }

    _FXSectionSkipWhitespace(state);

    size_t start = state.Position;
    while (state.Position < state.Expression.size() &&
           (std::isalnum(state.Expression[state.Position]) || state.Expression[state.Position] == '_'))
    {
        ++state.Position;
    }

    if (start == state.Position)
    {
        state.Error = "expected identifier in expression";
        return false;
    }

    String token = state.Expression.substr(start, state.Position - start);
    Bool hasMacro = state.Macros.count(token) > 0;
    return neg ? !hasMacro : hasMacro;
}

Bool RenderEffectCache::_FXSectionParsePrim(SectionResolverState& state)
{
    _FXSectionSkipWhitespace(state);
    if(state.Error)
        return false;
    if (state.Position < state.Expression.size() && state.Expression[state.Position] == '(')
    {
        ++state.Position;
        Bool val = _FXSectionParseOR(state);
        if (state.Error)
            return false;
        _FXSectionSkipWhitespace(state);
        if (state.Error)
            return false;
        if (state.Position >= state.Expression.size() || state.Expression[state.Position] != ')')
        {
            state.Error = "expected ')'";
            return false;
        }
        ++state.Position;
        return val;
    }
    return _FXSectionParseIdentifier(state);
}

Bool RenderEffectCache::_FXSectionParseAND(SectionResolverState& state)
{
    Bool left = _FXSectionParsePrim(state);
    if (state.Error)
        return false;
    while (true)
    {
        _FXSectionSkipWhitespace(state);
        if (state.Error)
            return false;
        if (state.Expression.compare(state.Position, 2, "&&") == 0)
        {
            state.Position += 2;
            Bool right = _FXSectionParsePrim(state);
            if (state.Error)
                return false;
            left = left && right;
        }
        else
        {
            break;
        }
    }
    return left;
}

Bool RenderEffectCache::_FXSectionParseOR(SectionResolverState& state)
{
    Bool left = _FXSectionParseAND(state);
    if (state.Error)
        return false;
    while (true)
    {
        _FXSectionSkipWhitespace(state);
        if (state.Expression.compare(state.Position, 2, "||") == 0)
        {
            state.Position += 2;
            Bool right = _FXSectionParseAND(state);
            if (state.Error)
                return false;
            left = left || right;
        }
        else
        {
            break;
        }
    }
    return left;
}

Bool RenderEffectCache::_FXSectionEval(SectionResolverState& state)
{
    Bool val = _FXSectionParseOR(state);
    if (state.Error)
        return false;
    _FXSectionSkipWhitespace(state);
    if (state.Position != state.Expression.size())
    {
        state.Error = "unexpected trailing characters in expression";
        return false;
    }
    return val;
}

String RenderEffectCache::_ResolveFXSections(const String& src, const std::vector<std::string>& macrosV, const String& fx)
{
    std::unordered_set<std::string> macros(macrosV.begin(), macrosV.end());
    std::istringstream stream{src};
    String line{};
    std::ostringstream output{};

    struct SectionState
    {
        I32 LineNumber = 1;
        Bool Enabled = false;
        Bool ElseEncountered = false;
    };

    std::vector<SectionState> sectionStack{};

    std::regex beginRegex(R"(\bTTE_SECTION_BEGIN\s*\(\s*([^\)]+?)\s*\)\s*.*)");
    std::regex elseRegex(R"(\bTTE_SECTION_ELSE\s*\(\s*\)\s*.*)");
    std::regex endRegex(R"(\bTTE_SECTION_END\s*\(\s*\)\s*.*)");

    I32 lineNumber = 0;
    while (std::getline(stream, line))
    {
        lineNumber++;
        std::smatch match;

        if (std::regex_search(line, match, beginRegex))
        {
            String expr = match[1].str();
            Bool result = false;
            SectionResolverState state(expr, macros);
            result = _FXSectionEval(state);
            if(state.Error)
            {
                TTE_LOG("At %s:%d => %s", fx.c_str(), lineNumber,state.Error);
                return "";
            }
            if (!sectionStack.empty() && !sectionStack.back().Enabled)
                result = false;
            sectionStack.push_back({ lineNumber, result, false });
            continue;
        }
        else if (std::regex_search(line, elseRegex))
        {
            if (!sectionStack.empty())
            {
                auto& top = sectionStack.back();
                if (!top.ElseEncountered)
                {
                    top.Enabled = !top.Enabled;
                    top.ElseEncountered = true;
                    top.LineNumber = lineNumber;
                    if (sectionStack.size() > 1)
                    {
                        auto it = sectionStack.rbegin();
                        ++it; // parent
                        if (!it->Enabled)
                            top.Enabled = false;
                    }
                }
                else
                {
                    TTE_LOG("At %s:%d => TTE_SECTION_ELSE(): repeated else block", fx.c_str(), lineNumber);
                    return "";
                }
            }
            else
            {
                TTE_LOG("At %s:%d => TTE_SECTION_ELSE(): no previous section statement found in chain", fx.c_str(), lineNumber);
                return "";
            }
            continue;
        }
        else if (std::regex_search(line, match, endRegex))
        {
            if (!sectionStack.empty())
            {
                sectionStack.pop_back();
            }
            else
            {
                TTE_LOG("At %s:%d => TTE_SECTION_END(): no previous section statement found in chain", fx.c_str(), lineNumber);
                return "";
            }
            continue;
        }
        else
        {
            if (sectionStack.empty() || sectionStack.back().Enabled)
            {
                output << line << '\n';
            }
        }
    }
    if(!sectionStack.empty())
    {
        TTE_LOG("At %s:%d: unexpected EOF: section chain not terminated from line %d", fx.c_str(), lineNumber, sectionStack.back().LineNumber);
    }
    return output.str();
}

String RenderEffectCache::_ParseFX(const String& fxStr, U8* pParameterSlotsOut, const ShaderParameterTypes& effectParameters, 
                                   const String& fxName, RenderEffectParams effectParams,  const std::vector<String>& macros, const String& builtName)
{
    FXResolveState resolver{pParameterSlotsOut, effectParameters, fxName, effectParams, builtName};
    resolver.TextureSamplersIndex = 0;
    resolver.GenericBuffersIndex = 0;

    // SECTIONS
    String fxResolved = _ResolveFXSections(fxStr, macros, fxName);
    if(fxResolved.length() == 0)
        return "";

    // UNIFORMS
    fxResolved = _ResolveFXBindings(fxResolved, "TTE_UNIFORM_BUFFER", ShaderParameterTypeClass::UNIFORM_BUFFER, &FXResolver_Uniform, resolver);
    if(fxResolved.length() == 0)
        return "";
    for(auto it = effectParameters.begin(); it != effectParameters.end(); it++)
    {
        if(ShaderParametersInfo[(U8)*it].Class == ShaderParameterTypeClass::UNIFORM_BUFFER && pParameterSlotsOut[(U8)*it] == 0xFF)
        {
            TTE_LOG("FX Parser error for variant %s: uniform buffer %s not declared as input at any point but is required", fxName.c_str(), ShaderParametersInfo[(U8)*it].Name);
            return "";
        }
    }
    // GENERICS
    fxResolved = _ResolveFXBindings(fxResolved, "TTE_GENERIC_BUFFER", ShaderParameterTypeClass::GENERIC_BUFFER, &FXResolver_Generic, resolver);
    if (fxResolved.length() == 0)
        return "";
    for (auto it = effectParameters.begin(); it != effectParameters.end(); it++)
    {
        if (ShaderParametersInfo[(U8)*it].Class == ShaderParameterTypeClass::GENERIC_BUFFER && pParameterSlotsOut[(U8)*it] == 0xFF)
        {
            TTE_LOG("FX Parser error for variant %s: uniform buffer %s not declared as input at any point but is required", fxName.c_str(), ShaderParametersInfo[(U8)*it].Name);
            return "";
        }
    }
    // TEXTURE SAMPLERS
    fxResolved = _ResolveFXBindings(fxResolved, "TTE_TEXTURE", ShaderParameterTypeClass::INDEX_BUFFER, &FXResolver_TextureSampler, resolver);
    if (fxResolved.length() == 0)
        return "";
    fxResolved = _ResolveFXBindings(fxResolved, "TTE_SAMPLER", ShaderParameterTypeClass::SAMPLER, &FXResolver_TextureSampler, resolver);
    if (fxResolved.length() == 0)
        return "";
    for (auto it = effectParameters.begin(); it != effectParameters.end(); it++)
    {
        if (ShaderParametersInfo[(U8)*it].Class == ShaderParameterTypeClass::SAMPLER && pParameterSlotsOut[(U8)*it] == 0xFF)
        {
            TTE_LOG("FX Parser error for variant %s: texture/sampler %s not declared as input at any point but is required", fxName.c_str(), ShaderParametersInfo[(U8)*it].Name);
            return "";
        }
    }
    // attribs
    fxResolved = _ResolveFXBindings(fxResolved, "TTE_VERTEX_ATTRIB", ShaderParameterTypeClass::VERTEX_BUFFER, &FXResolver_VertexAttribute, resolver);
    if (fxResolved.length() == 0)
        return "";
    return fxResolved;
}

Bool RenderEffectCache::_CreateProgram(const String& fxSrc, RenderEffectParams params, Ptr<RenderShader>& vert, Ptr<RenderShader>& pixel)
{
    U8 ParamSlotsVert[(U8)ShaderParameterType::PARAMETER_COUNT], ParamSlotsPixel[(U8)ShaderParameterType::PARAMETER_COUNT];
    memset(ParamSlotsVert, 0xFF, (U8)ShaderParameterType::PARAMETER_COUNT);
    memset(ParamSlotsPixel, 0xFF, (U8)ShaderParameterType::PARAMETER_COUNT);
    Memory::FastBufferAllocator fastAllocator{};
    ShaderParameterTypes vertexParameters = GetEffectDesc(params.Effect).VertexRequiredParameters;
    for (auto it = params.Features.begin(); it != params.Features.end(); it++)
        vertexParameters.Import(GetEffectFeatureDesc(*it).VertexRequiredParameters);
    ShaderParameterTypes pixelParameters = GetEffectDesc(params.Effect).PixelRequiredParameters;
    for (auto it = params.Features.begin(); it != params.Features.end(); it++)
        pixelParameters.Import(GetEffectFeatureDesc(*it).PixelRequiredParameters);
    VertexAttributesBitset vertexAttribs = GetEffectDesc(params.Effect).RequiredAttribs;
    for (auto it = params.Features.begin(); it != params.Features.end(); it++)
        vertexAttribs.Import(GetEffectFeatureDesc(*it).RequiredAttribs);

    String builtName = BuildName(params, ".ttefxb");

    std::vector<String> macros{};
    macros.push_back(GetEffectDesc(params.Effect).Macro);
    for (auto it = params.Features.begin(); it != params.Features.end(); it++)
    {
        macros.push_back(GetEffectFeatureDesc(*it).Macro);
    }

    macros.push_back("TTE_VERTEX");
    String fileNameFX = _FXFileNamePrefix + GetEffectDesc(params.Effect).FXFileName;
    String fxResolvedVertex = _ParseFX(fxSrc, ParamSlotsVert, vertexParameters, fileNameFX, params, macros, builtName + ".vertex");
    if(fxResolvedVertex.empty())
        return false;
    macros.pop_back();
    macros.push_back("TTE_PIXEL");
    String fxResolvedPixel = _ParseFX(fxSrc, ParamSlotsPixel, pixelParameters, fileNameFX, params, macros, builtName + ".pixel");
    if (fxResolvedPixel.empty())
        return false;
    macros.pop_back();

    RenderDeviceType deviceType = _Context->GetDeviceType();
    if (deviceType== RenderDeviceType::D3D12)
    {
#ifdef PLATFORM_WINDOWS
        D3D_SHADER_MACRO* pMacro = (D3D_SHADER_MACRO*)fastAllocator.Alloc(sizeof(D3D_SHADER_MACRO) * (macros.size()+2), alignof(D3D_SHADER_MACRO));
        memset(pMacro, 0, sizeof(D3D_SHADER_MACRO) * (macros.size()+2));
        for(U32 i = 0; i < macros.size(); i++)
        {
            pMacro[i].Name = macros[i].c_str();
            pMacro[i].Definition = "1";
        }
        pMacro[macros.size()].Name = "TTE_VERTEX";
        pMacro[macros.size()].Definition = "1";
        ID3DBlob* vertCode = NULL, *errors = NULL;
        HRESULT ResultVert = D3DCompile(fxResolvedVertex.c_str(), (SIZE_T)fxResolvedVertex.length(), builtName.c_str(), pMacro, NULL, RENDER_FX_ENTRY_POINT_VERT, "vs_5_0", 0, 0, &vertCode, &errors);
        if(ResultVert != S_OK)
        {
            String error = errors ? String((CString)errors->GetBufferPointer(), (size_t)errors->GetBufferSize()) : "Unspecified";
            TTE_LOG("Could not compile D3D shader %s with vertex shader profile: error message: %s", builtName.c_str(), error.c_str());
            if (errors)
                errors->Release();
            return false;
        }
        if(errors)
            errors->Release();
        pMacro[macros.size()].Name = "TTE_PIXEL";
        pMacro[macros.size()].Definition = "1";
        errors = NULL;
        ID3DBlob* pixelCode = NULL;
        HRESULT ResultPixel = D3DCompile(fxResolvedPixel.c_str(), (SIZE_T)fxResolvedPixel.length(), builtName.c_str(), pMacro, NULL, RENDER_FX_ENTRY_POINT_FRAG, "ps_5_0", 0, 0, &pixelCode, &errors);
        if (ResultPixel != S_OK)
        {
            String error = errors ? String((CString)errors->GetBufferPointer(), (size_t)errors->GetBufferSize()) : "Unspecified";
            TTE_LOG("Could not compile D3D shader %s with pixel shader profile: error message: %s", builtName.c_str(), error.c_str());
            if (errors)
                errors->Release();
            return false;
        }
        if (errors)
            errors->Release();
        vert = _CreateShader(vertexParameters, RenderShaderType::VERTEX, builtName + "_VERTEX", ParamSlotsVert,
                             vertexAttribs, vertCode->GetBufferPointer(), (U32)vertCode->GetBufferSize(), SDL_GPU_SHADERFORMAT_DXBC);
        pixel = _CreateShader(pixelParameters, RenderShaderType::FRAGMENT, builtName + "_FRAG", ParamSlotsPixel,
                              {}, pixelCode->GetBufferPointer(), (U32)pixelCode->GetBufferSize(), SDL_GPU_SHADERFORMAT_DXBC);
        if (vertCode) 
            vertCode->Release();
        if (pixelCode) 
            pixelCode->Release();
#else
        TTE_LOG("Cannot create D3D12 device shader on non-windows graphics device!");
        return false;
#endif
    }
    else if(deviceType == RenderDeviceType::METAL)
    {
#ifdef PLATFORM_MAC
        // No compilation needed, MSL.
        vert = _CreateShader(vertexParameters, RenderShaderType::VERTEX, builtName + "_VERTEX", ParamSlotsVert, 
            vertexAttribs, (const void*)fxResolvedVertex.c_str(), (U32)fxResolvedVertex.size(),
                                    SDL_GPU_SHADERFORMAT_MSL);
        pixel = _CreateShader(pixelParameters, RenderShaderType::FRAGMENT, builtName + "_FRAG", ParamSlotsPixel,
                              {}, (const void*)fxResolvedPixel.c_str(), (U32)fxResolvedPixel.size(),
                                    SDL_GPU_SHADERFORMAT_MSL);
#else
        TTE_LOG("Cannot create METAL device shader on non-apple graphics device!");
        return false;
#endif
    }
    else
    {
        TTE_ASSERT(false, "Unknown or unsupported GPU device type!");
        return false;
    }
    return true;
}
