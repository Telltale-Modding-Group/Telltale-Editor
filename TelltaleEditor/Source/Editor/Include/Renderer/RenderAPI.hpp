#pragma once

// Render types

#include <Core/Config.hpp>

#include <Renderer/LinearHeap.hpp>
#include <Renderer/Camera.hpp>
#include <Renderer/RenderParameters.hpp>

#include <Scheduler/JobScheduler.hpp>

#include <SDL3/SDL.h>
#include <SDL3/SDL_gpu.h>

#include <stack>
#include <vector>
#include <unordered_map>
#include <mutex>

class RenderContext;

enum class RenderTextureAddressMode
{
	CLAMP = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
	WRAP = SDL_GPU_SAMPLERADDRESSMODE_REPEAT,
	//BORDER. Border colors arent supported by SDL3 :( (if outside UV range)
};

enum class RenderTextureMipMapMode
{
	NEAREST = SDL_GPU_SAMPLERMIPMAPMODE_NEAREST,
	LINEAR = SDL_GPU_SAMPLERMIPMAPMODE_LINEAR,
};

enum class RenderShaderType
{
	FRAGMENT,
	VERTEX,
};

enum class RenderSurfaceFormat
{
	UNKNOWN,
	RGBA8,
};

enum class RenderBufferAttributeFormat : U32
{
	UNKNOWN,
	
	I32x1,
	I32x2,
	I32x3,
	I32x4,
	
	F32x1,
	F32x2,
	F32x3,
	F32x4,
	
	U32x1,
	U32x2,
	U32x3,
	U32x4,
	
};

enum class RenderAttributeType : U32
{
	POSITION = 0,
	NORMAL = 1,
	BINORMAL = 2,
	TANGENT = 3,
	BLEND_WEIGHT = 4,
	BLEND_INDEX = 5,
	COLOUR = 6,
	UV_DIFFUSE = 7,
	UV_LIGHTMAP = 8,
	UNKNOWN,
	COUNT = UNKNOWN,
};

using VertexAttributesBitset = BitSet<RenderAttributeType, (U32)RenderAttributeType::COUNT, RenderAttributeType::POSITION>;

static struct AttribInfo {
	RenderAttributeType Type;
	CString ConstantName;
}

constexpr AttribInfoMap[]
{
	{RenderAttributeType::POSITION, "kCommonMeshAttributePosition"},
	{RenderAttributeType::NORMAL, "kCommonMeshAttributeNormal"},
	{RenderAttributeType::BINORMAL, "kCommonMeshAttributeBinormal"},
	{RenderAttributeType::TANGENT, "kCommonMeshAttributeTangent"},
	{RenderAttributeType::BLEND_WEIGHT, "kCommonMeshAttributeBlendWeight"},
	{RenderAttributeType::BLEND_INDEX, "kCommonMeshAttributeBlendIndex"},
	{RenderAttributeType::COLOUR, "kCommonMeshAttributeColour"},
	{RenderAttributeType::UV_DIFFUSE, "kCommonMeshAttributeUVDiffuse"},
	{RenderAttributeType::UV_LIGHTMAP, "kCommonMeshAttributeUVLightMap"},
	{RenderAttributeType::UNKNOWN, "kCommonMeshAttributeUnknown"},
};

/// A texture.
struct RenderTexture
{
	
	enum TextureFlag
	{
		TEXTURE_FLAG_DELEGATED = 1, // dont release texture.
	};
	
	// Last frame number which buffer was used to a binding slot in.
	U64 LastUsedFrame = 0;
	// Last frame number which this buffer was uploaded from the CPU (updated), that the data is on GPU for.
	U64 LastUpdatedFrame = 0;
	
	RenderContext* _Context = nullptr;
	SDL_GPUTexture* _Handle = nullptr;
	
	Flags TextureFlags;
	
	U32 Width = 0, Height = 0;
	
	RenderSurfaceFormat Format = RenderSurfaceFormat::RGBA8;
	
	// If not created, creates the texture to a 2D texture all black.
	void Create2D(RenderContext*, U32 width, U32 height, RenderSurfaceFormat format, U32 nMips);
	
	~RenderTexture();
	
};

class RenderContext;

struct RenderScene;
struct RenderTexture;
struct RenderFrame;

enum class RenderBufferUsage : U32
{
	VERTEX = SDL_GPU_BUFFERUSAGE_VERTEX,
	INDICES = SDL_GPU_BUFFERUSAGE_INDEX,
	UNIFORM = SDL_GPU_BUFFERUSAGE_GRAPHICS_STORAGE_READ,
};

enum class RenderPrimitiveType : U32
{
	UNKNOWN,
	TRIANGLE_LIST,
	LINE_LIST,
};

// Returns number of expected vertices for a given primitive type above.
inline U32 GetNumVertsForPrimitiveType(RenderPrimitiveType type, U32 numPrims)
{
	if(type == RenderPrimitiveType::TRIANGLE_LIST)
		return 3 * numPrims; // a triangle is defiend by three points
	else if(type == RenderPrimitiveType::LINE_LIST)
		return 2 * numPrims; // a line is defined by two points
	return 0;
}

/// A buffer in the renderer holding data. You own these and can create them below.
struct RenderBuffer
{
	
	// Last frame number which buffer was used to a binding slot in.
	U64 LastUsedFrame = 0;
	// Last frame number which this buffer was uploaded from the CPU (updated), that the data is on GPU for.
	U64 LastUpdatedFrame = 0;
	
	RenderContext* _Context = nullptr;
	SDL_GPUBuffer* _Handle = nullptr;
	
	RenderBufferUsage Usage = RenderBufferUsage::VERTEX;
	U64 SizeBytes = 0; // size of the bytes of the buffer
	
	~RenderBuffer();
	
};

/// Internal transfer buffer for uploads.
struct _RenderTransferBuffer
{
	
	U64 LastUsedFrame = 0;
	SDL_GPUTransferBuffer* Handle = nullptr;
	U32 Capacity = 0; // total number available
	U32 Size = 0; // total used at the moment if acquired
	
	inline bool operator<(const _RenderTransferBuffer& rhs) const
	{
		return Capacity < rhs.Capacity;
	}
	
};

/// Render sampler which is bindable
struct RenderSampler
{
	
	U64 LastUsedFrame = 0;
	
	RenderContext* _Context = nullptr;
	SDL_GPUSampler* _Handle = nullptr;
	
	Float MipBias = 0.0f;
	RenderTextureAddressMode WrapU = RenderTextureAddressMode::WRAP;
	RenderTextureAddressMode WrapV = RenderTextureAddressMode::WRAP;
	RenderTextureMipMapMode MipMode = RenderTextureMipMapMode::NEAREST;
	
	// Test description, not the handle.
	inline Bool operator==(const RenderSampler& rhs)
	{
		return MipBias == rhs.MipBias && MipMode == rhs.MipMode && WrapU == rhs.WrapU && WrapV == rhs.WrapV;
	}
	
	~RenderSampler();
	
};

/// Used in pipeline state. Represents the format of a vertex data input array and input attributes. No ownership and they are part of a pipeline state descriptor.
struct RenderVertexState
{
	
	RenderContext* _Context = nullptr;
	
	struct VertexAttrib
	{
		RenderAttributeType Attrib = RenderAttributeType::UNKNOWN; //  ie binding slot.
		RenderBufferAttributeFormat Format = RenderBufferAttributeFormat::UNKNOWN;
		U32 VertexBufferIndex = 0; // 0 to 31 (which vertex buffer to use)
	};
	
	// === INFO FORMAT
	
	VertexAttrib Attribs[32];
	U32 BufferPitches[32] = {0,0,0,0}; // byte pitch between consecutive elements in each vertex buffer
	
	U32 NumVertexBuffers = 0; // between 0 and 32
	U32 NumVertexAttribs = 0; // between 0 and 32
	
	Bool IsHalf = true; // true means U16 indices, false meaning U32
	
	void Release(); // Releases this vertex state. Called automatically. Find() in context will return a new one after this.
	
};

/// Bindable pipeline state. Create lots at initialisation and then bind each and render, this is the modern typical best approach to rendering. Internal use. Represents a state of the rasterizer.
struct RenderPipelineState
{
	
	struct
	{
		
		SDL_GPUGraphicsPipeline* _Handle = nullptr; // SDL handle
		RenderContext* _Context; // handle to context
		
	} _Internal;
	
	U64 Hash = 0; // calculated pipeline hash. draw calls etc specify this.
	
	// SET BY INTERNAL USER
	
	String ShaderProgram = "";
	
	RenderPrimitiveType PrimitiveType = RenderPrimitiveType::TRIANGLE_LIST; // primitive type
	
	// FUNCTIONALITY
	
	RenderVertexState VertexState; // bindable buffer information (format) - no actual binds
	
	void Create(); // creates and sets hash.
	
	~RenderPipelineState(); // destroy if needed
	
};

struct DefaultRenderTexture
{
	
	DefaultRenderTextureType Type;
	std::shared_ptr<RenderTexture> Texture;
	
};

struct DefaultRenderMesh
{
	
	DefaultRenderMeshType Type;
	std::shared_ptr<RenderPipelineState> PipelineState;
	std::shared_ptr<RenderBuffer> IndexBuffer;
	std::shared_ptr<RenderBuffer> VertexBuffer; // only one vertex buffer for default ones, they are simple.
	U32 NumIndices = 0;
	
};

/// A render pass
struct RenderPass
{
	
	SDL_GPURenderPass* _Handle = nullptr;
	SDL_GPUCopyPass* _CopyHandle = nullptr;
	
	Colour ClearCol = Colour::Black;
	
};

/// Internal shader, lightweight object.
struct RenderShader
{
	
	String Name; // no extension
	RenderContext* Context = nullptr;
	SDL_GPUShader* Handle = nullptr;
	
	U8 ParameterSlots[PARAMETER_COUNT]; // parameter type => slot index
	VertexAttributesBitset Attribs; // for vertex shaders
	
	inline RenderShader()
	{
		memset(ParameterSlots, 0xFF, PARAMETER_COUNT);
	}
	
	~RenderShader();
	
};

/// A set of draw commands which are appended to and then finally submitted in one batch.
struct RenderCommandBuffer
{
	
	RenderContext* _Context = nullptr;
	SDL_GPUCommandBuffer* _Handle = nullptr;
	SDL_GPUFence* _SubmittedFence = nullptr; // wait object for command buffer in render thread
	RenderPass* _CurrentPass = nullptr; // pass stack for command buffer
	Bool _Submitted = false;
	
	std::vector<_RenderTransferBuffer> _AcquiredTransferBuffers; // cleared given to context once done.
	
	std::shared_ptr<RenderPipelineState> _BoundPipeline; // currently bound pipeline
	
	// bind a pipeline to this command list
	void BindPipeline(std::shared_ptr<RenderPipelineState>& state);
	
	// bind vertex buffers, (array). First is the first slot to bind to. Offsets can be NULL to mean all from start. Offsets is each offset.
	void BindVertexBuffers(std::shared_ptr<RenderBuffer>* buffers, U32* offsets, U32 first, U32 num);
	
	// bind an index buffer for a draw call. isHalf = true means U16 indices in the buffer, false = U32 indices. Pass in offset into buffer.
	void BindIndexBuffer(std::shared_ptr<RenderBuffer> indexBuffer, U32 startIndex, Bool isHalf);
	
	// Binds num textures along with samplers, starting at the given slot, to the pipeline fragment shader. num is 0 to 32.
	void BindTextures(U32 slot, U32 num, RenderShaderType shader,
					  std::shared_ptr<RenderTexture>* pTextures, std::shared_ptr<RenderSampler>* pSamplers);
	
	// Bind a default texture
	void BindDefaultTexture(U32 slot, RenderShaderType, std::shared_ptr<RenderSampler> sampler, DefaultRenderTextureType type);
	
	// Bind a default mesh
	void BindDefaultMesh(DefaultRenderMeshType type);
	
	// After calling bind default mesh and any other textures or generic buffers have been bound, draw a default mesh using this
	void DrawDefaultMesh(DefaultRenderMeshType type);
	
	// Binds num generic storage buffers (eg camera) to a specific shader in the pipeline.
	void BindGenericBuffers(U32 slot, U32 num, std::shared_ptr<RenderBuffer>* pBuffers, RenderShaderType shaderSlot);
	
	// Bind uniform data. Does not need to be within any pass.
	void BindUniformData(U32 slot, RenderShaderType shaderStage, const void* data, U32 size);
	
	// Perform a buffer upload. Prefer to use render frame update list! This is OK for use in defaults.
	void UploadBufferDataSlow(std::shared_ptr<RenderBuffer>& buffer, DataStreamRef srcStream, U64 srcOffset, U32 destOffset, U32 numBytes);
	
	// Perform a texture sub-image upload. Prefer to use render frame update list! This is OK for use in defaults.
	void UploadTextureDataSlow(std::shared_ptr<RenderTexture>& texture, DataStreamRef srcStream,
						   U64 srcOffset, U32 mip, U32 slice, U32 dataSize);
	
	// Draws indexed.
	void DrawIndexed(U32 numIndices, U32 numInstances, U32 indexStart, I32 vertexIndexOffset, U32 firstInstanceIndex);
	
	// Sets as many parameters as possible going down the given stack until all have been set that are needed in currently bound shaders
	void BindParameters(RenderFrame& frame, ShaderParametersStack* stack);
	
	// push a render pass to the stack. Pop the pass when needed. BindXXX calls must have a pass pushed.
	void StartPass(RenderPass&& pass);
	
	RenderPass EndPass(); // end pass, its invalid but its information is in the return value.
	
	// By default all memory uploads and other related operations create their own copy pass if this isn't called. Batch them ideally together
	void StartCopyPass();
	
	void Submit(); // submit. will not wait.
	
	void Wait(); // wait for commands to finish executing, if submitted. use in render thread.
	
	Bool Finished(); // Returns true if finished, or was never submitted.
	
};

enum class SceneMessageType : U32
{
	START_INTERNAL, // INTERNAL! do not post this externally. internally starts the scene in the next frame.
	STOP, // stops the scene rendering and removes it from the stack.
};

// A message that can be sent to a scene. This can be to update one of the agents positions, etc.
struct SceneMessage
{
	
	String Scene; // scene name
	Symbol Agent; // scene agent sym, if needed (is a lot)
	void* Arguments = nullptr; // arguments to this message if needed. Must allocate context.AllocateMessageAruments Freed after use internally.
	SceneMessageType Type; // messasge type
	U32 Priority = 0; // priority of the message. higher ones are done first.
	
	inline bool operator<(const SceneMessage& rhs) const
	{
		return Priority < rhs.Priority;
	}
	
};

// ============================================= HIGH LEVEL DRAW COMMAND STRUCTS =============================================

// A draw instance. These can be created by the render context only on the populater thread to issue high level draw commands.
// These boil down to in the render execution of these finding a pipeline state for this render instance
class RenderInst
{
	
	RenderInst* _Next = nullptr; // next in unordered array. this is sorted at execution.
	
	/*Used to sort for render layers. BIT FORMAT:
	 Bits 0-25: unused (all 0s for now)
 	 Bits 26-36: sub layer (0-1023)
	 Bits 37-45: unused (all 1s for now)
	 Bits 46-62: layer + 0x8000 (layer is signed short range, from -32767 to 32767)
	 Bits 63-64: transparent mode (MSB, meaning most significant in determining order). not used yet.
	 */
	U64 _SortKey;
	
	RenderVertexState _VertexStateInfo; // info (not actual) about vertex state
	
	RenderPrimitiveType _PrimitiveType = RenderPrimitiveType::TRIANGLE_LIST; // default triangles
	
	ShaderParametersStack* _Parameters = nullptr; // shader inputs
	
	//U32 _ObjectIndex = (U32)-1; // object arbitrary index. can be used to hide specific draws if the view says they arent visible.
	//U32 _BaseIndex = 0; ??
	//U32 _MinIndex = 0;
	//U32 _MaxIndex = 0; // vertex range
	
	U32 _StartIndex = 0; // start index in index buffer
	U32 _IndexCount = 0; // number of indices to draw
	U32 _InstanceCount = 0; // number of instances to draw
	I32 _BaseIndex = 0; // offset added to each index buffer value
	U32 _IndexBufferIndex = 0; // into vertex state
	
	DefaultRenderMeshType _DrawDefault = DefaultRenderMeshType::NONE;
	
	String Program;

public:
	
	/**
	 * Sets the render layer. This determines the draw order. layer is from -0x8000 to 0x7FFF and sublayer is from 0 to 0x3FF.
	 */
	inline void SetRenderLayer(I32 layer, U32 sublayer){
		U32 transparencyMode = 0; // no need rn
		layer = MIN(0x7FFF, MAX(layer, -0x8000));
		sublayer = MIN(0x3FF, MAX(sublayer, 0));
		_SortKey = ((U64)transparencyMode << 62) | ((U64)sublayer << 26) | ((U64)(layer + 0x8000) << 46) | 0x3FF000000000llu;
	}
	
	/**
	 * Gets the current render layer of this draw instance.
	 */
	inline I32 GetRenderLayer() {
		return (I32)((U32)(_SortKey >> 46)) - 0x8000;
	}
	
	/**
	 Gets the current sub-layer of this draw instance
	 */
	inline U32 GetRenderSubLayer(){
		return (U32)((_SortKey >> 26) & 0x3FF);
	}
	
	/**
	 * Sets the range of indices to draw.
	 
	inline void SetIndexRange(U32 minIndex, U32 maxIndex){
		_MinIndex = minIndex;
		_MaxIndex = maxIndex;
	} */
	
	/**
	 Draws a primitive array.
	 */
	inline void DrawPrimitives(RenderPrimitiveType type, U32 start_index, U32 prim_count, U32 instance_count, I32 baseIndex)
	{
		_StartIndex = start_index;
		_PrimitiveType = type;
		_InstanceCount = instance_count;
		_IndexCount = GetNumVertsForPrimitiveType(type, prim_count);
		_BaseIndex = baseIndex;
	}
	
	/**
	 Draws vertices for a given primitive.
	 */
	inline void DrawVertices(RenderPrimitiveType type, U32 start_index, U32 vertex_count, U32 instance_count, I32 baseIndex){
		_InstanceCount = instance_count;
		_PrimitiveType = type;
		_StartIndex = start_index;
		_IndexCount = vertex_count;
		_BaseIndex = baseIndex;
	}
	
	/**
	 Sets the vertex and fragment shaders. In the future this will be made obsolete by having a system where shaders are referenced by enums and variants.
	 */
	inline void SetShaderProgram(String name)
	{
		Program = std::move(name);
	}
	
	/**
	 Required if drawing a non-default mesh. Sets the vertex information layout.
	 */
	inline void SetVertexState(RenderVertexState& info)
	{
		_VertexStateInfo = info;
	}
	
	/**
	 Draws a default mesh. Ensure you bind the correct parameters required in the parameter stack somewhere for this default mesh type.
	 Nothing else needs to be specified. Shaders, blending and everything else is part of the default mesh pipeline so is not customisable.
	 */
	inline void DrawDefaultMesh(DefaultRenderMeshType type)
	{
		_DrawDefault = type;
		_InstanceCount = 1;
		_IndexCount = _IndexBufferIndex = 0; // set later
	}
	
	inline Bool operator<(const RenderInst& rhs) const {
		return _SortKey < rhs._SortKey;
	}
	
	friend struct RenderInstSorter;
	friend class RenderContext;
	
};

// Sorter for above
struct RenderInstSorter {
	
	inline Bool operator()(const RenderInst* lhs, const RenderInst* rhs) const {
		return lhs->_SortKey < rhs->_SortKey;
	}
	
};

// ============================================= RENDER FRAME =============================================

class RenderFrameUpdateList;

/// A render frame. Two are active at one time, one is filled up my the main thread and then executed and run on the GPU in the render job which is done async.
struct RenderFrame
{
	
	U64 FrameNumber = 0; // this frame number
	RenderTexture* BackBuffer = nullptr; // set at begin render frame internally
	
	std::vector<RenderCommandBuffer*> CommandBuffers; // command buffers which will be destroyed at frame end (render execution)
	
	RenderInst* DrawCalls = nullptr; // draw call list (high level)
	U32 NumDrawCalls = 0; // number of elements in linked list above
	
	RenderFrameUpdateList* UpdateList; // list of updates for frame.
	
	LinearHeap Heap; // frame heap memory
	
	// Reset and clear the frame to new frame number
	void Reset(RenderContext& context, U64 newFrameNumber);
	
	// Shared pointer will be released at end of frame after render. Use to keep pointers alive for duration of frame
	template<typename T>
	inline void PushAutorelease(std::shared_ptr<T> val)
	{
		auto it = _Autorelease.find((U64)val.get());
		if(it == _Autorelease.end())
			_Autorelease[(U64)val.get()] = std::move(val);
	}
	
	std::unordered_map<U64, std::shared_ptr<void>> _Autorelease; // released at end of frame.
	
};

/**
 This class is a per-frame managed state which takes care of handling copy pass information.
 This gets executed before any draw calls such that all updates complete before the renderfer needs them.
 */
class RenderFrameUpdateList
{
	
	// No staging buffer. Static data.
	struct MetaBufferUpload
	{
		
		MetaBufferUpload* Next = nullptr;
		
		Meta::BinaryBuffer Data;
		
		std::shared_ptr<RenderBuffer> Buffer;
		
		U64 DestPosition = 0; // dest position in Buffer
		
	};
	
	struct DataStreamBufferUpload
	{
		
		DataStreamBufferUpload* Next = nullptr;
		
		DataStreamRef Src;
		std::shared_ptr<RenderBuffer> Buffer;
		
		U64 Position = 0; // start pos
		U64 UploadSize = 0;
		
		U64 DestPosition = 0; // dest position Buffer
		
	};
	
	friend class RenderContext;
	
	RenderContext& _Context;
	RenderFrame& _Frame;
	
	U64 _TotalBufferUploadSize = 0; // total bytes this frame so far being uploaded to GPU
	U32 _TotalNumUploads = 0; // number of uploads to process this frame
	
	MetaBufferUpload* _MetaUploads = nullptr;
	DataStreamBufferUpload* _StreamUploads = nullptr;
	
	// Begins the render frame internally. This does all uploading updates to GPU.
	void BeginRenderFrame(RenderCommandBuffer* pCopyCommands);
	
	void EndRenderFrame();
	
	// Remove the buffer from any previous uploads, as a new upload is present which would override it.
	void _DismissBuffer(const std::shared_ptr<RenderBuffer>& buf);
	
public:
	
	inline RenderFrameUpdateList(RenderContext& context, RenderFrame& frame) : _Frame(frame), _Context(context) {}
	
	void UpdateBufferMeta(const Meta::BinaryBuffer& metaBuffer, const std::shared_ptr<RenderBuffer>& destBuffer, U64 destOffset);
	
	void UpdateBufferDataStream(const DataStreamRef& srcStream, const std::shared_ptr<RenderBuffer>& destBuffer, U64 srcPos, U64 nBytes, U64 destOffset);
	
};


// registers common mesh constants
void RegisterRenderConstants(LuaFunctionCollection& Col);

// Utilities for rendering. Only to be used in scene::asyncrender and its calls
namespace RenderUtility
{
	
	/**
	 Sets the object uniform buffer parameters, setting the world matrix and diffuse colour (includes alpha value).
	 */
	void SetObjectParameters(RenderContext& context, ShaderParameter_Object* obj, Matrix4 objectWorldMatrix, Colour colour);
	
	/**
	 Sets the camera uniform buffer parameters from a given camera.
	 */
	void SetCameraParameters(RenderContext& context, ShaderParameter_Camera* cam, Camera* pCamera);
	
	/**
	 Creates a scaling and transforming matrix for the given bounding box so its the same
	 */
	inline Matrix4 CreateBoundingBoxModelMatrix(BoundingBox box) {
		// Calculate the center of the bounding box from _Min and _Max
		Float centerX = (box._Min.x + box._Max.x) / 2.0f;
		Float centerY = (box._Min.y + box._Max.y) / 2.0f;
		Float centerZ = (box._Min.z + box._Max.z) / 2.0f;
		
		// Calculate the size (dimensions) of the bounding box
		Float width = box._Max.x - box._Min.x;
		Float height = box._Max.y - box._Min.y;
		Float depth = box._Max.z - box._Min.z;
		
		// Scale factors to transform the bounding box into a unit box from -1 to 1
		Float scaleX = 0.5f * width;
		Float scaleY = 0.5f * height;
		Float scaleZ = 0.5f * depth;
		
		// Translation to move the center of the bounding box to the origin
		Float translateX = -centerX;
		Float translateY = -centerY;
		Float translateZ = -centerZ;
		
		// Create the transformation matrix using scaling and translation
		Matrix4 modelMatrix{};
		
		modelMatrix = MatrixScaling(scaleX, scaleY, scaleZ) * MatrixTranslation(Vector3(translateX, translateY, translateZ));
		
		return modelMatrix;
	}

	/**
	 Creates a scaling and transforming matrix for the given sphere such that it can be drawn as default sphere or wireframe sphere
	 */
	inline Matrix4 CreateSphereModelMatrix(Sphere sphere)
	{
		return MatrixTransformation(Vector3(sphere._Radius), Quaternion::kIdentity, sphere._Center);
	}
	
	// internal draw. null camera means a higher level camera will be searched for in the base parameters stack
	void _DrawInternal(RenderContext& context, Camera* cam, Matrix4 world, Colour col, DefaultRenderMeshType primitive, ShaderParametersStack* pBaseParams);
	
	/**
	 Draws a wire sphere with the given camera and model matrix specifying its world transformation. Vertices from -1 to 1. Camera can be null. Pass in base parameters.
	 */
	inline void DrawWireSphere(RenderContext& context, Camera* cam, Matrix4 model, Colour col, ShaderParametersStack* pBaseParams)
	{
		_DrawInternal(context, cam, model, col, DefaultRenderMeshType::WIREFRAME_SPHERE, pBaseParams);
	}
	
	/**
	 Draws a wireframe unit capsule with the given camera and model matrix specifying its world transformation. Vertices are from -1 to 1! Camera can be null. Pass in base parameters.
	 */
	inline void DrawWireCapsule(RenderContext& context, Camera* cam, Matrix4 model, Colour col, ShaderParametersStack* pBaseParams)
	{
		_DrawInternal(context, cam, model, col, DefaultRenderMeshType::WIREFRAME_CAPSULE, pBaseParams);
	}
	
	/**
	 Draws a wireframe box with the given camera and model matrix specifying its world transformation.  Vertices from -1 to 1 Camera can be null. Pass in base parameters.
	 */
	inline void DrawWireBox(RenderContext& context, Camera* cam, Matrix4 model, Colour col, ShaderParametersStack* pBaseParams)
	{
		_DrawInternal(context, cam, model, col, DefaultRenderMeshType::WIREFRAME_BOX, pBaseParams);
	}
	
	/**
	 Draws a filled coloured box with the given camera and model matrix specifying its world transformation.  Vertices from -1 to 1 Camera can be null. Pass in base parameters.
	 */
	inline void DrawFilledBox(RenderContext& context, Camera* cam, Matrix4 model, Colour col, ShaderParametersStack* pBaseParams)
	{
		_DrawInternal(context, cam, model, col, DefaultRenderMeshType::FILLED_BOX, pBaseParams);
	}
	
	/**
	 Draws a filled in sphere with the given camera and model matrix specifying its world transformation.  Vertices from -1 to 1 Camera can be null. Pass in base parameters.
	 */
	inline void DrawFilledSphere(RenderContext& context, Camera* cam, Matrix4 model, Colour col, ShaderParametersStack* pBaseParams)
	{
		_DrawInternal(context, cam, model, col, DefaultRenderMeshType::FILLED_SPHERE, pBaseParams);
	}
	
	/**
	 Draws a filled in cone with the given camera and model matrix specifying its world transformation.  Vertices from -1 to 1 in height and width. Camera can be null. Pass in base parameters.
	 */
	inline void DrawFilledCone(RenderContext& context, Camera* cam, Matrix4 model, Colour col, ShaderParametersStack* pBaseParams)
	{
		_DrawInternal(context, cam, model, col, DefaultRenderMeshType::FILLED_CONE, pBaseParams);
	}
	
	/**
	 Draws a wireframe cone with the given camera and model matrix specifying its world transformation.  Vertices from -1 to 1 Camera can be null. Pass in base parameters.
	 */
	inline void DrawWireCone(RenderContext& context, Camera* cam, Matrix4 model, Colour col, ShaderParametersStack* pBaseParams)
	{
		_DrawInternal(context, cam, model, col, DefaultRenderMeshType::WIREFRAME_CONE, pBaseParams);
	}
	
	/**
	 Draws a filled in cylinder with the given camera and model matrix specifying its world transformation.  Vertices from -1 to 1 in height and width. Camera can be null. Pass in base parameters.
	 */
	inline void DrawFilledCylinder(RenderContext& context, Camera* cam, Matrix4 model, Colour col, ShaderParametersStack* pBaseParams)
	{
		_DrawInternal(context, cam, model, col, DefaultRenderMeshType::FILLED_CYLINDER, pBaseParams);
	}
	
	/**
	 Draws a wireframe cylinder with the given camera and model matrix specifying its world transformation.  Vertices from -1 to 1 Camera can be null. Pass in base parameters.
	 */
	inline void DrawWireCylinder(RenderContext& context, Camera* cam, Matrix4 model, Colour col, ShaderParametersStack* pBaseParams)
	{
		_DrawInternal(context, cam, model, col, DefaultRenderMeshType::WIREFRAME_CYLINDER, pBaseParams);
	}
	
}
