require("ToolLibrary/Game/Common/LuaPropertySet.lua")
require("ToolLibrary/Game/VersionCRC.lua")

require("ToolLibrary/Game/BN100/D3DTexture.lua")
require("ToolLibrary/Game/BN100/D3DMesh.lua")

-- registers two types: one with baseclass_containerinterface and one without (vers exists for both). pass in k and v table (or k being SArray N)
function RegisterBoneCollection(containerInterfaceTbl, name, k, v)
	local withBaseclass = { VersionIndex = 0 } -- eg DCArray_String_(1j6j2xe).vers. each array has two versions, one without the baseclass container member
	withBaseclass.Name = name
	withBaseclass.Members = {}
	withBaseclass.Members[1] = { Name = "Baseclass_ContainerInterface", Class = containerInterfaceTbl, Flags = kMetaMemberMemoryDisable + kMetaMemberBaseClass }
	withBaseclass.Members[2] = { Name = "mSize", Class = kMetaInt, Flags = kMetaMemberMemoryDisable }
	withBaseclass.Members[3] = { Name = "mCapacity", Class = kMetaInt, Flags = kMetaMemberMemoryDisable }
	MetaRegisterCollection(withBaseclass, k, v)

	local without = { VersionIndex = 1 } -- two array types exist, one with no members (here) and the one above....
	without.Name = name
	without.Members = {}
	without.Members[1] = { Name = "mSize", Class = kMetaInt, Flags = kMetaMemberMemoryDisable }
	without.Members[2] = { Name = "mCapacity", Class = kMetaInt, Flags = kMetaMemberMemoryDisable }
	MetaRegisterCollection(without, k, v)

	return withBaseclass, without -- return both
end

-- registers Handle<T>. ensure the name is correct.
function RegisterBoneHandle(name)
	local MetaHandle = { VersionIndex = 0 }
	MetaHandle.Name = name
	MetaHandle.Flags = kMetaClassNonBlocked + kMetaClassIntrinsic -- not in version headers ? idk why
	MetaHandle.Members = {}
	-- below member doesnt exist in game (has custom serialiser) but lets just store it as a di
	MetaHandle.Members[1] = { Name = "mHandle", Class = kMetaClassSymbol, Flags = kMetaMemberVersionDisable } -- serialise yes, no version hash though.
	MetaRegisterClass(MetaHandle)
	return MetaHandle
end

function RegisterBoneScriptEnum(typeName)
	local scriptEnum = NewClass("ScriptEnum:" .. typeName, 0)
	scriptEnum.Members[1] = NewMember("mCurValue", kMetaClassString, 0)
	MetaRegisterClass(scriptEnum)
	return scriptEnum
end

function RegisterBoneANMValue(base, typeName)
	local anmV = NewClass(typeName, 0)
	anmV.Members[1] = NewMember("Baseclass_AnimationValueInterfaceBase", base)
	MetaRegisterClass(anmV)
	return anmV
end

function RegisterBoneKeyframedValue(ci, typedInterface, typeName, typeTable)
	local sample = NewClass("class KeyframedValue<" .. typeName .. ">::Sample", 0)
	sample.Members[1] = NewMember("mTime", kMetaFloat)
	sample.Members[2] = NewMember("mbInterpolateToNextKey", kMetaBool)
	sample.Members[3] = NewMember("mValue", typeTable)
	MetaRegisterClass(sample)

	arraySample, _ = RegisterBoneCollection(ci, "class DCArray<class KeyframedValue<" .. typeName .. ">::Sample>", nil, sample)

	local keyframed = NewClass("class KeyframedValue<" .. typeName .. ">", 0)
	keyframed.Members[1] = NewMember("Baseclass_AnimatedValueInterface<T>", NewProxyClass(typedInterface.Name .. " *", "mVal", typedInterface))
	keyframed.Members[2] = NewMember("mMinVal", typeTable)
	keyframed.Members[3] = NewMember("mMaxVal", typeTable)
	keyframed.Members[4] = NewMember("mSamples", arraySample)
	MetaRegisterClass(keyframed)

	return keyframed
end

function RegisterBone100(vendor, platform)

	MetaSetVersionFn("VersionCRC_V0") -- Set version CRC calculation function, before anything else.
	MetaRegisterIntrinsics() -- First, we must register all of the intrinsic types.

	local MetaVec2 = { VersionIndex = 0 }
	MetaVec2.Name = "class Vector2"
	MetaVec2.Flags = kMetaClassNonBlocked
	MetaVec2.Members = {}
	MetaVec2.Members[1] = { Name = "x", Class = kMetaFloat }
	MetaVec2.Members[2] = { Name = "y", Class = kMetaFloat }
	MetaRegisterClass(MetaVec2)

	local MetaVec3 = { VersionIndex = 0 }
	MetaVec3.Name = "class Vector3"
	MetaVec3.Flags = kMetaClassNonBlocked
	MetaVec3.Members = {}
	MetaVec3.Members[1] = { Name = "x", Class = kMetaFloat }
	MetaVec3.Members[2] = { Name = "y", Class = kMetaFloat }
	MetaVec3.Members[3] = { Name = "z", Class = kMetaFloat }
	MetaRegisterClass(MetaVec3)

	local MetaPolar = { VersionIndex = 0 }
	MetaPolar.Name = "class Polar"
	MetaPolar.Members = {}
	MetaPolar.Members[1] = { Name = "mR", Class = kMetaFloat }
	MetaPolar.Members[2] = { Name = "mTheta", Class = kMetaFloat }
	MetaPolar.Members[3] = { Name = "mPhi", Class = kMetaFloat }
	MetaRegisterClass(MetaPolar)

	local MetaVec3Empty = { VersionIndex = 1 }
	MetaVec3Empty.Name = "class Vector3"
	MetaRegisterClass(MetaVec3Empty) -- empty but a vers for this type exists

	local MetaCol = { VersionIndex = 0 }
	MetaCol.Name = "class Color"
	MetaCol.Flags = kMetaClassNonBlocked
	MetaCol.Members = {}
	MetaCol.Members[1] = { Name = "r", Class = kMetaFloat }
	MetaCol.Members[2] = { Name = "g", Class = kMetaFloat }
	MetaCol.Members[3] = { Name = "b", Class = kMetaFloat }
	MetaCol.Members[4] = { Name = "a", Class = kMetaFloat }
	MetaRegisterClass(MetaCol)

	local MetaRect = { VersionIndex = 0 }
	MetaRect.Name = "class Rect"
	MetaRect.Members = {}
	MetaRect.Flags = kMetaClassNonBlocked
	MetaRect.Members[1] = { Name = "left", Class = kMetaInt }
	MetaRect.Members[2] = { Name = "right", Class = kMetaInt }
	MetaRect.Members[3] = { Name = "top", Class = kMetaInt }
	MetaRect.Members[4] = { Name = "bottom", Class = kMetaInt }
	MetaRegisterClass(MetaRect)

	local MetaTRectF = { VersionIndex = 0 }
	MetaTRectF.Name = "class TRect<float>"
	MetaTRectF.Members = {}
	MetaTRectF.Flags = kMetaClassNonBlocked
	MetaTRectF.Members[1] = { Name = "left", Class = kMetaFloat }
	MetaTRectF.Members[2] = { Name = "right", Class = kMetaFloat }
	MetaTRectF.Members[3] = { Name = "top", Class = kMetaFloat }
	MetaTRectF.Members[4] = { Name = "bottom", Class = kMetaFloat }
	MetaRegisterClass(MetaTRectF)

	local MetaQuat = { VersionIndex = 0 }
	MetaQuat.Name = "class Quaternion"
	MetaQuat.Flags = kMetaClassNonBlocked
	MetaQuat.Members = {}
	MetaQuat.Members[1] = { Name = "x", Class = kMetaFloat }
	MetaQuat.Members[2] = { Name = "y", Class = kMetaFloat }
	MetaQuat.Members[3] = { Name = "z", Class = kMetaFloat }
	MetaQuat.Members[4] = { Name = "w", Class = kMetaFloat }
	MetaRegisterClass(MetaQuat)

	local ColEmpty = NewClass("class Color", 1) -- .vers exists for empty color struct
	MetaRegisterClass(ColEmpty)

	local MetaFlags = { VersionIndex = 0 }
	MetaFlags.Name = "class Flags"
	MetaFlags.Flags = kMetaClassNonBlocked
	MetaFlags.Members = {}
	MetaFlags.Members[1] = { Name = "mFlags", Class = kMetaInt }
	MetaRegisterClass(MetaFlags)

	local MetaFlagsEmpty = NewClass("class Flags", 1)
	MetaRegisterClass(MetaFlagsEmpty) -- empty version exists

	local MetaCI = { VersionIndex = 0 } -- baseclass for containers
	MetaCI.Name = "class ContainerInterface *" -- see below
	MetaCI.Flags = kMetaClassAbstract
	MetaRegisterClass(MetaCI)

	-- ALL HANDLE TYPES
	hAnim = RegisterBoneHandle("class Handle<class Animation>")
	hChore = RegisterBoneHandle("class Handle<class Chore>")
	hMesh = RegisterBoneHandle("class Handle<class D3DMesh>")
	hTexture = RegisterBoneHandle("class Handle<class D3DTexture>")
	hDlgResource = RegisterBoneHandle("class Handle<class DialogResource>")
	hProp = RegisterBoneHandle("class Handle<class PropertySet>")
	hAud = RegisterBoneHandle("class Handle<class AudioData")
	hScene = RegisterBoneHandle("class Handle<class Scene>")
	hSkeleton = RegisterBoneHandle("class Handle<class Skeleton>")
	hStyle = RegisterBoneHandle("class Handle<class StyleGuide>")
	hVoiceData = RegisterBoneHandle("class Handle<class VoiceData>")
	hWalkBoxes = RegisterBoneHandle("class Handle<class WalkBoxes>")
	hFont = RegisterBoneHandle("class Handle<class Font>")
	
	-- array of prop file names (for parent list in prop). only list in this game, rest are arrays etc
	local MetaHandleArrayProp = { VersionIndex = 0}
	MetaHandleArrayProp.Name = "class List<class Handle<class PropertySet> >"
	MetaHandleArrayProp.Members = {} -- no members in this one for some reason
	MetaRegisterCollection(MetaHandleArrayProp, nil, hProp)

	-- .PROP FILES
	local prop = {VersionIndex = 0 }
	prop.Extension = "prop"
	prop.Name = "class PropertySet"
	prop.Members = {}
	prop.Members[1] = { Name = "mPropertyFlags", Class = MetaFlags }
	prop.Members[2] = { Name = "mParentList", Class = MetaHandleArrayProp }
	prop.Serialiser = "SerialisePropertySet_V0"
	MetaRegisterClass(prop)

	-- .AAM FILES
	local aam = { VersionIndex = 0 }
	aam.Name = "class ActorAgentMapper"
	aam.Members = {}
	aam.Members[1] = { Name = "mActorAgentMap", Class = prop }
	MetaRegisterClass(aam)

	local animOrChore = { VersionIndex = 0 }
	animOrChore.Name = "class AnimOrChore"
	animOrChore.Members = {}
	animOrChore.Members[1] = { Name = "mhAnim", Class = hAnim }
	animOrChore.Members[2] = { Name = "mhChore", Class = hChore }
	MetaRegisterClass(animOrChore)

	local bb = NewClass("class BoundingBox", 0)
	bb.Flags = kMetaClassNonBlocked
	bb.Members[1] = NewMember("mMin", MetaVec3, 0)
	bb.Members[2] = NewMember("mMax", MetaVec3, 0)
	MetaRegisterClass(bb)

	local enumNavMode = NewClass("enum NavCam::Mode", 0)
	enumNavMode.Flags = kMetaClassNonBlocked -- this is a wrapper. needed so versions match, just remaps int
	enumNavMode.Members[1] = NewMember("mVal", kMetaInt, kMetaMemberVersionDisable)
	AddEnum(enumNavMode, 1, "eNone", 1)
	AddEnum(enumNavMode, 1, "eLookAt", 2)
	AddEnum(enumNavMode, 1, "eOrbit", 3)
	AddEnum(enumNavMode, 1, "eAnimation_Track", 4)
	AddEnum(enumNavMode, 1, "eAnimation_Time", 5)
	AddEnum(enumNavMode, 1, "eAnimation_Pos_ProcedualLookAt", 6)
	AddEnum(enumNavMode, 1, "eScenePosition", 7)
	MetaRegisterClass(enumNavMode)

	local navCamMode = NewClass("class Enum<enum NavCam::Mode,1,2>", 0)
	navCamMode.Members[1] = NewMember("mVal", enumNavMode, kMetaMemberEnum) 
	-- mVal type name for versioning must be enum NavCam::Mode, not int, so we need the wrapper above
	MetaRegisterClass(navCamMode)

	structNavMode = MetaRegisterDuplicate(navCamMode, "struct NavCam::EnumMode", 0)

	local imapMapping = NewClass("class InputMapper::EventMapping", 0)
	imapMapping.Members[1] = NewMember("mInputCode", NewProxyClass("enum InputCode", "mVal", kMetaInt) , kMetaMemberEnum)
	imapMapping.Members[2] = NewMember("mEvent", NewProxyClass("enum InputMapper::EventType", "mVal", kMetaInt), kMetaMemberEnum)
	imapMapping.Members[3] = NewMember("mScriptFunction", kMetaClassString, 0)
	MetaRegisterClass(imapMapping)

	local lightType = NewClass("class LightType", 0)
	lightType.Members[1] = NewMember("mLightType", kMetaInt, 0)
	MetaRegisterClass(lightType)

	local textAlign = NewClass("class TextAlignmentType", 0)
	textAlign.Members[1] = NewMember("mAlignmentType", kMetaInt, 0)
	MetaRegisterClass(textAlign)

	RegisterBoneScriptEnum("Chore")
	RegisterBoneScriptEnum("Cursors")
	RegisterBoneScriptEnum("Evidence Comparison")
	RegisterBoneScriptEnum("Evidence State")
	RegisterBoneScriptEnum("Evidence")
	RegisterBoneScriptEnum("EvidenceUpdate")
	RegisterBoneScriptEnum("FactLog")
	RegisterBoneScriptEnum("Lab Equipment Type")
	RegisterBoneScriptEnum("Location")
	RegisterBoneScriptEnum("Reconstructions")
	RegisterBoneScriptEnum("Suspect")
	RegisterBoneScriptEnum("Topics")
	RegisterBoneScriptEnum("Trinity Evidence")
	RegisterBoneScriptEnum("Trinity Suspect")
	RegisterBoneScriptEnum("Victim")
	RegisterBoneScriptEnum("View")
	RegisterBoneScriptEnum("Warrant")
	RegisterBoneScriptEnum("Warrent") -- they added this probably because of spelling .. lol.

	local rangef = NewClass("class TRange<float>", 0)
	rangef.Flags = kMetaClassNonBlocked
	rangef.Members[1] = NewMember("min", kMetaFloat)
	rangef.Members[2] = NewMember("max", kMetaFloat)
	MetaRegisterClass(rangef)

	local transform = NewClass("class Transform", 0)
	transform.Members[1] = NewMember("mRot", MetaQuat)
	transform.Members[2] = NewMember("mTrans", MetaVec3)
	MetaRegisterClass(transform)

	arrayHandleChore, _ = RegisterBoneCollection(MetaCI, "class DCArray<class Handle<class Chore> >", nil, hChore)
	arrayHandleAnim , _ = RegisterBoneCollection(MetaCI, "class DCArray<class Handle<class Animation> >", nil, hAnim)
	arrayHandleAnimChore , _ = RegisterBoneCollection(MetaCI, "class DCArray<class Handle<class AnimOrChore> >", nil, hAnim)
	arrayAnimChore , _ = RegisterBoneCollection(MetaCI, "class DCArray<class AnimOrChore>", nil, hAnim)

	arrayClassString, _ = RegisterBoneCollection(MetaCI, "class DCArray<class String>", nil, kMetaClassString)

	local enumActiveDuring = NewProxyClass("struct ActingPalette::EnumActiveDuring", "mVal", kMetaInt)

	local actingPalette0 = NewClass("class ActingPalette", 1)
	actingPalette0.Members[1] = NewMember("mName", kMetaClassString)
	actingPalette0.Members[2] = NewMember("mPriority", kMetaInt)
	actingPalette0.Members[3] = NewMember("mbActiveByDefault", kMetaBool)
	actingPalette0.Members[4] = NewMember("mTimeBetweenActions", rangef)
	actingPalette0.Members[5] = NewMember("mChores", arrayHandleChore)
	actingPalette0.Members[6] = NewMember("mAnimations", arrayHandleAnim)
	MetaRegisterClass(actingPalette0)

	local actingPalette3 = NewClass("class ActingPalette", 3)
	actingPalette3.Members[1] = NewMember("mName", kMetaClassString)
	actingPalette3.Members[2] = NewMember("mPriority", kMetaInt)
	actingPalette3.Members[3] = NewMember("mbActiveByDefault", kMetaBool)
	actingPalette3.Members[4] = NewMember("mTimeBetweenActions", rangef)
	actingPalette3.Members[5] = NewMember("mChores", arrayHandleChore)
	actingPalette3.Members[6] = NewMember("mAnimations", arrayHandleAnim)
	actingPalette3.Members[7] = NewMember("mActiveDuring", enumActiveDuring)
	MetaRegisterClass(actingPalette3)

	local actingPalette1 = NewClass("class ActingPalette", 0)
	actingPalette1.Members[1] = NewMember("mName", kMetaClassString)
	actingPalette1.Members[2] = NewMember("mPriority", kMetaInt)
	actingPalette1.Members[3] = NewMember("mbActiveByDefault", kMetaBool)
	actingPalette1.Members[4] = NewMember("mTimeBetweenActions", rangef)
	actingPalette1.Members[5] = NewMember("mAnimsOrChores", arrayAnimChore) -- THIS IS THE ACTUAL ONE USED IN THE GAME. OTHERS MAY EXIST BUT NOT USED (AT ALL?)
	actingPalette1.Members[6] = NewMember("mActiveDuring", enumActiveDuring)
	MetaRegisterClass(actingPalette1)

	local actingPalette2 = NewClass("class ActingPalette", 2)
	actingPalette2.Members[1] = NewMember("mName", kMetaClassString)
	actingPalette2.Members[2] = NewMember("mPriority", kMetaInt)
	actingPalette2.Members[3] = NewMember("mbActiveByDefault", kMetaBool)
	actingPalette2.Members[4] = NewMember("mTimeBetweenActions", rangef)
	actingPalette2.Members[5] = NewMember("mAnimsOrChores", arrayHandleAnimChore) -- array of handles to animchores (?)
	actingPalette2.Members[6] = NewMember("mActiveDuring", enumActiveDuring)
	MetaRegisterClass(actingPalette2)

	arrayPalette, _ = RegisterBoneCollection(MetaCI, "class DCArray<class ActingPalette>", nil, actingPalette1)

	local actingPaletteClass = NewClass("class ActingPaletteClass", 0)
	actingPaletteClass.Members[1] = NewMember("mName", kMetaClassString)
	actingPaletteClass.Members[2] = NewMember("mKeywords", arrayClassString)
	actingPaletteClass.Members[3] = NewMember("mPalettes", arrayPalette)
	MetaRegisterClass(actingPaletteClass)

	local audStreamed = NewClass("struct AudioData::Streamed", 0)
	audStreamed.Members[1] = NewMember("mStreamRegionSize", kMetaInt)
	audStreamed.Members[2] = NewMember("mStreamBufferSecs", kMetaFloat)
	MetaRegisterClass(audStreamed)

	-- .AUD FILES
	local aud = NewClass("class AudioData", 0)
	aud.Extension = "aud"
	aud.Members[1] = NewMember("mFilename", kMetaClassString)
	aud.Members[2] = NewMember("mLength", kMetaFloat)
	aud.Members[3] = NewMember("mbStreamed", kMetaBool)
	aud.Members[4] = NewMember("mDataFormat", kMetaInt)
	aud.Members[5] = NewMember("mBytesPerSecond", kMetaInt)
	aud.Members[6] = NewMember("mDSBufferBytes", kMetaInt) -- DataStream buffer size
	aud.Members[7] = NewMember("mStreamInfo", audStreamed)
	MetaRegisterClass(aud)

	local anmBase = NewClass("class AnimationValueInterfaceBase", 0)
	anmBase.Members[1] = NewMember("mName", kMetaClassString)
	anmBase.Members[2] = NewMember("mFlags", kMetaInt)
	MetaRegisterClass(anmBase)

	animatedBool = RegisterBoneANMValue(anmBase, "class AnimatedValueInterface<bool>")
	animatedColor = RegisterBoneANMValue(anmBase, "class AnimatedValueInterface<class Color>")
	animatedFloat = RegisterBoneANMValue(anmBase, "class AnimatedValueInterface<float>")
	animatedQuat = RegisterBoneANMValue(anmBase, "class AnimatedValueInterface<class Quaternion>")
	animatedStr = RegisterBoneANMValue(anmBase, "class AnimatedValueInterface<class String>")
	animatedVec3 = RegisterBoneANMValue(anmBase, "class AnimatedValueInterface<class Vector3>")

	RegisterBoneKeyframedValue(MetaCI, animatedBool, "bool", kMetaBool)
	RegisterBoneKeyframedValue(MetaCI, animatedColor, "class Color", MetaCol)
	RegisterBoneKeyframedValue(MetaCI, animatedFloat, "float", kMetaFloat)
	RegisterBoneKeyframedValue(MetaCI, animatedQuat, "class Quaternion", MetaQuat)
	RegisterBoneKeyframedValue(MetaCI, animatedStr, "class String", kMetaClassString)
	RegisterBoneKeyframedValue(MetaCI, animatedVec3, "class Vector3", MetaVec3)

	local anm1 = NewClass("class Animation", 1) -- not sure if this used anywhere
	anm1.Extension = "anm"
	anm1.Members[1] = NewMember("mName", kMetaClassString)
	anm1.Members[2] = NewMember("mLength", kMetaFloat)
	MetaRegisterClass(anm1)

	-- .ANM FILES
	local anm0 = NewClass("class Animation", 0) -- this one is used
	anm0.Extension = "anm"
	anm0.Members[1] = NewMember("mFlags", MetaFlags)
	anm0.Members[2] = NewMember("mName", kMetaClassString)
	anm0.Members[3] = NewMember("mLength", kMetaFloat)
	MetaRegisterClass(anm0)
	-- ANIMATION TODO: has a custom serialiser.

	imapMappingArray, _ = RegisterBoneCollection(MetaCI, "class DCArray<class InputMapper::EventMapping>", nil, imapMapping)

	-- .IMAP FILES
	local imap = NewClass("class InputMapper", 0)
	imap.Extension = "imap"
	imap.Members[1] = NewMember("mName", kMetaClassString)
	imap.Members[2] = NewMember("mMappedEvents", imapMappingArray)
	MetaRegisterClass(imap)

	-- .LANGRES FILES
	local langr = NewClass("class LanguageResource", 0)
	langr.Extension = "langres"
	langr.Members[1] = NewMember("mId", kMetaInt)
	langr.Members[2] = NewMember("mPrefix", kMetaClassString)
	langr.Members[3] = NewMember("mText", kMetaClassString)
	langr.Members[4] = NewMember("mhAnimation", hAnim)
	langr.Members[5] = NewMember("mhVoiceData", hVoiceData)
	langr.Members[6] = NewMember("mShared", kMetaBool)
	langr.Members[7] = NewMember("mAllowSharing", kMetaBool)
	langr.Members[8] = NewMember("mbNoAnim", kMetaBool)
	langr.Members[9] = NewMember("mFlags", kMetaInt)
	MetaRegisterClass(langr) -- has customer serialise which just calls default

	local langr1 = NewClass("class LanguageResource", 1) -- older versions below (likely during development, VERS files were spit out if needed, hence these..)
	langr1.Members[1] = NewMember("mId", kMetaInt)
	langr1.Members[2] = NewMember("mPrefix", kMetaClassString)
	langr1.Members[3] = NewMember("mText", kMetaClassString)
	langr1.Members[4] = NewMember("mhAnimation", hAnim)
	langr1.Members[5] = NewMember("mhVoiceData", hVoiceData)
	langr1.Extension = "langres"
	MetaRegisterClass(langr1)

	local langr2 = NewClass("class LanguageResource", 2)
	langr2.Members[1] = NewMember("mId", kMetaInt)
	langr2.Members[2] = NewMember("mPrefix", kMetaClassString)
	langr2.Members[3] = NewMember("mText", kMetaClassString)
	langr2.Members[4] = NewMember("mhAnimation", hAnim)
	langr2.Members[5] = NewMember("mhVoiceData", hVoiceData)
	langr2.Members[6] = NewMember("mShared", kMetaBool)
	langr2.Members[7] = NewMember("mAllowSharing", kMetaBool)
	langr2.Extension = "langres"
	MetaRegisterClass(langr2) -- has customer serialise which just calls default

	local langr3 = NewClass("class LanguageResource", 3)
	langr3.Members[1] = NewMember("mId", kMetaInt)
	langr3.Members[2] = NewMember("mPrefix", kMetaClassString)
	langr3.Members[3] = NewMember("mText", kMetaClassString)
	langr3.Members[4] = NewMember("mhAnimation", hAnim)
	langr3.Members[5] = NewMember("mhVoiceData", hVoiceData)
	langr3.Members[6] = NewMember("mShared", kMetaBool)
	langr3.Extension = "langres"
	MetaRegisterClass(langr3) -- has customer serialise which just calls default

	local langp = NewClass("class LanguageResourceProxy", 0)
	langp.Members[1] = NewMember("mLangID", kMetaInt)
	MetaRegisterClass(langp)

	local langp1 = NewClass("class LanguageResourceProxy", 1) -- for some reason the default version has less. maybe not cached? altho maybe changed to serialise disable
	langp1.Members[1] = NewMember("mPrefixProxy", kMetaClassString)
	langp1.Members[2] = NewMember("mTextProxy", kMetaClassString)
	langp1.Members[3] = NewMember("mLangID", kMetaInt)
	MetaRegisterClass(langp1)

	mapIntLangRes, _ = RegisterBoneCollection(MetaCI, "class Map<int,class LanguageResource,struct std::less<int> >", kMetaInt, langr) -- dont change name! hashed.

	-- .LANGDB FILES
	local langdb = NewClass("class LanguageDatabase", 0)
	langdb.Extension = "langdb"
	langdb.Members[1] = NewMember("mLanguageResources", mapIntLangRes)
	MetaRegisterClass(langdb)

	local rule0 = NewClass("class Rule", 0) -- default used
	rule0.Members[1] = NewMember("mName", kMetaClassString)
	rule0.Members[2] = NewMember("mFlags", MetaFlags)
	rule0.Members[3] = NewMember("mConditionalProperties", prop)
	rule0.Members[4] = NewMember("mActionProperties", prop)
	rule0.Members[5] = NewMember("mbVersionHasAgents", kMetaBool)
	MetaRegisterClass(rule0)

	local rule1 = NewClass("class Rule", 1)
	rule1.Members[1] = NewMember("mhLogicProps", hProp)
	rule1.Members[2] = NewMember("mConditionalProperties", prop)
	rule1.Members[3] = NewMember("mActionProperties", prop)
	MetaRegisterClass(rule1)

	local rule2 = NewClass("class Rule", 2)
	rule2.Members[1] = NewMember("mhLogicProps", hProp)
	rule2.Members[2] = NewMember("mFlags", MetaFlags)
	rule2.Members[3] = NewMember("mConditionalProperties", prop)
	rule2.Members[4] = NewMember("mActionProperties", prop)
	MetaRegisterClass(rule2)

	local rule3 = NewClass("class Rule", 3)
	rule3.Members[1] = NewMember("mName", kMetaClassString)
	rule3.Members[2] = NewMember("mFlags", MetaFlags)
	rule3.Members[3] = NewMember("mConditionalProperties", prop)
	rule3.Members[4] = NewMember("mActionProperties", prop)
	MetaRegisterClass(rule3)

	local rule4 = NewClass("class Rule", 4)
	rule4.Members[1] = NewMember("mhLogicProps", hProp)
	rule4.Members[2] = NewMember("mName", kMetaClassString)
	rule4.Members[3] = NewMember("mFlags", MetaFlags)
	rule4.Members[4] = NewMember("mConditionalProperties", prop)
	rule4.Members[5] = NewMember("mActionProperties", prop)
	MetaRegisterClass(rule4)

	local ruleInf = NewClass("struct Rule::AgentInfo", 0)
	ruleInf.Members[1] = NewMember("mName", kMetaClassString)
	ruleInf.Members[2] = NewMember("mConditionalProperties", prop)
	ruleInf.Members[3] = NewMember("mActionProperties", prop)
	MetaRegisterClass(ruleInf)

	-- .RULES FILES. TODO we have a custom serialiser
	local rules = NewClass("class Rules", 0) -- there are two other versions but they are empty (unusable files)
	rules.Extension = "rules"
	rules.Members[1] = NewMember("mFlags", MetaFlags)
	rules.Members[2] = NewMember("mhLogicProps", hProp)
	MetaRegisterClass(rules)

	local sgAgent = NewClass("class SaveGame::AgentInfo", 0)
	sgAgent.Members[1] = NewMember("mAgentName", kMetaClassString)
	sgAgent.Members[2] = NewMember("mPosition", MetaVec3)
	sgAgent.Members[3] = NewMember("mQuaternion", MetaQuat)
	sgAgent.Members[4] = NewMember("mbAttached", kMetaBool)
	sgAgent.Members[5] = NewMember("mAttachedToAgent", kMetaClassString)
	sgAgent.Members[6] = NewMember("mAttachedToNode", kMetaClassString)
	MetaRegisterClass(sgAgent)

	local sgAgent1 = NewClass("class SaveGame::AgentInfo", 1) -- older version
	sgAgent1.Members[1] = NewMember("mAgentName", kMetaClassString)
	sgAgent1.Members[2] = NewMember("mPosition", MetaVec3)
	sgAgent1.Members[3] = NewMember("mQuaternion", MetaQuat)
	MetaRegisterClass(sgAgent1)

	arraySaveGameAgent, _ = RegisterBoneCollection(MetaCI, "class DCArray<class SaveGame::AgentInfo>", nil, sgAgent)

	local setString = { VersionIndex = 0 }
	setString.Name = "class Set<class String,struct std::less<class String> >"
	setString.Members = {}
	setString.Members[1] = { Name = "Baseclass_ContainerInterface", Class = MetaCI, Flags = kMetaMemberMemoryDisable + kMetaMemberBaseClass }
	MetaRegisterCollection(setString, nil, kMetaClassString)

	local sg0 = NewClass("class SaveGame", 0)
	sg0.Extension = "save"
	sg0.Members[1] = NewMember("mLuaDoFile", kMetaClassString)
	sg0.Members[2] = NewMember("mAgentInfo", arraySaveGameAgent)
	sg0.Members[3] = NewMember("mRuntimePropNames", setString)
	sg0.Members[4] = NewMember("mAdditionalPropNames", setString)
	MetaRegisterClass(sg0)

	local sg1 = NewClass("class SaveGame", 1) -- older versions
	sg1.Extension = "save"
	sg1.Members[1] = NewMember("mLuaDoFile", kMetaClassString)
	sg1.Members[2] = NewMember("mAgentInfo", arraySaveGameAgent)
	sg1.Members[3] = NewMember("mRuntimePropNames", setString)
	MetaRegisterClass(sg1)

	local sg2 = NewClass("class SaveGame", 2) -- older versions
	sg2.Extension = "save"
	sg2.Members[1] = NewMember("mLuaDoFile", kMetaClassString)
	sg2.Members[2] = NewMember("mRuntimePropNames", setString)
	MetaRegisterClass(sg2)

	local sceneAgent0 = NewClass("class Scene::AgentInfo", 0) -- scene agent information. the are like 5 other old versions, won't bother with them, not used.
	sceneAgent0.Members[1] = NewMember("mAgentName", kMetaClassString)
	sceneAgent0.Members[2] = NewMember("mStartPos", MetaVec3)
	sceneAgent0.Members[3] = NewMember("mStartQuat", MetaQuat)
	sceneAgent0.Members[4] = NewMember("mbVisible", kMetaBool)
	sceneAgent0.Members[5] = NewMember("mbTransient", kMetaBool)
	sceneAgent0.Members[6] = NewMember("mbAttached", kMetaBool)
	sceneAgent0.Members[7] = NewMember("mAttachAgent", kMetaClassString)
	sceneAgent0.Members[8] = NewMember("mAttachAgentNode", kMetaClassString)
	sceneAgent0.Members[9] = NewMember("mAgentSceneProps", prop)
	sceneAgent0.Members[10] = NewMember("mbMembersImportedIntoSceneProps", kMetaBool)
	MetaRegisterClass(sceneAgent0)

	local scene = NewClass("class Scene", 0) -- TODO custom serialiser
	scene.Extension = "scene"
	scene.Members[1] = NewMember("mbHidden", kMetaBool)
	scene.Members[2] = NewMember("mName", kMetaClassString)
	MetaRegisterClass(scene)

	mapStringFloat, _ = RegisterBoneCollection(MetaCI, "class Map<class String,float,struct std::less<class String> >", kMetaClassString, kMetaFloat)

	local sklEntry = NewClass("class Skeleton::Entry", 0)
	sklEntry.Members[1] = NewMember("mJointName", kMetaClassString)
	sklEntry.Members[2] = NewMember("mParentName", kMetaClassString)
	sklEntry.Members[3] = NewMember("mParentIndex", kMetaInt)
	sklEntry.Members[4] = NewMember("mLocalPos", MetaVec3)
	sklEntry.Members[5] = NewMember("mLocalQuat", MetaQuat)
	sklEntry.Members[6] = NewMember("mRestXform", transform)
	sklEntry.Members[7] = NewMember("mGlobalTranslationScale", MetaVec3)
	sklEntry.Members[8] = NewMember("mLocalTranslationScale", MetaVec3)
	sklEntry.Members[9] = NewMember("mAnimTranslationScale", MetaVec3)
	sklEntry.Members[10] = NewMember("mResourceGroupMembership", mapStringFloat)
	MetaRegisterClass(sklEntry)

	arraySklEntry, _ = RegisterBoneCollection(MetaCI, "class DCArray<class Skeleton::Entry>", nil, sklEntry)

	local skl = NewClass("class Skeleton", 0)
	skl.Extension = "skl"
	skl.Members[1] = NewMember("mEntries", arraySklEntry)
	MetaRegisterClass(skl)

	arrayPaletteClass, _ = RegisterBoneCollection(MetaCI, "class DCArray<class ActingPaletteClass>", nil, actingPaletteClass)

	local style0 = NewClass("class StyleGuide", 0)
	style0.Extension = "style"
	style0.Members[1] = NewMember("mDefPaletteClassIndex", kMetaInt)
	style0.Members[2] = NewMember("mPaletteClasses", arrayPaletteClass)
	style0.Members[3] = NewMember("mbGeneratesLookAts", kMetaBool)
	MetaRegisterClass(style0)

	local style1 = NewClass("class StyleGuide", 1)
	style1.Extension = "style"
	style1.Members[1] = NewMember("mDefPaletteClassIndex", kMetaInt)
	style1.Members[2] = NewMember("mPaletteClasses", arrayPaletteClass)
	MetaRegisterClass(style1)

	local style2 = NewClass("class StyleGuide", 2)
	style2.Extension = "style"
	style2.Members[1] = NewMember("mPaletteClasses", arrayPaletteClass)
	MetaRegisterClass(style2)

	local style3 = NewClass("class StyleGuide", 3)
	style3.Extension = "style"
	style3.Members[1] = NewMember("mDefPaletteClassIndex", kMetaInt)
	style3.Members[2] = NewMember("mPaletteClasses", arrayPaletteClass)
	style3.Members[3] = NewMember("mAgentName", kMetaClassString)
	MetaRegisterClass(style3)

	arrayBool, _ = RegisterBoneCollection(MetaCI, "class DArray<bool>", nil, kMetaBool)
	arrayDInt, _ = RegisterBoneCollection(MetaCI, "class DArray<int>", nil, kMetaInt)
	arrayDCInt, _ = RegisterBoneCollection(MetaCI, "class DCArray<int>", nil, kMetaInt)

	local styleRef = NewClass("class StyleGuideRef", 0) -- has customer serialiser, just calls default
	styleRef.Members[1] = NewMember("mhStyleGuide", hStyle)
	styleRef.Members[2] = NewMember("mPaletteClassIndex", kMetaInt)
	styleRef.Members[3] = NewMember("mPalettesUsed", arrayBool)
	MetaRegisterClass(styleRef)

	local vox0 = NewClass("class VoiceData", 0) -- TODO custom serialiser
	vox0.Extension = "vox"
	vox0.Members[1] = NewMember("mbEncrypted", kMetaBool)
	vox0.Members[2] = NewMember("mLength", kMetaFloat)
	vox0.Members[3] = NewMember("mAllPacketsSize", kMetaInt)
	vox0.Members[4] = NewMember("mPacketSamples", kMetaInt)
	vox0.Members[5] = NewMember("mSampleRate", kMetaInt)
	vox0.Members[6] = NewMember("mMode", kMetaInt)
	vox0.Members[7] = NewMember("mPacketPositions", arrayDInt)
	MetaRegisterClass(vox0)

	local vox1 = NewClass("class VoiceData", 1) -- TODO custom serialiser
	vox1.Extension = "vox"
	vox1.Members[1] = NewMember("mLength", kMetaFloat)
	vox1.Members[2] = NewMember("mAllPacketsSize", kMetaInt)
	vox1.Members[3] = NewMember("mPacketSamples", kMetaInt)
	vox1.Members[4] = NewMember("mSampleRate", kMetaInt)
	vox1.Members[5] = NewMember("mMode", kMetaInt)
	vox1.Members[6] = NewMember("mPacketPositions", arrayDInt)
	MetaRegisterClass(vox1)

	local wboxEdge = NewClass("class WalkBoxes::Edge", 0) -- there about a billion different vers file for each of these. only doing the one used
	wboxEdge.Members[1] = NewMember("mV1", kMetaInt)
	wboxEdge.Members[2] = NewMember("mV2", kMetaInt)
	wboxEdge.Members[3] = NewMember("mEdgeDest", kMetaInt)
	wboxEdge.Members[4] = NewMember("mEdgeDestEdge", kMetaInt)
	wboxEdge.Members[5] = NewMember("mEdgeDir", kMetaInt)
	wboxEdge.Members[6] = NewMember("mMaxRadius", kMetaFloat)
	MetaRegisterClass(wboxEdge)

	local array3Int = NewClass("class SArray<int,3>", 0)
	MetaRegisterCollection(array3Int, 3, kMetaInt)

	local array4Int = NewClass("class SArray<int,4>", 0)
	MetaRegisterCollection(array4Int, 4, kMetaInt)

	local array3Float = NewClass("class SArray<float,3>", 0)
	MetaRegisterCollection(array3Float, 3, kMetaFloat)

	local array3Edge = NewClass("class SArray<class WalkBoxes::Edge,3>", 0)
	MetaRegisterCollection(array3Edge, 3, wboxEdge)

	local wboxTri = NewClass("class WalkBoxes::Tri", 0)
	wboxTri.Members[1] = NewMember("mFlags", MetaFlags)
	wboxTri.Members[2] = NewMember("mNormal", kMetaInt)
	wboxTri.Members[3] = NewMember("mQuadBuddy", kMetaInt)
	wboxTri.Members[4] = NewMember("mMaxRadius", kMetaFloat)
	wboxTri.Members[5] = NewMember("mVerts", array3Int)
	wboxTri.Members[6] = NewMember("mEdgeInfo", array3Edge)
	wboxTri.Members[7] = NewMember("mVertOffsets", array3Int)
	wboxTri.Members[8] = NewMember("mVertScales", array3Float)
	MetaRegisterClass(wboxTri)

	local wboxVert = NewClass("class WalkBoxes::Vert", 0)
	wboxVert.Members[1] = NewMember("mFlags", MetaFlags)
	wboxVert.Members[2] = NewMember("mPos", MetaVec3)
	MetaRegisterClass(wboxVert)

	local wboxQuad = NewClass("class WalkBoxes::Quad", 0)
	wboxQuad.Members[1] = NewMember("mVerts", array4Int)
	MetaRegisterClass(wboxQuad)

	arrayVec3, _ = RegisterBoneCollection(MetaCI, "class DCArray<class Vector3>", nil, MetaVec3)
	arrayQuad, _ = RegisterBoneCollection(MetaCI, "class DCArray<class WalkBoxes::Quad>", nil, wboxQuad)
	arrayVert, _ = RegisterBoneCollection(MetaCI, "class DCArray<class WalkBoxes::Vert>", nil, wboxVert)
	arrayTri, _ = RegisterBoneCollection(MetaCI, "class DCArray<class WalkBoxes::Tri>", nil, wboxTri)

	local wbox = NewClass("class WalkBoxes", 0) -- TODO custom serialiser
	wbox.Extension = "wbox"
	wbox.Members[1] = NewMember("mName", kMetaClassString)
	wbox.Members[2] = NewMember("mTris", arrayTri)
	wbox.Members[3] = NewMember("mVerts", arrayVert)
	wbox.Members[4] = NewMember("mNormals", arrayVec3)
	wbox.Members[5] = NewMember("mQuads", arrayQuad)
	MetaRegisterClass(wbox)

	local choreAgentAttach = NewClass("class ChoreAgent::Attachment", 0)
	choreAgentAttach.Members[1] = NewMember("mbDoAttach", kMetaBool)
	choreAgentAttach.Members[2] = NewMember("mAttachTo", kMetaClassString)
	choreAgentAttach.Members[3] = NewMember("mAttachToNode", kMetaClassString)
	choreAgentAttach.Members[4] = NewMember("mAttachPos", MetaVec3)
	choreAgentAttach.Members[5] = NewMember("mAttachQuat", MetaQuat)
	choreAgentAttach.Members[6] = NewMember("mbAttachPreserveWorldPos", kMetaBool)
	choreAgentAttach.Members[7] = NewMember("mbLeaveAttachedWhenComplete", kMetaBool)
	MetaRegisterClass(choreAgentAttach)

	local choreAgent = NewClass("class ChoreAgent", 0) -- TODO custom serialiser
	choreAgent.Members[1] = NewMember("mAgentName", kMetaClassString)
	choreAgent.Members[2] = NewMember("mFlags", MetaFlags)
	choreAgent.Members[3] = NewMember("mResources", arrayDCInt)
	choreAgent.Members[4] = NewMember("mAttachment", choreAgentAttach)
	MetaRegisterClass(choreAgent)

	local choreBlock = NewClass("class ChoreResource::Block", 0)
	choreBlock.Members[1] = NewMember("mStartTime", kMetaFloat)
	choreBlock.Members[2] = NewMember("mEndTime", kMetaFloat)
	choreBlock.Members[3] = NewMember("mbLoopingBlock", kMetaBool)
	choreBlock.Members[4] = NewMember("mScale", kMetaFloat)
	MetaRegisterClass(choreBlock)

	arrayChoreBlock, _ = RegisterBoneCollection(MetaCI, "class DCArray<class ChoreResource::Block>", nil, choreBlock)

	local choreResource = NewClass("class ChoreResource", 0) -- TODO custom serialiser
	choreResource.Members[1] = NewMember("mResName", kMetaClassString)
	choreResource.Members[2] = NewMember("mResLength", kMetaFloat)
	choreResource.Members[3] = NewMember("mPriority", kMetaInt)
	choreResource.Members[4] = NewMember("mFlags", MetaFlags)
	choreResource.Members[5] = NewMember("mResourceGroup", kMetaClassString)
	choreResource.Members[6] = NewMember("mControlAnimation", anm0)
	choreResource.Members[7] = NewMember("mBlocks", arrayChoreBlock)
	choreResource.Members[8] = NewMember("mbNoPose", kMetaBool)
	choreResource.Members[9] = NewMember("mbEmbedded", kMetaBool)
	choreResource.Members[10] = NewMember("mbEnabled", kMetaBool)
	choreResource.Members[11] = NewMember("mbIsAgentResource", kMetaBool)
	choreResource.Members[12] = NewMember("mbViewGraphs", kMetaBool)
	choreResource.Members[13] = NewMember("mbViewProperties", kMetaBool)
	choreResource.Members[14] = NewMember("mbViewResourceGroups", kMetaBool)
	choreResource.Members[15] = NewMember("mResourceProperties", prop)
	choreResource.Members[16] = NewMember("mResourceGroupInclude", mapStringFloat)
	choreResource.Members[17] = NewMember("mAAStatus", NewProxyClass("enum ChoreResource::AAStatus", "mVal", kMetaInt))
	MetaRegisterClass(choreResource)

	arrayChoreAgent, _ = RegisterBoneCollection(MetaCI, "class DCArray<class ChoreAgent>", nil, choreAgent)
	arrayChoreResource, _ = RegisterBoneCollection(MetaCI, "class DCArray<class ChoreResource>", nil, choreResource)

	local chore = NewClass("class Chore", 0) -- TODO custom serialiser
	chore.Extension = "chore"
	chore.Members[1] = NewMember("mName", kMetaClassString)
	chore.Members[2] = NewMember("mbResetNavCamsOnExit", kMetaBool)
	chore.Members[3] = NewMember("mLength", kMetaFloat)
	chore.Members[4] = NewMember("mNumResources", kMetaInt)
	chore.Members[5] = NewMember("mNumAgents", kMetaInt)
	chore.Members[6] = NewMember("mResources", arrayChoreResource)
	chore.Members[7] = NewMember("mAgents", arrayChoreAgent)
	chore.Members[8] = NewMember("mEditorProps", prop)
	chore.Members[9] = NewMember("mbChoreBackgroundFade", kMetaBool)
	chore.Members[10] = NewMember("mbChoreBackgroundLoop", kMetaBool)
	chore.Members[11] = NewMember("mChoreSceneFile", kMetaClassString)
	MetaRegisterClass(chore)

	mesh = RegisterBoneD3DMesh(platform, vendor, bb, hTexture, arrayDCInt, MetaCol, MetaCI)

	tex = RegisterBoneD3DTexture(vendor, platform)

	local glyph = NewClass("struct Font::GlyphInfo", 0)
	glyph.Members[1] = NewMember("mTexturePage", kMetaInt)
	glyph.Members[2] = NewMember("mGlyph", MetaTRectF)
	MetaRegisterClass(glyph)

	arrayGlyph, _ = RegisterBoneCollection(MetaCI, "class DCArray<struct Font::GlyphInfo>", nil, glyph)
	arrayTex, _ = RegisterBoneCollection(MetaCI, "class DCArray<class D3DTexture>", nil, tex)

	local font = NewClass("class Font", 0) -- todo custom ser
	font.Extension = "font"
	font.Members[1] = NewMember("mName", kMetaClassString)
	font.Members[2] = NewMember("mbFiltered", kMetaBool)
	font.Members[3] = NewMember("mHeight", kMetaFloat)
	font.Members[4] = NewMember("mGlyphInfo", arrayGlyph)
	font.Members[5] = NewMember("mTexturePages", arrayTex)
	MetaRegisterClass(font)

	local dlg = NewClass("class DialogResource", 0) -- custom serialiser
	dlg.Extension = "dlg"
	dlg.Members[1] = NewMember("miNextDialogID", kMetaInt)
	dlg.Members[2] = NewMember("miNextBranchID", kMetaInt)
	dlg.Members[3] = NewMember("miNextItemID", kMetaInt)
	dlg.Members[4] = NewMember("miNextExchangeID", kMetaInt)
	dlg.Members[5] = NewMember("miNextLineID", kMetaInt)
	dlg.Members[6] = NewMember("miNextTextID", kMetaInt)
	dlg.Members[7] = NewMember("miNextChoreID", kMetaInt)
	dlg.Members[8] = NewMember("mDialogs", arrayDInt)
	dlg.Members[9] = NewMember("mSoloItems", arrayDInt)
	dlg.Members[10] = NewMember("mTexts", arrayDInt)
	MetaRegisterClass(dlg)

	local dlgBase = NewClass("class DialogBase *", 0) -- all dialog ones have custom serialisers. ignore the *. it needs to be there! error in engine
	dlgBase.Members[1] = NewMember("mDialogElemType", NewProxyClass("enum DialogUtils::DialogElemT", "mVal", kMetaInt))
	dlgBase.Members[2] = NewMember("mUID", kMetaInt)
	dlgBase.Members[3] = NewMember("mVisConditions", prop)
	dlgBase.Members[4] = NewMember("mPostActions", prop)
	dlgBase.Members[5] = NewMember("mhBackgroundChore", hChore)
	dlgBase.Members[6] = NewMember("mRule", rule0)
	dlgBase.Members[7] = NewMember("mbHasStyleGuides", kMetaBool)
	dlgBase.Members[8] = NewMember("mDependentVisBranch", kMetaClassString)
	MetaRegisterClass(dlgBase)

	local dlgBranch = NewClass("class DialogBranch", 0)
	dlgBranch.Members[1] = NewMember("Baseclass_DialogBase", dlgBase)
	dlgBranch.Members[2] = NewMember("mName", kMetaClassString)
	dlgBranch.Members[3] = NewMember("mItems", arrayDInt)
	dlgBranch.Members[4] = NewMember("mEnterItemID", kMetaInt)
	dlgBranch.Members[5] = NewMember("mExitItemID", kMetaInt)
	dlgBranch.Members[6] = NewMember("mEnterItems", arrayDInt)
	dlgBranch.Members[7] = NewMember("mExitItems", arrayDInt)
	dlgBranch.Members[8] = NewMember("mEnterScript", kMetaClassString)
	dlgBranch.Members[9] = NewMember("mExitScript", kMetaClassString)
	MetaRegisterClass(dlgBranch)

	local dlgDlg = NewClass("class DialogDialog", 0)
	dlgDlg.Members[1] = NewMember("Baseclass_DialogBase", dlgBase)
	dlgDlg.Members[2] = NewMember("mName", kMetaClassString)
	dlgDlg.Members[3] = NewMember("mBranches", arrayDInt)
	MetaRegisterClass(dlgDlg)

	local dlgEx = NewClass("class DialogExchange", 0)
	dlgEx.Members[1] = NewMember("Baseclass_DialogBase", dlgBase)
	dlgEx.Members[2] = NewMember("mLines", arrayDInt)
	dlgEx.Members[3] = NewMember("mBranchLink", kMetaClassString)
	dlgEx.Members[4] = NewMember("mEnterScript", kMetaClassString)
	dlgEx.Members[5] = NewMember("mExitScript", kMetaClassString)
	dlgEx.Members[6] = NewMember("mDispTextProxy", langp)
	dlgEx.Members[7] = NewMember("mExitTrigger", kMetaInt)
	dlgEx.Members[8] = NewMember("mhChore", hChore)
	MetaRegisterClass(dlgEx)

	local dlgItem = NewClass("class DialogItem", 0)
	dlgItem.Members[1] = NewMember("Baseclass_DialogBase", dlgBase)
	dlgItem.Members[2] = NewMember("mExchanges", arrayDInt)
	dlgItem.Members[3] = NewMember("mName", kMetaClassString)
	dlgItem.Members[4] = NewMember("mDispTextProxy", langp)
	dlgItem.Members[5] = NewMember("mPlaybackMode", kMetaInt)
	dlgItem.Members[6] = NewMember("mEnterScript", kMetaClassString)
	dlgItem.Members[7] = NewMember("mExitScript", kMetaClassString)
	dlgItem.Members[8] = NewMember("mExitTrigger", kMetaInt)
	dlgItem.Members[9] = NewMember("mBranchLink", kMetaClassString)
	dlgItem.Members[10] = NewMember("mbSpoken", kMetaBool)
	dlgItem.Members[11] = NewMember("mbFallbackModeOn", kMetaBool)
	dlgItem.Members[12] = NewMember("mFallbackInput", kMetaInt)
	dlgItem.Members[13] = NewMember("mbResetCurExchangeOnBranchReEntry", kMetaBool)
	MetaRegisterClass(dlgItem)

	local dlgLine = NewClass("class DialogLine", 0)
	dlgLine.Members[1] = NewMember("Baseclass_DialogBase", dlgBase)
	dlgLine.Members[2] = NewMember("mLangResProxy", langp)
	MetaRegisterClass(dlgLine)

	local dlgText = NewClass("class DialogText", 0)
	dlgText.Members[1] = NewMember("Baseclass_DialogBase", dlgBase)
	dlgText.Members[2] = NewMember("mName", kMetaClassString)
	dlgText.Members[3] = NewMember("mLangResProxy", langp)
	MetaRegisterClass(dlgText)

	RegisterBoneCollection(MetaCI, "class DCArray<class Handle<class AudioData> >", nil, hAud)
	RegisterBoneCollection(MetaCI, "class Map<class String,class String,struct std::less<class String> >", kMetaClassString, kMetaClassString)
  
	MetaDumpVersions() -- dbg out

	return true
end


--[=====[ Below is the contents of nametable.dat, shipped along with the macos versions. in old games, typenames didn't match on different compilers
so they used this (before having a function which changes typenames by removing 'class ' 'std::' and stuff which worked on all platforms).
some of this is wrong, for example 18ContainerInterface (clang format has length first for simple types) should be class ContainerInterface, but is 
class ContainerInterface * (a pointer version!?)

6d_bool
bool
N9AudioData8StreamedE
struct AudioData::Streamed
N13ChoreResource8AAStatusE
enum ChoreResource::AAStatus
9InputCode
enum InputCode
N11InputMapper9EventTypeE
enum InputMapper::EventType
N6NavCam8EnumModeE
struct NavCam::EnumMode
N6NavCam4ModeE
enum NavCam::Mode
3MapI6StringS0_St4lessIS0_EE
class Map<class String,class String,struct std::less<class String> >
3MapI6String13StyleGuideRefSt4lessIS0_EE
class Map<class String,class StyleGuideRef,struct std::less<class String> >
3SetI6StringSt4lessIS0_EE
class Set<class String,struct std::less<class String> >
3MapI6StringPN11PropertySet7KeyInfoESt4lessIS0_EE
class Map<class String,class PropertySet::KeyInfo *,struct std::less<class String> >
7DCArrayIS_IN7D3DMesh12PaletteEntryEEE
class DCArray<class DCArray<struct D3DMesh::PaletteEntry> >
7DCArrayIN7D3DMesh12PaletteEntryEE
class DCArray<struct D3DMesh::PaletteEntry>
N7D3DMesh12PaletteEntryE
struct D3DMesh::PaletteEntry
P18ContainerInterface
class ContainerInterface *
7DCArrayIiE
class DCArray<int>
3MapI6StringfSt4lessIS0_EE
class Map<class String,float,struct std::less<class String> >
3MapIi16LanguageResourceSt4lessIiEE
class Map<int,class LanguageResource,struct std::less<int> >
6DArrayIiE
class DArray<int>
P10DialogBase
class DialogBase *
N11DialogUtils11DialogElemTE
enum DialogUtils::DialogElemT
a
signed char
b
bool
i
int
c
char
d
double
e
long double
f
float
h
unsigned char
j
unsigned int
l
long
m
unsigned long
s
short
t
unsigned short
v
void
w
wchar_t
x
long long
y
unsigned long long
z
...
10D3DTexture
class D3DTexture
10Quaternion
class Quaternion
10StyleGuide
class StyleGuide
11AnimOrChore
class AnimOrChore
11InputMapper
class InputMapper
11PropertySet
class PropertySet
13StyleGuideRef
class StyleGuideRef
14DialogResource
class DialogResource
14KeyframedValueI10QuaternionE
class KeyframedValue<class Quaternion>
14KeyframedValueI5ColorE
class KeyframedValue<class Color>
14KeyframedValueI6StringE
class KeyframedValue<class String>
14KeyframedValueI6d_boolE
class KeyframedValue<bool>
14KeyframedValueI7Vector3E
class KeyframedValue<class Vector3>
14KeyframedValueIfE
class KeyframedValue<float>
14ProceduralEyes
class ProceduralEyes
16ActorAgentMapper
class ActorAgentMapper
16LanguageDatabase
class LanguageDatabase
17Procedural_LookAt
class Procedural_LookAt
17TextAlignmentType
class TextAlignmentType
21CompressedVector3Keys
class CompressedVector3Keys
23Procedural_LookAt_Value
class Procedural_LookAt_Value
24CompressedQuaternionKeys
class CompressedQuaternionKeys
24ProceduralEyes_Eye_Value
class ProceduralEyes_Eye_Value
27ProceduralEyes_EyeLid_Value
class ProceduralEyes_EyeLid_Value
4Font
class Font
4Rect
class Rect
4Rule
class Rule
5Chore
class Chore
5Color
class Color
5Polar
class Polar
5Rules
class Rules
5Scene
class Scene
6HandleI10D3DTextureE
class Handle<class D3DTexture>
6HandleI10StyleGuideE
class Handle<class StyleGuide>
6HandleI11InputMapperE
class Handle<class InputMapper>
6HandleI11PropertySetE
class Handle<class PropertySet>
6HandleI14DialogResourceE
class Handle<class DialogResource>
6HandleI16ActorAgentMapperE
class Handle<class ActorAgentMapper>
6HandleI16LanguageDatabaseE
class Handle<class LanguageDatabase>
6HandleI16LanguageResourceE
class Handle<class LanguageResource>
6HandleI4FontE
class Handle<class Font>
6HandleI5ChoreE
class Handle<class Chore>
6HandleI5RulesE
class Handle<class Rules>
6HandleI5SceneE
class Handle<class Scene>
6HandleI7D3DMeshE
class Handle<class D3DMesh>
6HandleI8SaveGameE
class Handle<class SaveGame>
6HandleI8SkeletonE
class Handle<class Skeleton>
6HandleI9AnimationE
class Handle<class Animation>
6HandleI9AudioDataE
class Handle<class AudioData>
6HandleI9VoiceDataE
class Handle<class VoiceData>
6HandleI9WalkBoxesE
class Handle<class WalkBoxes>
6String
class String
7D3DMesh
class D3DMesh
7DCArrayI6HandleI11PropertySetEE
class DCArray<class Handle<class PropertySet> >
7DCArrayI6HandleI9AudioDataEE
class DCArray<class Handle<class AudioData> >
7DCArrayI6StringE
class DCArray<class String>
7DCArrayI7Vector3E
class DCArray<class Vector3>
7Vector3
class Vector3
8SaveGame
class SaveGame
8Skeleton
class Skeleton
9Animation
class Animation
9AudioData
class AudioData
9LightType
class LightType
9VoiceData
class VoiceData
9WalkBoxes
class WalkBoxes
4ListI6HandleI11PropertySetEE
class List<class Handle<class PropertySet> >
5Flags
class Flags
10LinkedListIN5Scene9AgentInfoEE
class LinkedList<class Scene::AgentInfo>
3PtrI18CachedObjInterfaceE
class Ptr<class CachedObjInterface>
3PtrI5AgentE
class Ptr<class Agent>
N5Scene9AgentInfoE
class Scene::AgentInfo
13SceneInstData
class SceneInstData
18ContainerInterface
class ContainerInterface
7DCArrayIN7D3DMesh11TriangleSetEE
class DCArray<class D3DMesh::TriangleSet>
14D3DIndexBuffer
class D3DIndexBuffer
10Selectable
class Selectable
11BoundingBox
class BoundingBox
15D3DVertexBuffer
class D3DVertexBuffer
17RenderObject_Mesh
class RenderObject_Mesh
23RenderObject_Selectable
class RenderObject_Selectable
3PtrI14D3DPixelShaderE
class Ptr<class D3DPixelShader>
3PtrI9D3DShaderE
class Ptr<class D3DShader>
6Camera
class Camera
N7D3DMesh11TriangleSetE
class D3DMesh::TriangleSet
17RenderObject_Text
class RenderObject_Text
7DCArrayIN8Skeleton5EntryEE
class DCArray<class Skeleton::Entry>
8Rollover
class Rollover
9Transform
class Transform
N8Skeleton5EntryE
class Skeleton::Entry
13LightInstance
class LightInstance
16SkeletonInstance
class SkeletonInstance
18CachedObjInterface
class CachedObjInterface
5Agent
class Agent
10DialogBase
class DialogBase
10DialogItem
class DialogItem
10DialogLine
class DialogLine
10DialogText
class DialogText
12DialogBranch
class DialogBranch
12DialogDialog
class DialogDialog
14DialogExchange
class DialogExchange
21LanguageResourceProxy
class LanguageResourceProxy
7DCArrayIN11InputMapper12EventMappingEE
class DCArray<class InputMapper::EventMapping>
7DCArrayI10ChoreAgentE
class DCArray<class ChoreAgent>
7DCArrayI13ChoreResourceE
class DCArray<class ChoreResource>
N11InputMapper12EventMappingE
class InputMapper::EventMapping
10ChoreAgent
class ChoreAgent
13ChoreResource
class ChoreResource
3PtrI5ChoreE
class Ptr<class Chore>
7DCArrayIN13ChoreResource5BlockEE
class DCArray<class ChoreResource::Block>
N10ChoreAgent10AttachmentE
class ChoreAgent::Attachment
22AnimatedValueInterfaceIfE
class AnimatedValueInterface<float>
23KeyframedValueInterface
class KeyframedValueInterface
27AnimationValueInterfaceBase
class AnimationValueInterfaceBase
7DCArrayI3PtrI27AnimationValueInterfaceBaseEE
class DCArray<class Ptr<class AnimationValueInterfaceBase> >
7DCArrayIN14KeyframedValueIfE6SampleEE
class DCArray<class KeyframedValue<float>::Sample>
P22AnimatedValueInterfaceIfE
class AnimatedValueInterface<float> *
P27AnimationValueInterfaceBase
class AnimationValueInterfaceBase *
N14KeyframedValueIfE6SampleE
class KeyframedValue<float>::Sample
N13ChoreResource5BlockE
class ChoreResource::Block
22AnimatedValueInterfaceI10QuaternionE
class AnimatedValueInterface<class Quaternion>
7DCArrayIN14KeyframedValueI10QuaternionE6SampleEE
class DCArray<class KeyframedValue<class Quaternion>::Sample>
P22AnimatedValueInterfaceI10QuaternionE
class AnimatedValueInterface<class Quaternion> *
N14KeyframedValueI10QuaternionE6SampleE
class KeyframedValue<class Quaternion>::Sample
22AnimatedValueInterfaceI7Vector3E
class AnimatedValueInterface<class Vector3>
7DCArrayIN14KeyframedValueI7Vector3E6SampleEE
class DCArray<class KeyframedValue<class Vector3>::Sample>
P22AnimatedValueInterfaceI7Vector3E
class AnimatedValueInterface<class Vector3> *
10AudioSound
class AudioSound
18PlaybackController
class PlaybackController
22AnimatedValueInterfaceI6d_boolE
class AnimatedValueInterface<bool>
7DCArrayIN14KeyframedValueI6d_boolE6SampleEE
class DCArray<class KeyframedValue<bool>::Sample>
N14KeyframedValueI7Vector3E6SampleE
class KeyframedValue<class Vector3>::Sample
P22AnimatedValueInterfaceI6d_boolE
class AnimatedValueInterface<bool> *
N14KeyframedValueI6d_boolE6SampleE
class KeyframedValue<bool>::Sample
16AnimationManager
class AnimationManager
5Mover
class Mover
9ChoreInst
class ChoreInst
7DCArrayI10D3DTextureE
class DCArray<class D3DTexture>
7DCArrayIN4Font9GlyphInfoEE
class DCArray<struct Font::GlyphInfo>
5TRectIfE
class TRect<float>
N4Font9GlyphInfoE
struct Font::GlyphInfo
10VoiceSound
class VoiceSound
6PathTo
class PathTo
7DCArrayIN9WalkBoxes3TriEE
class DCArray<class WalkBoxes::Tri>
7DCArrayIN9WalkBoxes4QuadEE
class DCArray<class WalkBoxes::Quad>
7DCArrayIN9WalkBoxes4VertEE
class DCArray<class WalkBoxes::Vert>
6SArrayIN9WalkBoxes4EdgeELi3EE
class SArray<class WalkBoxes::Edge,3>
6SArrayIfLi3EE
class SArray<float,3>
6SArrayIiLi3EE
class SArray<int,3>
N9WalkBoxes3TriE
class WalkBoxes::Tri
N9WalkBoxes4EdgeE
class WalkBoxes::Edge
N9WalkBoxes4VertE
class WalkBoxes::Vert
12WalkAnimator
class WalkAnimator
14ChoreAgentInst
class ChoreAgentInst
22AnimatedValueInterfaceI5ColorE
class AnimatedValueInterface<class Color>
6NavCam
class NavCam
7DCArrayIN14KeyframedValueI5ColorE6SampleEE
class DCArray<class KeyframedValue<class Color>::Sample>
9FootSteps
class FootSteps
P22AnimatedValueInterfaceI5ColorE
class AnimatedValueInterface<class Color> *
N14KeyframedValueI5ColorE6SampleE
class KeyframedValue<class Color>::Sample
22AnimatedValueInterfaceI6StringE
class AnimatedValueInterface<class String>
7DCArrayI13StyleGuideRefE
class DCArray<class StyleGuideRef>
7DCArrayIN14KeyframedValueI6StringE6SampleEE
class DCArray<class KeyframedValue<class String>::Sample>
9PathMover
class PathMover
P22AnimatedValueInterfaceI6StringE
class AnimatedValueInterface<class String> *
N14KeyframedValueI6StringE6SampleE
class KeyframedValue<class String>::Sample
7DCArrayIN8SaveGame9AgentInfoEE
class DCArray<class SaveGame::AgentInfo>
8Subtitle
class Subtitle
N14DialogInstance10InstanceIDE
class DialogInstance::InstanceID
N8SaveGame9AgentInfoE
class SaveGame::AgentInfo
7Trigger
class Trigger
N4Rule9AgentInfoE
struct Rule::AgentInfo
6DArrayI6d_boolE
class DArray<bool>


--]=====]