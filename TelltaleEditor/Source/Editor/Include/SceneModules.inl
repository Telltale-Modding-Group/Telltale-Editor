#include <Core/Config.hpp>
#include <Core/BitSet.hpp>
#include <Symbols.hpp>

#include <Renderer/Camera.hpp>
#include <Renderer/Text.hpp>

#ifdef TEXT
#undef TEXT
#endif

class Scene;
struct SceneAgent;

// ========================================= ENUM =========================================

// Scene modules are module_xxx.prop parent properties. they are 'components' or 'behaviour's of game objects.
// in the engine these correspond to XXX::OnSetupAgent(Ptr<Agent> pAgentGettingCreated, Handle<PropertySet>& hRenderableProps) functions.
enum class SceneModuleType : I32
{

    // Two groups. First set here don't depend on the renderable module.

    // === PRE RENDERABLE MODULES

    RENDERABLE      ,
    SKELETON        ,
    CAMERA          ,
    SCENE           ,

    // === POST RENDERABLE MODULES

    TEXT            , // Must stay 1st here.

    // ===

    NUM, LAST_POSTRENDERABLE = NUM - 1, UNKNOWN = -1, FIRST_PRERENDERABLE = 0,
    LAST_PRERENDERABLE = TEXT - 1, FIRST_POSTRENDERABLE = LAST_PRERENDERABLE + 1,

};

// Bitset for specifying module types attached to an agent
using SceneModuleTypes = BitSet<SceneModuleType, (U32)SceneModuleType::NUM, (SceneModuleType)0>;

template<SceneModuleType> struct SceneModule;

struct SceneModuleBase
{
    Ptr<Node> AgentNode;
};

// ========================================= SCENE MODULES =========================================
// ========================================= SCENE MODULES =========================================
// ========================================= SCENE MODULES =========================================

// Renderable scene component (RenderObject_Mesh* obj).
// Any actual render types (eg RenderVertexState) can be filled up, but not created
// As in the actual buffers should only be in data streams on CPU side.
template<> struct SceneModule<SceneModuleType::RENDERABLE> : SceneModuleBase
{

    static constexpr SceneModuleType ModuleType = SceneModuleType::RENDERABLE;
    static constexpr CString ModuleID = "renderable";
    static constexpr CString ModuleName = "Renderable";

    static inline String GetModulePropertySet()
    {
        return kRenderablePropName;
    }

    Mesh Renderable; // group of meshes

    // on setups create and gather stuff which is otherwise in the agent props or object cache to speed up.
    void OnSetupAgent(SceneAgent* pAgentGettingCreated);

    void OnModuleRemove(SceneAgent* pAttachedAgent);

};

// Skeleton component
template<> struct SceneModule<SceneModuleType::SKELETON> : SceneModuleBase
{

    static constexpr SceneModuleType ModuleType = SceneModuleType::SKELETON;
    static constexpr CString ModuleID = "skeleton";
    static constexpr CString ModuleName = "Skeleton";

    static inline String GetModulePropertySet()
    {
        return kSkeletonPropName;
    }

    Handle<Skeleton> Skl; // skeleton handle

    // impl in animationmgr.cpp
    void OnSetupAgent(SceneAgent* pAgentGettingCreated);

    void OnModuleRemove(SceneAgent* pAttachedAgent);

};

// Camera component
template<> struct SceneModule<SceneModuleType::CAMERA> : SceneModuleBase
{

    static constexpr SceneModuleType ModuleType = SceneModuleType::CAMERA;
    static constexpr CString ModuleID = "camera";
    static constexpr CString ModuleName = "Camera";

    static inline String GetModulePropertySet()
    {
        return kCameraPropName;
    }

    // telltale stores most stuff like this in the obj data array, we can speed by storing here. thats used for other runtime stuff.
    // need to use a pointer though, as vector data (ie this) can move and we need a stable pointer for callbacks.

    Ptr<Camera> Cam;

    // in modules.cpp
    void OnSetupAgent(SceneAgent* pAgentGettingCreated);

    inline void OnModuleRemove(SceneAgent* pAttachedAgent) {}

};

// Text component
template<> struct SceneModule<SceneModuleType::TEXT> : SceneModuleBase
{

    static constexpr SceneModuleType ModuleType = SceneModuleType::TEXT;
    static constexpr CString ModuleID = "text";
    static constexpr CString ModuleName = "Text";

    static inline String GetModulePropertySet()
    {
        return kTextPropName;
    }

    // in modules.cpp
    void OnSetupAgent(SceneAgent* pAgentGettingCreated);

    inline void OnModuleRemove(SceneAgent* pAttachedAgent) {}

    Ptr<RenderText> Text;

};

enum class TonemapType
{
    DEFAULT = 1,
    FILMIC = 2,
};

// Scene instance at runtime data. Yes it's a lot. Not my fault
struct SceneInstData
{
    Colour _AmbientColor;
    Colour _ShadowColor;
    String _ActiveCamera;
    Symbol _WalkBoxes;
    Symbol _FootstepWalkBoxes;
    I32 _SceneRenderPriority;
    I32 _SceneRenderLayer;
    Bool _ExcludeFromSaveGames;
    Float _SceneTimeScale;
    Bool _SceneInputEnabled;
    String _SceneAudioListener;
    String _SceneAudioPlayerOrigin;
    Float _SceneAudioMaster;
    Symbol _SceneAudioSnapshotSuite;
    Symbol _SceneAudioReverbDefinition;
    Float _SceneAudioReverb;
    std::vector<Symbol> _SceneAudioEventBanks;
    Bool _ScenePreloadable;
    Bool _ScenePreloadShaders;
    Bool _AfterEffectsEnabled;
    Bool _GenerateNPRLines;
    Float _NPRLinesFalloff;
    Float _NPRLinesBias;
    Float _NPRLinesAlphaFalloff;
    Float _NPRLinesAlphaBias;
    Colour _GlowClearColor;
    Float _GlowSigmaScale;
    Bool _FXAntiAliasing;
    Float _FXTAAWeight;
    Bool _FXColorEnabled;
    Colour _FXColorTint;
    Float _FXColorOpacity;
    Float _FXNoiseScale;
    Bool _FXSharpShadowsEnabled;
    Bool _FXLevelsEnabled;
    Float _FXLevelsBlackPoint;
    Float _FXLevelsWhitePoint;
    Float _FXLevelsIntensity;
    Float _FXLevelsBlackPointHDR;
    Float _FXLevelsWhitePointHDR;
    Float _FXLevelsIntensityHDR;
    TonemapType _FXTonemapType;
    Bool _FXTonemapEnabled;
    Bool _FXTonemapDOFEnabled;
    Float _FXTonemapIntensity;
    Float _FXTonemapWhitePoint;
    Float _FXTonemapBlackPoint;
    Float _FXTonemapFilmicPivot;
    Float _FXTonemapFilmicShoulderIntensity;
    Float _FXTonemapFilmicToeIntensity;
    Float _FXTonemapFilmicSign;
    Float _FXTonemapFarWhitePoint;
    Float _FXTonemapFarBlackPoint;
    Float _FXTonemapFarFilmicPivot;
    Float _FXTonemapFarFilmicShoulderIntensity;
    Float _FXTonemapFarFilmicToeIntensity;
    Float _FXTonemapFarFilmicSign;
    Bool _FXTonemapRGBEnabled;
    Bool _FXTonemapRGBDOFEnabled;
    Vector3 _FXTonemapRGBBlackPoints;
    Vector3 _FXTonemapRGBWhitePoints;
    Vector3 _FXTonemapRGBPivots;
    Vector3 _FXTonemapRGBShoulderIntensities;
    Vector3 _FXTonemapRGBToeIntensities;
    Vector3 _FXTonemapRGBSigns;
    Vector3 _FXTonemapRGBFarBlackPoints;
    Vector3 _FXTonemapRGBFarWhitePoints;
    Vector3 _FXTonemapRGBFarPivots;
    Vector3 _FXTonemapRGBFarShoulderIntensities;
    Vector3 _FXTonemapRGBFarToeIntensities;
    Vector3 _FXTonemapRGBFarSigns;
    Float _FXBloomThreshold;
    Float _FXBloomIntensity;
    Bool _FXAmbientOcclusionEnabled;
    Float _FXAmbientOcclusionIntensity;
    Float _FXAmbientOcclusionFalloff;
    Float _FXAmbientOcclusionRadius;
    Bool _FXAmbientOcclusionLightmap;
    Bool _FXDOFEnabled;
    Bool _FXDOFFOVAdjustEnabled;
    Bool _FXDOFAutoFocusEnabled;
    Float _FXDOFNear;
    Float _FXDOFFar;
    Float _FXDOFNearFalloff;
    Float _FXDOFFarFalloff;
    Float _FXDOFNearMax;
    Float _FXDOFFarMax;
    Float _FXDOFVignetteMax;
    Bool _FXDOFDebug;
    Float _FXDOFCoverageBoost;
    Float _FXForceLinearDepthOffset;
    Bool _FXVignetteTintEnabled;
    Bool _FXVignetteDOFEnabled;
    Colour _FXVignetteTint;
    Float _FXVignetteFalloff;
    Vector2 _FXVignetteCenter;
    Vector4 _FXVignetteCorners;
    Bool _HBAOEnabled;
    Bool _HBAODebug;
    Float _HBAORadius;
    Float _HBAOMaxRadiusPercent;
    Float _HBAOHemisphereBias;
    Float _HBAOIntensity;
    Float _HBAOOcclusionScale;
    Float _HBAOLuminanceScale;
    Float _HBAOMaxDistance;
    Float _HBAODistanceFalloff;
    Float _HBAOBlurSharpness;
    Bool _ScreenSpaceLinesEnabled;
    Colour _ScreenSpaceLinesColor;
    Float _ScreenSpaceLinesThickness;
    Float _ScreenSpaceLinesDepthFadeNear;
    Float _ScreenSpaceLinesDepthFadeFar;
    Float _ScreenSpaceLinesDepthMagnitude;
    Float _ScreenSpaceLinesDepthExponent;
    Vector3 _ScreenSpaceLinesLightDirection;
    Float _ScreenSpaceLinesLightMagnitude;
    Float _ScreenSpaceLinesLightExponent;
    Bool _ScreenSpaceLinesDebug;
    Bool _FogEnabled;
    Colour _FogColor;
    Float _FogAlpha;
    Float _FogNearPlane;
    Float _FogFarPlane;
    Float _HDRPaperWhiteNits;
    Float _GraphicBlackThreshold;
    Float _GraphicBlackSoftness;
    Float _GraphicBlackAlpha;
    Float _GraphicBlackNear;
    Float _GraphicBlackFar;
    Vector3 _WindDirection;
    Float _WindSpeed;
    Float _WindIdleStrength;
    Float _WindIdleSpacialFrequency;
    Float _WindGustSpeed;
    Float _WindGustStrength;
    Float _WindGustSpacialFrequency;
    Float _WindGustIdleStrengthMultiplier;
    Float _WindGustSeparationExponent;
    Bool _LightEnvReflectionEnabled;
    Bool _LightEnvBakeEnabled;
    Symbol _LightEnvReflectionTexture;
    Float _LightEnvReflectionIntensity;
    Colour _LightEnvReflectionTint;
    Bool _LightEnvEnabled;
    Symbol _LightEnvProbeData; // PROBE file
    Float _LightEnvIntensity;
    Float _LightEnvReflectionIntensityShadow;
    Float _LightEnvSaturation;
    Colour _LightEnvTint;
    Colour _LightEnvBackgroundColor;
    Vector2 _LightEnvProbeResolutionXZ;
    Float _LightEnvProbeResolutionY;
    Float _LightEnvShadowMomentBias;
    Float _LightEnvShadowDepthBias;
    Float _LightEnvShadowPositionOffsetBias;
    Float _LightEnvShadowLightBleedReduction;
    Float _LightEnvShadowMinDistance;
    Float _LightEnvShadowMaxDistance;
    Float _LightEnvDynamicShadowMaxDistance;
    Float _LightEnvShadowCascadeSplitBias;
    Bool _LightEnvShadowAutoDepthBounds;
    I32 _LightEnvShadowMaxUpdates;
    Float _LightShadowTraceMaxDistance;
    Float _LightStaticShadowBoundsMin;
    Float _LightStaticShadowBoundsMax;
    Symbol _EnvLightShadowGoboTexture;
    Bool _FXBrushDOFEnable;
    Bool _FXBrushOutlineEnable;
    Bool _FXBrushOutlineFilterEnable;
    Float _FXBrushOutlineSize;
    Float _FXBrushOutlineThreshold;
    Float _FXBrushOutlineColorThreshold;
    Float _FXBrushOutlineFalloff;
    Float _FXBrushNearOutlineScale;
    Symbol _FXBrushNearTexture;
    Symbol _FXBrushFarTexture;
    Float _FXBrushNearScale;
    Float _FXBrushNearDetail;
    Float _FXBrushFarScale;
    Float _FXBrushFarDetail;
    Float _FXBrushFarScaleBoost;
    Float _FXBrushFarPlane;
    Float _FXBrushFarPlaneFalloff;
    Float _FXBrushFarPlaneMaxScale;
    Float _FrameBufferScaleOverride;
    Float _FrameBufferScaleFactor;
    Bool _SpecularMultiplierEnabled;
    Colour _SpecularColorMultiplier;
    Float _SpecularIntensityMultiplier;
    Float _SpecularExponentMultiplier;
    Bool _HDRLightmapsEnabled;
    Float _HDRLightmapsIntensity;
    Float _CameraCutPositionThreshold;
    Float _CameraCutRotationThreshold;
    Float _ViewportScissorLeft;
    Float _ViewportScissorTop;
    Float _ViewportScissorRight;
    Float _ViewportScissorBottom;
    Symbol _ScenePhysicsData;

    // Inline setters
    inline void SetAmbientColor(Colour val) { _AmbientColor = val; }
    inline void SetShadowColor(Colour val) { _ShadowColor = val; }
    inline void SetActiveCamera(String val) { _ActiveCamera = val; }
    inline void SetWalkBoxes(Symbol val) { _WalkBoxes = val; }
    inline void SetFootstepWalkBoxes(Symbol val) { _FootstepWalkBoxes = val; }
    inline void SetSceneRenderPriority(I32 val) { _SceneRenderPriority = val; }
    inline void SetSceneRenderLayer(I32 val) { _SceneRenderLayer = val; }
    inline void SetExcludeFromSaveGames(Bool val) { _ExcludeFromSaveGames = val; }
    inline void SetSceneTimeScale(Float val) { _SceneTimeScale = val; }
    inline void SetSceneInputEnabled(Bool val) { _SceneInputEnabled = val; }
    inline void SetSceneAudioListener(String val) { _SceneAudioListener = val; }
    inline void SetSceneAudioPlayerOrigin(String val) { _SceneAudioPlayerOrigin = val; }
    inline void SetSceneAudioMaster(Float val) { _SceneAudioMaster = val; }
    inline void SetSceneAudioSnapshotSuite(Symbol val) { _SceneAudioSnapshotSuite = val; }
    inline void SetSceneAudioReverbDefinition(Symbol val) { _SceneAudioReverbDefinition = val; }
    inline void SetSceneAudioReverb(Float val) { _SceneAudioReverb = val; }
    inline void SetSceneAudioEventBanks(std::vector<Symbol> val) { _SceneAudioEventBanks = val; }
    inline void SetScenePreloadable(Bool val) { _ScenePreloadable = val; }
    inline void SetScenePreloadShaders(Bool val) { _ScenePreloadShaders = val; }
    inline void SetAfterEffectsEnabled(Bool val) { _AfterEffectsEnabled = val; }
    inline void SetGenerateNPRLines(Bool val) { _GenerateNPRLines = val; }
    inline void SetNPRLinesFalloff(Float val) { _NPRLinesFalloff = val; }
    inline void SetNPRLinesBias(Float val) { _NPRLinesBias = val; }
    inline void SetNPRLinesAlphaFalloff(Float val) { _NPRLinesAlphaFalloff = val; }
    inline void SetNPRLinesAlphaBias(Float val) { _NPRLinesAlphaBias = val; }
    inline void SetGlowClearColor(Colour val) { _GlowClearColor = val; }
    inline void SetGlowSigmaScale(Float val) { _GlowSigmaScale = val; }
    inline void SetFXAntiAliasing(Bool val) { _FXAntiAliasing = val; }
    inline void SetFXTAAWeight(Float val) { _FXTAAWeight = val; }
    inline void SetFXColorEnabled(Bool val) { _FXColorEnabled = val; }
    inline void SetFXColorTint(Colour val) { _FXColorTint = val; }
    inline void SetFXColorOpacity(Float val) { _FXColorOpacity = val; }
    inline void SetFXNoiseScale(Float val) { _FXNoiseScale = val; }
    inline void SetFXSharpShadowsEnabled(Bool val) { _FXSharpShadowsEnabled = val; }
    inline void SetFXLevelsEnabled(Bool val) { _FXLevelsEnabled = val; }
    inline void SetFXLevelsBlackPoint(Float val) { _FXLevelsBlackPoint = val; }
    inline void SetFXLevelsWhitePoint(Float val) { _FXLevelsWhitePoint = val; }
    inline void SetFXLevelsIntensity(Float val) { _FXLevelsIntensity = val; }
    inline void SetFXLevelsBlackPointHDR(Float val) { _FXLevelsBlackPointHDR = val; }
    inline void SetFXLevelsWhitePointHDR(Float val) { _FXLevelsWhitePointHDR = val; }
    inline void SetFXLevelsIntensityHDR(Float val) { _FXLevelsIntensityHDR = val; }
    inline void SetFXTonemapType(TonemapType val) { _FXTonemapType = val; }
    inline void SetFXTonemapEnabled(Bool val) { _FXTonemapEnabled = val; }
    inline void SetFXTonemapDOFEnabled(Bool val) { _FXTonemapDOFEnabled = val; }
    inline void SetFXTonemapIntensity(Float val) { _FXTonemapIntensity = val; }
    inline void SetFXTonemapWhitePoint(Float val) { _FXTonemapWhitePoint = val; }
    inline void SetFXTonemapBlackPoint(Float val) { _FXTonemapBlackPoint = val; }
    inline void SetFXTonemapFilmicPivot(Float val) { _FXTonemapFilmicPivot = val; }
    inline void SetFXTonemapFilmicShoulderIntensity(Float val) { _FXTonemapFilmicShoulderIntensity = val; }
    inline void SetFXTonemapFilmicToeIntensity(Float val) { _FXTonemapFilmicToeIntensity = val; }
    inline void SetFXTonemapFilmicSign(Float val) { _FXTonemapFilmicSign = val; }
    inline void SetFXTonemapFarWhitePoint(Float val) { _FXTonemapFarWhitePoint = val; }
    inline void SetFXTonemapFarBlackPoint(Float val) { _FXTonemapFarBlackPoint = val; }
    inline void SetFXTonemapFarFilmicPivot(Float val) { _FXTonemapFarFilmicPivot = val; }
    inline void SetFXTonemapFarFilmicShoulderIntensity(Float val) { _FXTonemapFarFilmicShoulderIntensity = val; }
    inline void SetFXTonemapFarFilmicToeIntensity(Float val) { _FXTonemapFarFilmicToeIntensity = val; }
    inline void SetFXTonemapFarFilmicSign(Float val) { _FXTonemapFarFilmicSign = val; }
    inline void SetFXTonemapRGBEnabled(Bool val) { _FXTonemapRGBEnabled = val; }
    inline void SetFXTonemapRGBDOFEnabled(Bool val) { _FXTonemapRGBDOFEnabled = val; }
    inline void SetFXTonemapRGBBlackPoints(Vector3 val) { _FXTonemapRGBBlackPoints = val; }
    inline void SetFXTonemapRGBWhitePoints(Vector3 val) { _FXTonemapRGBWhitePoints = val; }
    inline void SetFXTonemapRGBPivots(Vector3 val) { _FXTonemapRGBPivots = val; }
    inline void SetFXTonemapRGBShoulderIntensities(Vector3 val) { _FXTonemapRGBShoulderIntensities = val; }
    inline void SetFXTonemapRGBToeIntensities(Vector3 val) { _FXTonemapRGBToeIntensities = val; }
    inline void SetFXTonemapRGBSigns(Vector3 val) { _FXTonemapRGBSigns = val; }
    inline void SetFXTonemapRGBFarBlackPoints(Vector3 val) { _FXTonemapRGBFarBlackPoints = val; }
    inline void SetFXTonemapRGBFarWhitePoints(Vector3 val) { _FXTonemapRGBFarWhitePoints = val; }
    inline void SetFXTonemapRGBFarPivots(Vector3 val) { _FXTonemapRGBFarPivots = val; }
    inline void SetFXTonemapRGBFarShoulderIntensities(Vector3 val) { _FXTonemapRGBFarShoulderIntensities = val; }
    inline void SetFXTonemapRGBFarToeIntensities(Vector3 val) { _FXTonemapRGBFarToeIntensities = val; }
    inline void SetFXTonemapRGBFarSigns(Vector3 val) { _FXTonemapRGBFarSigns = val; }
    inline void SetFXBloomThreshold(Float val) { _FXBloomThreshold = val; }
    inline void SetFXBloomIntensity(Float val) { _FXBloomIntensity = val; }
    inline void SetFXAmbientOcclusionEnabled(Bool val) { _FXAmbientOcclusionEnabled = val; }
    inline void SetFXAmbientOcclusionIntensity(Float val) { _FXAmbientOcclusionIntensity = val; }
    inline void SetFXAmbientOcclusionFalloff(Float val) { _FXAmbientOcclusionFalloff = val; }
    inline void SetFXAmbientOcclusionRadius(Float val) { _FXAmbientOcclusionRadius = val; }
    inline void SetFXAmbientOcclusionLightmap(Bool val) { _FXAmbientOcclusionLightmap = val; }
    inline void SetFXDOFEnabled(Bool val) { _FXDOFEnabled = val; }
    inline void SetFXDOFFOVAdjustEnabled(Bool val) { _FXDOFFOVAdjustEnabled = val; }
    inline void SetFXDOFAutoFocusEnabled(Bool val) { _FXDOFAutoFocusEnabled = val; }
    inline void SetFXDOFNear(Float val) { _FXDOFNear = val; }
    inline void SetFXDOFFar(Float val) { _FXDOFFar = val; }
    inline void SetFXDOFNearFalloff(Float val) { _FXDOFNearFalloff = val; }
    inline void SetFXDOFFarFalloff(Float val) { _FXDOFFarFalloff = val; }
    inline void SetFXDOFNearMax(Float val) { _FXDOFNearMax = val; }
    inline void SetFXDOFFarMax(Float val) { _FXDOFFarMax = val; }
    inline void SetFXDOFVignetteMax(Float val) { _FXDOFVignetteMax = val; }
    inline void SetFXDOFDebug(Bool val) { _FXDOFDebug = val; }
    inline void SetFXDOFCoverageBoost(Float val) { _FXDOFCoverageBoost = val; }
    inline void SetFXForceLinearDepthOffset(Float val) { _FXForceLinearDepthOffset = val; }
    inline void SetFXVignetteTintEnabled(Bool val) { _FXVignetteTintEnabled = val; }
    inline void SetFXVignetteDOFEnabled(Bool val) { _FXVignetteDOFEnabled = val; }
    inline void SetFXVignetteTint(Colour val) { _FXVignetteTint = val; }
    inline void SetFXVignetteFalloff(Float val) { _FXVignetteFalloff = val; }
    inline void SetFXVignetteCenter(Vector2 val) { _FXVignetteCenter = val; }
    inline void SetFXVignetteCorners(Vector4 val) { _FXVignetteCorners = val; }
    inline void SetHBAOEnabled(Bool val) { _HBAOEnabled = val; }
    inline void SetHBAODebug(Bool val) { _HBAODebug = val; }
    inline void SetHBAORadius(Float val) { _HBAORadius = val; }
    inline void SetHBAOMaxRadiusPercent(Float val) { _HBAOMaxRadiusPercent = val; }
    inline void SetHBAOHemisphereBias(Float val) { _HBAOHemisphereBias = val; }
    inline void SetHBAOIntensity(Float val) { _HBAOIntensity = val; }
    inline void SetHBAOOcclusionScale(Float val) { _HBAOOcclusionScale = val; }
    inline void SetHBAOLuminanceScale(Float val) { _HBAOLuminanceScale = val; }
    inline void SetHBAOMaxDistance(Float val) { _HBAOMaxDistance = val; }
    inline void SetHBAODistanceFalloff(Float val) { _HBAODistanceFalloff = val; }
    inline void SetHBAOBlurSharpness(Float val) { _HBAOBlurSharpness = val; }
    inline void SetScreenSpaceLinesEnabled(Bool val) { _ScreenSpaceLinesEnabled = val; }
    inline void SetScreenSpaceLinesColor(Colour val) { _ScreenSpaceLinesColor = val; }
    inline void SetScreenSpaceLinesThickness(Float val) { _ScreenSpaceLinesThickness = val; }
    inline void SetScreenSpaceLinesDepthFadeNear(Float val) { _ScreenSpaceLinesDepthFadeNear = val; }
    inline void SetScreenSpaceLinesDepthFadeFar(Float val) { _ScreenSpaceLinesDepthFadeFar = val; }
    inline void SetScreenSpaceLinesDepthMagnitude(Float val) { _ScreenSpaceLinesDepthMagnitude = val; }
    inline void SetScreenSpaceLinesDepthExponent(Float val) { _ScreenSpaceLinesDepthExponent = val; }
    inline void SetScreenSpaceLinesLightDirection(Vector3 val) { _ScreenSpaceLinesLightDirection = val; }
    inline void SetScreenSpaceLinesLightMagnitude(Float val) { _ScreenSpaceLinesLightMagnitude = val; }
    inline void SetScreenSpaceLinesLightExponent(Float val) { _ScreenSpaceLinesLightExponent = val; }
    inline void SetScreenSpaceLinesDebug(Bool val) { _ScreenSpaceLinesDebug = val; }
    inline void SetFogEnabled(Bool val) { _FogEnabled = val; }
    inline void SetFogColor(Colour val) { _FogColor = val; }
    inline void SetFogAlpha(Float val) { _FogAlpha = val; }
    inline void SetFogNearPlane(Float val) { _FogNearPlane = val; }
    inline void SetFogFarPlane(Float val) { _FogFarPlane = val; }
    inline void SetHDRPaperWhiteNits(Float val) { _HDRPaperWhiteNits = val; }
    inline void SetGraphicBlackThreshold(Float val) { _GraphicBlackThreshold = val; }
    inline void SetGraphicBlackSoftness(Float val) { _GraphicBlackSoftness = val; }
    inline void SetGraphicBlackAlpha(Float val) { _GraphicBlackAlpha = val; }
    inline void SetGraphicBlackNear(Float val) { _GraphicBlackNear = val; }
    inline void SetGraphicBlackFar(Float val) { _GraphicBlackFar = val; }
    inline void SetWindDirection(Vector3 val) { _WindDirection = val; }
    inline void SetWindSpeed(Float val) { _WindSpeed = val; }
    inline void SetWindIdleStrength(Float val) { _WindIdleStrength = val; }
    inline void SetWindIdleSpacialFrequency(Float val) { _WindIdleSpacialFrequency = val; }
    inline void SetWindGustSpeed(Float val) { _WindGustSpeed = val; }
    inline void SetWindGustStrength(Float val) { _WindGustStrength = val; }
    inline void SetWindGustSpacialFrequency(Float val) { _WindGustSpacialFrequency = val; }
    inline void SetWindGustIdleStrengthMultiplier(Float val) { _WindGustIdleStrengthMultiplier = val; }
    inline void SetWindGustSeparationExponent(Float val) { _WindGustSeparationExponent = val; }
    inline void SetLightEnvReflectionEnabled(Bool val) { _LightEnvReflectionEnabled = val; }
    inline void SetLightEnvBakeEnabled(Bool val) { _LightEnvBakeEnabled = val; }
    inline void SetLightEnvReflectionTexture(Symbol val) { _LightEnvReflectionTexture = val; }
    inline void SetLightEnvReflectionIntensity(Float val) { _LightEnvReflectionIntensity = val; }
    inline void SetLightEnvReflectionTint(Colour val) { _LightEnvReflectionTint = val; }
    inline void SetLightEnvEnabled(Bool val) { _LightEnvEnabled = val; }
    inline void SetLightEnvProbeData(Symbol val) { _LightEnvProbeData = val; }
    inline void SetLightEnvIntensity(Float val) { _LightEnvIntensity = val; }
    inline void SetLightEnvReflectionIntensityShadow(Float val) { _LightEnvReflectionIntensityShadow = val; }
    inline void SetLightEnvSaturation(Float val) { _LightEnvSaturation = val; }
    inline void SetLightEnvTint(Colour val) { _LightEnvTint = val; }
    inline void SetLightEnvBackgroundColor(Colour val) { _LightEnvBackgroundColor = val; }
    inline void SetLightEnvProbeResolutionXZ(Vector2 val) { _LightEnvProbeResolutionXZ = val; }
    inline void SetLightEnvProbeResolutionY(Float val) { _LightEnvProbeResolutionY = val; }
    inline void SetLightEnvShadowMomentBias(Float val) { _LightEnvShadowMomentBias = val; }
    inline void SetLightEnvShadowDepthBias(Float val) { _LightEnvShadowDepthBias = val; }
    inline void SetLightEnvShadowPositionOffsetBias(Float val) { _LightEnvShadowPositionOffsetBias = val; }
    inline void SetLightEnvShadowLightBleedReduction(Float val) { _LightEnvShadowLightBleedReduction = val; }
    inline void SetLightEnvShadowMinDistance(Float val) { _LightEnvShadowMinDistance = val; }
    inline void SetLightEnvShadowMaxDistance(Float val) { _LightEnvShadowMaxDistance = val; }
    inline void SetLightEnvDynamicShadowMaxDistance(Float val) { _LightEnvDynamicShadowMaxDistance = val; }
    inline void SetLightEnvShadowCascadeSplitBias(Float val) { _LightEnvShadowCascadeSplitBias = val; }
    inline void SetLightEnvShadowAutoDepthBounds(Bool val) { _LightEnvShadowAutoDepthBounds = val; }
    inline void SetLightEnvShadowMaxUpdates(I32 val) { _LightEnvShadowMaxUpdates = val; }
    inline void SetLightShadowTraceMaxDistance(Float val) { _LightShadowTraceMaxDistance = val; }
    inline void SetLightStaticShadowBoundsMin(Float val) { _LightStaticShadowBoundsMin = val; }
    inline void SetLightStaticShadowBoundsMax(Float val) { _LightStaticShadowBoundsMax = val; }
    inline void SetEnvLightShadowGoboTexture(Symbol val) { _EnvLightShadowGoboTexture = val; }
    inline void SetFXBrushDOFEnable(Bool val) { _FXBrushDOFEnable = val; }
    inline void SetFXBrushOutlineEnable(Bool val) { _FXBrushOutlineEnable = val; }
    inline void SetFXBrushOutlineFilterEnable(Bool val) { _FXBrushOutlineFilterEnable = val; }
    inline void SetFXBrushOutlineSize(Float val) { _FXBrushOutlineSize = val; }
    inline void SetFXBrushOutlineThreshold(Float val) { _FXBrushOutlineThreshold = val; }
    inline void SetFXBrushOutlineColorThreshold(Float val) { _FXBrushOutlineColorThreshold = val; }
    inline void SetFXBrushOutlineFalloff(Float val) { _FXBrushOutlineFalloff = val; }
    inline void SetFXBrushNearOutlineScale(Float val) { _FXBrushNearOutlineScale = val; }
    inline void SetFXBrushNearTexture(Symbol val) { _FXBrushNearTexture = val; }
    inline void SetFXBrushFarTexture(Symbol val) { _FXBrushFarTexture = val; }
    inline void SetFXBrushNearScale(Float val) { _FXBrushNearScale = val; }
    inline void SetFXBrushNearDetail(Float val) { _FXBrushNearDetail = val; }
    inline void SetFXBrushFarScale(Float val) { _FXBrushFarScale = val; }
    inline void SetFXBrushFarDetail(Float val) { _FXBrushFarDetail = val; }
    inline void SetFXBrushFarScaleBoost(Float val) { _FXBrushFarScaleBoost = val; }
    inline void SetFXBrushFarPlane(Float val) { _FXBrushFarPlane = val; }
    inline void SetFXBrushFarPlaneFalloff(Float val) { _FXBrushFarPlaneFalloff = val; }
    inline void SetFXBrushFarPlaneMaxScale(Float val) { _FXBrushFarPlaneMaxScale = val; }
    inline void SetFrameBufferScaleOverride(Float val) { _FrameBufferScaleOverride = val; }
    inline void SetFrameBufferScaleFactor(Float val) { _FrameBufferScaleFactor = val; }
    inline void SetSpecularMultiplierEnabled(Bool val) { _SpecularMultiplierEnabled = val; }
    inline void SetSpecularColorMultiplier(Colour val) { _SpecularColorMultiplier = val; }
    inline void SetSpecularIntensityMultiplier(Float val) { _SpecularIntensityMultiplier = val; }
    inline void SetSpecularExponentMultiplier(Float val) { _SpecularExponentMultiplier = val; }
    inline void SetHDRLightmapsEnabled(Bool val) { _HDRLightmapsEnabled = val; }
    inline void SetHDRLightmapsIntensity(Float val) { _HDRLightmapsIntensity = val; }
    inline void SetCameraCutPositionThreshold(Float val) { _CameraCutPositionThreshold = val; }
    inline void SetCameraCutRotationThreshold(Float val) { _CameraCutRotationThreshold = val; }
    inline void SetViewportScissorLeft(Float val) { _ViewportScissorLeft = val; }
    inline void SetViewportScissorTop(Float val) { _ViewportScissorTop = val; }
    inline void SetViewportScissorRight(Float val) { _ViewportScissorRight = val; }
    inline void SetViewportScissorBottom(Float val) { _ViewportScissorBottom = val; }
    inline void SetScenePhysicsData(Symbol val) { _ScenePhysicsData = val; }

};

template<> struct SceneModule<SceneModuleType::SCENE> : SceneModuleBase
{

    static constexpr SceneModuleType ModuleType = SceneModuleType::SCENE;
    static constexpr CString ModuleID = "scene";
    static constexpr CString ModuleName = "Scene";

    static inline String GetModulePropertySet()
    {
        return kScenePropName;
    }

    // in modules.cpp
    void OnSetupAgent(SceneAgent* pAgentGettingCreated);

    Ptr<SceneInstData> SceneData; // runtime scene data

    inline void OnModuleRemove(SceneAgent* pAttachedAgent) {}

};

// ========================================= DATA-DRIVEN CONTAINERS =========================================

template<SceneModuleType Module = (SceneModuleType)0>
class _ContainedModuleBaton : public _ContainedModuleBaton<(SceneModuleType)((U32)Module + 1)>
{
public:
    std::vector<SceneModule<Module>> DataArray;
};

template<>
class _ContainedModuleBaton<SceneModuleType::NUM>
{
};

class SceneModuleContainer
{

    _ContainedModuleBaton<> _Container;

public:

    template<SceneModuleType Module>
    inline std::vector<SceneModule<Module>>& GetModuleArray()
    {
        return this->_Container._ContainedModuleBaton<Module>::DataArray;
    }

    template<SceneModuleType Module>
    inline const std::vector<SceneModule<Module>>& GetModuleArray() const
    {
        return this->_Container._ContainedModuleBaton<Module>::DataArray;
    }

};
