#pragma once

#include <Core/BitSet.hpp>
#include <Renderer/RenderParameters.hpp>
#include <Resource/ResourceRegistry.hpp>

#include <set>
#include <map>
#include <unordered_set>

#include <SDL3/SDL_gpu.h>

// ============================================= MISC =============================================

class RenderShader;
class RenderPipelineState;
struct SDL_GPUDevice;
class RenderContext;
enum class RenderShaderType;

#define RENDER_FX_MAX_RUNTIME_PROGRAMS 4096

#define RENDER_FX_ENTRY_POINT_VERT "VertexMain"
#define RENDER_FX_ENTRY_POINT_FRAG "PixelMain"

// ============================================= FX TYPES =============================================

// All effect types (shader effects, programs). This is the shader variant system.

// A render effect. These essentially map to different shader programs. Each of these can have a set of variants, see below.
enum class RenderEffect : U32
{
    MESH,
    FLAT, // flat meaning simply just a position input and object colour output (used for default meshes etc)
    COUNT,
};

// These are optional features which can be enabled for each render effect shader. These are shader variants.
enum class RenderEffectFeature : U32
{
    DEFORMABLE, // Deformable by animation.
    KEY_LIGHT, // Key (main) scene light source.
    COUNT,
};

using RenderEffectsBitSet = BitSet<RenderEffect, (U32)RenderEffect::COUNT, (RenderEffect)0>;
using RenderEffectFeaturesBitSet = BitSet<RenderEffectFeature, (U32)RenderEffectFeature::COUNT, (RenderEffectFeature)0>;

// Descriptor for a render effect.
struct RenderEffectDesc
{

    RenderEffect Effect;
    CString Name;
    CString FXFileName;
    CString Macro;
    RenderEffectFeaturesBitSet ValidFeatures;
    ShaderParameterTypes VertexRequiredParameters, PixelRequiredParameters;
    VertexAttributesBitset RequiredAttribs;

    constexpr inline RenderEffectDesc(RenderEffect Fx, CString Nm, CString fxFile, CString m, const RenderEffectFeaturesBitSet& validFeatures,
                                        const ShaderParameterTypes& requiredParamsVertex, const ShaderParameterTypes& requiredParamsPixel, const VertexAttributesBitset& reqAttr)
        : Effect(Fx), Name(Nm), FXFileName(fxFile), ValidFeatures(validFeatures), PixelRequiredParameters(requiredParamsPixel),
                                        VertexRequiredParameters(requiredParamsVertex), RequiredAttribs(reqAttr), Macro(m) {}

};

// Descriptor for a render effect feature variant.
struct RenderEffectFeatureDesc
{

    RenderEffectFeature Feature;
    CString Name; // name
    CString Macro; // macro for shader test
    CString Tag; // command line tag                        
    ShaderParameterTypes VertexRequiredParameters, PixelRequiredParameters;
    VertexAttributesBitset RequiredAttribs;

    constexpr inline RenderEffectFeatureDesc(RenderEffectFeature ft, CString nm, CString macro, CString t,
                                             const ShaderParameterTypes& requiredParamsVertex, const ShaderParameterTypes& requiredParamsPixel, const VertexAttributesBitset& reqAttr)
            : Name(nm), Macro(macro), Tag(t), PixelRequiredParameters(requiredParamsPixel),
                                             VertexRequiredParameters(requiredParamsVertex), RequiredAttribs(reqAttr), Feature(ft) {}

};

constexpr RenderEffectDesc RenderEffectDescriptors[] = 
{
    RenderEffectDesc( // Default mesh shader.
    /*GENERAL INFO:*/           RenderEffect::MESH, "Mesh", "Base.fx", "EFFECT_MESH",
    /*VALID FEATURES: */        RenderEffectFeaturesBitSet::MakeWith<RenderEffectFeature::DEFORMABLE, RenderEffectFeature::KEY_LIGHT>(),
    /*VERT REQUIRED PARAMS: */  ShaderParameterTypes::MakeWith<ShaderParameterType::PARAMETER_CAMERA, ShaderParameterType::PARAMETER_OBJECT>(),
    /*PIX REQUIRED PARAMS: */   ShaderParameterTypes::MakeWith<ShaderParameterType::PARAMETER_SAMPLER_DIFFUSE>(),
    /*REQUIRED ATTRIBS:*/       VertexAttributesBitset::MakeWith<RenderAttributeType::POSITION>()
    ),
    RenderEffectDesc( // Simple untextured flat shader (solid colour)
    /*GENERAL INFO:*/           RenderEffect::FLAT, "Flat", "Base.fx", "EFFECT_FLAT",
    /*VALID FEATURES: */        RenderEffectFeaturesBitSet::MakeWith<>(),
    /*VERT REQUIRED PARAMS: */  ShaderParameterTypes::MakeWith<ShaderParameterType::PARAMETER_CAMERA, ShaderParameterType::PARAMETER_OBJECT>(),
    /*PIX REQUIRED PARAMS: */   ShaderParameterTypes::MakeWith(),
    /*REQUIRED ATTRIBS:*/       VertexAttributesBitset::MakeWith<RenderAttributeType::POSITION>()
    ),
    RenderEffectDesc(
        RenderEffect::COUNT, nullptr, nullptr, nullptr,
        RenderEffectFeaturesBitSet::MakeWith(),
        ShaderParameterTypes::MakeWith(),
        ShaderParameterTypes::MakeWith(),
        VertexAttributesBitset::MakeWith()
    ),
};

constexpr RenderEffectFeatureDesc RenderEffectFeatureDescriptors[] =
{
    RenderEffectFeatureDesc(RenderEffectFeature::DEFORMABLE,
    /*GENERAL INFO:*/           "Deformable", "FEATURE_DEFORMABLE", "deform",
    /*VERT REQUIRED PARAMS:*/   ShaderParameterTypes::MakeWith<ShaderParameterType::PARAMETER_GENERIC0 /*bone buffer*/>(),
    /*PIX REQUIRED PARAMS:*/    ShaderParameterTypes::MakeWith(),
    /*REQUIRED ATTRIBS:*/       VertexAttributesBitset::MakeWith<RenderAttributeType::BLEND_INDEX, RenderAttributeType::BLEND_WEIGHT>()
    ),
    RenderEffectFeatureDesc(RenderEffectFeature::KEY_LIGHT,
    /*GENERAL INFO:*/       "KeyLight", "FEATURE_KEY_LIGHT", "keylight",
    /*VERT REQUIRED PARAMS:*/   ShaderParameterTypes::MakeWith(), // buffer lights TODO
    /*PIX REQUIRED PARAMS:*/    ShaderParameterTypes::MakeWith(),
    /*REQUIRED ATTRIBS:*/   VertexAttributesBitset::MakeWith<RenderAttributeType::NORMAL>() // for shading.
    ),
    RenderEffectFeatureDesc(RenderEffectFeature::COUNT,
          nullptr, nullptr, nullptr,
          ShaderParameterTypes::MakeWith(),
          ShaderParameterTypes::MakeWith(),
          VertexAttributesBitset::MakeWith()
    ),
};

// ============================================= PUB API =============================================

// Reference to a shader variant
struct RenderEffectRef
{

    U64 EffectHash;

    // Returns if valid.
    inline operator Bool() const
    {
        return EffectHash != 0;
    }

};

// Effect ref is generated from a parameters struct like this
struct RenderEffectParams
{

    RenderEffect Effect;
    RenderEffectFeaturesBitSet Features;

    RenderEffectParams() = default;

};

// ============================================= API CLASSES =============================================

enum class RenderEffectProgramFlag
{
    LOADED = 1,
};

// Loaded program
struct RenderEffectProgram
{

    U64 FXFileHash; // for reloads
    U64 EffectHash; // calcualted hash for this effect program variant
    RenderShader* Vert, *Pixel; // raw shader pointers.
    Flags ProgramFlags;
    RenderEffectParams Params;

    RenderEffectProgram() = default;

    inline Bool operator<(const RenderEffectProgram& rhs) const
    {
        return EffectHash < rhs.EffectHash;
    }

};

// ============================================= FX CACHE =============================================

enum class RenderEffectCacheFlag
{
    HOT_RELOADING = 1,
    INIT = 2,
};

// Manager for shaders. Aggregated into the render context.
class RenderEffectCache
{
public:

    static const RenderEffectDesc& GetEffectDesc(RenderEffect effect);
    static const RenderEffectFeatureDesc& GetEffectFeatureDesc(RenderEffectFeature effectFeat);

    // Construct
    RenderEffectCache(Ptr<ResourceRegistry> reg, RenderContext* pContext);

    // get and hash effect ref
    RenderEffectRef GetEffectRef(RenderEffect effect, RenderEffectFeaturesBitSet variantFeatures);

    // resolve one for context
    Bool ResolveEffectRef(RenderEffectRef ref, Ptr<RenderShader>& programVert, Ptr<RenderShader>& programPixel);

    // Reload all pipelines
    void ReloadPipelines(const std::vector<Ptr<RenderPipelineState>>& pipelines);

    // Release all shaders and loaded resources
    void Release();

    // Toggles to enable / disable shader hot reloading
    void ToggleHotReloading(Bool bOnOff);

    // Create shader variant name
    String BuildName(RenderEffectParams params, String extension); // Build name of variant program file
    
    // Returns it's name if it is found
    String LocateEffectName(RenderEffectRef ref);

    void Initialise();

private:
        
    struct FXResolveState
    {
        U8* ParameterSlotsOut;
        const ShaderParameterTypes& EffectParameters;
        const String& FXName;
        const RenderEffectParams& Effect;
        const String& BuiltName;
#ifdef PLATFORM_WINDOWS
        union
        {
            mutable U32 TextureSamplersIndex; mutable U32 GenericBuffersIndex;
        };
#else
        mutable U32 TextureSamplersIndex;
        mutable U32 GenericBuffersIndex;
#endif
        mutable U32 UniformBuffersIndex = 0;
    };

    struct SectionResolverState
    {

        String Expression;
        size_t Position;
        const std::unordered_set<std::string>& Macros;
        CString Error;

        inline SectionResolverState(const String& expr, const std::unordered_set<std::string>& macros)
            : Expression(expr), Position(0), Macros(macros), Error(nullptr) {}

    };

    using FXHashIterator = typename std::map<U64, String>::iterator;

    static U32 FXResolver_Uniform(const String& arg, U32 line, const FXResolveState& state);
    static U32 FXResolver_Generic(const String& arg, U32 line, const FXResolveState& state);
    // combined sampler and texture for now. can split by adding seperate counters in above and changing context set param.
    static U32 FXResolver_TextureSampler(const String& arg, U32 line, const FXResolveState& state); 
    static U32 FXResolver_VertexAttribute(const String& arg, U32 line, const FXResolveState& state);

    U64 _ComputeEffectHash(RenderEffectParams params);

    Bool _CreateProgram(const String& fxSrc, RenderEffectParams params, Ptr<RenderShader>& vert, Ptr<RenderShader>& pixel);

    Ptr<RenderShader> _CreateShader(const ShaderParameterTypes& parameters, RenderShaderType shtype, const String& shaderName, U8* pParameterSlots,
                                    const VertexAttributesBitset& vertexStateAttribs, const void* pShaderBinary, U32 binarySize, SDL_GPUShaderFormat expectedFormat);

    // parse buffer, texture etc macro to actual buffer indices
    String _ParseFX(const String& fxStr, U8* pParameterSlotsOut, const ShaderParameterTypes& effectParameters,
                    const String& fxName, RenderEffectParams effectParams, const std::vector<String>& macros, const String& builtName);

    void _FXSectionSkipWhitespace(SectionResolverState& state);
    Bool _FXSectionParseIdentifier(SectionResolverState& state);
    Bool _FXSectionParsePrim(SectionResolverState& state);
    Bool _FXSectionParseAND(SectionResolverState& state);
    Bool _FXSectionParseOR(SectionResolverState& state);
    Bool _FXSectionEval(SectionResolverState& state);

    String _ResolveFXSections(const String& src, const std::vector<std::string>& macros, const String& fxName);

    String _ResolveFXBindings(const String& source, const String& macroName, ShaderParameterTypeClass clz,
                              U32(*resolver)(const String&, U32 line, const FXResolveState&), const FXResolveState& resolverState);

    // inserts source and source hash from resolve fx source
    FXHashIterator _ResolveFX(RenderEffect effect);

    // resolve and load it (forced, will always reload from resource sys) the full shader file source 
    String _ResolveFXSource(RenderEffect effect);

    // find from program array. return index
    U32 _ResolveExistingRef(RenderEffectRef ref);

    // cast to strong
    Ptr<RenderShader> _ResolveInternalShader(RenderShader* pShader);

    // insert into strong array
    void _InsertInternalShader(Ptr<RenderShader> pShader);

    // swaps shader. set new to nullptr to remove it and not replace it.
    void _SwapInternalShader(Ptr<RenderShader> pShader, Ptr<RenderShader> pNewShader);

    // insert into program array returning the index
    U32 _InsertProgramRef(RenderEffectParams params, U64 fxSourceHash);

    Ptr<ResourceRegistry> _Registry;
    String _ProgramsResourceLocation;
    Flags _Flags;
    RenderContext* _Context;

    std::set<Ptr<RenderShader>> _StrongShaderRefs; // store raw pointers internally. keep alive here
    std::map<U64, String> _FXHash; // fx shader source hash => shader source

    void (*_PsuedoMacroTranslate)(String& out, U32 resolvedIndex, ShaderParameterTypeClass paramClass/*vertex buffer => attribute index, index buffer => texture*/) = nullptr;
    String _FXFileNamePrefix = "";

    U32 _ProgramCount;
    RenderEffectProgram _Program[RENDER_FX_MAX_RUNTIME_PROGRAMS]; // Keep here because we want it close by.

};