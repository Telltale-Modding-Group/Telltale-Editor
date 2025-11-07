#include <Common/Scene.hpp>
#include <Resource/PropertySet.hpp>
#include <Renderer/Text.hpp>

// ============================================== MODULE IMPLEMENTATION ==============================================
// ================================================== CAMERA MODULE ==================================================
// ============================================== MODULE IMPLEMENTATION ==============================================

#define TRACKING_REF_CAMERA 0xDEADBEEF

void Camera::PushCamera(bool onOff)
{
    if(_BPushed != onOff)
    {
        if(onOff)
        {
            // push cam to scene
            AttachScene->_ViewStack.push_back(weak_from_this());
        }
        else
        {
            // remove cam
            for(auto it = AttachScene->_ViewStack.begin(); it != AttachScene->_ViewStack.end(); it++)
            {
                if(it->lock().get() == this)
                {
                    AttachScene->_ViewStack.erase(it);
                    break;
                }
            }
        }
    }
}

void SceneModule<SceneModuleType::CAMERA>::OnSetupAgent(SceneAgent* pAgentGettingCreated)
{
    // Setup callbacks
    if(!Cam)
    {
        Cam = TTE_NEW_PTR(Camera, MEMORY_TAG_SCENE_DATA);
        Cam->AttachScene = pAgentGettingCreated->OwningScene;

        Meta::ClassInstance props = pAgentGettingCreated->OwningScene->GetAgentProps(pAgentGettingCreated->NameSymbol);

        PropertySet::AddCallback(props, kFieldOfView, ALLOCATE_METHOD_CALLBACK_1(Cam, SetHFOV, Camera, float), TRACKING_REF_CAMERA);
        PropertySet::AddCallback(props, kFieldOfViewScale, ALLOCATE_METHOD_CALLBACK_1(Cam, SetHFOVScale, Camera, float), TRACKING_REF_CAMERA);
        PropertySet::AddCallback(props, kAspectRatio, ALLOCATE_METHOD_CALLBACK_1(Cam, SetAspectRatio, Camera, float), TRACKING_REF_CAMERA);
        PropertySet::AddCallback(props, kClipPlaneNear, ALLOCATE_METHOD_CALLBACK_1(Cam, SetNearClip, Camera, float), TRACKING_REF_CAMERA);
        PropertySet::AddCallback(props, kClipPlaneFar, ALLOCATE_METHOD_CALLBACK_1(Cam, SetFarClip, Camera, float), TRACKING_REF_CAMERA);
        PropertySet::AddCallback(props, kCameraBlendEnable, ALLOCATE_METHOD_CALLBACK_1(Cam, SetAllowBlending, Camera, bool), TRACKING_REF_CAMERA);
        PropertySet::AddCallback(props, kCullObjects, ALLOCATE_METHOD_CALLBACK_1(Cam, SetCullObjects, Camera, bool), TRACKING_REF_CAMERA);
        PropertySet::AddCallback(props, kDepthOfFieldEnabled, ALLOCATE_METHOD_CALLBACK_1(Cam, SetDOFEnabled, Camera, bool), TRACKING_REF_CAMERA);
        PropertySet::AddCallback(props, kUseHighQualityDOF, ALLOCATE_METHOD_CALLBACK_1(Cam, SetUseHQDOF, Camera, bool), TRACKING_REF_CAMERA);
        PropertySet::AddCallback(props, kDepthOfFieldNear, ALLOCATE_METHOD_CALLBACK_1(Cam, SetDOFNear, Camera, float), TRACKING_REF_CAMERA);
        PropertySet::AddCallback(props, kDepthOfFieldFar, ALLOCATE_METHOD_CALLBACK_1(Cam, SetDOFFar, Camera, float), TRACKING_REF_CAMERA);
        PropertySet::AddCallback(props, kDepthOfFieldFallOffNear, ALLOCATE_METHOD_CALLBACK_1(Cam, SetDOFNearFallOff, Camera, float), TRACKING_REF_CAMERA);
        PropertySet::AddCallback(props, kDepthOfFieldFallOffFar, ALLOCATE_METHOD_CALLBACK_1(Cam, SetDOFFarFallOff, Camera, float), TRACKING_REF_CAMERA);
        PropertySet::AddCallback(props, kDepthOfFieldMaxNear, ALLOCATE_METHOD_CALLBACK_1(Cam, SetDOFNearMax, Camera, float), TRACKING_REF_CAMERA);
        PropertySet::AddCallback(props, kDepthOfFieldMaxFar, ALLOCATE_METHOD_CALLBACK_1(Cam, SetDOFFarMax, Camera, float), TRACKING_REF_CAMERA);
        PropertySet::AddCallback(props, kDepthOfFieldDebug, ALLOCATE_METHOD_CALLBACK_1(Cam, SetDOFDebug, Camera, float), TRACKING_REF_CAMERA);
        PropertySet::AddCallback(props, kDepthOfFieldCoverageBoost, ALLOCATE_METHOD_CALLBACK_1(Cam, SetDOFCoverageBoost, Camera, float), TRACKING_REF_CAMERA);
        PropertySet::AddCallback(props, kBokehPatternTexture, ALLOCATE_METHOD_CALLBACK_1(Cam, SetBokehTexture, Camera, Symbol), TRACKING_REF_CAMERA);
        PropertySet::AddCallback(props, kUseBokeh, ALLOCATE_METHOD_CALLBACK_1(Cam, SetUseBokeh, Camera, bool), TRACKING_REF_CAMERA);
        PropertySet::AddCallback(props, kBokehBrightnessThreshold, ALLOCATE_METHOD_CALLBACK_1(Cam, SetBokehBrightnessThreshold, Camera, float), TRACKING_REF_CAMERA);
        PropertySet::AddCallback(props, kBokehBrightnessDeltaThreshold, ALLOCATE_METHOD_CALLBACK_1(Cam, SetBokehBrightnessDeltaThreshold, Camera, float), TRACKING_REF_CAMERA);
        PropertySet::AddCallback(props, kBokehBlurThreshold, ALLOCATE_METHOD_CALLBACK_1(Cam, SetBokehBlurThreshold, Camera, float), TRACKING_REF_CAMERA);
        PropertySet::AddCallback(props, kBokehMinSize, ALLOCATE_METHOD_CALLBACK_1(Cam, SetBokehMinSize, Camera, float), TRACKING_REF_CAMERA);
        PropertySet::AddCallback(props, kBokehMaxSize, ALLOCATE_METHOD_CALLBACK_1(Cam, SetBokehMaxSize, Camera, float), TRACKING_REF_CAMERA);
        PropertySet::AddCallback(props, kBokehFalloff, ALLOCATE_METHOD_CALLBACK_1(Cam, SetBokehFalloff, Camera, float), TRACKING_REF_CAMERA);
        PropertySet::AddCallback(props, kBokehAberrationOffsetsX, ALLOCATE_METHOD_CALLBACK_1(Cam, SetBokehAberrationOffsetsX, Camera, Vector3), TRACKING_REF_CAMERA);
        PropertySet::AddCallback(props, kBokehAberrationOffsetsY, ALLOCATE_METHOD_CALLBACK_1(Cam, SetBokehAberrationOffsetsY, Camera, Vector3), TRACKING_REF_CAMERA);
        PropertySet::AddCallback(props, kMaxBokehBufferAmount, ALLOCATE_METHOD_CALLBACK_1(Cam, SetMaxBokehBufferVertexAmount, Camera, float), TRACKING_REF_CAMERA);
        PropertySet::AddCallback(props, kExposure, ALLOCATE_METHOD_CALLBACK_1(Cam, SetExposure, Camera, float), TRACKING_REF_CAMERA);
        PropertySet::AddCallback(props, kFXColourEnabled, ALLOCATE_METHOD_CALLBACK_1(Cam, SetFXColourActive, Camera, bool), TRACKING_REF_CAMERA);
        PropertySet::AddCallback(props, kFXColourTint, ALLOCATE_METHOD_CALLBACK_1(Cam, SetFXColourTint, Camera, float), TRACKING_REF_CAMERA);
        PropertySet::AddCallback(props, kFXColourOpacity, ALLOCATE_METHOD_CALLBACK_1(Cam, SetFXColourOpacity, Camera, float), TRACKING_REF_CAMERA);
        PropertySet::AddCallback(props, kFXLevelsEnabled, ALLOCATE_METHOD_CALLBACK_1(Cam, SetFXLevelsActive, Camera, bool), TRACKING_REF_CAMERA);
        PropertySet::AddCallback(props, kFXLevelsWhitePoint, ALLOCATE_METHOD_CALLBACK_1(Cam, SetFXLevelsWhite, Camera, float), TRACKING_REF_CAMERA);
        PropertySet::AddCallback(props, kFXLevelsBlackPoint, ALLOCATE_METHOD_CALLBACK_1(Cam, SetFXLevelsBlack, Camera, float), TRACKING_REF_CAMERA);
        PropertySet::AddCallback(props, kFXLevelsIntensity, ALLOCATE_METHOD_CALLBACK_1(Cam, SetFXLevelsIntensity, Camera, float), TRACKING_REF_CAMERA);
        PropertySet::AddCallback(props, kFxRadialBlurEnabled, ALLOCATE_METHOD_CALLBACK_1(Cam, SetFXRadialBlurActive, Camera, bool), TRACKING_REF_CAMERA);
        PropertySet::AddCallback(props, kFxRadialBlurIntensity, ALLOCATE_METHOD_CALLBACK_1(Cam, SetFXRadialBlurIntensity, Camera, float), TRACKING_REF_CAMERA);
        PropertySet::AddCallback(props, kFxRadialBlurInRadius, ALLOCATE_METHOD_CALLBACK_1(Cam, SetFXRadialBlurInRadius, Camera, float), TRACKING_REF_CAMERA);
        PropertySet::AddCallback(props, kFxRadialBlurOutRadius, ALLOCATE_METHOD_CALLBACK_1(Cam, SetFXRadialBlurOutRadius, Camera, float), TRACKING_REF_CAMERA);
        PropertySet::AddCallback(props, kFxRadialBlurTint, ALLOCATE_METHOD_CALLBACK_1(Cam, SetFXRadialBlurTint, Camera, Colour), TRACKING_REF_CAMERA);
        PropertySet::AddCallback(props, kFxRadialBlurTintIntensity, ALLOCATE_METHOD_CALLBACK_1(Cam, SetFXRadialBlurTintIntensity, Camera, float), TRACKING_REF_CAMERA);
        PropertySet::AddCallback(props, kFxRadialBlurScale, ALLOCATE_METHOD_CALLBACK_1(Cam, SetFXRadialBlurScale, Camera, float), TRACKING_REF_CAMERA);
        PropertySet::AddCallback(props, kFxMotionBlurEnabled, ALLOCATE_METHOD_CALLBACK_1(Cam, SetFXMotionBlurActive, Camera, bool), TRACKING_REF_CAMERA);
        PropertySet::AddCallback(props, kFxMotionBlurIntensity, ALLOCATE_METHOD_CALLBACK_1(Cam, SetFXMotionBlurIntensity, Camera, float), TRACKING_REF_CAMERA);
        PropertySet::AddCallback(props, kFxMotionBlurMovementThresholdEnabled, ALLOCATE_METHOD_CALLBACK_1(Cam, SetFXMotionBlurMovementThresholdActive, Camera, bool), TRACKING_REF_CAMERA);
        PropertySet::AddCallback(props, kFxMotionBlurMovementThreshold, ALLOCATE_METHOD_CALLBACK_1(Cam, SetFXMotionBlurMovementThreshold, Camera, float), TRACKING_REF_CAMERA);
        PropertySet::AddCallback(props, kFxMotionBlurRotationThresholdEnabled, ALLOCATE_METHOD_CALLBACK_1(Cam, SetFXMotionBlurRotationThresholdActive, Camera, bool), TRACKING_REF_CAMERA);
        PropertySet::AddCallback(props, kFxMotionBlurRotationThreshold, ALLOCATE_METHOD_CALLBACK_1(Cam, SetFXMotionBlurRotationThreshold, Camera, float), TRACKING_REF_CAMERA);
        PropertySet::AddCallback(props, kFxDelayMotionBlur, ALLOCATE_METHOD_CALLBACK_1(Cam, SetFXMotionBlurDelay, Camera, bool), TRACKING_REF_CAMERA);
        PropertySet::AddCallback(props, kAudioListenerOverride, ALLOCATE_METHOD_CALLBACK_1(Cam, SetAudioListenerOverrideAgent, Camera, String), TRACKING_REF_CAMERA);
        PropertySet::AddCallback(props, kAudioPlayerOriginOverride, ALLOCATE_METHOD_CALLBACK_1(Cam, SetPlayerOriginOverrideAgent, Camera, String), TRACKING_REF_CAMERA);
        PropertySet::AddCallback(props, kAudioReverbOverride, ALLOCATE_METHOD_CALLBACK_1(Cam, SetAudioReverbOverride, Camera, Symbol), TRACKING_REF_CAMERA);

        // pushes a cam to the scene. in lua, agentProps["Camera Push"] = true/false will do this if the agent is a camera!
        PropertySet::AddCallback(props, kCameraPush, ALLOCATE_METHOD_CALLBACK_1(Cam, PushCamera, Camera, Bool));

        // TODO: audio snapshot event override, its a SoundEventName<SNAPSHOT>

        // This looks unused as in set from Lua. We could in the future if really needed to coerce to a set from a meta collection (which is coercable from any lua table)
        // PropertySet::AddCallback(props, kExcludeAgents, ALLOCATE_METHOD_CALLBACK_1(Cam, SetExcludeAgents, Camera, std::set<Symbol>));

        PropertySet::CallAllCallbacks(props, pAgentGettingCreated->OwningScene->GetRegistry());
    }
}

void SceneModule<SceneModuleType::CAMERA>::OnModuleRemove(SceneAgent *pAttachedAgent)
{
    Meta::ClassInstance props = pAttachedAgent->OwningScene->GetAgentProps(pAttachedAgent->NameSymbol);
    PropertySet::ClearCallbacks(props, TRACKING_REF_CAMERA);
}

// ============================================== MODULE IMPLEMENTATION ==============================================
// =================================================== TEXT MODULE ===================================================
// ============================================== MODULE IMPLEMENTATION ==============================================

#define TRACKING_REF_TEXT 0x1BADDEAD

// Getters
bool RenderText::GetWorldSpaceZ() const { return _WorldSpaceZ; }
float RenderText::GetWidth() const { return _Width; }
VerticalAlignmentType RenderText::GetVerticalAlignment() const { return _VAlignType; }
HorizontalAlignmentType RenderText::GetHorizontalAlignment() const { return _HAlignType; }
float RenderText::GetAlphaScale() const { return _AlphaScale; }
const String& RenderText::GetText() const { return _Text; }
float RenderText::GetSkew() const { return _Skew; }
float RenderText::GetShadowHeight() const { return _ShadowHeight; }
const Colour& RenderText::GetShadowColour() const { return _ShadowColour; }
const Vector3& RenderText::GetNonPropScale() const { return _NonPropScale; }
float RenderText::GetScale() const { return _Scale; }
const Vector2& RenderText::GetRefScreenSize() const { return _RefScreenSize; }
float RenderText::GetPlaybackSpeed() const { return _PlaybackSpeed; }
const Vector3& RenderText::GetOffset() const { return _Offset; }
float RenderText::GetPercentToDisplay() const { return _PercentToDisplay; }
float RenderText::GetMinWidth() const { return _MinWidth; }
float RenderText::GetMinHeight() const { return _MinHeight; }
I32 RenderText::GetMaxLinesToDisplay() const { return _MaxLinesToDisplay; }
float RenderText::GetLeading() const { return _Leading; }
float RenderText::GetKerning() const { return _Kerning; }
const Symbol& RenderText::GetFontFile() const { return _FontFile; }
float RenderText::GetExtrudeX() const { return _ExtrudeX; }
float RenderText::GetExtrudeY() const { return _ExtrudeY; }
const String& RenderText::GetDialogNodeName() const { return _DialogNodeName; }
const Symbol& RenderText::GetDialogFile() const { return _DialogFile; }
const String& RenderText::GetDialogTextResource() const { return _DialogTextResource; }
const Symbol& RenderText::GetDialogResFile() const { return _DialogResFile; }
I32 RenderText::GetCurrentDisplayPage() const { return _CurrentDisplayPage; }
const Colour& RenderText::GetTextColour() const { return _TextColour; }
const Colour& RenderText::GetTextBackgroundColor() const { return _TextBackgroundColor; }
float RenderText::GetTextBackgroundAlphaScale() const { return _TextBackgroundAlphaScale; }

// Setters
void RenderText::SetWorldSpaceZ(bool value) { _WorldSpaceZ = value; }
void RenderText::SetWidth(float value) { _Width = value; }
void RenderText::SetVerticalAlignment(Enum<VerticalAlignmentType> value) { _VAlignType = value.Value; }
void RenderText::SetHorizontalAlignment(Enum<HorizontalAlignmentType> value) { _HAlignType = value.Value; }
void RenderText::SetAlphaScale(float value) { _AlphaScale = value; }
void RenderText::SetText(String value) { _Text = value; }
void RenderText::SetSkew(float value) { _Skew = value; }
void RenderText::SetShadowHeight(float value) { _ShadowHeight = value; }
void RenderText::SetShadowColour(Colour value) { _ShadowColour = value; }
void RenderText::SetNonPropScale(Vector3 value) { _NonPropScale = value; }
void RenderText::SetScale(float value) { _Scale = value; }
void RenderText::SetRefScreenSize(Vector2 value) { _RefScreenSize = value; }
void RenderText::SetPlaybackSpeed(float value) { _PlaybackSpeed = value; }
void RenderText::SetOffset(Vector3 value) { _Offset = value; }
void RenderText::SetPercentToDisplay(float value) { _PercentToDisplay = value; }
void RenderText::SetMinWidth(float value) { _MinWidth = value; }
void RenderText::SetMinHeight(float value) { _MinHeight = value; }
void RenderText::SetMaxLinesToDisplay(I32 value) { _MaxLinesToDisplay = value; }
void RenderText::SetLeading(float value) { _Leading = value; }
void RenderText::SetKerning(float value) { _Kerning = value; }
void RenderText::SetFontFile(Symbol value) { _FontFile = value; }
void RenderText::SetExtrudeX(float value) { _ExtrudeX = value; }
void RenderText::SetExtrudeY(float value) { _ExtrudeY = value; }
void RenderText::SetDialogNodeName(String value) { _DialogNodeName = value; }
void RenderText::SetDialogFile(Symbol value) { _DialogFile = value; }
void RenderText::SetDialogTextResource(String value) { _DialogTextResource = value; }
void RenderText::SetDialogResFile(Symbol value) { _DialogResFile = value; }
void RenderText::SetCurrentDisplayPage(I32 value) { _CurrentDisplayPage = value; }
void RenderText::SetTextColour(Colour value) { _TextColour = value; }
void RenderText::SetTextBackgroundColor(Colour value) { _TextBackgroundColor = value; }
void RenderText::SetTextBackgroundAlphaScale(float value) { _TextBackgroundAlphaScale = value; }

void SceneModule<SceneModuleType::TEXT>::OnSetupAgent(SceneAgent* pAgentGettingCreated)
{
    if(!Text)
    {
        Text = TTE_NEW_PTR(RenderText, MEMORY_TAG_SCENE_DATA);

        Meta::ClassInstance props = pAgentGettingCreated->OwningScene->GetAgentProps(pAgentGettingCreated->NameSymbol);

        PropertySet::AddCallback(props, kTextWorldSpaceZ,            ALLOCATE_METHOD_CALLBACK_1(Text, SetWorldSpaceZ, RenderText, Bool), TRACKING_REF_TEXT);
        PropertySet::AddCallback(props, kTextWidth,                  ALLOCATE_METHOD_CALLBACK_1(Text, SetWidth, RenderText, float), TRACKING_REF_TEXT);
        PropertySet::AddCallback(props, kTextAlignmentVertical,      ALLOCATE_METHOD_CALLBACK_1(Text, SetVerticalAlignment, RenderText, Enum<VerticalAlignmentType>), TRACKING_REF_TEXT);
        PropertySet::AddCallback(props, kTextAlignmentHorizontal,    ALLOCATE_METHOD_CALLBACK_1(Text, SetHorizontalAlignment, RenderText, Enum<HorizontalAlignmentType>), TRACKING_REF_TEXT);
        PropertySet::AddCallback(props, kTextAlphaMultiply,          ALLOCATE_METHOD_CALLBACK_1(Text, SetAlphaScale, RenderText, float), TRACKING_REF_TEXT);
        PropertySet::AddCallback(props, kTextString,                 ALLOCATE_METHOD_CALLBACK_1(Text, SetText, RenderText, String), TRACKING_REF_TEXT);
        PropertySet::AddCallback(props, kTextSkew,                   ALLOCATE_METHOD_CALLBACK_1(Text, SetSkew, RenderText, float), TRACKING_REF_TEXT);
        PropertySet::AddCallback(props, kTextShadowHeight,           ALLOCATE_METHOD_CALLBACK_1(Text, SetShadowHeight, RenderText, float), TRACKING_REF_TEXT);
        PropertySet::AddCallback(props, kTextShadowColor,            ALLOCATE_METHOD_CALLBACK_1(Text, SetShadowColour, RenderText, Colour), TRACKING_REF_TEXT);
        PropertySet::AddCallback(props, kTextNonProportionalScale,   ALLOCATE_METHOD_CALLBACK_1(Text, SetNonPropScale, RenderText, Vector3), TRACKING_REF_TEXT);
        PropertySet::AddCallback(props, kTextScale,                  ALLOCATE_METHOD_CALLBACK_1(Text, SetScale, RenderText, float), TRACKING_REF_TEXT);
        PropertySet::AddCallback(props, kTextReferenceScreenSize,    ALLOCATE_METHOD_CALLBACK_1(Text, SetRefScreenSize, RenderText, Vector2), TRACKING_REF_TEXT);
        PropertySet::AddCallback(props, kTextPlaybackSpeed,          ALLOCATE_METHOD_CALLBACK_1(Text, SetPlaybackSpeed, RenderText, float), TRACKING_REF_TEXT);
        PropertySet::AddCallback(props, kTextOffset,                 ALLOCATE_METHOD_CALLBACK_1(Text, SetOffset, RenderText, Vector3), TRACKING_REF_TEXT);
        PropertySet::AddCallback(props, kTextPercentToDisplay,       ALLOCATE_METHOD_CALLBACK_1(Text, SetPercentToDisplay, RenderText, float), TRACKING_REF_TEXT);
        PropertySet::AddCallback(props, kTextMinWidth,               ALLOCATE_METHOD_CALLBACK_1(Text, SetMinWidth, RenderText, float), TRACKING_REF_TEXT);
        PropertySet::AddCallback(props, kTextMinHeight,              ALLOCATE_METHOD_CALLBACK_1(Text, SetMinHeight, RenderText, float), TRACKING_REF_TEXT);
        PropertySet::AddCallback(props, kTextMaxLinesToDisplay,      ALLOCATE_METHOD_CALLBACK_1(Text, SetMaxLinesToDisplay, RenderText, I32), TRACKING_REF_TEXT);
        PropertySet::AddCallback(props, kTextLeading,                ALLOCATE_METHOD_CALLBACK_1(Text, SetLeading, RenderText, float), TRACKING_REF_TEXT);
        PropertySet::AddCallback(props, kTextKerning,                ALLOCATE_METHOD_CALLBACK_1(Text, SetKerning, RenderText, float), TRACKING_REF_TEXT);
        PropertySet::AddCallback(props, kTextFont,                   ALLOCATE_METHOD_CALLBACK_1(Text, SetFontFile, RenderText, Symbol), TRACKING_REF_TEXT);
        PropertySet::AddCallback(props, kTextExtrudeX,               ALLOCATE_METHOD_CALLBACK_1(Text, SetExtrudeX, RenderText, float), TRACKING_REF_TEXT);
        PropertySet::AddCallback(props, kTextExtrudeY,               ALLOCATE_METHOD_CALLBACK_1(Text, SetExtrudeY, RenderText, float), TRACKING_REF_TEXT);
        PropertySet::AddCallback(props, kTextDialog2NodeName,        ALLOCATE_METHOD_CALLBACK_1(Text, SetDialogNodeName, RenderText, String), TRACKING_REF_TEXT);
        PropertySet::AddCallback(props, kTextDialogFile,             ALLOCATE_METHOD_CALLBACK_1(Text, SetDialogFile, RenderText, Symbol), TRACKING_REF_TEXT);
        PropertySet::AddCallback(props, kTextDialogTextResource,     ALLOCATE_METHOD_CALLBACK_1(Text, SetDialogTextResource, RenderText, String), TRACKING_REF_TEXT);
        PropertySet::AddCallback(props, kTextDialog2File,            ALLOCATE_METHOD_CALLBACK_1(Text, SetDialogResFile, RenderText, Symbol), TRACKING_REF_TEXT);
        PropertySet::AddCallback(props, kTextRenderLayer,            ALLOCATE_METHOD_CALLBACK_1(Text, SetCurrentDisplayPage, RenderText, I32), TRACKING_REF_TEXT);
        PropertySet::AddCallback(props, kTextColor,                  ALLOCATE_METHOD_CALLBACK_1(Text, SetTextColour, RenderText, Colour), TRACKING_REF_TEXT);
        PropertySet::AddCallback(props, kTextBackgroundColor,        ALLOCATE_METHOD_CALLBACK_1(Text, SetTextBackgroundColor, RenderText, Colour), TRACKING_REF_TEXT);
        PropertySet::AddCallback(props, kTextBackgroundAlphaMultiply,ALLOCATE_METHOD_CALLBACK_1(Text, SetTextBackgroundAlphaScale, RenderText, float), TRACKING_REF_TEXT);

        PropertySet::CallAllCallbacks(props, pAgentGettingCreated->OwningScene->GetRegistry());

    }
}

void SceneModule<SceneModuleType::TEXT>::OnModuleRemove(SceneAgent *pAttachedAgent)
{
    Meta::ClassInstance props = pAttachedAgent->OwningScene->GetAgentProps(pAttachedAgent->NameSymbol);
    PropertySet::ClearCallbacks(props, TRACKING_REF_TEXT);
}

// ============================================== MODULE IMPLEMENTATION ==============================================
// ================================================== SCENE  MODULE ==================================================
// ============================================== MODULE IMPLEMENTATION ==============================================

#define TRACKING_REF_SCENE 0xBEEFB00B

void SceneModule<SceneModuleType::SCENE>::OnSetupAgent(SceneAgent* pAgentGettingCreated)
{
    if(!SceneData)
    {
        SceneData = TTE_NEW_PTR(SceneInstData, MEMORY_TAG_SCENE_DATA);
        Meta::ClassInstance props = pAgentGettingCreated->OwningScene->GetAgentProps(pAgentGettingCreated->NameSymbol);
        PropertySet::AddCallback(props, kSceneAmbientColor, ALLOCATE_METHOD_CALLBACK_1(SceneData, SetAmbientColor, SceneInstData, Colour), TRACKING_REF_SCENE);
        PropertySet::AddCallback(props, kSceneShadowColor, ALLOCATE_METHOD_CALLBACK_1(SceneData, SetShadowColor, SceneInstData, Colour), TRACKING_REF_SCENE);
        PropertySet::AddCallback(props, kSceneActiveCamera, ALLOCATE_METHOD_CALLBACK_1(SceneData, SetActiveCamera, SceneInstData, String), TRACKING_REF_SCENE);
        PropertySet::AddCallback(props, kSceneWalkBoxes, ALLOCATE_METHOD_CALLBACK_1(SceneData, SetWalkBoxes, SceneInstData, Symbol), TRACKING_REF_SCENE);
        PropertySet::AddCallback(props, kSceneFootstepWalkBoxes, ALLOCATE_METHOD_CALLBACK_1(SceneData, SetFootstepWalkBoxes, SceneInstData, Symbol), TRACKING_REF_SCENE);
        PropertySet::AddCallback(props, kSceneRenderPriority, ALLOCATE_METHOD_CALLBACK_1(SceneData, SetSceneRenderPriority, SceneInstData, I32), TRACKING_REF_SCENE);
        PropertySet::AddCallback(props, kSceneRenderLayer, ALLOCATE_METHOD_CALLBACK_1(SceneData, SetSceneRenderLayer, SceneInstData, I32), TRACKING_REF_SCENE);
        PropertySet::AddCallback(props, kSceneExcludeFromSaveGames, ALLOCATE_METHOD_CALLBACK_1(SceneData, SetExcludeFromSaveGames, SceneInstData, Bool), TRACKING_REF_SCENE);
        PropertySet::AddCallback(props, kSceneTimeScale, ALLOCATE_METHOD_CALLBACK_1(SceneData, SetSceneTimeScale, SceneInstData, Float), TRACKING_REF_SCENE);
        PropertySet::AddCallback(props, kSceneInputEnabled, ALLOCATE_METHOD_CALLBACK_1(SceneData, SetSceneInputEnabled, SceneInstData, Bool), TRACKING_REF_SCENE);
        PropertySet::AddCallback(props, kSceneAudioListener, ALLOCATE_METHOD_CALLBACK_1(SceneData, SetSceneAudioListener, SceneInstData, String), TRACKING_REF_SCENE);
        PropertySet::AddCallback(props, kSceneAudioPlayerOrigin, ALLOCATE_METHOD_CALLBACK_1(SceneData, SetSceneAudioPlayerOrigin, SceneInstData, String), TRACKING_REF_SCENE);
        PropertySet::AddCallback(props, kSceneAudioMaster, ALLOCATE_METHOD_CALLBACK_1(SceneData, SetSceneAudioMaster, SceneInstData, Float), TRACKING_REF_SCENE);
        PropertySet::AddCallback(props, kSceneAudioSnapshotSuite, ALLOCATE_METHOD_CALLBACK_1(SceneData, SetSceneAudioSnapshotSuite, SceneInstData, Symbol), TRACKING_REF_SCENE);
        PropertySet::AddCallback(props, kSceneAudioReverbDefinition, ALLOCATE_METHOD_CALLBACK_1(SceneData, SetSceneAudioReverbDefinition, SceneInstData, Symbol), TRACKING_REF_SCENE);
        PropertySet::AddCallback(props, kSceneAudioReverb, ALLOCATE_METHOD_CALLBACK_1(SceneData, SetSceneAudioReverb, SceneInstData, Float), TRACKING_REF_SCENE);
        PropertySet::AddCallback(props, kSceneAudioEventBanks, ALLOCATE_METHOD_CALLBACK_1(SceneData, SetSceneAudioEventBanks, SceneInstData, std::vector<Symbol>), TRACKING_REF_SCENE);
        PropertySet::AddCallback(props, kScenePreloadable, ALLOCATE_METHOD_CALLBACK_1(SceneData, SetScenePreloadable, SceneInstData, Bool), TRACKING_REF_SCENE);
        PropertySet::AddCallback(props, kScenePreloadShaders, ALLOCATE_METHOD_CALLBACK_1(SceneData, SetScenePreloadShaders, SceneInstData, Bool), TRACKING_REF_SCENE);
        PropertySet::AddCallback(props, kSceneAfterEffectsEnabled, ALLOCATE_METHOD_CALLBACK_1(SceneData, SetAfterEffectsEnabled, SceneInstData, Bool), TRACKING_REF_SCENE);
        PropertySet::AddCallback(props, kSceneGenerateNPRLines, ALLOCATE_METHOD_CALLBACK_1(SceneData, SetGenerateNPRLines, SceneInstData, Bool), TRACKING_REF_SCENE);
        PropertySet::AddCallback(props, kSceneNPRLinesFalloff, ALLOCATE_METHOD_CALLBACK_1(SceneData, SetNPRLinesFalloff, SceneInstData, Float), TRACKING_REF_SCENE);
        PropertySet::AddCallback(props, kSceneNPRLinesBias, ALLOCATE_METHOD_CALLBACK_1(SceneData, SetNPRLinesBias, SceneInstData, Float), TRACKING_REF_SCENE);
        PropertySet::AddCallback(props, kSceneNPRLinesAlphaFalloff, ALLOCATE_METHOD_CALLBACK_1(SceneData, SetNPRLinesAlphaFalloff, SceneInstData, Float), TRACKING_REF_SCENE);
        PropertySet::AddCallback(props, kSceneNPRLinesAlphaBias, ALLOCATE_METHOD_CALLBACK_1(SceneData, SetNPRLinesAlphaBias, SceneInstData, Float), TRACKING_REF_SCENE);
        PropertySet::AddCallback(props, kSceneGlowClearColor, ALLOCATE_METHOD_CALLBACK_1(SceneData, SetGlowClearColor, SceneInstData, Colour), TRACKING_REF_SCENE);
        PropertySet::AddCallback(props, kSceneGlowSigmaScale, ALLOCATE_METHOD_CALLBACK_1(SceneData, SetGlowSigmaScale, SceneInstData, Float), TRACKING_REF_SCENE);
        PropertySet::AddCallback(props, kSceneFXAntiAliasing, ALLOCATE_METHOD_CALLBACK_1(SceneData, SetFXAntiAliasing, SceneInstData, Bool), TRACKING_REF_SCENE);
        PropertySet::AddCallback(props, kSceneFXTAAWeight, ALLOCATE_METHOD_CALLBACK_1(SceneData, SetFXTAAWeight, SceneInstData, Float), TRACKING_REF_SCENE);
        PropertySet::AddCallback(props, kSceneFXColorEnabled, ALLOCATE_METHOD_CALLBACK_1(SceneData, SetFXColorEnabled, SceneInstData, Bool), TRACKING_REF_SCENE);
        PropertySet::AddCallback(props, kSceneFXColorTint, ALLOCATE_METHOD_CALLBACK_1(SceneData, SetFXColorTint, SceneInstData, Colour), TRACKING_REF_SCENE);
        PropertySet::AddCallback(props, kSceneFXColorOpacity, ALLOCATE_METHOD_CALLBACK_1(SceneData, SetFXColorOpacity, SceneInstData, Float), TRACKING_REF_SCENE);
        PropertySet::AddCallback(props, kSceneFXNoiseScale, ALLOCATE_METHOD_CALLBACK_1(SceneData, SetFXNoiseScale, SceneInstData, Float), TRACKING_REF_SCENE);
        PropertySet::AddCallback(props, kSceneFXSharpShadowsEnabled, ALLOCATE_METHOD_CALLBACK_1(SceneData, SetFXSharpShadowsEnabled, SceneInstData, Bool), TRACKING_REF_SCENE);
        PropertySet::AddCallback(props, kSceneFXLevelsEnabled, ALLOCATE_METHOD_CALLBACK_1(SceneData, SetFXLevelsEnabled, SceneInstData, Bool), TRACKING_REF_SCENE);
        PropertySet::AddCallback(props, kSceneFXLevelsBlackPoint, ALLOCATE_METHOD_CALLBACK_1(SceneData, SetFXLevelsBlackPoint, SceneInstData, Float), TRACKING_REF_SCENE);
        PropertySet::AddCallback(props, kSceneFXLevelsWhitePoint, ALLOCATE_METHOD_CALLBACK_1(SceneData, SetFXLevelsWhitePoint, SceneInstData, Float), TRACKING_REF_SCENE);
        PropertySet::AddCallback(props, kSceneFXLevelsIntensity, ALLOCATE_METHOD_CALLBACK_1(SceneData, SetFXLevelsIntensity, SceneInstData, Float), TRACKING_REF_SCENE);
        PropertySet::AddCallback(props, kSceneFXLevelsBlackPointHDR, ALLOCATE_METHOD_CALLBACK_1(SceneData, SetFXLevelsBlackPointHDR, SceneInstData, Float), TRACKING_REF_SCENE);
        PropertySet::AddCallback(props, kSceneFXLevelsWhitePointHDR, ALLOCATE_METHOD_CALLBACK_1(SceneData, SetFXLevelsWhitePointHDR, SceneInstData, Float), TRACKING_REF_SCENE);
        PropertySet::AddCallback(props, kSceneFXLevelsIntensityHDR, ALLOCATE_METHOD_CALLBACK_1(SceneData, SetFXLevelsIntensityHDR, SceneInstData, Float), TRACKING_REF_SCENE);
        PropertySet::AddCallback(props, kSceneFXTonemapType, ALLOCATE_METHOD_CALLBACK_1(SceneData, SetFXTonemapType, SceneInstData, Enum<TonemapType>), TRACKING_REF_SCENE);
        PropertySet::AddCallback(props, kSceneFXTonemapEnabled, ALLOCATE_METHOD_CALLBACK_1(SceneData, SetFXTonemapEnabled, SceneInstData, Bool), TRACKING_REF_SCENE);
        PropertySet::AddCallback(props, kSceneFXTonemapDOFEnabled, ALLOCATE_METHOD_CALLBACK_1(SceneData, SetFXTonemapDOFEnabled, SceneInstData, Bool), TRACKING_REF_SCENE);
        PropertySet::AddCallback(props, kSceneFXTonemapIntensity, ALLOCATE_METHOD_CALLBACK_1(SceneData, SetFXTonemapIntensity, SceneInstData, Float), TRACKING_REF_SCENE);
        PropertySet::AddCallback(props, kSceneFXTonemapWhitePoint, ALLOCATE_METHOD_CALLBACK_1(SceneData, SetFXTonemapWhitePoint, SceneInstData, Float), TRACKING_REF_SCENE);
        PropertySet::AddCallback(props, kSceneFXTonemapBlackPoint, ALLOCATE_METHOD_CALLBACK_1(SceneData, SetFXTonemapBlackPoint, SceneInstData, Float), TRACKING_REF_SCENE);
        PropertySet::AddCallback(props, kSceneFXTonemapFilmicPivot, ALLOCATE_METHOD_CALLBACK_1(SceneData, SetFXTonemapFilmicPivot, SceneInstData, Float), TRACKING_REF_SCENE);
        PropertySet::AddCallback(props, kSceneFXTonemapFilmicShoulderIntensity, ALLOCATE_METHOD_CALLBACK_1(SceneData, SetFXTonemapFilmicShoulderIntensity, SceneInstData, Float), TRACKING_REF_SCENE);
        PropertySet::AddCallback(props, kSceneFXTonemapFilmicToeIntensity, ALLOCATE_METHOD_CALLBACK_1(SceneData, SetFXTonemapFilmicToeIntensity, SceneInstData, Float), TRACKING_REF_SCENE);
        PropertySet::AddCallback(props, kSceneFXTonemapFilmicSign, ALLOCATE_METHOD_CALLBACK_1(SceneData, SetFXTonemapFilmicSign, SceneInstData, Float), TRACKING_REF_SCENE);
        PropertySet::AddCallback(props, kSceneFXTonemapFarWhitePoint, ALLOCATE_METHOD_CALLBACK_1(SceneData, SetFXTonemapFarWhitePoint, SceneInstData, Float), TRACKING_REF_SCENE);
        PropertySet::AddCallback(props, kSceneFXTonemapFarBlackPoint, ALLOCATE_METHOD_CALLBACK_1(SceneData, SetFXTonemapFarBlackPoint, SceneInstData, Float), TRACKING_REF_SCENE);
        PropertySet::AddCallback(props, kSceneFXTonemapFarFilmicPivot, ALLOCATE_METHOD_CALLBACK_1(SceneData, SetFXTonemapFarFilmicPivot, SceneInstData, Float), TRACKING_REF_SCENE);
        PropertySet::AddCallback(props, kSceneFXTonemapFarFilmicShoulderIntensity, ALLOCATE_METHOD_CALLBACK_1(SceneData, SetFXTonemapFarFilmicShoulderIntensity, SceneInstData, Float), TRACKING_REF_SCENE);
        PropertySet::AddCallback(props, kSceneFXTonemapFarFilmicToeIntensity, ALLOCATE_METHOD_CALLBACK_1(SceneData, SetFXTonemapFarFilmicToeIntensity, SceneInstData, Float), TRACKING_REF_SCENE);
        PropertySet::AddCallback(props, kSceneFXTonemapFarFilmicSign, ALLOCATE_METHOD_CALLBACK_1(SceneData, SetFXTonemapFarFilmicSign, SceneInstData, Float), TRACKING_REF_SCENE);
        PropertySet::AddCallback(props, kSceneFXTonemapRGBEnabled, ALLOCATE_METHOD_CALLBACK_1(SceneData, SetFXTonemapRGBEnabled, SceneInstData, Bool), TRACKING_REF_SCENE);
        PropertySet::AddCallback(props, kSceneFXTonemapRGBDOFEnabled, ALLOCATE_METHOD_CALLBACK_1(SceneData, SetFXTonemapRGBDOFEnabled, SceneInstData, Bool), TRACKING_REF_SCENE);
        PropertySet::AddCallback(props, kSceneFXTonemapRGBBlackPoints, ALLOCATE_METHOD_CALLBACK_1(SceneData, SetFXTonemapRGBBlackPoints, SceneInstData, Vector3), TRACKING_REF_SCENE);
        PropertySet::AddCallback(props, kSceneFXTonemapRGBWhitePoints, ALLOCATE_METHOD_CALLBACK_1(SceneData, SetFXTonemapRGBWhitePoints, SceneInstData, Vector3), TRACKING_REF_SCENE);
        PropertySet::AddCallback(props, kSceneFXTonemapRGBPivots, ALLOCATE_METHOD_CALLBACK_1(SceneData, SetFXTonemapRGBPivots, SceneInstData, Vector3), TRACKING_REF_SCENE);
        PropertySet::AddCallback(props, kSceneFXTonemapRGBShoulderIntensities, ALLOCATE_METHOD_CALLBACK_1(SceneData, SetFXTonemapRGBShoulderIntensities, SceneInstData, Vector3), TRACKING_REF_SCENE);
        PropertySet::AddCallback(props, kSceneFXTonemapRGBToeIntensities, ALLOCATE_METHOD_CALLBACK_1(SceneData, SetFXTonemapRGBToeIntensities, SceneInstData, Vector3), TRACKING_REF_SCENE);
        PropertySet::AddCallback(props, kSceneFXTonemapRGBSigns, ALLOCATE_METHOD_CALLBACK_1(SceneData, SetFXTonemapRGBSigns, SceneInstData, Vector3), TRACKING_REF_SCENE);
        PropertySet::AddCallback(props, kSceneFXTonemapRGBFarBlackPoints, ALLOCATE_METHOD_CALLBACK_1(SceneData, SetFXTonemapRGBFarBlackPoints, SceneInstData, Vector3), TRACKING_REF_SCENE);
        PropertySet::AddCallback(props, kSceneFXTonemapRGBFarWhitePoints, ALLOCATE_METHOD_CALLBACK_1(SceneData, SetFXTonemapRGBFarWhitePoints, SceneInstData, Vector3), TRACKING_REF_SCENE);
        PropertySet::AddCallback(props, kSceneFXTonemapRGBFarPivots, ALLOCATE_METHOD_CALLBACK_1(SceneData, SetFXTonemapRGBFarPivots, SceneInstData, Vector3), TRACKING_REF_SCENE);
        PropertySet::AddCallback(props, kSceneFXTonemapRGBFarShoulderIntensities, ALLOCATE_METHOD_CALLBACK_1(SceneData, SetFXTonemapRGBFarShoulderIntensities, SceneInstData, Vector3), TRACKING_REF_SCENE);
        PropertySet::AddCallback(props, kSceneFXTonemapRGBFarToeIntensities, ALLOCATE_METHOD_CALLBACK_1(SceneData, SetFXTonemapRGBFarToeIntensities, SceneInstData, Vector3), TRACKING_REF_SCENE);
        PropertySet::AddCallback(props, kSceneFXTonemapRGBFarSigns, ALLOCATE_METHOD_CALLBACK_1(SceneData, SetFXTonemapRGBFarSigns, SceneInstData, Vector3), TRACKING_REF_SCENE);
        PropertySet::AddCallback(props, kSceneFXBloomThreshold, ALLOCATE_METHOD_CALLBACK_1(SceneData, SetFXBloomThreshold, SceneInstData, Float), TRACKING_REF_SCENE);
        PropertySet::AddCallback(props, kSceneFXBloomIntensity, ALLOCATE_METHOD_CALLBACK_1(SceneData, SetFXBloomIntensity, SceneInstData, Float), TRACKING_REF_SCENE);
        PropertySet::AddCallback(props, kSceneFXAmbientOcclusionEnabled, ALLOCATE_METHOD_CALLBACK_1(SceneData, SetFXAmbientOcclusionEnabled, SceneInstData, Bool), TRACKING_REF_SCENE);
        PropertySet::AddCallback(props, kSceneFXAmbientOcclusionIntensity, ALLOCATE_METHOD_CALLBACK_1(SceneData, SetFXAmbientOcclusionIntensity, SceneInstData, Float), TRACKING_REF_SCENE);
        PropertySet::AddCallback(props, kSceneFXAmbientOcclusionFalloff, ALLOCATE_METHOD_CALLBACK_1(SceneData, SetFXAmbientOcclusionFalloff, SceneInstData, Float), TRACKING_REF_SCENE);
        PropertySet::AddCallback(props, kSceneFXAmbientOcclusionRadius, ALLOCATE_METHOD_CALLBACK_1(SceneData, SetFXAmbientOcclusionRadius, SceneInstData, Float), TRACKING_REF_SCENE);
        PropertySet::AddCallback(props, kSceneFXAmbientOcclusionLightmap, ALLOCATE_METHOD_CALLBACK_1(SceneData, SetFXAmbientOcclusionLightmap, SceneInstData, Bool), TRACKING_REF_SCENE);
        PropertySet::AddCallback(props, kSceneFXDOFEnabled, ALLOCATE_METHOD_CALLBACK_1(SceneData, SetFXDOFEnabled, SceneInstData, Bool), TRACKING_REF_SCENE);
        PropertySet::AddCallback(props, kSceneFXDOFFOVAdjustEnabled, ALLOCATE_METHOD_CALLBACK_1(SceneData, SetFXDOFFOVAdjustEnabled, SceneInstData, Bool), TRACKING_REF_SCENE);
        PropertySet::AddCallback(props, kSceneFXDOFAutoFocusEnabled, ALLOCATE_METHOD_CALLBACK_1(SceneData, SetFXDOFAutoFocusEnabled, SceneInstData, Bool), TRACKING_REF_SCENE);
        PropertySet::AddCallback(props, kSceneFXDOFNear, ALLOCATE_METHOD_CALLBACK_1(SceneData, SetFXDOFNear, SceneInstData, Float), TRACKING_REF_SCENE);
        PropertySet::AddCallback(props, kSceneFXDOFFar, ALLOCATE_METHOD_CALLBACK_1(SceneData, SetFXDOFFar, SceneInstData, Float), TRACKING_REF_SCENE);
        PropertySet::AddCallback(props, kSceneFXDOFNearFalloff, ALLOCATE_METHOD_CALLBACK_1(SceneData, SetFXDOFNearFalloff, SceneInstData, Float), TRACKING_REF_SCENE);
        PropertySet::AddCallback(props, kSceneFXDOFFarFalloff, ALLOCATE_METHOD_CALLBACK_1(SceneData, SetFXDOFFarFalloff, SceneInstData, Float), TRACKING_REF_SCENE);
        PropertySet::AddCallback(props, kSceneFXDOFNearMax, ALLOCATE_METHOD_CALLBACK_1(SceneData, SetFXDOFNearMax, SceneInstData, Float), TRACKING_REF_SCENE);
        PropertySet::AddCallback(props, kSceneFXDOFFarMax, ALLOCATE_METHOD_CALLBACK_1(SceneData, SetFXDOFFarMax, SceneInstData, Float), TRACKING_REF_SCENE);
        PropertySet::AddCallback(props, kSceneFXDOFVignetteMax, ALLOCATE_METHOD_CALLBACK_1(SceneData, SetFXDOFVignetteMax, SceneInstData, Float), TRACKING_REF_SCENE);
        PropertySet::AddCallback(props, kSceneFXDOFDebug, ALLOCATE_METHOD_CALLBACK_1(SceneData, SetFXDOFDebug, SceneInstData, Bool), TRACKING_REF_SCENE);
        PropertySet::AddCallback(props, kSceneFXDOFCoverageBoost, ALLOCATE_METHOD_CALLBACK_1(SceneData, SetFXDOFCoverageBoost, SceneInstData, Float), TRACKING_REF_SCENE);
        PropertySet::AddCallback(props, kSceneFXForceLinearDepthOffset, ALLOCATE_METHOD_CALLBACK_1(SceneData, SetFXForceLinearDepthOffset, SceneInstData, Float), TRACKING_REF_SCENE);
        PropertySet::AddCallback(props, kSceneFXVignetteTintEnabled, ALLOCATE_METHOD_CALLBACK_1(SceneData, SetFXVignetteTintEnabled, SceneInstData, Bool), TRACKING_REF_SCENE);
        PropertySet::AddCallback(props, kSceneFXVignetteDOFEnabled, ALLOCATE_METHOD_CALLBACK_1(SceneData, SetFXVignetteDOFEnabled, SceneInstData, Bool), TRACKING_REF_SCENE);
        PropertySet::AddCallback(props, kSceneFXVignetteTint, ALLOCATE_METHOD_CALLBACK_1(SceneData, SetFXVignetteTint, SceneInstData, Colour), TRACKING_REF_SCENE);
        PropertySet::AddCallback(props, kSceneFXVignetteFalloff, ALLOCATE_METHOD_CALLBACK_1(SceneData, SetFXVignetteFalloff, SceneInstData, Float), TRACKING_REF_SCENE);
        PropertySet::AddCallback(props, kSceneFXVignetteCenter, ALLOCATE_METHOD_CALLBACK_1(SceneData, SetFXVignetteCenter, SceneInstData, Vector2), TRACKING_REF_SCENE);
        PropertySet::AddCallback(props, kSceneFXVignetteCorners, ALLOCATE_METHOD_CALLBACK_1(SceneData, SetFXVignetteCorners, SceneInstData, Vector4), TRACKING_REF_SCENE);
        PropertySet::AddCallback(props, kSceneHBAOEnabled, ALLOCATE_METHOD_CALLBACK_1(SceneData, SetHBAOEnabled, SceneInstData, Bool), TRACKING_REF_SCENE);
        PropertySet::AddCallback(props, kSceneHBAODebug, ALLOCATE_METHOD_CALLBACK_1(SceneData, SetHBAODebug, SceneInstData, Bool), TRACKING_REF_SCENE);
        PropertySet::AddCallback(props, kSceneHBAORadius, ALLOCATE_METHOD_CALLBACK_1(SceneData, SetHBAORadius, SceneInstData, Float), TRACKING_REF_SCENE);
        PropertySet::AddCallback(props, kSceneHBAOMaxRadiusPercent, ALLOCATE_METHOD_CALLBACK_1(SceneData, SetHBAOMaxRadiusPercent, SceneInstData, Float), TRACKING_REF_SCENE);
        PropertySet::AddCallback(props, kSceneHBAOHemisphereBias, ALLOCATE_METHOD_CALLBACK_1(SceneData, SetHBAOHemisphereBias, SceneInstData, Float), TRACKING_REF_SCENE);
        PropertySet::AddCallback(props, kSceneHBAOIntensity, ALLOCATE_METHOD_CALLBACK_1(SceneData, SetHBAOIntensity, SceneInstData, Float), TRACKING_REF_SCENE);
        PropertySet::AddCallback(props, kSceneHBAOOcclusionScale, ALLOCATE_METHOD_CALLBACK_1(SceneData, SetHBAOOcclusionScale, SceneInstData, Float), TRACKING_REF_SCENE);
        PropertySet::AddCallback(props, kSceneHBAOLuminanceScale, ALLOCATE_METHOD_CALLBACK_1(SceneData, SetHBAOLuminanceScale, SceneInstData, Float), TRACKING_REF_SCENE);
        PropertySet::AddCallback(props, kSceneHBAOMaxDistance, ALLOCATE_METHOD_CALLBACK_1(SceneData, SetHBAOMaxDistance, SceneInstData, Float), TRACKING_REF_SCENE);
        PropertySet::AddCallback(props, kSceneHBAODistanceFalloff, ALLOCATE_METHOD_CALLBACK_1(SceneData, SetHBAODistanceFalloff, SceneInstData, Float), TRACKING_REF_SCENE);
        PropertySet::AddCallback(props, kSceneHBAOBlurSharpness, ALLOCATE_METHOD_CALLBACK_1(SceneData, SetHBAOBlurSharpness, SceneInstData, Float), TRACKING_REF_SCENE);
        PropertySet::AddCallback(props, kSceneScreenSpaceLinesEnabled, ALLOCATE_METHOD_CALLBACK_1(SceneData, SetScreenSpaceLinesEnabled, SceneInstData, Bool), TRACKING_REF_SCENE);
        PropertySet::AddCallback(props, kSceneScreenSpaceLinesColor, ALLOCATE_METHOD_CALLBACK_1(SceneData, SetScreenSpaceLinesColor, SceneInstData, Colour), TRACKING_REF_SCENE);
        PropertySet::AddCallback(props, kSceneScreenSpaceLinesThickness, ALLOCATE_METHOD_CALLBACK_1(SceneData, SetScreenSpaceLinesThickness, SceneInstData, Float), TRACKING_REF_SCENE);
        PropertySet::AddCallback(props, kSceneScreenSpaceLinesDepthFadeNear, ALLOCATE_METHOD_CALLBACK_1(SceneData, SetScreenSpaceLinesDepthFadeNear, SceneInstData, Float), TRACKING_REF_SCENE);
        PropertySet::AddCallback(props, kSceneScreenSpaceLinesDepthFadeFar, ALLOCATE_METHOD_CALLBACK_1(SceneData, SetScreenSpaceLinesDepthFadeFar, SceneInstData, Float), TRACKING_REF_SCENE);
        PropertySet::AddCallback(props, kSceneScreenSpaceLinesDepthMagnitude, ALLOCATE_METHOD_CALLBACK_1(SceneData, SetScreenSpaceLinesDepthMagnitude, SceneInstData, Float), TRACKING_REF_SCENE);
        PropertySet::AddCallback(props, kSceneScreenSpaceLinesDepthExponent, ALLOCATE_METHOD_CALLBACK_1(SceneData, SetScreenSpaceLinesDepthExponent, SceneInstData, Float), TRACKING_REF_SCENE);
        PropertySet::AddCallback(props, kSceneScreenSpaceLinesLightDirection, ALLOCATE_METHOD_CALLBACK_1(SceneData, SetScreenSpaceLinesLightDirection, SceneInstData, Vector3), TRACKING_REF_SCENE);
        PropertySet::AddCallback(props, kSceneScreenSpaceLinesLightMagnitude, ALLOCATE_METHOD_CALLBACK_1(SceneData, SetScreenSpaceLinesLightMagnitude, SceneInstData, Float), TRACKING_REF_SCENE);
        PropertySet::AddCallback(props, kSceneScreenSpaceLinesLightExponent, ALLOCATE_METHOD_CALLBACK_1(SceneData, SetScreenSpaceLinesLightExponent, SceneInstData, Float), TRACKING_REF_SCENE);
        PropertySet::AddCallback(props, kSceneScreenSpaceLinesDebug, ALLOCATE_METHOD_CALLBACK_1(SceneData, SetScreenSpaceLinesDebug, SceneInstData, Bool), TRACKING_REF_SCENE);
        PropertySet::AddCallback(props, kSceneFogEnabled, ALLOCATE_METHOD_CALLBACK_1(SceneData, SetFogEnabled, SceneInstData, Bool), TRACKING_REF_SCENE);
        PropertySet::AddCallback(props, kSceneFogColor, ALLOCATE_METHOD_CALLBACK_1(SceneData, SetFogColor, SceneInstData, Colour), TRACKING_REF_SCENE);
        PropertySet::AddCallback(props, kSceneFogAlpha, ALLOCATE_METHOD_CALLBACK_1(SceneData, SetFogAlpha, SceneInstData, Float), TRACKING_REF_SCENE);
        PropertySet::AddCallback(props, kSceneFogNearPlane, ALLOCATE_METHOD_CALLBACK_1(SceneData, SetFogNearPlane, SceneInstData, Float), TRACKING_REF_SCENE);
        PropertySet::AddCallback(props, kSceneFogFarPlane, ALLOCATE_METHOD_CALLBACK_1(SceneData, SetFogFarPlane, SceneInstData, Float), TRACKING_REF_SCENE);
        PropertySet::AddCallback(props, kSceneHDRPaperWhiteNits, ALLOCATE_METHOD_CALLBACK_1(SceneData, SetHDRPaperWhiteNits, SceneInstData, Float), TRACKING_REF_SCENE);
        PropertySet::AddCallback(props, kSceneGraphicBlackThreshold, ALLOCATE_METHOD_CALLBACK_1(SceneData, SetGraphicBlackThreshold, SceneInstData, Float), TRACKING_REF_SCENE);
        PropertySet::AddCallback(props, kSceneGraphicBlackSoftness, ALLOCATE_METHOD_CALLBACK_1(SceneData, SetGraphicBlackSoftness, SceneInstData, Float), TRACKING_REF_SCENE);
        PropertySet::AddCallback(props, kSceneGraphicBlackAlpha, ALLOCATE_METHOD_CALLBACK_1(SceneData, SetGraphicBlackAlpha, SceneInstData, Float), TRACKING_REF_SCENE);
        PropertySet::AddCallback(props, kSceneGraphicBlackNear, ALLOCATE_METHOD_CALLBACK_1(SceneData, SetGraphicBlackNear, SceneInstData, Float), TRACKING_REF_SCENE);
        PropertySet::AddCallback(props, kSceneGraphicBlackFar, ALLOCATE_METHOD_CALLBACK_1(SceneData, SetGraphicBlackFar, SceneInstData, Float), TRACKING_REF_SCENE);
        PropertySet::AddCallback(props, kSceneWindDirection, ALLOCATE_METHOD_CALLBACK_1(SceneData, SetWindDirection, SceneInstData, Vector3), TRACKING_REF_SCENE);
        PropertySet::AddCallback(props, kSceneWindSpeed, ALLOCATE_METHOD_CALLBACK_1(SceneData, SetWindSpeed, SceneInstData, Float), TRACKING_REF_SCENE);
        PropertySet::AddCallback(props, kSceneWindIdleStrength, ALLOCATE_METHOD_CALLBACK_1(SceneData, SetWindIdleStrength, SceneInstData, Float), TRACKING_REF_SCENE);
        PropertySet::AddCallback(props, kSceneWindIdleSpacialFrequency, ALLOCATE_METHOD_CALLBACK_1(SceneData, SetWindIdleSpacialFrequency, SceneInstData, Float), TRACKING_REF_SCENE);
        PropertySet::AddCallback(props, kSceneWindGustSpeed, ALLOCATE_METHOD_CALLBACK_1(SceneData, SetWindGustSpeed, SceneInstData, Float), TRACKING_REF_SCENE);
        PropertySet::AddCallback(props, kSceneWindGustStrength, ALLOCATE_METHOD_CALLBACK_1(SceneData, SetWindGustStrength, SceneInstData, Float), TRACKING_REF_SCENE);
        PropertySet::AddCallback(props, kSceneWindGustSpacialFrequency, ALLOCATE_METHOD_CALLBACK_1(SceneData, SetWindGustSpacialFrequency, SceneInstData, Float), TRACKING_REF_SCENE);
        PropertySet::AddCallback(props, kSceneWindGustIdleStrengthMultiplier, ALLOCATE_METHOD_CALLBACK_1(SceneData, SetWindGustIdleStrengthMultiplier, SceneInstData, Float), TRACKING_REF_SCENE);
        PropertySet::AddCallback(props, kSceneWindGustSeparationExponent, ALLOCATE_METHOD_CALLBACK_1(SceneData, SetWindGustSeparationExponent, SceneInstData, Float), TRACKING_REF_SCENE);
        PropertySet::AddCallback(props, kSceneLightEnvReflectionEnabled, ALLOCATE_METHOD_CALLBACK_1(SceneData, SetLightEnvReflectionEnabled, SceneInstData, Bool), TRACKING_REF_SCENE);
        PropertySet::AddCallback(props, kSceneLightEnvBakeEnabled, ALLOCATE_METHOD_CALLBACK_1(SceneData, SetLightEnvBakeEnabled, SceneInstData, Bool), TRACKING_REF_SCENE);
        PropertySet::AddCallback(props, kSceneLightEnvReflectionTexture, ALLOCATE_METHOD_CALLBACK_1(SceneData, SetLightEnvReflectionTexture, SceneInstData, Symbol), TRACKING_REF_SCENE);
        PropertySet::AddCallback(props, kSceneLightEnvReflectionIntensity, ALLOCATE_METHOD_CALLBACK_1(SceneData, SetLightEnvReflectionIntensity, SceneInstData, Float), TRACKING_REF_SCENE);
        PropertySet::AddCallback(props, kSceneLightEnvReflectionTint, ALLOCATE_METHOD_CALLBACK_1(SceneData, SetLightEnvReflectionTint, SceneInstData, Colour), TRACKING_REF_SCENE);
        PropertySet::AddCallback(props, kSceneLightEnvEnabled, ALLOCATE_METHOD_CALLBACK_1(SceneData, SetLightEnvEnabled, SceneInstData, Bool), TRACKING_REF_SCENE);
        PropertySet::AddCallback(props, kSceneLightEnvProbeData, ALLOCATE_METHOD_CALLBACK_1(SceneData, SetLightEnvProbeData, SceneInstData, Symbol), TRACKING_REF_SCENE);
        PropertySet::AddCallback(props, kSceneLightEnvIntensity, ALLOCATE_METHOD_CALLBACK_1(SceneData, SetLightEnvIntensity, SceneInstData, Float), TRACKING_REF_SCENE);
        PropertySet::AddCallback(props, kSceneLightEnvReflectionIntensityShadow, ALLOCATE_METHOD_CALLBACK_1(SceneData, SetLightEnvReflectionIntensityShadow, SceneInstData, Float), TRACKING_REF_SCENE);
        PropertySet::AddCallback(props, kSceneLightEnvSaturation, ALLOCATE_METHOD_CALLBACK_1(SceneData, SetLightEnvSaturation, SceneInstData, Float), TRACKING_REF_SCENE);
        PropertySet::AddCallback(props, kSceneLightEnvTint, ALLOCATE_METHOD_CALLBACK_1(SceneData, SetLightEnvTint, SceneInstData, Colour), TRACKING_REF_SCENE);
        PropertySet::AddCallback(props, kSceneLightEnvBackgroundColor, ALLOCATE_METHOD_CALLBACK_1(SceneData, SetLightEnvBackgroundColor, SceneInstData, Colour), TRACKING_REF_SCENE);
        PropertySet::AddCallback(props, kSceneLightEnvProbeResolutionXZ, ALLOCATE_METHOD_CALLBACK_1(SceneData, SetLightEnvProbeResolutionXZ, SceneInstData, Vector2), TRACKING_REF_SCENE);
        PropertySet::AddCallback(props, kSceneLightEnvProbeResolutionY, ALLOCATE_METHOD_CALLBACK_1(SceneData, SetLightEnvProbeResolutionY, SceneInstData, Float), TRACKING_REF_SCENE);
        PropertySet::AddCallback(props, kSceneLightEnvShadowMomentBias, ALLOCATE_METHOD_CALLBACK_1(SceneData, SetLightEnvShadowMomentBias, SceneInstData, Float), TRACKING_REF_SCENE);
        PropertySet::AddCallback(props, kSceneLightEnvShadowDepthBias, ALLOCATE_METHOD_CALLBACK_1(SceneData, SetLightEnvShadowDepthBias, SceneInstData, Float), TRACKING_REF_SCENE);
        PropertySet::AddCallback(props, kSceneLightEnvShadowPositionOffsetBias, ALLOCATE_METHOD_CALLBACK_1(SceneData, SetLightEnvShadowPositionOffsetBias, SceneInstData, Float), TRACKING_REF_SCENE);
        PropertySet::AddCallback(props, kSceneLightEnvShadowLightBleedReduction, ALLOCATE_METHOD_CALLBACK_1(SceneData, SetLightEnvShadowLightBleedReduction, SceneInstData, Float), TRACKING_REF_SCENE);
        PropertySet::AddCallback(props, kSceneLightEnvShadowMinDistance, ALLOCATE_METHOD_CALLBACK_1(SceneData, SetLightEnvShadowMinDistance, SceneInstData, Float), TRACKING_REF_SCENE);
        PropertySet::AddCallback(props, kSceneLightEnvShadowMaxDistance, ALLOCATE_METHOD_CALLBACK_1(SceneData, SetLightEnvShadowMaxDistance, SceneInstData, Float), TRACKING_REF_SCENE);
        PropertySet::AddCallback(props, kSceneLightEnvDynamicShadowMaxDistance, ALLOCATE_METHOD_CALLBACK_1(SceneData, SetLightEnvDynamicShadowMaxDistance, SceneInstData, Float), TRACKING_REF_SCENE);
        PropertySet::AddCallback(props, kSceneLightEnvShadowCascadeSplitBias, ALLOCATE_METHOD_CALLBACK_1(SceneData, SetLightEnvShadowCascadeSplitBias, SceneInstData, Float), TRACKING_REF_SCENE);
        PropertySet::AddCallback(props, kSceneLightEnvShadowAutoDepthBounds, ALLOCATE_METHOD_CALLBACK_1(SceneData, SetLightEnvShadowAutoDepthBounds, SceneInstData, Bool), TRACKING_REF_SCENE);
        PropertySet::AddCallback(props, kSceneLightEnvShadowMaxUpdates, ALLOCATE_METHOD_CALLBACK_1(SceneData, SetLightEnvShadowMaxUpdates, SceneInstData, I32), TRACKING_REF_SCENE);
        PropertySet::AddCallback(props, kSceneLightShadowTraceMaxDistance, ALLOCATE_METHOD_CALLBACK_1(SceneData, SetLightShadowTraceMaxDistance, SceneInstData, Float), TRACKING_REF_SCENE);
        PropertySet::AddCallback(props, kSceneLightStaticShadowBoundsMin, ALLOCATE_METHOD_CALLBACK_1(SceneData, SetLightStaticShadowBoundsMin, SceneInstData, Float), TRACKING_REF_SCENE);
        PropertySet::AddCallback(props, kSceneLightStaticShadowBoundsMax, ALLOCATE_METHOD_CALLBACK_1(SceneData, SetLightStaticShadowBoundsMax, SceneInstData, Float), TRACKING_REF_SCENE);
        PropertySet::AddCallback(props, kSceneEnvLightShadowGoboTexture, ALLOCATE_METHOD_CALLBACK_1(SceneData, SetEnvLightShadowGoboTexture, SceneInstData, Symbol), TRACKING_REF_SCENE);
        PropertySet::AddCallback(props, kSceneFXBrushDOFEnable, ALLOCATE_METHOD_CALLBACK_1(SceneData, SetFXBrushDOFEnable, SceneInstData, Bool), TRACKING_REF_SCENE);
        PropertySet::AddCallback(props, kSceneFXBrushOutlineEnable, ALLOCATE_METHOD_CALLBACK_1(SceneData, SetFXBrushOutlineEnable, SceneInstData, Bool), TRACKING_REF_SCENE);
        PropertySet::AddCallback(props, kSceneFXBrushOutlineFilterEnable, ALLOCATE_METHOD_CALLBACK_1(SceneData, SetFXBrushOutlineFilterEnable, SceneInstData, Bool), TRACKING_REF_SCENE);
        PropertySet::AddCallback(props, kSceneFXBrushOutlineSize, ALLOCATE_METHOD_CALLBACK_1(SceneData, SetFXBrushOutlineSize, SceneInstData, Float), TRACKING_REF_SCENE);
        PropertySet::AddCallback(props, kSceneFXBrushOutlineThreshold, ALLOCATE_METHOD_CALLBACK_1(SceneData, SetFXBrushOutlineThreshold, SceneInstData, Float), TRACKING_REF_SCENE);
        PropertySet::AddCallback(props, kSceneFXBrushOutlineColorThreshold, ALLOCATE_METHOD_CALLBACK_1(SceneData, SetFXBrushOutlineColorThreshold, SceneInstData, Float), TRACKING_REF_SCENE);
        PropertySet::AddCallback(props, kSceneFXBrushOutlineFalloff, ALLOCATE_METHOD_CALLBACK_1(SceneData, SetFXBrushOutlineFalloff, SceneInstData, Float), TRACKING_REF_SCENE);
        PropertySet::AddCallback(props, kSceneFXBrushNearOutlineScale, ALLOCATE_METHOD_CALLBACK_1(SceneData, SetFXBrushNearOutlineScale, SceneInstData, Float), TRACKING_REF_SCENE);
        PropertySet::AddCallback(props, kSceneFXBrushNearTexture, ALLOCATE_METHOD_CALLBACK_1(SceneData, SetFXBrushNearTexture, SceneInstData, Symbol), TRACKING_REF_SCENE);
        PropertySet::AddCallback(props, kSceneFXBrushFarTexture, ALLOCATE_METHOD_CALLBACK_1(SceneData, SetFXBrushFarTexture, SceneInstData, Symbol), TRACKING_REF_SCENE);
        PropertySet::AddCallback(props, kSceneFXBrushNearScale, ALLOCATE_METHOD_CALLBACK_1(SceneData, SetFXBrushNearScale, SceneInstData, Float), TRACKING_REF_SCENE);
        PropertySet::AddCallback(props, kSceneFXBrushNearDetail, ALLOCATE_METHOD_CALLBACK_1(SceneData, SetFXBrushNearDetail, SceneInstData, Float), TRACKING_REF_SCENE);
        PropertySet::AddCallback(props, kSceneFXBrushFarScale, ALLOCATE_METHOD_CALLBACK_1(SceneData, SetFXBrushFarScale, SceneInstData, Float), TRACKING_REF_SCENE);
        PropertySet::AddCallback(props, kSceneFXBrushFarDetail, ALLOCATE_METHOD_CALLBACK_1(SceneData, SetFXBrushFarDetail, SceneInstData, Float), TRACKING_REF_SCENE);
        PropertySet::AddCallback(props, kSceneFXBrushFarScaleBoost, ALLOCATE_METHOD_CALLBACK_1(SceneData, SetFXBrushFarScaleBoost, SceneInstData, Float), TRACKING_REF_SCENE);
        PropertySet::AddCallback(props, kSceneFXBrushFarPlane, ALLOCATE_METHOD_CALLBACK_1(SceneData, SetFXBrushFarPlane, SceneInstData, Float), TRACKING_REF_SCENE);
        PropertySet::AddCallback(props, kSceneFXBrushFarPlaneFalloff, ALLOCATE_METHOD_CALLBACK_1(SceneData, SetFXBrushFarPlaneFalloff, SceneInstData, Float), TRACKING_REF_SCENE);
        PropertySet::AddCallback(props, kSceneFXBrushFarPlaneMaxScale, ALLOCATE_METHOD_CALLBACK_1(SceneData, SetFXBrushFarPlaneMaxScale, SceneInstData, Float), TRACKING_REF_SCENE);
        PropertySet::AddCallback(props, kSceneFrameBufferScaleOverride, ALLOCATE_METHOD_CALLBACK_1(SceneData, SetFrameBufferScaleOverride, SceneInstData, Float), TRACKING_REF_SCENE);
        PropertySet::AddCallback(props, kSceneFrameBufferScaleFactor, ALLOCATE_METHOD_CALLBACK_1(SceneData, SetFrameBufferScaleFactor, SceneInstData, Float), TRACKING_REF_SCENE);
        PropertySet::AddCallback(props, kSceneSpecularMultiplierEnabled, ALLOCATE_METHOD_CALLBACK_1(SceneData, SetSpecularMultiplierEnabled, SceneInstData, Bool), TRACKING_REF_SCENE);
        PropertySet::AddCallback(props, kSceneSpecularColorMultiplier, ALLOCATE_METHOD_CALLBACK_1(SceneData, SetSpecularColorMultiplier, SceneInstData, Colour), TRACKING_REF_SCENE);
        PropertySet::AddCallback(props, kSceneSpecularIntensityMultiplier, ALLOCATE_METHOD_CALLBACK_1(SceneData, SetSpecularIntensityMultiplier, SceneInstData, Float), TRACKING_REF_SCENE);
        PropertySet::AddCallback(props, kSceneSpecularExponentMultiplier, ALLOCATE_METHOD_CALLBACK_1(SceneData, SetSpecularExponentMultiplier, SceneInstData, Float), TRACKING_REF_SCENE);
        PropertySet::AddCallback(props, kSceneHDRLightmapsEnabled, ALLOCATE_METHOD_CALLBACK_1(SceneData, SetHDRLightmapsEnabled, SceneInstData, Bool), TRACKING_REF_SCENE);
        PropertySet::AddCallback(props, kSceneHDRLightmapsIntensity, ALLOCATE_METHOD_CALLBACK_1(SceneData, SetHDRLightmapsIntensity, SceneInstData, Float), TRACKING_REF_SCENE);
        PropertySet::AddCallback(props, kSceneCameraCutPositionThreshold, ALLOCATE_METHOD_CALLBACK_1(SceneData, SetCameraCutPositionThreshold, SceneInstData, Float), TRACKING_REF_SCENE);
        PropertySet::AddCallback(props, kSceneCameraCutRotationThreshold, ALLOCATE_METHOD_CALLBACK_1(SceneData, SetCameraCutRotationThreshold, SceneInstData, Float), TRACKING_REF_SCENE);
        PropertySet::AddCallback(props, kSceneViewportScissorLeft, ALLOCATE_METHOD_CALLBACK_1(SceneData, SetViewportScissorLeft, SceneInstData, Float), TRACKING_REF_SCENE);
        PropertySet::AddCallback(props, kSceneViewportScissorTop, ALLOCATE_METHOD_CALLBACK_1(SceneData, SetViewportScissorTop, SceneInstData, Float), TRACKING_REF_SCENE);
        PropertySet::AddCallback(props, kSceneViewportScissorRight, ALLOCATE_METHOD_CALLBACK_1(SceneData, SetViewportScissorRight, SceneInstData, Float), TRACKING_REF_SCENE);
        PropertySet::AddCallback(props, kSceneViewportScissorBottom, ALLOCATE_METHOD_CALLBACK_1(SceneData, SetViewportScissorBottom, SceneInstData, Float), TRACKING_REF_SCENE);
        PropertySet::AddCallback(props, kSceneScenePhysicsData, ALLOCATE_METHOD_CALLBACK_1(SceneData, SetScenePhysicsData, SceneInstData, Symbol), TRACKING_REF_SCENE);

        PropertySet::CallAllCallbacks(props, pAgentGettingCreated->OwningScene->GetRegistry());
    }
}

void SceneModule<SceneModuleType::SCENE>::OnModuleRemove(SceneAgent *pAttachedAgent)
{
    Meta::ClassInstance props = pAttachedAgent->OwningScene->GetAgentProps(pAttachedAgent->NameSymbol);
    PropertySet::ClearCallbacks(props, TRACKING_REF_SCENE);
}

// ============================================== MODULE IMPLEMENTATION ==============================================
// =================================================== LIGHT MODULE ==================================================
// ============================================== MODULE IMPLEMENTATION ==============================================

#define TRACKING_REF_LIGHT 0x1BADB00B

// Colors
void LightInstance::SetLightColour(Colour c) { _Data._LightColour = c; }
void LightInstance::SetLightColourDark(Colour c) { _Data._LightColourDark = c; }
void LightInstance::SetCell0Color(Colour c) { _Data._Cell0Color = c; }
void LightInstance::SetCell1Color(Colour c) { _Data._Cell1Color = c; }
void LightInstance::SetCell2Color(Colour c) { _Data._Cell2Color = c; }
void LightInstance::SetCell3Color(Colour c) { _Data._Cell3Color = c; }

// Intensity
void LightInstance::SetLightIntensity(Float f) { _Data._LightIntensity = f; }
void LightInstance::SetLightIntensityDiffuse(Float f) { _Data._LightIntensityDiffuse = f; }
void LightInstance::SetLightIntensitySpecular(Float f) { _Data._LightIntensitySpecular = f; }
void LightInstance::SetNPRSpecularIntensity(Float f) { _Data._NPRSpecularIntensity = f; }

// Distance
void LightInstance::SetLightMaxDistance(Float f) { _Data._LightMaxDistance = f; }
void LightInstance::SetLightMinDistance(Float f) { _Data._LightMinDistance = f; }

// Shadows
void LightInstance::SetLightShadowMax(Float f) { _Data._LightShadowMax = f; }
void LightInstance::SetLightShadowDistanceFalloff(Float f) { _Data._LightShadowDistanceFalloff = f; }
void LightInstance::SetLightShadowCascades(I32 i) { _Data._LightShadowCascades = i; }
void LightInstance::SetLightShadowBias(Float f) { _Data._LightShadowBias = f; }

// General properties
void LightInstance::SetLightDimmer(Float f) { _Data._LightDimmer = f; }
void LightInstance::SetLightColorCorrection(Float f) { _Data._LightColorCorrection = f; }
void LightInstance::SetLightToonPriority(Float f) { _Data._LightToonPriority = f; }
void LightInstance::SetLightToonOpacity(Float f) { _Data._LightToonOpacity = f; }
void LightInstance::SetLightType(I32 i) { _Data._LightType = i; }
void LightInstance::SetLightKeyLight(Bool b) { _Data._LightKeyLight = b; }
void LightInstance::SetDynamicOnLightMap(Bool b) { _Data._DynamicOnLightMap = b; }
void LightInstance::SetLightTurnedOn(Bool b) { _Data._LightTurnedOn = b; }
void LightInstance::SetLightWrapAround(Float f) { _Data._LightWrapAround = f; }
void LightInstance::SetLightRenderLayer(I32 i) { _Data._LightRenderLayer = i; }

// Spot light properties
void LightInstance::SetLightSpotInnerRadius(Float f) { _Data._LightSpotInnerRadius = f; }
void LightInstance::SetLightSpotOuterRadius(Float f) { _Data._LightSpotOuterRadius = f; }
void LightInstance::SetLightSpotTexture(Symbol s) { _Data._LightSpotTexture = s; }
void LightInstance::SetLightSpotAlphaMode(I32 i) { _Data._LightSpotAlphaMode = i; }
void LightInstance::SetLightSpotAlpha(Float f) { _Data._LightSpotAlpha = f; }
void LightInstance::SetLightSpotTextureTranslate(Vector2 v) { _Data._LightSpotTextureTranslate = v; }
void LightInstance::SetLightSpotTextureScale(Vector2 v) { _Data._LightSpotTextureScale = v; }
void LightInstance::SetLightSpotTextureShear(Vector2 v) { _Data._LightSpotTextureShear = v; }
void LightInstance::SetLightSpotTextureShearOrigin(Vector2 v) { _Data._LightSpotTextureShearOrigin = v; }
void LightInstance::SetLightSpotTextureRotate(Float f) { _Data._LightSpotTextureRotate = f; }
void LightInstance::SetLightSpotTextureRotateOrigin(Vector2 v) { _Data._LightSpotTextureRotateOrigin = v; }

// Ambient and Rim
void LightInstance::SetLightAmbientOcclusion(Float f) { _Data._LightAmbientOcclusion = f; }
void LightInstance::SetLightRimIntensity(Float f) { _Data._LightRimIntensity = f; }
void LightInstance::SetLightRimWrapAround(Float f) { _Data._LightRimWrapAround = f; }
void LightInstance::SetLightRimOcclusion(Float f) { _Data._LightRimOcclusion = f; }

// Cell shading
void LightInstance::SetCellBlendMode(I32 i) { _Data._CellBlendMode = i; }
void LightInstance::SetCellBlendWeight(Float f) { _Data._CellBlendWeight = f; }
void LightInstance::SetCellLightBlendMask(Float f) { _Data._CellLightBlendMask = f; }
void LightInstance::SetLightStatic(Bool b) { _Data._LightStatic = b; }

void SceneModule<SceneModuleType::LIGHT>::OnSetupAgent(SceneAgent* pAgentGettingCreated)
{
    if(!LightInst)
    {
        LightInst = TTE_NEW_PTR(LightInstance, MEMORY_TAG_SCENE_DATA);

        Meta::ClassInstance props = pAgentGettingCreated->OwningScene->GetAgentProps(pAgentGettingCreated->NameSymbol);

        // Colors
        PropertySet::AddCallback(props, kLightColour, ALLOCATE_METHOD_CALLBACK_1(LightInst, SetLightColour, LightInstance, Colour), TRACKING_REF_LIGHT);
        PropertySet::AddCallback(props, kLightColourDark, ALLOCATE_METHOD_CALLBACK_1(LightInst, SetLightColourDark, LightInstance, Colour), TRACKING_REF_LIGHT);
        PropertySet::AddCallback(props, kCell0Colour, ALLOCATE_METHOD_CALLBACK_1(LightInst, SetCell0Color, LightInstance, Colour), TRACKING_REF_LIGHT);
        PropertySet::AddCallback(props, kCell1Colour, ALLOCATE_METHOD_CALLBACK_1(LightInst, SetCell1Color, LightInstance, Colour), TRACKING_REF_LIGHT);
        PropertySet::AddCallback(props, kCell2Colour, ALLOCATE_METHOD_CALLBACK_1(LightInst, SetCell2Color, LightInstance, Colour), TRACKING_REF_LIGHT);
        PropertySet::AddCallback(props, kCell3Colour, ALLOCATE_METHOD_CALLBACK_1(LightInst, SetCell3Color, LightInstance, Colour), TRACKING_REF_LIGHT);

        // Intensity
        PropertySet::AddCallback(props, kLightIntensity, ALLOCATE_METHOD_CALLBACK_1(LightInst, SetLightIntensity, LightInstance, Float), TRACKING_REF_LIGHT);
        PropertySet::AddCallback(props, kLightIntensityDiffuse, ALLOCATE_METHOD_CALLBACK_1(LightInst, SetLightIntensityDiffuse, LightInstance, Float), TRACKING_REF_LIGHT);
        PropertySet::AddCallback(props, kLightIntensitySpecular, ALLOCATE_METHOD_CALLBACK_1(LightInst, SetLightIntensitySpecular, LightInstance, Float), TRACKING_REF_LIGHT);
        PropertySet::AddCallback(props, kNprSpecularIntensity, ALLOCATE_METHOD_CALLBACK_1(LightInst, SetNPRSpecularIntensity, LightInstance, Float), TRACKING_REF_LIGHT);

        // Distance
        PropertySet::AddCallback(props, kLightMaxDistance, ALLOCATE_METHOD_CALLBACK_1(LightInst, SetLightMaxDistance, LightInstance, Float), TRACKING_REF_LIGHT);
        PropertySet::AddCallback(props, kLightMinDistance, ALLOCATE_METHOD_CALLBACK_1(LightInst, SetLightMinDistance, LightInstance, Float), TRACKING_REF_LIGHT);

        // Shadows
        PropertySet::AddCallback(props, kLightShadowMax, ALLOCATE_METHOD_CALLBACK_1(LightInst, SetLightShadowMax, LightInstance, Float), TRACKING_REF_LIGHT);
        PropertySet::AddCallback(props, kLightShadowDistanceFalloff, ALLOCATE_METHOD_CALLBACK_1(LightInst, SetLightShadowDistanceFalloff, LightInstance, Float), TRACKING_REF_LIGHT);
        PropertySet::AddCallback(props, kLightShadowCascades, ALLOCATE_METHOD_CALLBACK_1(LightInst, SetLightShadowCascades, LightInstance, I32), TRACKING_REF_LIGHT);
        PropertySet::AddCallback(props, kLightShadowBias, ALLOCATE_METHOD_CALLBACK_1(LightInst, SetLightShadowBias, LightInstance, Float), TRACKING_REF_LIGHT);

        // General properties
        PropertySet::AddCallback(props, kLightDimmer, ALLOCATE_METHOD_CALLBACK_1(LightInst, SetLightDimmer, LightInstance, Float), TRACKING_REF_LIGHT);
        PropertySet::AddCallback(props, kLightColorCorrection, ALLOCATE_METHOD_CALLBACK_1(LightInst, SetLightColorCorrection, LightInstance, Float), TRACKING_REF_LIGHT);
        PropertySet::AddCallback(props, kLightToonPriority, ALLOCATE_METHOD_CALLBACK_1(LightInst, SetLightToonPriority, LightInstance, Float), TRACKING_REF_LIGHT);
        PropertySet::AddCallback(props, kLightToonOpacity, ALLOCATE_METHOD_CALLBACK_1(LightInst, SetLightToonOpacity, LightInstance, Float), TRACKING_REF_LIGHT);
        PropertySet::AddCallback(props, kLightType, ALLOCATE_METHOD_CALLBACK_1(LightInst, SetLightType, LightInstance, I32), TRACKING_REF_LIGHT);
        PropertySet::AddCallback(props, kLightKeyLight, ALLOCATE_METHOD_CALLBACK_1(LightInst, SetLightKeyLight, LightInstance, Bool), TRACKING_REF_LIGHT);
        PropertySet::AddCallback(props, kDynamicOnLightMap, ALLOCATE_METHOD_CALLBACK_1(LightInst, SetDynamicOnLightMap, LightInstance, Bool), TRACKING_REF_LIGHT);
        PropertySet::AddCallback(props, kLightTurnedOn, ALLOCATE_METHOD_CALLBACK_1(LightInst, SetLightTurnedOn, LightInstance, Bool), TRACKING_REF_LIGHT);
        PropertySet::AddCallback(props, kLightWrapAround, ALLOCATE_METHOD_CALLBACK_1(LightInst, SetLightWrapAround, LightInstance, Float), TRACKING_REF_LIGHT);
        PropertySet::AddCallback(props, kLightRenderLayer, ALLOCATE_METHOD_CALLBACK_1(LightInst, SetLightRenderLayer, LightInstance, I32), TRACKING_REF_LIGHT);

        // Spot light properties
        PropertySet::AddCallback(props, kLightSpotInnerRadius, ALLOCATE_METHOD_CALLBACK_1(LightInst, SetLightSpotInnerRadius, LightInstance, Float), TRACKING_REF_LIGHT);
        PropertySet::AddCallback(props, kLightSpotOuterRadius, ALLOCATE_METHOD_CALLBACK_1(LightInst, SetLightSpotOuterRadius, LightInstance, Float), TRACKING_REF_LIGHT);
        PropertySet::AddCallback(props, kLightSpotTexture, ALLOCATE_METHOD_CALLBACK_1(LightInst, SetLightSpotTexture, LightInstance, Symbol), TRACKING_REF_LIGHT);
        PropertySet::AddCallback(props, kLightSpotAlphaMode, ALLOCATE_METHOD_CALLBACK_1(LightInst, SetLightSpotAlphaMode, LightInstance, I32), TRACKING_REF_LIGHT);
        PropertySet::AddCallback(props, kLightSpotAlpha, ALLOCATE_METHOD_CALLBACK_1(LightInst, SetLightSpotAlpha, LightInstance, Float), TRACKING_REF_LIGHT);
        PropertySet::AddCallback(props, kLightSpotTextureTranslate, ALLOCATE_METHOD_CALLBACK_1(LightInst, SetLightSpotTextureTranslate, LightInstance, Vector2), TRACKING_REF_LIGHT);
        PropertySet::AddCallback(props, kLightSpotTextureScale, ALLOCATE_METHOD_CALLBACK_1(LightInst, SetLightSpotTextureScale, LightInstance, Vector2), TRACKING_REF_LIGHT);
        PropertySet::AddCallback(props, kLightSpotTextureShear, ALLOCATE_METHOD_CALLBACK_1(LightInst, SetLightSpotTextureShear, LightInstance, Vector2), TRACKING_REF_LIGHT);
        PropertySet::AddCallback(props, kLightSpotTextureShearOrigin, ALLOCATE_METHOD_CALLBACK_1(LightInst, SetLightSpotTextureShearOrigin, LightInstance, Vector2), TRACKING_REF_LIGHT);
        PropertySet::AddCallback(props, kLightSpotTextureRotate, ALLOCATE_METHOD_CALLBACK_1(LightInst, SetLightSpotTextureRotate, LightInstance, Float), TRACKING_REF_LIGHT);
        PropertySet::AddCallback(props, kLightSpotTextureRotateOrigin, ALLOCATE_METHOD_CALLBACK_1(LightInst, SetLightSpotTextureRotateOrigin, LightInstance, Vector2), TRACKING_REF_LIGHT);

        // Ambient and Rim
        PropertySet::AddCallback(props, kLightAmbientOcclusion, ALLOCATE_METHOD_CALLBACK_1(LightInst, SetLightAmbientOcclusion, LightInstance, Float), TRACKING_REF_LIGHT);
        PropertySet::AddCallback(props, kLightRimIntensity, ALLOCATE_METHOD_CALLBACK_1(LightInst, SetLightRimIntensity, LightInstance, Float), TRACKING_REF_LIGHT);
        PropertySet::AddCallback(props, kLightRimWrapAround, ALLOCATE_METHOD_CALLBACK_1(LightInst, SetLightRimWrapAround, LightInstance, Float), TRACKING_REF_LIGHT);
        PropertySet::AddCallback(props, kLightRimOcclusion, ALLOCATE_METHOD_CALLBACK_1(LightInst, SetLightRimOcclusion, LightInstance, Float), TRACKING_REF_LIGHT);

        // Cell shading
        PropertySet::AddCallback(props, kCellBlendMode, ALLOCATE_METHOD_CALLBACK_1(LightInst, SetCellBlendMode, LightInstance, I32), TRACKING_REF_LIGHT);
        PropertySet::AddCallback(props, kCellBlendWeight, ALLOCATE_METHOD_CALLBACK_1(LightInst, SetCellBlendWeight, LightInstance, Float));
        PropertySet::AddCallback(props, kCellLightBlendMask, ALLOCATE_METHOD_CALLBACK_1(LightInst, SetCellLightBlendMask, LightInstance, Float), TRACKING_REF_LIGHT);
        PropertySet::AddCallback(props, kLightStatic, ALLOCATE_METHOD_CALLBACK_1(LightInst, SetLightStatic, LightInstance, Bool), TRACKING_REF_LIGHT);


        PropertySet::CallAllCallbacks(props, pAgentGettingCreated->OwningScene->GetRegistry());
    }
}

void SceneModule<SceneModuleType::LIGHT>::OnModuleRemove(SceneAgent *pAttachedAgent)
{
    Meta::ClassInstance props = pAttachedAgent->OwningScene->GetAgentProps(pAttachedAgent->NameSymbol);
    PropertySet::ClearCallbacks(props, TRACKING_REF_LIGHT);
}

// ============================================== MODULE IMPLEMENTATION ==============================================
// ================================================ SELECTABLE MODULE ================================================
// ============================================== MODULE IMPLEMENTATION ==============================================

#define TRACKING_REF_SELECTABLE 0x1BADB002

void SceneModule<SceneModuleType::SELECTABLE>::SetExtentsMin(Vector3 vec)
{
    BBox._Min = vec;
}

void SceneModule<SceneModuleType::SELECTABLE>::SetExtentsMax(Vector3 vec)
{
    BBox._Max = vec;
}

void SceneModule<SceneModuleType::SELECTABLE>::SetGameSelectable(Bool onOff)
{
    GameSelectable = onOff;
}

void SceneModule<SceneModuleType::SELECTABLE>::OnSetupAgent(SceneAgent* pAgentGettingCreated)
{

    Meta::ClassInstance props = pAgentGettingCreated->OwningScene->GetAgentProps(pAgentGettingCreated->NameSymbol);

    PropertySet::AddCallback(props, kSelectableExtentsMin, ALLOCATE_METHOD_CALLBACK_1(this, SetExtentsMin,
        SceneModule<SceneModuleType::SELECTABLE>, Vector3), TRACKING_REF_SELECTABLE);
    PropertySet::AddCallback(props, kSelectableExtentsMax, ALLOCATE_METHOD_CALLBACK_1(this, SetExtentsMax,
        SceneModule<SceneModuleType::SELECTABLE>, Vector3), TRACKING_REF_SELECTABLE);
    PropertySet::AddCallback(props, kSelectableOnOff, ALLOCATE_METHOD_CALLBACK_1(this, SetGameSelectable,
        SceneModule<SceneModuleType::SELECTABLE>, Bool), TRACKING_REF_SELECTABLE);
    PropertySet::CallAllCallbacks(props, pAgentGettingCreated->OwningScene->GetRegistry());
}

void SceneModule<SceneModuleType::SELECTABLE>::OnModuleRemove(SceneAgent *pAttachedAgent)
{
    Meta::ClassInstance props = pAttachedAgent->OwningScene->GetAgentProps(pAttachedAgent->NameSymbol);
    PropertySet::ClearCallbacks(props, TRACKING_REF_SELECTABLE);
}

// ============================================== MODULE IMPLEMENTATION ==============================================
// ================================================= ROLLOVER MODULE =================================================
// ============================================== MODULE IMPLEMENTATION ==============================================

#define TRACKING_REF_ROLLOVER 0x1BADBEEF

void SceneModule<SceneModuleType::ROLLOVER>::SetCursorProps(Symbol v)
{
    CursorPropHandle = v;
}

void SceneModule<SceneModuleType::ROLLOVER>::SetCursorMesh(Symbol v)
{
    MeshHandle = v;
}

void SceneModule<SceneModuleType::ROLLOVER>::SetText(String v)
{
    Text = v;
}

void SceneModule<SceneModuleType::ROLLOVER>::SetTextBackground(Colour c)
{
    TextBackground = c;
}

void SceneModule<SceneModuleType::ROLLOVER>::SetTextColour(Colour c)
{
    TextForeground = c;
}

void SceneModule<SceneModuleType::ROLLOVER>::OnSetupAgent(SceneAgent* pAgentGettingCreated)
{

    Meta::ClassInstance props = pAgentGettingCreated->OwningScene->GetAgentProps(pAgentGettingCreated->NameSymbol);

    PropertySet::AddCallback(props, kRolloverText, ALLOCATE_METHOD_CALLBACK_1(this, SetText,
        SceneModule<SceneModuleType::ROLLOVER>, String), TRACKING_REF_ROLLOVER);
    PropertySet::AddCallback(props, kRolloverCursorProps, ALLOCATE_METHOD_CALLBACK_1(this, SetCursorProps,
        SceneModule<SceneModuleType::ROLLOVER>, Symbol), TRACKING_REF_ROLLOVER);
    PropertySet::AddCallback(props, kRolloverTextColour, ALLOCATE_METHOD_CALLBACK_1(this, SetTextColour,
        SceneModule<SceneModuleType::ROLLOVER>, Colour), TRACKING_REF_ROLLOVER);
    PropertySet::AddCallback(props, kRolloverTextBackgroundColour, ALLOCATE_METHOD_CALLBACK_1(this,
        SetTextBackground, SceneModule<SceneModuleType::ROLLOVER>, Colour), TRACKING_REF_ROLLOVER);
    PropertySet::AddCallback(props, kRolloverMesh, ALLOCATE_METHOD_CALLBACK_1(this,
        SetCursorMesh, SceneModule<SceneModuleType::ROLLOVER>, Symbol), TRACKING_REF_ROLLOVER);
    PropertySet::CallAllCallbacks(props, pAgentGettingCreated->OwningScene->GetRegistry());
}

void SceneModule<SceneModuleType::ROLLOVER>::OnModuleRemove(SceneAgent *pAttachedAgent)
{
    Meta::ClassInstance props = pAttachedAgent->OwningScene->GetAgentProps(pAttachedAgent->NameSymbol);
    PropertySet::ClearCallbacks(props, TRACKING_REF_ROLLOVER);
}

// ============================================== MODULE IMPLEMENTATION ==============================================
// ================================================= D CHOICE MODULE =================================================
// ============================================== MODULE IMPLEMENTATION ==============================================

#define TRACKING_REF_DLG_CHOICE 0x1BADBEFE

void SceneModule<SceneModuleType::DIALOG_CHOICE>::SetChoice(String ch)
{
    Choice = ch;
}

void SceneModule<SceneModuleType::DIALOG_CHOICE>::OnModuleRemove(SceneAgent *pAttachedAgent)
{
    Meta::ClassInstance props = pAttachedAgent->OwningScene->GetAgentProps(pAttachedAgent->NameSymbol);
    PropertySet::ClearCallbacks(props, TRACKING_REF_DLG_CHOICE);
}

void SceneModule<SceneModuleType::DIALOG_CHOICE>::OnSetupAgent(SceneAgent *pAgentGettingCreated)
{

    Meta::ClassInstance props = pAgentGettingCreated->OwningScene->GetAgentProps(pAgentGettingCreated->NameSymbol);

    PropertySet::AddCallback(props, kDialogChoiceChoice, ALLOCATE_METHOD_CALLBACK_1(this, SetChoice,
        SceneModule<SceneModuleType::DIALOG_CHOICE>, String), TRACKING_REF_DLG_CHOICE);
    PropertySet::CallAllCallbacks(props, pAgentGettingCreated->OwningScene->GetRegistry());
}

// ============================================== MODULE IMPLEMENTATION ==============================================
// ================================================== DIALOG MODULE ==================================================
// ============================================== MODULE IMPLEMENTATION ==============================================

#define TRACKING_REF_DLG 0x1BADBAD1

void SceneModule<SceneModuleType::DIALOG>::SetName(String ch)
{
    Name = ch;
}

void SceneModule<SceneModuleType::DIALOG>::SetResource(Symbol ch)
{
    Resource = ch;
}

void SceneModule<SceneModuleType::DIALOG>::OnModuleRemove(SceneAgent *pAttachedAgent)
{
    Meta::ClassInstance props = pAttachedAgent->OwningScene->GetAgentProps(pAttachedAgent->NameSymbol);
    PropertySet::ClearCallbacks(props, TRACKING_REF_DLG);
}

void SceneModule<SceneModuleType::DIALOG>::OnSetupAgent(SceneAgent *pAgentGettingCreated)
{

    Meta::ClassInstance props = pAgentGettingCreated->OwningScene->GetAgentProps(pAgentGettingCreated->NameSymbol);

    PropertySet::AddCallback(props, kDialogName, ALLOCATE_METHOD_CALLBACK_1(this, SetName,
        SceneModule<SceneModuleType::DIALOG>, String), TRACKING_REF_DLG);
    PropertySet::AddCallback(props, kDialogResource, ALLOCATE_METHOD_CALLBACK_1(this, SetResource,
        SceneModule<SceneModuleType::DIALOG>, Symbol), TRACKING_REF_DLG);
    PropertySet::CallAllCallbacks(props, pAgentGettingCreated->OwningScene->GetRegistry());
}

// ============================================== MODULE IMPLEMENTATION ==============================================
// ================================================== W. ANIM MODULE =================================================
// ============================================== MODULE IMPLEMENTATION ==============================================

#define TRACKING_REF_WANIM 0x1BADBADE

void SceneModule<SceneModuleType::WALK_ANIMATOR>::SetIdleAnim(Symbol ch)
{
    IdleAnim = ch;
}

void SceneModule<SceneModuleType::WALK_ANIMATOR>::SetForwardAnim(Symbol ch)
{
    ForwardAnim = ch;
}

void SceneModule<SceneModuleType::WALK_ANIMATOR>::OnModuleRemove(SceneAgent *pAttachedAgent)
{
    Meta::ClassInstance props = pAttachedAgent->OwningScene->GetAgentProps(pAttachedAgent->NameSymbol);
    PropertySet::ClearCallbacks(props, TRACKING_REF_WANIM);
}

void SceneModule<SceneModuleType::WALK_ANIMATOR>::OnSetupAgent(SceneAgent *pAgentGettingCreated)
{

    Meta::ClassInstance props = pAgentGettingCreated->OwningScene->GetAgentProps(pAgentGettingCreated->NameSymbol);

    PropertySet::AddCallback(props, kWalkAnimatorIdleAnimation, ALLOCATE_METHOD_CALLBACK_1(this, SetIdleAnim,
        SceneModule<SceneModuleType::WALK_ANIMATOR>, Symbol), TRACKING_REF_WANIM);
    PropertySet::AddCallback(props, kWalkAnimatorForwardAnimation, ALLOCATE_METHOD_CALLBACK_1(this, SetForwardAnim,
        SceneModule<SceneModuleType::WALK_ANIMATOR>, Symbol), TRACKING_REF_WANIM);
    PropertySet::CallAllCallbacks(props, pAgentGettingCreated->OwningScene->GetRegistry());
}

// ============================================== MODULE IMPLEMENTATION ==============================================
// ================================================== TRIGGER MODULE =================================================
// ============================================== MODULE IMPLEMENTATION ==============================================

#define TRACKING_REF_TRIGGER 0xABADFACE

void SceneModule<SceneModuleType::TRIGGER>::SetEnterCallback(String ch)
{
    EnterCallback = ch;
}

void SceneModule<SceneModuleType::TRIGGER>::SetExitCallback(String h)
{
    ExitCallback = h;
}

void SceneModule<SceneModuleType::TRIGGER>::SetOnOff(Bool h)
{
    Enabled = h;
}

void SceneModule<SceneModuleType::TRIGGER>::OnModuleRemove(SceneAgent *pAttachedAgent)
{
    Meta::ClassInstance props = pAttachedAgent->OwningScene->GetAgentProps(pAttachedAgent->NameSymbol);
    PropertySet::ClearCallbacks(props, TRACKING_REF_TRIGGER);
}

void SceneModule<SceneModuleType::TRIGGER>::OnSetupAgent(SceneAgent *pAgentGettingCreated)
{

    Meta::ClassInstance props = pAgentGettingCreated->OwningScene->GetAgentProps(pAgentGettingCreated->NameSymbol);

    PropertySet::AddCallback(props, kTriggerEnabled, ALLOCATE_METHOD_CALLBACK_1(this, SetOnOff,
        SceneModule<SceneModuleType::TRIGGER>, Bool), TRACKING_REF_TRIGGER);
    PropertySet::AddCallback(props, kTriggerEnterCallback, ALLOCATE_METHOD_CALLBACK_1(this, SetEnterCallback,
        SceneModule<SceneModuleType::TRIGGER>, String), TRACKING_REF_TRIGGER);
    PropertySet::AddCallback(props, kTriggerExitCallback, ALLOCATE_METHOD_CALLBACK_1(this, SetExitCallback,
        SceneModule<SceneModuleType::TRIGGER>, String), TRACKING_REF_TRIGGER);
    PropertySet::CallAllCallbacks(props, pAgentGettingCreated->OwningScene->GetRegistry());
}

// ============================================== MODULE IMPLEMENTATION ==============================================
// ================================================== NAV CAM MODULE =================================================
// ============================================== MODULE IMPLEMENTATION ==============================================

#define TRACKING_REF_NAV_CAM 0xFACEBEEF

void SceneModule<SceneModuleType::NAV_CAM>::SetAnimationFile(Symbol file)
{
    AnimationFile = file;
}

void SceneModule<SceneModuleType::NAV_CAM>::SetAnimationTime(Float time)
{
    AnimationTime = time;
}

void SceneModule<SceneModuleType::NAV_CAM>::SetDampen(Float v)
{
    Dampen = v;
}

void SceneModule<SceneModuleType::NAV_CAM>::SetTriggerHPercent(Float v)
{
    TriggerHPercent = v;
}

void SceneModule<SceneModuleType::NAV_CAM>::SetTriggerVPercent(Float v)
{
    TriggerVPercent = v;
}

void SceneModule<SceneModuleType::NAV_CAM>::SetTargetAgent(String v)
{
    TargetAgent = v;
}

void SceneModule<SceneModuleType::NAV_CAM>::SetTargetOffset(Vector3 v)
{
    TargetOffset = v;
}

void SceneModule<SceneModuleType::NAV_CAM>::SetOrbitMin(Polar v)
{
    OrbitMin = v;
}

void SceneModule<SceneModuleType::NAV_CAM>::SetOrbitMax(Polar v)
{
    OrbitMax = v;
}

void SceneModule<SceneModuleType::NAV_CAM>::SetOrbitOffset(Polar v)
{
    OrbitOffset = v;
}

void SceneModule<SceneModuleType::NAV_CAM>::SetMode(Enum<NavCamMode> v)
{
    NavMode = v.Value;
}

void SceneModule<SceneModuleType::NAV_CAM>::OnModuleRemove(SceneAgent *pAttachedAgent)
{
    Meta::ClassInstance props = pAttachedAgent->OwningScene->GetAgentProps(pAttachedAgent->NameSymbol);
    PropertySet::ClearCallbacks(props, TRACKING_REF_NAV_CAM);
}

void SceneModule<SceneModuleType::NAV_CAM>::OnSetupAgent(SceneAgent *pAgentGettingCreated)
{

    Meta::ClassInstance props = pAgentGettingCreated->OwningScene->GetAgentProps(pAgentGettingCreated->NameSymbol);

    PropertySet::AddCallback(props, kNavCamOrbitMin, ALLOCATE_METHOD_CALLBACK_1(this, SetOrbitMin,
        SceneModule<SceneModuleType::NAV_CAM>, Polar), TRACKING_REF_NAV_CAM);
    PropertySet::AddCallback(props, kNavCamOrbitMax, ALLOCATE_METHOD_CALLBACK_1(this, SetOrbitOffset,
        SceneModule<SceneModuleType::NAV_CAM>, Polar), TRACKING_REF_NAV_CAM);
    PropertySet::AddCallback(props, kNavCamOrbitOffset, ALLOCATE_METHOD_CALLBACK_1(this, SetOrbitMax,
        SceneModule<SceneModuleType::NAV_CAM>, Polar), TRACKING_REF_NAV_CAM);
    PropertySet::AddCallback(props, kNavCamTargetAgent, ALLOCATE_METHOD_CALLBACK_1(this, SetTargetAgent,
        SceneModule<SceneModuleType::NAV_CAM>, String), TRACKING_REF_NAV_CAM);
    PropertySet::AddCallback(props, kNavCamTargetOffset, ALLOCATE_METHOD_CALLBACK_1(this, SetTargetOffset,
        SceneModule<SceneModuleType::NAV_CAM>, Vector3), TRACKING_REF_NAV_CAM);
    PropertySet::AddCallback(props, kNavCamAnimation, ALLOCATE_METHOD_CALLBACK_1(this, SetAnimationFile,
        SceneModule<SceneModuleType::NAV_CAM>, Symbol), TRACKING_REF_NAV_CAM);
    PropertySet::AddCallback(props, kNavCamAnimationTime, ALLOCATE_METHOD_CALLBACK_1(this, SetAnimationTime,
        SceneModule<SceneModuleType::NAV_CAM>, Float), TRACKING_REF_NAV_CAM);
    PropertySet::AddCallback(props, kNavCamDampen, ALLOCATE_METHOD_CALLBACK_1(this, SetDampen,
        SceneModule<SceneModuleType::NAV_CAM>, Float), TRACKING_REF_NAV_CAM);
    PropertySet::AddCallback(props, kNavCamTriggerHPercent, ALLOCATE_METHOD_CALLBACK_1(this, SetTriggerHPercent,
        SceneModule<SceneModuleType::NAV_CAM>, Float), TRACKING_REF_NAV_CAM);
    PropertySet::AddCallback(props, kNavCamTriggerVPercent, ALLOCATE_METHOD_CALLBACK_1(this, SetTriggerVPercent,
        SceneModule<SceneModuleType::NAV_CAM>, Float), TRACKING_REF_NAV_CAM);
    PropertySet::AddCallback(props, kNavCamMode, ALLOCATE_METHOD_CALLBACK_1(this, SetMode,
        SceneModule<SceneModuleType::NAV_CAM>, Enum<NavCamMode>), TRACKING_REF_NAV_CAM);
    PropertySet::CallAllCallbacks(props, pAgentGettingCreated->OwningScene->GetRegistry());
}

// ============================================== MODULE IMPLEMENTATION ==============================================
// ================================================== PATH TO MODULE =================================================
// ============================================== MODULE IMPLEMENTATION ==============================================

#define TRACKING_REF_PATHTO 0xABADBEEA

void SceneModule<SceneModuleType::PATH_TO>::SetWalkRadius(Float h)
{
    WalkRadius = h;
}

void SceneModule<SceneModuleType::PATH_TO>::OnModuleRemove(SceneAgent *pAttachedAgent)
{
    Meta::ClassInstance props = pAttachedAgent->OwningScene->GetAgentProps(pAttachedAgent->NameSymbol);
    PropertySet::ClearCallbacks(props, TRACKING_REF_PATHTO);
}

void SceneModule<SceneModuleType::PATH_TO>::OnSetupAgent(SceneAgent *pAgentGettingCreated)
{

    Meta::ClassInstance props = pAgentGettingCreated->OwningScene->GetAgentProps(pAgentGettingCreated->NameSymbol);
    PropertySet::AddCallback(props, kPathToWalkRadius, ALLOCATE_METHOD_CALLBACK_1(this,
            SetWalkRadius, SceneModule<SceneModuleType::PATH_TO>, Float), TRACKING_REF_PATHTO);
    PropertySet::CallAllCallbacks(props, pAgentGettingCreated->OwningScene->GetRegistry());
}
