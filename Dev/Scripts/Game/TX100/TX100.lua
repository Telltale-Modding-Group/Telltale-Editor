-- Telltale Texas Hold'em Implementation

require("ToolLibrary/Game/Common/LuaPropertySet.lua")
require("ToolLibrary/Game/VersionCRC.lua")

require("ToolLibrary/Game/TX100/D3DTexture.lua")
require("ToolLibrary/Game/TX100/D3DMesh.lua")
require("ToolLibrary/Game/Common/Scene.lua")
require("ToolLibrary/Game/Common/Audio.lua")
require("ToolLibrary/Game/Common/Animation.lua")
require("ToolLibrary/Game/Common/Chore.lua")
require("ToolLibrary/Game/Common/InputMapper.lua")

function TX100_GetGameDescriptor()
	local texasHoldem = {}
	texasHoldem.Name = "Telltale Texax Hold'em"
	texasHoldem.ID = "TX100" -- Texas S1 (100)
	texasHoldem.DefaultMetaVersion = "MBIN"
	texasHoldem.LuaVersion = "5.0.2"
	texasHoldem.IsArchive2 = false
	texasHoldem.ArchiveVersion = 0
	texasHoldem.Vendors = ""
	texasHoldem.Platforms = "PC"
	MetaPushGameCapability(texasHoldem, kGameCapSeparateAnimationTransform)
	MetaPushGameCapability(texasHoldem, kGameCapUsesLenc)
	MetaPushGameCapability(texasHoldem, kGameCapRawClassNames)
	MetaRegisterGame(texasHoldem) -- does not have any encryption, so no encryption keys
	return texasHoldem
end

function RegisterTXCollection(name, k, v, fl)
	local without = { VersionIndex = 0 }
	without.Name = name
	without.Members = {}
	without.Flags = fl
	if k == nil and type(k) ~= "number" then
		without.Members[1] = { Name = "mSize", Class = kMetaInt, Flags = kMetaMemberMemoryDisable }
		without.Members[2] = { Name = "mCapacity", Class = kMetaInt, Flags = kMetaMemberMemoryDisable }
	end
	MetaRegisterCollection(without, k, v)
	return without -- return both
end

-- registers Handle<T>. ensure the name is correct.
function RegisterTXHandle(name)
	local MetaHandle = { VersionIndex = 0 }
	MetaHandle.Name = name
	MetaHandle.Flags = kMetaClassIntrinsic
	MetaHandle.Members = {}
	MetaHandle.Members[1] = { Name = "mHandle", Class = kMetaClassSymbol, Flags = kMetaMemberVersionDisable }
	MetaRegisterClass(MetaHandle)
	return MetaHandle
end

function RegisterTXANMValue(base, typeName)
	local anmV = NewClass(typeName, 0)
	anmV.Members[1] = NewMember("Baseclass_AnimationValueInterfaceBase", base)
	MetaRegisterClass(anmV)
	return anmV
end

function RegisterTXKeyframedValue(typedInterface, typeName, typeTable)
	local sample = NewClass("class KeyframedValue<" .. typeName .. ">::Sample", 0)
	sample.Members[1] = NewMember("mTime", kMetaFloat)
	sample.Members[2] = NewMember("mbInterpolateToNextKey", kMetaBool)
	sample.Members[3] = NewMember("mValue", typeTable)
	MetaRegisterClass(sample)

	local arraySample = RegisterTXCollection("class DCArray<class KeyframedValue<" .. typeName .. ">::Sample>",
		nil, sample)

	local keyframed = NewClass("class KeyframedValue<" .. typeName .. ">", 0)
	keyframed.Members[1] = NewMember("Baseclass_AnimatedValueInterface<T>",
		NewProxyClass(typedInterface.Name .. " *", "mVal", typedInterface))
	keyframed.Members[2] = NewMember("mMinVal", typeTable)
	keyframed.Members[3] = NewMember("mMaxVal", typeTable)
	keyframed.Members[4] = NewMember("mSamples", arraySample)
	MetaRegisterClass(keyframed)

	return keyframed
end

function RegisterTX100(vendor)
	MetaSetVersionFn("VersionCRC_V0_PointerFix")
	MetaRegisterIntrinsics()

	local MetaVec3, MetaCol, MetaPolar, MetaRect, MetaTRectF, MetaQuat, MetaFlags, transform = RegisterIntrinsics2()

	MetaAssociateFolderExtension("TX100", "*.vers", "Meta/")
	MetaAssociateFolderExtension("TX100", "*.lua", "Scripts/")
	MetaAssociateFolderExtension("TX100", "*.d3dmesh", "Meshes/")
	MetaAssociateFolderExtension("TX100", "*.d3dtx", "Textures/")

	-- ALL HANDLE TYPES
	local hAnim = RegisterTXHandle("class Handle<class Animation>")
	local hChore = RegisterTXHandle("class Handle<class Chore>")
	local hMesh = RegisterTXHandle("class Handle<class D3DMesh>")
	local hTexture = RegisterTXHandle("class Handle<class D3DTexture>")
	local hDlg = RegisterTXHandle("class Handle<class DialogResource>")
	local hProp = RegisterTXHandle("class Handle<class PropertySet>")
	local hAud = RegisterTXHandle("class Handle<class AudioData")
	local hScene = RegisterTXHandle("class Handle<class Scene>")
	local hSkeleton = RegisterTXHandle("class Handle<class Skeleton>")
	local hVoiceData = RegisterTXHandle("class Handle<class VoiceData>")
	local hWalkBoxes = RegisterTXHandle("class Handle<class WalkBoxes>")
	local hFont = RegisterTXHandle("class Handle<class Font>")

	local MetaHandleArrayProp = { VersionIndex = 0 }
	MetaHandleArrayProp.Name = "class List<class Handle<class PropertySet> >"
	MetaHandleArrayProp.Members = {}
	MetaRegisterCollection(MetaHandleArrayProp, nil, hProp)

	-- .PROP FILES
	local prop = { VersionIndex = 0 }
	prop.Extension = "prop"
	prop.Flags = kMetaClassAttachable
	prop.Name = "class PropertySet"
	prop.Members = {}
	prop.Members[1] = { Name = "mPropertyFlags", Class = MetaFlags }
	prop.Members[2] = { Name = "mParentList", Class = MetaHandleArrayProp }
	prop.Serialiser = "SerialisePropertySet0"
	MetaRegisterClass(prop)
	MetaAssociateFolderExtension("TX100", "module_*.prop", "Properties/Modules/")
	MetaAssociateFolderExtension("TX100", "prefs_*.prop", "Properties/Preferences/")
	MetaAssociateFolderExtension("TX100", "*.prop", "Properties/")

	local enumNavMode = NewClass("enum NavCam::Mode", 0)
	enumNavMode.Flags = kMetaClassNonBlocked + kMetaClassEnumWrapper
	enumNavMode.Members[1] = NewMember("mVal", kMetaInt, kMetaMemberVersionDisable + kMetaMemberEnum)
	AddEnum(enumNavMode, 1, "eNone", 1)
	AddEnum(enumNavMode, 1, "eLookAt", 2)
	AddEnum(enumNavMode, 1, "eOrbit", 3)
	AddEnum(enumNavMode, 1, "eAnimation_Track", 4)
	AddEnum(enumNavMode, 1, "eAnimation_Time", 5)
	MetaRegisterClass(enumNavMode)

	local navCamMode = NewClass("struct NavCam::EnumMode", 0)
	navCamMode.Flags = kMetaClassEnumWrapper
	navCamMode.Members[1] = NewMember("mVal", enumNavMode, kMetaMemberEnum)
	MetaRegisterClass(navCamMode)

	local imapMapping = NewClass("class InputMapper::EventMapping", 0)
	imapMapping.Members[1] = NewMember("mInputCode", NewProxyClass("enum InputCode", "mVal", kMetaInt), kMetaMemberEnum)
	imapMapping.Members[2] = NewMember("mEvent", NewProxyClass("enum InputMapper::EventType", "mVal", kMetaInt),
		kMetaMemberEnum)
	imapMapping.Members[3] = NewMember("mScriptFunction", kMetaClassString, 0)
	MetaRegisterClass(imapMapping)

	local imapMappingArray = RegisterTXCollection("class DCArray<class InputMapper::EventMapping>", nil,
		imapMapping)

	-- .IMAP FILES
	local imap = NewClass("class InputMapper", 0)
	imap.Extension = "imap"
	imap.Normaliser = "NormaliseInputMapper0"
	imap.Members[1] = NewMember("mName", kMetaClassString)
	imap.Members[2] = NewMember("mMappedEvents", imapMappingArray)
	MetaRegisterClass(imap)
	MetaAssociateFolderExtension("TX100", "*.imap", "InputMappers/")

	local lightType = NewClass("class LightType", 0)
	lightType.Members[1] = NewMember("mLightType", kMetaInt, 0)
	MetaRegisterClass(lightType)

	local textAlign = NewClass("class TextAlignmentType", 0)
	textAlign.Members[1] = NewMember("mAlignmentType", kMetaInt, 0)
	MetaRegisterClass(textAlign)


	local audStreamed = NewClass("struct AudioData::Streamed", 0)
	audStreamed.Members[1] = NewMember("mStreamRegionSize", kMetaInt)
	audStreamed.Members[2] = NewMember("mStreamBufferSecs", kMetaFloat)
	MetaRegisterClass(audStreamed)

	-- .AUD FILES
	local aud = NewClass("class AudioData", 0)
	aud.Extension = "aud"
	aud.Serialiser = "SerialiseAud0"
	aud.Members[1] = NewMember("mFilename", kMetaClassString)
	aud.Members[2] = NewMember("mLength", kMetaFloat)
	aud.Members[3] = NewMember("mbStreamed", kMetaBool)
	aud.Members[4] = NewMember("mDataFormat", kMetaInt)
	aud.Members[5] = NewMember("mBytesPerSecond", kMetaInt)
	aud.Members[6] = NewMember("mDSBufferBytes", kMetaInt) -- DataStream buffer size
	aud.Members[7] = NewMember("mStreamInfo", audStreamed)
	aud.Members[8] = NewMember("_mVorbisAudio", kMetaClassInternalBinaryBuffer,
		kMetaMemberSerialiseDisable + kMetaMemberVersionDisable)
	aud.Members[9] = NewMember("_mNumChannels", kMetaInt, kMetaMemberSerialiseDisable + kMetaMemberVersionDisable)
	aud.Members[10] = NewMember("_mSampleSizeBytes", kMetaInt, kMetaMemberSerialiseDisable + kMetaMemberVersionDisable)
	MetaRegisterClass(aud)
	MetaAssociateFolderExtension("TX100", "*.aud", "Audio/")

	local anmBase = NewClass("class AnimationValueInterfaceBase", 0)
	anmBase.Members[1] = NewMember("mName", kMetaClassString)
	anmBase.Members[2] = NewMember("mFlags", kMetaInt)
	MetaRegisterClass(anmBase)

	local animatedFloat = RegisterTXANMValue(anmBase, "class AnimatedValueInterface<float>")
	local animatedQuat = RegisterTXANMValue(anmBase, "class AnimatedValueInterface<class Quaternion>")
	local animatedStr = RegisterTXANMValue(anmBase, "class AnimatedValueInterface<class String>")
	local animatedVec3 = RegisterTXANMValue(anmBase, "class AnimatedValueInterface<class Vector3>")

	RegisterTXKeyframedValue(animatedFloat, "float", kMetaFloat)
	RegisterTXKeyframedValue(animatedQuat, "class Quaternion", MetaQuat)
	RegisterTXKeyframedValue(animatedStr, "class String", kMetaClassString)
	RegisterTXKeyframedValue(animatedVec3, "class Vector3", MetaVec3)

	local anm = NewClass("class Animation", 0)
	anm.Extension = "anm"
	anm.Flags = kMetaClassAttachable
	anm.Serialiser = "SerialiseAnimation0"
	anm.Normaliser = "NormaliseAnimation0"
	anm.Members[1] = NewMember("mName", kMetaClassString)
	anm.Members[2] = NewMember("mLength", kMetaFloat)
	MetaRegisterClass(anm)
	MetaAssociateFolderExtension("TX100", "*.anm", "Animations/")

	local proc, procval = RegisterProceduralLookAts0(MetaVec3, animatedQuat, anm)
	local vecKeys, quatKeys = RegisterCompressedVecAndQuats0()

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
	MetaRegisterClass(langr)
	MetaAssociateFolderExtension("BN100", "*.langres", "Language/")

	local mapIntLangRes = RegisterTXCollection("class Map<int,class LanguageResource,struct std::less<int> >", kMetaInt,
		langr)

	-- .LANGDB FILES
	local langdb = NewClass("class LanguageDatabase", 0)
	langdb.Extension = "langdb"
	langdb.Members[1] = NewMember("mLanguageResources", mapIntLangRes)
	MetaRegisterClass(langdb)
	MetaAssociateFolderExtension("TX100", "*.langdb", "Language/")

	local sceneAgent0 = NewClass("class Scene::AgentInfo", 0)
	sceneAgent0.Members[1] = NewMember("mAgentName", kMetaClassString)
	sceneAgent0.Members[2] = NewMember("mStartPos", MetaVec3)
	sceneAgent0.Members[3] = NewMember("mStartQuat", MetaQuat)
	sceneAgent0.Members[4] = NewMember("mbVisible", kMetaBool)
	sceneAgent0.Members[5] = NewMember("mbTransient", kMetaBool)
	sceneAgent0.Members[6] = NewMember("mbAttached", kMetaBool)
	sceneAgent0.Members[7] = NewMember("mAttachAgent", kMetaClassString)
	sceneAgent0.Members[8] = NewMember("mAttachAgentNode", kMetaClassString)
	sceneAgent0.Members[9] = NewMember("mAgentSceneProps", prop)
	MetaRegisterClass(sceneAgent0)

	local arrayAgents = RegisterTXCollection("class DCArray<class Scene::AgentInfo>", nil, sceneAgent0)

	local scene = NewClass("class Scene", 0)
	scene.Extension = "scene"
	scene.Serialiser = "SerialiseScene0"
	scene.Members[1] = NewMember("mbHidden", kMetaBool)
	scene.Members[2] = NewMember("mName", kMetaClassString)
	scene.Members[3] = NewMember("_mAgents", arrayAgents, kMetaMemberVersionDisable + kMetaMemberSerialiseDisable)
	MetaRegisterClass(scene)
	MetaAssociateFolderExtension("TX100", "*.scene", "Scenes/")

	local mapStringFloat, _ = RegisterTXCollection("class Map<class String,float,struct std::less<class String> >",
		kMetaClassString, kMetaFloat)

	local skl = RegisterSkeleton0(RegisterTXCollection, MetaVec3, MetaQuat, transform, mapStringFloat)
	MetaAssociateFolderExtension("TX100", "*.skl", "Meshes/Skeletons/")

	local arrayDInt = RegisterTXCollection("class DArray<int>", nil, kMetaInt)
	local arrayDCInt = RegisterTXCollection("class DCArray<int>", nil, kMetaInt)

	local vox0 = NewClass("class VoiceData", 0)
	vox0.Extension = "vox"
	vox0.Serialiser = "SerialiseVox0"
	vox0.Members[1] = NewMember("mLength", kMetaFloat)
	vox0.Members[2] = NewMember("mAllPacketsSize", kMetaInt)
	vox0.Members[3] = NewMember("mPacketSamples", kMetaInt)
	vox0.Members[4] = NewMember("mSampleRate", kMetaInt)
	vox0.Members[5] = NewMember("mMode", kMetaInt)
	vox0.Members[6] = NewMember("mPacketPositions", arrayDInt)
	vox0.Members[7] = NewMember("_mPacketData", kMetaClassInternalBinaryBuffer,
		kMetaMemberSerialiseDisable + kMetaMemberVersionDisable)
	MetaRegisterClass(vox0)
	MetaAssociateFolderExtension("TX100", "*.vox", "Audio/Voice/")

	local choreAgentAttach = NewClass("class ChoreAgent::Attachment", 0)
	choreAgentAttach.Members[1] = NewMember("mbDoAttach", kMetaBool)
	choreAgentAttach.Members[2] = NewMember("mAttachTo", kMetaClassString)
	choreAgentAttach.Members[3] = NewMember("mAttachToNode", kMetaClassString)
	choreAgentAttach.Members[4] = NewMember("mAttachPos", MetaVec3)
	choreAgentAttach.Members[5] = NewMember("mAttachQuat", MetaQuat)
	choreAgentAttach.Members[6] = NewMember("mbAttachPreserveWorldPos", kMetaBool)
	choreAgentAttach.Members[7] = NewMember("mbLeaveAttachedWhenComplete", kMetaBool)
	MetaRegisterClass(choreAgentAttach)

	local choreAgent = NewClass("class ChoreAgent", 0)
	choreAgent.Members[1] = NewMember("mAgentName", kMetaClassString)
	choreAgent.Members[2] = NewMember("mResources", arrayDCInt)
	choreAgent.Members[3] = NewMember("mChoreProps", prop)
	choreAgent.Members[4] = NewMember("mChorePosition", MetaVec3)
	choreAgent.Members[5] = NewMember("mChoreQuat", MetaQuat)
	MetaRegisterClass(choreAgent)

	local choreBlock = RegisterChoreResourceBlock()

	local arrayChoreBlock = RegisterTXCollection("class DCArray<class ChoreResource::Block>", nil, choreBlock)

	local choreResource = NewClass("class ChoreResource", 0)
	choreResource.Serialiser = "SerialiseChoreResource0"
	choreResource.Flags = kMetaClassAttachable
	choreResource.Members[1] = NewMember("mResName", kMetaClassString)
	choreResource.Members[2] = NewMember("mResLength", kMetaFloat)
	choreResource.Members[3] = NewMember("mPriority", kMetaInt)
	choreResource.Members[4] = NewMember("mResourceGroup", kMetaClassString)
	choreResource.Members[5] = NewMember("mControlAnimation", anm)
	choreResource.Members[6] = NewMember("mBlocks", arrayChoreBlock)
	choreResource.Members[7] = NewMember("mbNoPose", kMetaBool)
	choreResource.Members[8] = NewMember("mbEmbedded", kMetaBool)
	choreResource.Members[9] = NewMember("mbEnabled", kMetaBool)
	choreResource.Members[10] = NewMember("mbIsAgentResource", kMetaBool)
	choreResource.Members[11] = NewMember("mbViewGraphs", kMetaBool)
	choreResource.Members[12] = NewMember("mbViewProperties", kMetaBool)
	choreResource.Members[13] = NewMember("mbViewResourceGroups", kMetaBool)
	choreResource.Members[14] = NewMember("mResourceProperties", prop)
	choreResource.Members[15] = NewMember("mResourceGroupInclude", mapStringFloat)
	MetaRegisterClass(choreResource)

	local arrayChoreAgent = RegisterTXCollection("class DCArray<class ChoreAgent>", nil, choreAgent)
	local arrayChoreResource = RegisterTXCollection("class DCArray<class ChoreResource>", nil, choreResource)

	local chore = NewClass("class Chore", 0)
	chore.Extension = "chore"
	chore.Serialiser = "SerialiseChore0"
	chore.Members[1] = NewMember("mName", kMetaClassString)
	chore.Members[2] = NewMember("mhChoreScene", hScene)
	chore.Members[3] = NewMember("mLength", kMetaFloat)
	chore.Members[4] = NewMember("mNumResources", kMetaInt)
	chore.Members[5] = NewMember("mNumAgents", kMetaInt)
	chore.Members[6] = NewMember("mResources", arrayChoreResource) -- WHY? this isnt even used.
	chore.Members[7] = NewMember("mAgents", arrayChoreAgent)    -- AGAIN THEY DONT USE THIS!
	chore.Members[8] = NewMember("mEditorProps", prop)
	chore.Members[9] = NewMember("_mResources", arrayChoreResource,
	kMetaMemberSerialiseDisable + kMetaMemberVersionDisable)
	chore.Members[10] = NewMember("_mAgents", arrayChoreAgent, kMetaMemberSerialiseDisable + kMetaMemberVersionDisable)
	MetaRegisterClass(chore)
	MetaAssociateFolderExtension("TX100", "*.chore", "Chores/")

	local mesh = RegisterTXD3DMesh(vendor, hTexture)
	local tex = RegisterTXD3DTexture(vendor)

	local glyph = NewClass("struct Font::GlyphInfo", 0)
	glyph.Members[1] = NewMember("mTexturePage", kMetaInt)
	glyph.Members[2] = NewMember("mGlyph", MetaTRectF)
	MetaRegisterClass(glyph)

	local arrayGlyph = RegisterTXCollection("class DCArray<struct Font::GlyphInfo>", nil, glyph)
	local arrayTex = RegisterTXCollection("class DCArray<class D3DTexture>", nil, tex)

	local font = NewClass("class Font", 0)
	font.Extension = "font"
	font.Members[1] = NewMember("mName", kMetaClassString)
	font.Members[2] = NewMember("mHeight", kMetaFloat)
	font.Members[3] = NewMember("mGlyphInfo", arrayGlyph)
	font.Members[4] = NewMember("mTexturePages", arrayTex)
	MetaRegisterClass(font)
	MetaAssociateFolderExtension("TX100", "*.font", "Fonts/")

	return true
end
