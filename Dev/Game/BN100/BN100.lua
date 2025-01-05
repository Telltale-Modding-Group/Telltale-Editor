require("ToolLibrary/Game/BN100/LuaContainer.lua")

-- C++ code temp below for calculating version crc in later games.
--[=====[           // endian?
            U32 buf = (clz.Flags & CLASS_NON_BLOCKED) == 0 ? 0xFFFF'FFFFu : 0u;
            U32 crc = CRC32((U8*)&buf, 4, 0);
            for(auto& member: clz.Members)
            {
                if((member.Flags & MEMBER_SKIP) != 0)
                    continue; // skip
                crc = CRC32((U8*)member.Name.c_str(), (U32)member.Name.size(), crc);
                crc = CRC32((U8*)&Classes[member.ClassID].TypeHash, 8, crc);
                U8 blocked = (Classes[member.ClassID].Flags & CLASS_NON_BLOCKED) == 0 ? 1 : 0;
                crc = CRC32(&blocked, 1, crc);
            }
            return crc;
--]=====]

-- Calculate the version CRC of a given class in Bone: Out from Boneville. In the macOS executable for Application1, see function at 0xDE28E
function VersionCRCBone100(classTable)
	temp = 0
	if not MetaFlagQuery(classTable.Flags, kMetaClassNonBlocked) then
		temp = tonumber("0xFFFF")
	end
	hash = MetaHashInt(0, temp)

	if type(classTable.Members) == "table" then -- if we have members iterate through them
		for _,member in pairs(classTable.Members) do
			if not MetaFlagQuery(member.Flags, kMetaMemberSerialiseDisable) and not MetaFlagQuery(member.Flags, kMetaMemberVersionDisable) then
				hash = MetaHashString(hash, member.Name)
				hash = MetaHashString(hash, member.Class.Name)
			end
		end
	end

	return hash
end

function SerialisePropertySet(metaStream, propInstance, isWrite)
	-- start by calling default meta serialise (serialise members, so flags and parent list)
	if not MetaSerialiseDefault(metaStream, propInstance, isWrite) then
		return false;
	end
	MetaStreamBeginBlock(metaStream, isWrite) -- rest is inside a block in the binary

	if not isWrite then
		numTypes = MetaStreamReadInt(metaStream)
		for i=1,numTypes do
			propType, propTypeVersionIndex = MetaStreamFindClass(metaStream, MetaStreamReadSymbol(metaStream)) -- read class symbol
			numOfThatType = MetaStreamReadInt(metaStream)
			print(string.format("i have found %d classes of the class: %s (%d)", numOfThatType, propType, propTypeVersionIndex))
			for j=1, numOfThatType do
				key = SymbolTableFind(MetaStreamReadSymbol(metaStream))
				inst_of_type = MetaCreateInstance(propType, propTypeVersionIndex)
				if not MetaSerialise(metaStream, inst_of_type, isWrite) then
					return false
				end
				print(string.format("we have %s => %s", key, MetaToString(inst_of_type)))
			end
		end
	end -- skip writes for now

	MetaStreamEndBlock(metaStream, isWrite)
	return true -- ok for now
end

function RegisterBone100(vendor, platform)

	MetaSetVersionFn("VersionCRCBone100") -- Set version CRC calculation function, before anything else.
	MetaRegisterIntrinsics() -- First, we must register all of the intrinsic types.

	-- TEMPORARY TESTING CODE. WILL BE REPLACED.
	-- I will also note here that type names that have 'class ' 'struct ''enum ' and 'std::' etc in them are removed from the class name
	-- this was realised by telltale when they started using hashed versions, because compilers name things differently
	-- so in this version they actually still use those, right? (... need to check this)

	local MetaVec3 = { VersionIndex = 0 }
	MetaVec3.Name = "class Vector3"
	MetaVec3.Flags = kMetaClassNonBlocked
	MetaVec3.Members = {}
	MetaVec3.Members[1] = { Name = "x", Class = kMetaFloat }
	MetaVec3.Members[2] = { Name = "y", Class = kMetaFloat }
	MetaVec3.Members[3] = { Name = "z", Class = kMetaFloat }
	MetaRegisterClass(MetaVec3)

	local MetaCol = { VersionIndex = 0 }
	MetaCol.Name = "class Color"
	MetaCol.Flags = kMetaClassNonBlocked
	MetaCol.Members = {}
	MetaCol.Members[1] = { Name = "r", Class = kMetaFloat }
	MetaCol.Members[2] = { Name = "g", Class = kMetaFloat }
	MetaCol.Members[3] = { Name = "b", Class = kMetaFloat }
	MetaCol.Members[4] = { Name = "a", Class = kMetaFloat }
	MetaRegisterClass(MetaCol)

	local MetaFlags = { VersionIndex = 0 } -- CHECKED. version CRCs match
	MetaFlags.Name = "class Flags"
	MetaFlags.Flags = kMetaClassNonBlocked
	MetaFlags.Members = {}
	MetaFlags.Members[1] = { Name = "mFlags", Class = kMetaInt }
	MetaRegisterClass(MetaFlags)

	local MetaCITest = {VersionIndex = 0} -- baseclass for all containers in most games (when writing these scripts, we wil havea funtion which sets this for each)
	MetaCITest.Name = "class ContainerInterface *" -- see below
	MetaCITest.Flags = kMetaClassAbstract
	MetaRegisterClass(MetaCITest)

	-- CRCS MATCH FOR BELOW.
	local MetaComplexTest = { VersionIndex = 0 } -- DCArray_String_(1j6j2xe).vers. each array has two versions, one without the baseclass container member. (todo make other)
	MetaComplexTest.Name = "class DCArray<class String>"
	MetaComplexTest.Members = {}
	MetaComplexTest.Members[1] = { Name = "Baseclass_ContainerInterface", Class = MetaCITest, Flags = kMetaMemberMemoryDisable + kMetaMemberBaseClass }
	MetaComplexTest.Members[2] = { Name = "mSize", Class = kMetaInt, Flags = kMetaMemberMemoryDisable }
	MetaComplexTest.Members[3] = { Name = "mCapacity", Class = kMetaInt, Flags = kMetaMemberMemoryDisable }
	MetaRegisterCollection(MetaComplexTest, nil, kMetaString)

	local complex2 = { VersionIndex = 1 } -- two array types exist, one with no members (here) and the one above....
	complex2.Name = "class DCArray<class String>"
	complex2.Members = {}
	complex2.Members[1] = { Name = "mSize", Class = kMetaInt, Flags = kMetaMemberMemoryDisable }
	complex2.Members[2] = { Name = "mCapacity", Class = kMetaInt, Flags = kMetaMemberMemoryDisable }
	MetaRegisterCollection(complex2, nil, kMetaString)

	-- TEST BELOW FOR .PROP. Note that theres lots of stuff here which can be put into functions (eg for array initialisation etc).
	-- this will be done when actually doing it properly, this is just a test!

	local MetaHandleProp = { VersionIndex = 0 }
	MetaHandleProp.Name = "class Handle<class PropertySet>"
	MetaHandleProp.Flags = kMetaClassNonBlocked + kMetaClassIntrinsic -- not in version headers ? idk why
	MetaHandleProp.Members = {}
	-- below member doesnt exist in game (has custom serialiser) but lets just store it as a di
	MetaHandleProp.Members[1] = { Name = "mHandle", Class = kMetaSymbol, Flags = kMetaMemberVersionDisable } -- serialise yes, no version hash though.
	MetaRegisterClass(MetaHandleProp)

	local h0 = { VersionIndex = 0 }
	h0.Name = "class Handle<class DialogResource>"
	h0.Flags = kMetaClassNonBlocked + kMetaClassIntrinsic -- not in version headers ? idk why
	h0.Members = {}
	h0.Members[1] = { Name = "mHandle", Class = kMetaSymbol, Flags = kMetaMemberVersionDisable } -- serialise yes, no version hash though.
	MetaRegisterClass(h0)

	local h1 = { VersionIndex = 0 }
	h1.Name = "class Handle<class Font>"
	h1.Flags = kMetaClassNonBlocked + kMetaClassIntrinsic -- not in version headers ? idk why
	h1.Members = {}
	h1.Members[1] = { Name = "mHandle", Class = kMetaSymbol, Flags = kMetaMemberVersionDisable } -- serialise yes, no version hash though.
	MetaRegisterClass(h1)
	
	-- array of prop file names (for parent list in prop)
	local MetaHandleArrayProp = { VersionIndex = 0}
	MetaHandleArrayProp.Name = "class List<class Handle<class PropertySet> >"
	MetaHandleArrayProp.Members = {} -- no members in this one for some reason
	MetaRegisterCollection(MetaHandleArrayProp, nil, MetaHandleProp)

	local prop = {VersionIndex = 0 }
	prop.Name = "class PropertySet"
	prop.Members = {}
	prop.Members[1] = { Name = "mPropertyFlags", Class = MetaFlags }
	prop.Members[2] = { Name = "mParentList", Class = MetaHandleArrayProp }
	prop.Serialiser = "SerialisePropertySet"
	MetaRegisterClass(prop)

	print(string.format("version crc is %x", MetaGetVersionCRC(prop)))

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