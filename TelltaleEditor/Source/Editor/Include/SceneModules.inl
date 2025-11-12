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
    LIGHT           ,
    SELECTABLE      ,
    DIALOG_CHOICE   ,
    DIALOG          ,
    WALK_ANIMATOR   ,
    TRIGGER         ,
    NAV_CAM         ,
    PATH_TO         ,

    // === POST RENDERABLE MODULES

    TEXT            , // Must stay 1st here.
    ROLLOVER        ,

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
    
    inline void OnSceneChange(Scene* newScene) {} // if any cache the scene weak ptr, change it here
    
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

    void OnModuleRemove(SceneAgent* pAttachedAgent);
    
    void OnSceneChange(Scene* newScene);

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

    void OnModuleRemove(SceneAgent* pAttachedAgent);

    Ptr<RenderText> Text;

};

// Scene instance at runtime data. Yes it's a lot. Not my fault
struct SceneInstData
{

    String _ActiveCamera;
    String _SceneAudioListener;
    String _SceneAudioPlayerOrigin;
    std::vector<Symbol> _SceneAudioEventBanks;

    struct PODData
    {

        Colour _AmbientColor;
        Colour _ShadowColor;
        Symbol _WalkBoxes;
        Symbol _FootstepWalkBoxes;
        I32 _SceneRenderPriority;
        I32 _SceneRenderLayer;
        Bool _ExcludeFromSaveGames;
        Float _SceneTimeScale;
        Bool _SceneInputEnabled;
        Float _SceneAudioMaster;
        Symbol _SceneAudioSnapshotSuite;
        Symbol _SceneAudioReverbDefinition;
        Float _SceneAudioReverb;
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

    };

    PODData _Data;

    // Inline setters
    inline void SetAmbientColor(Colour val) { _Data._AmbientColor = val; }
    inline void SetShadowColor(Colour val) { _Data._ShadowColor = val; }
    inline void SetActiveCamera(String val) { _ActiveCamera = val; }
    inline void SetWalkBoxes(Symbol val) { _Data._WalkBoxes = val; }
    inline void SetFootstepWalkBoxes(Symbol val) { _Data._FootstepWalkBoxes = val; }
    inline void SetSceneRenderPriority(I32 val) { _Data._SceneRenderPriority = val; }
    inline void SetSceneRenderLayer(I32 val) { _Data._SceneRenderLayer = val; }
    inline void SetExcludeFromSaveGames(Bool val) { _Data._ExcludeFromSaveGames = val; }
    inline void SetSceneTimeScale(Float val) { _Data._SceneTimeScale = val; }
    inline void SetSceneInputEnabled(Bool val) { _Data._SceneInputEnabled = val; }
    inline void SetSceneAudioListener(String val) { _SceneAudioListener = val; }
    inline void SetSceneAudioPlayerOrigin(String val) { _SceneAudioPlayerOrigin = val; }
    inline void SetSceneAudioMaster(Float val) { _Data._SceneAudioMaster = val; }
    inline void SetSceneAudioSnapshotSuite(Symbol val) { _Data._SceneAudioSnapshotSuite = val; }
    inline void SetSceneAudioReverbDefinition(Symbol val) { _Data._SceneAudioReverbDefinition = val; }
    inline void SetSceneAudioReverb(Float val) { _Data._SceneAudioReverb = val; }
    inline void SetSceneAudioEventBanks(std::vector<Symbol> val) { _SceneAudioEventBanks = val; }
    inline void SetScenePreloadable(Bool val) { _Data._ScenePreloadable = val; }
    inline void SetScenePreloadShaders(Bool val) { _Data._ScenePreloadShaders = val; }
    inline void SetAfterEffectsEnabled(Bool val) { _Data._AfterEffectsEnabled = val; }
    inline void SetGenerateNPRLines(Bool val) { _Data._GenerateNPRLines = val; }
    inline void SetNPRLinesFalloff(Float val) { _Data._NPRLinesFalloff = val; }
    inline void SetNPRLinesBias(Float val) { _Data._NPRLinesBias = val; }
    inline void SetNPRLinesAlphaFalloff(Float val) { _Data._NPRLinesAlphaFalloff = val; }
    inline void SetNPRLinesAlphaBias(Float val) { _Data._NPRLinesAlphaBias = val; }
    inline void SetGlowClearColor(Colour val) { _Data._GlowClearColor = val; }
    inline void SetGlowSigmaScale(Float val) { _Data._GlowSigmaScale = val; }
    inline void SetFXAntiAliasing(Bool val) { _Data._FXAntiAliasing = val; }
    inline void SetFXTAAWeight(Float val) { _Data._FXTAAWeight = val; }
    inline void SetFXColorEnabled(Bool val) { _Data._FXColorEnabled = val; }
    inline void SetFXColorTint(Colour val) { _Data._FXColorTint = val; }
    inline void SetFXColorOpacity(Float val) { _Data._FXColorOpacity = val; }
    inline void SetFXNoiseScale(Float val) { _Data._FXNoiseScale = val; }
    inline void SetFXSharpShadowsEnabled(Bool val) { _Data._FXSharpShadowsEnabled = val; }
    inline void SetFXLevelsEnabled(Bool val) { _Data._FXLevelsEnabled = val; }
    inline void SetFXLevelsBlackPoint(Float val) { _Data._FXLevelsBlackPoint = val; }
    inline void SetFXLevelsWhitePoint(Float val) { _Data._FXLevelsWhitePoint = val; }
    inline void SetFXLevelsIntensity(Float val) { _Data._FXLevelsIntensity = val; }
    inline void SetFXLevelsBlackPointHDR(Float val) { _Data._FXLevelsBlackPointHDR = val; }
    inline void SetFXLevelsWhitePointHDR(Float val) { _Data._FXLevelsWhitePointHDR = val; }
    inline void SetFXLevelsIntensityHDR(Float val) { _Data._FXLevelsIntensityHDR = val; }
    inline void SetFXTonemapType(Enum<TonemapType> val) { _Data._FXTonemapType = val.Value; }
    inline void SetFXTonemapEnabled(Bool val) { _Data._FXTonemapEnabled = val; }
    inline void SetFXTonemapDOFEnabled(Bool val) { _Data._FXTonemapDOFEnabled = val; }
    inline void SetFXTonemapIntensity(Float val) { _Data._FXTonemapIntensity = val; }
    inline void SetFXTonemapWhitePoint(Float val) { _Data._FXTonemapWhitePoint = val; }
    inline void SetFXTonemapBlackPoint(Float val) { _Data._FXTonemapBlackPoint = val; }
    inline void SetFXTonemapFilmicPivot(Float val) { _Data._FXTonemapFilmicPivot = val; }
    inline void SetFXTonemapFilmicShoulderIntensity(Float val) { _Data._FXTonemapFilmicShoulderIntensity = val; }
    inline void SetFXTonemapFilmicToeIntensity(Float val) { _Data._FXTonemapFilmicToeIntensity = val; }
    inline void SetFXTonemapFilmicSign(Float val) { _Data._FXTonemapFilmicSign = val; }
    inline void SetFXTonemapFarWhitePoint(Float val) { _Data._FXTonemapFarWhitePoint = val; }
    inline void SetFXTonemapFarBlackPoint(Float val) { _Data._FXTonemapFarBlackPoint = val; }
    inline void SetFXTonemapFarFilmicPivot(Float val) { _Data._FXTonemapFarFilmicPivot = val; }
    inline void SetFXTonemapFarFilmicShoulderIntensity(Float val) { _Data._FXTonemapFarFilmicShoulderIntensity = val; }
    inline void SetFXTonemapFarFilmicToeIntensity(Float val) { _Data._FXTonemapFarFilmicToeIntensity = val; }
    inline void SetFXTonemapFarFilmicSign(Float val) { _Data._FXTonemapFarFilmicSign = val; }
    inline void SetFXTonemapRGBEnabled(Bool val) { _Data._FXTonemapRGBEnabled = val; }
    inline void SetFXTonemapRGBDOFEnabled(Bool val) { _Data._FXTonemapRGBDOFEnabled = val; }
    inline void SetFXTonemapRGBBlackPoints(Vector3 val) { _Data._FXTonemapRGBBlackPoints = val; }
    inline void SetFXTonemapRGBWhitePoints(Vector3 val) { _Data._FXTonemapRGBWhitePoints = val; }
    inline void SetFXTonemapRGBPivots(Vector3 val) { _Data._FXTonemapRGBPivots = val; }
    inline void SetFXTonemapRGBShoulderIntensities(Vector3 val) { _Data._FXTonemapRGBShoulderIntensities = val; }
    inline void SetFXTonemapRGBToeIntensities(Vector3 val) { _Data._FXTonemapRGBToeIntensities = val; }
    inline void SetFXTonemapRGBSigns(Vector3 val) { _Data._FXTonemapRGBSigns = val; }
    inline void SetFXTonemapRGBFarBlackPoints(Vector3 val) { _Data._FXTonemapRGBFarBlackPoints = val; }
    inline void SetFXTonemapRGBFarWhitePoints(Vector3 val) { _Data._FXTonemapRGBFarWhitePoints = val; }
    inline void SetFXTonemapRGBFarPivots(Vector3 val) { _Data._FXTonemapRGBFarPivots = val; }
    inline void SetFXTonemapRGBFarShoulderIntensities(Vector3 val) { _Data._FXTonemapRGBFarShoulderIntensities = val; }
    inline void SetFXTonemapRGBFarToeIntensities(Vector3 val) { _Data._FXTonemapRGBFarToeIntensities = val; }
    inline void SetFXTonemapRGBFarSigns(Vector3 val) { _Data._FXTonemapRGBFarSigns = val; }
    inline void SetFXBloomThreshold(Float val) { _Data._FXBloomThreshold = val; }
    inline void SetFXBloomIntensity(Float val) { _Data._FXBloomIntensity = val; }
    inline void SetFXAmbientOcclusionEnabled(Bool val) { _Data._FXAmbientOcclusionEnabled = val; }
    inline void SetFXAmbientOcclusionIntensity(Float val) { _Data._FXAmbientOcclusionIntensity = val; }
    inline void SetFXAmbientOcclusionFalloff(Float val) { _Data._FXAmbientOcclusionFalloff = val; }
    inline void SetFXAmbientOcclusionRadius(Float val) { _Data._FXAmbientOcclusionRadius = val; }
    inline void SetFXAmbientOcclusionLightmap(Bool val) { _Data._FXAmbientOcclusionLightmap = val; }
    inline void SetFXDOFEnabled(Bool val) { _Data._FXDOFEnabled = val; }
    inline void SetFXDOFFOVAdjustEnabled(Bool val) { _Data._FXDOFFOVAdjustEnabled = val; }
    inline void SetFXDOFAutoFocusEnabled(Bool val) { _Data._FXDOFAutoFocusEnabled = val; }
    inline void SetFXDOFNear(Float val) { _Data._FXDOFNear = val; }
    inline void SetFXDOFFar(Float val) { _Data._FXDOFFar = val; }
    inline void SetFXDOFNearFalloff(Float val) { _Data._FXDOFNearFalloff = val; }
    inline void SetFXDOFFarFalloff(Float val) { _Data._FXDOFFarFalloff = val; }
    inline void SetFXDOFNearMax(Float val) { _Data._FXDOFNearMax = val; }
    inline void SetFXDOFFarMax(Float val) { _Data._FXDOFFarMax = val; }
    inline void SetFXDOFVignetteMax(Float val) { _Data._FXDOFVignetteMax = val; }
    inline void SetFXDOFDebug(Bool val) { _Data._FXDOFDebug = val; }
    inline void SetFXDOFCoverageBoost(Float val) { _Data._FXDOFCoverageBoost = val; }
    inline void SetFXForceLinearDepthOffset(Float val) { _Data._FXForceLinearDepthOffset = val; }
    inline void SetFXVignetteTintEnabled(Bool val) { _Data._FXVignetteTintEnabled = val; }
    inline void SetFXVignetteDOFEnabled(Bool val) { _Data._FXVignetteDOFEnabled = val; }
    inline void SetFXVignetteTint(Colour val) { _Data._FXVignetteTint = val; }
    inline void SetFXVignetteFalloff(Float val) { _Data._FXVignetteFalloff = val; }
    inline void SetFXVignetteCenter(Vector2 val) { _Data._FXVignetteCenter = val; }
    inline void SetFXVignetteCorners(Vector4 val) { _Data._FXVignetteCorners = val; }
    inline void SetHBAOEnabled(Bool val) { _Data._HBAOEnabled = val; }
    inline void SetHBAODebug(Bool val) { _Data._HBAODebug = val; }
    inline void SetHBAORadius(Float val) { _Data._HBAORadius = val; }
    inline void SetHBAOMaxRadiusPercent(Float val) { _Data._HBAOMaxRadiusPercent = val; }
    inline void SetHBAOHemisphereBias(Float val) { _Data._HBAOHemisphereBias = val; }
    inline void SetHBAOIntensity(Float val) { _Data._HBAOIntensity = val; }
    inline void SetHBAOOcclusionScale(Float val) { _Data._HBAOOcclusionScale = val; }
    inline void SetHBAOLuminanceScale(Float val) { _Data._HBAOLuminanceScale = val; }
    inline void SetHBAOMaxDistance(Float val) { _Data._HBAOMaxDistance = val; }
    inline void SetHBAODistanceFalloff(Float val) { _Data._HBAODistanceFalloff = val; }
    inline void SetHBAOBlurSharpness(Float val) { _Data._HBAOBlurSharpness = val; }
    inline void SetScreenSpaceLinesEnabled(Bool val) { _Data._ScreenSpaceLinesEnabled = val; }
    inline void SetScreenSpaceLinesColor(Colour val) { _Data._ScreenSpaceLinesColor = val; }
    inline void SetScreenSpaceLinesThickness(Float val) { _Data._ScreenSpaceLinesThickness = val; }
    inline void SetScreenSpaceLinesDepthFadeNear(Float val) { _Data._ScreenSpaceLinesDepthFadeNear = val; }
    inline void SetScreenSpaceLinesDepthFadeFar(Float val) { _Data._ScreenSpaceLinesDepthFadeFar = val; }
    inline void SetScreenSpaceLinesDepthMagnitude(Float val) { _Data._ScreenSpaceLinesDepthMagnitude = val; }
    inline void SetScreenSpaceLinesDepthExponent(Float val) { _Data._ScreenSpaceLinesDepthExponent = val; }
    inline void SetScreenSpaceLinesLightDirection(Vector3 val) { _Data._ScreenSpaceLinesLightDirection = val; }
    inline void SetScreenSpaceLinesLightMagnitude(Float val) { _Data._ScreenSpaceLinesLightMagnitude = val; }
    inline void SetScreenSpaceLinesLightExponent(Float val) { _Data._ScreenSpaceLinesLightExponent = val; }
    inline void SetScreenSpaceLinesDebug(Bool val) { _Data._ScreenSpaceLinesDebug = val; }
    inline void SetFogEnabled(Bool val) { _Data._FogEnabled = val; }
    inline void SetFogColor(Colour val) { _Data._FogColor = val; }
    inline void SetFogAlpha(Float val) { _Data._FogAlpha = val; }
    inline void SetFogNearPlane(Float val) { _Data._FogNearPlane = val; }
    inline void SetFogFarPlane(Float val) { _Data._FogFarPlane = val; }
    inline void SetHDRPaperWhiteNits(Float val) { _Data._HDRPaperWhiteNits = val; }
    inline void SetGraphicBlackThreshold(Float val) { _Data._GraphicBlackThreshold = val; }
    inline void SetGraphicBlackSoftness(Float val) { _Data._GraphicBlackSoftness = val; }
    inline void SetGraphicBlackAlpha(Float val) { _Data._GraphicBlackAlpha = val; }
    inline void SetGraphicBlackNear(Float val) { _Data._GraphicBlackNear = val; }
    inline void SetGraphicBlackFar(Float val) { _Data._GraphicBlackFar = val; }
    inline void SetWindDirection(Vector3 val) { _Data._WindDirection = val; }
    inline void SetWindSpeed(Float val) { _Data._WindSpeed = val; }
    inline void SetWindIdleStrength(Float val) { _Data._WindIdleStrength = val; }
    inline void SetWindIdleSpacialFrequency(Float val) { _Data._WindIdleSpacialFrequency = val; }
    inline void SetWindGustSpeed(Float val) { _Data._WindGustSpeed = val; }
    inline void SetWindGustStrength(Float val) { _Data._WindGustStrength = val; }
    inline void SetWindGustSpacialFrequency(Float val) { _Data._WindGustSpacialFrequency = val; }
    inline void SetWindGustIdleStrengthMultiplier(Float val) { _Data._WindGustIdleStrengthMultiplier = val; }
    inline void SetWindGustSeparationExponent(Float val) { _Data._WindGustSeparationExponent = val; }
    inline void SetLightEnvReflectionEnabled(Bool val) { _Data._LightEnvReflectionEnabled = val; }
    inline void SetLightEnvBakeEnabled(Bool val) { _Data._LightEnvBakeEnabled = val; }
    inline void SetLightEnvReflectionTexture(Symbol val) { _Data._LightEnvReflectionTexture = val; }
    inline void SetLightEnvReflectionIntensity(Float val) { _Data._LightEnvReflectionIntensity = val; }
    inline void SetLightEnvReflectionTint(Colour val) { _Data._LightEnvReflectionTint = val; }
    inline void SetLightEnvEnabled(Bool val) { _Data._LightEnvEnabled = val; }
    inline void SetLightEnvProbeData(Symbol val) { _Data._LightEnvProbeData = val; }
    inline void SetLightEnvIntensity(Float val) { _Data._LightEnvIntensity = val; }
    inline void SetLightEnvReflectionIntensityShadow(Float val) { _Data._LightEnvReflectionIntensityShadow = val; }
    inline void SetLightEnvSaturation(Float val) { _Data._LightEnvSaturation = val; }
    inline void SetLightEnvTint(Colour val) { _Data._LightEnvTint = val; }
    inline void SetLightEnvBackgroundColor(Colour val) { _Data._LightEnvBackgroundColor = val; }
    inline void SetLightEnvProbeResolutionXZ(Vector2 val) { _Data._LightEnvProbeResolutionXZ = val; }
    inline void SetLightEnvProbeResolutionY(Float val) { _Data._LightEnvProbeResolutionY = val; }
    inline void SetLightEnvShadowMomentBias(Float val) { _Data._LightEnvShadowMomentBias = val; }
    inline void SetLightEnvShadowDepthBias(Float val) { _Data._LightEnvShadowDepthBias = val; }
    inline void SetLightEnvShadowPositionOffsetBias(Float val) { _Data._LightEnvShadowPositionOffsetBias = val; }
    inline void SetLightEnvShadowLightBleedReduction(Float val) { _Data._LightEnvShadowLightBleedReduction = val; }
    inline void SetLightEnvShadowMinDistance(Float val) { _Data._LightEnvShadowMinDistance = val; }
    inline void SetLightEnvShadowMaxDistance(Float val) { _Data._LightEnvShadowMaxDistance = val; }
    inline void SetLightEnvDynamicShadowMaxDistance(Float val) { _Data._LightEnvDynamicShadowMaxDistance = val; }
    inline void SetLightEnvShadowCascadeSplitBias(Float val) { _Data._LightEnvShadowCascadeSplitBias = val; }
    inline void SetLightEnvShadowAutoDepthBounds(Bool val) { _Data._LightEnvShadowAutoDepthBounds = val; }
    inline void SetLightEnvShadowMaxUpdates(I32 val) { _Data._LightEnvShadowMaxUpdates = val; }
    inline void SetLightShadowTraceMaxDistance(Float val) { _Data._LightShadowTraceMaxDistance = val; }
    inline void SetLightStaticShadowBoundsMin(Float val) { _Data._LightStaticShadowBoundsMin = val; }
    inline void SetLightStaticShadowBoundsMax(Float val) { _Data._LightStaticShadowBoundsMax = val; }
    inline void SetEnvLightShadowGoboTexture(Symbol val) { _Data._EnvLightShadowGoboTexture = val; }
    inline void SetFXBrushDOFEnable(Bool val) { _Data._FXBrushDOFEnable = val; }
    inline void SetFXBrushOutlineEnable(Bool val) { _Data._FXBrushOutlineEnable = val; }
    inline void SetFXBrushOutlineFilterEnable(Bool val) { _Data._FXBrushOutlineFilterEnable = val; }
    inline void SetFXBrushOutlineSize(Float val) { _Data._FXBrushOutlineSize = val; }
    inline void SetFXBrushOutlineThreshold(Float val) { _Data._FXBrushOutlineThreshold = val; }
    inline void SetFXBrushOutlineColorThreshold(Float val) { _Data._FXBrushOutlineColorThreshold = val; }
    inline void SetFXBrushOutlineFalloff(Float val) { _Data._FXBrushOutlineFalloff = val; }
    inline void SetFXBrushNearOutlineScale(Float val) { _Data._FXBrushNearOutlineScale = val; }
    inline void SetFXBrushNearTexture(Symbol val) { _Data._FXBrushNearTexture = val; }
    inline void SetFXBrushFarTexture(Symbol val) { _Data._FXBrushFarTexture = val; }
    inline void SetFXBrushNearScale(Float val) { _Data._FXBrushNearScale = val; }
    inline void SetFXBrushNearDetail(Float val) { _Data._FXBrushNearDetail = val; }
    inline void SetFXBrushFarScale(Float val) { _Data._FXBrushFarScale = val; }
    inline void SetFXBrushFarDetail(Float val) { _Data._FXBrushFarDetail = val; }
    inline void SetFXBrushFarScaleBoost(Float val) { _Data._FXBrushFarScaleBoost = val; }
    inline void SetFXBrushFarPlane(Float val) { _Data._FXBrushFarPlane = val; }
    inline void SetFXBrushFarPlaneFalloff(Float val) { _Data._FXBrushFarPlaneFalloff = val; }
    inline void SetFXBrushFarPlaneMaxScale(Float val) { _Data._FXBrushFarPlaneMaxScale = val; }
    inline void SetFrameBufferScaleOverride(Float val) { _Data._FrameBufferScaleOverride = val; }
    inline void SetFrameBufferScaleFactor(Float val) { _Data._FrameBufferScaleFactor = val; }
    inline void SetSpecularMultiplierEnabled(Bool val) { _Data._SpecularMultiplierEnabled = val; }
    inline void SetSpecularColorMultiplier(Colour val) { _Data._SpecularColorMultiplier = val; }
    inline void SetSpecularIntensityMultiplier(Float val) { _Data._SpecularIntensityMultiplier = val; }
    inline void SetSpecularExponentMultiplier(Float val) { _Data._SpecularExponentMultiplier = val; }
    inline void SetHDRLightmapsEnabled(Bool val) { _Data._HDRLightmapsEnabled = val; }
    inline void SetHDRLightmapsIntensity(Float val) { _Data._HDRLightmapsIntensity = val; }
    inline void SetCameraCutPositionThreshold(Float val) { _Data._CameraCutPositionThreshold = val; }
    inline void SetCameraCutRotationThreshold(Float val) { _Data._CameraCutRotationThreshold = val; }
    inline void SetViewportScissorLeft(Float val) { _Data._ViewportScissorLeft = val; }
    inline void SetViewportScissorTop(Float val) { _Data._ViewportScissorTop = val; }
    inline void SetViewportScissorRight(Float val) { _Data._ViewportScissorRight = val; }
    inline void SetViewportScissorBottom(Float val) { _Data._ViewportScissorBottom = val; }
    inline void SetScenePhysicsData(Symbol val) { _Data._ScenePhysicsData = val; }

    inline SceneInstData()
    {
        memset(&_Data, 0, sizeof(PODData));
    }

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

    void OnModuleRemove(SceneAgent* pAttachedAgent);

};

struct LightInstance
{

    struct PODData
    {

        // Colors
        Colour _LightColour;
        Colour _LightColourDark;
        Colour _Cell0Color;
        Colour _Cell1Color;
        Colour _Cell2Color;
        Colour _Cell3Color;

        // Intensity
        Float _LightIntensity;
        Float _LightIntensityDiffuse;
        Float _LightIntensitySpecular;
        Float _NPRSpecularIntensity;

        // Distance
        Float _LightMaxDistance;
        Float _LightMinDistance;

        // Shadows
        Float _LightShadowMax;
        Float _LightShadowDistanceFalloff;
        I32   _LightShadowCascades;
        Float _LightShadowBias;

        // General properties
        Float _LightDimmer;
        Float _LightColorCorrection;
        Float   _LightToonPriority;
        Float _LightToonOpacity;
        I32   _LightType; //   LightType ENUM
        Bool  _LightKeyLight;
        Bool  _DynamicOnLightMap;
        Bool  _LightTurnedOn;
        Float  _LightWrapAround;
        I32 _LightRenderLayer;

        // Spot light properties                   
        Float _LightSpotInnerRadius;
        Float _LightSpotOuterRadius;
        Symbol _LightSpotTexture;
        I32   _LightSpotAlphaMode;
        Float _LightSpotAlpha;
        Vector2 _LightSpotTextureTranslate;
        Vector2 _LightSpotTextureScale;
        Vector2 _LightSpotTextureShear;
        Vector2 _LightSpotTextureShearOrigin;
        Float _LightSpotTextureRotate;
        Vector2 _LightSpotTextureRotateOrigin;

        // Ambient and Rim                         
        Float _LightAmbientOcclusion;
        Float _LightRimIntensity;
        Float _LightRimWrapAround;
        Float _LightRimOcclusion;

        // Cell shading
        I32   _CellBlendMode; // EnumLightCellBlendMode ENUM x
        Float _CellBlendWeight;
        Float  _CellLightBlendMask;
        Bool  _LightStatic;

    };

    void SetLightColour(Colour c);
    void SetLightColourDark(Colour c);
    void SetCell0Color(Colour c);
    void SetCell1Color(Colour c);
    void SetCell2Color(Colour c);
    void SetCell3Color(Colour c);

    void SetLightIntensity(Float f);
    void SetLightIntensityDiffuse(Float f);
    void SetLightIntensitySpecular(Float f);
    void SetNPRSpecularIntensity(Float f);

    void SetLightMaxDistance(Float f);
    void SetLightMinDistance(Float f);

    void SetLightShadowMax(Float f);
    void SetLightShadowDistanceFalloff(Float f);
    void SetLightShadowCascades(I32 i);
    void SetLightShadowBias(Float f);

    void SetLightDimmer(Float f);
    void SetLightColorCorrection(Float f);
    void SetLightToonPriority(Float f);
    void SetLightToonOpacity(Float f);
    void SetLightType(I32 i);
    void SetLightKeyLight(Bool b);
    void SetDynamicOnLightMap(Bool b);
    void SetLightTurnedOn(Bool b);
    void SetLightWrapAround(Float f);
    void SetLightRenderLayer(I32 i);

    void SetLightSpotInnerRadius(Float f);
    void SetLightSpotOuterRadius(Float f);
    void SetLightSpotTexture(Symbol s);
    void SetLightSpotAlphaMode(I32 i);
    void SetLightSpotAlpha(Float f);
    void SetLightSpotTextureTranslate(Vector2 v);
    void SetLightSpotTextureScale(Vector2 v);
    void SetLightSpotTextureShear(Vector2 v);
    void SetLightSpotTextureShearOrigin(Vector2 v);
    void SetLightSpotTextureRotate(Float f);
    void SetLightSpotTextureRotateOrigin(Vector2 v);

    void SetLightAmbientOcclusion(Float f);
    void SetLightRimIntensity(Float f);
    void SetLightRimWrapAround(Float f);
    void SetLightRimOcclusion(Float f);

    void SetCellBlendMode(I32 i);
    void SetCellBlendWeight(Float f);
    void SetCellLightBlendMask(Float f);
    void SetLightStatic(Bool b);

    PODData _Data;

    std::vector<String> _Groups;

    inline LightInstance()
    {
        memset(&_Data, 0, sizeof(PODData));
    }

};

template<> struct SceneModule<SceneModuleType::LIGHT> : SceneModuleBase
{

    static constexpr SceneModuleType ModuleType = SceneModuleType::LIGHT;
    static constexpr CString ModuleID = "light";
    static constexpr CString ModuleName = "Light";

    static inline String GetModulePropertySet()
    {
        return kLightPropName;
    }

    Ptr<LightInstance> LightInst;

    // in modules.cpp
    void OnSetupAgent(SceneAgent* pAgentGettingCreated);

    void OnModuleRemove(SceneAgent* pAttachedAgent);

};

template<> struct SceneModule<SceneModuleType::SELECTABLE> : SceneModuleBase
{

    static constexpr SceneModuleType ModuleType = SceneModuleType::SELECTABLE;
    static constexpr CString ModuleID = "selectable";
    static constexpr CString ModuleName = "Selectable";

    static inline String GetModulePropertySet()
    {
        return kSelectablePropName;
    }

    BoundingBox BBox; // in which a click counts towards this selectable
    Bool GameSelectable = false; // currently on?
    
    void SetExtentsMin(Vector3 vec);
    void SetExtentsMax(Vector3 vec);
    void SetGameSelectable(Bool onOff);

    // in modules.cpp
    void OnSetupAgent(SceneAgent* pAgentGettingCreated);

    void OnModuleRemove(SceneAgent* pAttachedAgent);

};

// for when an agent is hovered over, show some mesh + text
template<> struct SceneModule<SceneModuleType::ROLLOVER> : SceneModuleBase
{

    static constexpr SceneModuleType ModuleType = SceneModuleType::ROLLOVER;
    static constexpr CString ModuleID = "rollover";
    static constexpr CString ModuleName = "Rollover";

    static inline String GetModulePropertySet()
    {
        return kRolloverPropName;
    }

    Symbol CursorPropHandle;
    Symbol MeshHandle;
    String Text;
    Colour TextBackground;
    Colour TextForeground;
    
    void SetCursorProps(Symbol v);
    void SetCursorMesh(Symbol v);
    void SetText(String v);
    void SetTextBackground(Colour c);
    void SetTextColour(Colour c);

    // in modules.cpp
    void OnSetupAgent(SceneAgent* pAgentGettingCreated);

    void OnModuleRemove(SceneAgent* pAttachedAgent);

};

// derives from text and selectable. so if this module is present, it will contain those too (the module prop should have those parents!)
template<> struct SceneModule<SceneModuleType::DIALOG_CHOICE> : SceneModuleBase
{

    static constexpr SceneModuleType ModuleType = SceneModuleType::DIALOG_CHOICE;
    static constexpr CString ModuleID = "dlgChoice";
    static constexpr CString ModuleName = "Dialog Choice";

    static inline String GetModulePropertySet()
    {
        return kDialogChoicePropName;
    }

    String Choice;
    
    void SetChoice(String ch);

    // in modules.cpp
    void OnSetupAgent(SceneAgent* pAgentGettingCreated);

    void OnModuleRemove(SceneAgent* pAttachedAgent);

};

template<> struct SceneModule<SceneModuleType::DIALOG> : SceneModuleBase
{

    static constexpr SceneModuleType ModuleType = SceneModuleType::DIALOG;
    static constexpr CString ModuleID = "dialog";
    static constexpr CString ModuleName = "Dialog";

    static inline String GetModulePropertySet()
    {
        return kDialogPropName;
    }

    String Name;
    Symbol Resource;
    
    void SetName(String v);
    void SetResource(Symbol v);

    // in modules.cpp
    void OnSetupAgent(SceneAgent* pAgentGettingCreated);

    void OnModuleRemove(SceneAgent* pAttachedAgent);

};

template<> struct SceneModule<SceneModuleType::WALK_ANIMATOR> : SceneModuleBase
{

    static constexpr SceneModuleType ModuleType = SceneModuleType::WALK_ANIMATOR;
    static constexpr CString ModuleID = "walkAnimator";
    static constexpr CString ModuleName = "Walk Animator";

    static inline String GetModulePropertySet()
    {
        return kWalkAnimatorPropName;
    }

    Symbol ForwardAnim, IdleAnim;
    
    void SetIdleAnim(Symbol v);
    void SetForwardAnim(Symbol v);

    // in modules.cpp
    void OnSetupAgent(SceneAgent* pAgentGettingCreated);

    void OnModuleRemove(SceneAgent* pAttachedAgent);

};

template<> struct SceneModule<SceneModuleType::TRIGGER> : SceneModuleBase
{

    static constexpr SceneModuleType ModuleType = SceneModuleType::TRIGGER;
    static constexpr CString ModuleID = "trigger";
    static constexpr CString ModuleName = "Trigger";

    static inline String GetModulePropertySet()
    {
        return kTriggerPropName;
    }

    String EnterCallback, ExitCallback;
    Bool Enabled = false;
    
    void SetEnterCallback(String v);
    void SetExitCallback(String v);
    void SetOnOff(Bool v);

    // in modules.cpp
    void OnSetupAgent(SceneAgent* pAgentGettingCreated);

    void OnModuleRemove(SceneAgent* pAttachedAgent);

};

template<> struct SceneModule<SceneModuleType::NAV_CAM> : SceneModuleBase
{

    static constexpr SceneModuleType ModuleType = SceneModuleType::NAV_CAM;
    static constexpr CString ModuleID = "navCam";
    static constexpr CString ModuleName = "Navigation Camera";

    static inline String GetModulePropertySet()
    {
        return kNavCamPropName;
    }
    
    NavCamMode NavMode = NavCamMode::NONE;
    Symbol AnimationFile;
    Float AnimationTime = 0.0f;
    Float Dampen = 0.0f;
    Float TriggerHPercent = 0.0f, TriggerVPercent = 0.0f;
    String TargetAgent;
    Vector3 TargetOffset;
    Polar OrbitMin, OrbitMax, OrbitOffset;
    
    void SetMode(Enum<NavCamMode> mode); // ENUM: so we use the Enum struct type wrapper which has traits and meta conversion.
    void SetAnimationFile(Symbol file);
    void SetAnimationTime(Float time);
    void SetDampen(Float v);
    void SetTriggerHPercent(Float v);
    void SetTriggerVPercent(Float v);
    void SetTargetAgent(String v);
    void SetTargetOffset(Vector3 v);
    void SetOrbitMin(Polar v);
    void SetOrbitMax(Polar v);
    void SetOrbitOffset(Polar v);
    
    // in modules.cpp
    void OnSetupAgent(SceneAgent* pAgentGettingCreated);

    void OnModuleRemove(SceneAgent* pAttachedAgent);

};

template<> struct SceneModule<SceneModuleType::PATH_TO> : SceneModuleBase
{
    
    static constexpr SceneModuleType ModuleType = SceneModuleType::PATH_TO;
    static constexpr CString ModuleID = "pathTo";
    static constexpr CString ModuleName = "Pathing To";
    
    static inline String GetModulePropertySet()
    {
        return kPathToPropName;
    }
    
    Float WalkRadius = 0.0f;
    
    void SetWalkRadius(Float v);
    
    // in modules.cpp
    void OnSetupAgent(SceneAgent* pAgentGettingCreated);

    void OnModuleRemove(SceneAgent* pAttachedAgent);
    
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
