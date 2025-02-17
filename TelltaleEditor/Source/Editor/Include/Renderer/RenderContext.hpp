#pragma once

// Low level render API. Use RenderContext and then everything else from the higher level API.

#include <Core/Context.hpp>
#include <Renderer/RenderAPI.hpp>

#include <Resource/DataStream.hpp>

#include <vector>
#include <set>

class RenderContext;

struct RenderScene;
struct RenderTexture;
struct RenderFrame;

enum class RenderBufferUsage
{
	VERTEX = SDL_GPU_BUFFERUSAGE_VERTEX,
	INDICES = SDL_GPU_BUFFERUSAGE_INDEX,
	READONLY = SDL_GPU_BUFFERUSAGE_GRAPHICS_STORAGE_READ,
};

/// A buffer in the renderer holding data. You own these and can create them below.
struct RenderBuffer
{
	
	RenderContext* _Context = nullptr;
	SDL_GPUBuffer* _Handle = nullptr;
	
	RenderBufferUsage Usage = RenderBufferUsage::VERTEX;
	U64 SizeBytes = 0; // size of the bytes of the buffer
	
	~RenderBuffer();
	
};

/// Internal transfer buffer for uploads.
struct _RenderTransferBuffer
{
	
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
	
};

/// Used in pipeline state. Represents the format of a vertex data input array and input attributes. No ownership and they are part of a pipeline state descriptor.
struct RenderVertexState
{
	
	RenderContext* _Context = nullptr;
	
	struct VertexAttrib
	{
		RenderBufferAttributeFormat Format = RenderBufferAttributeFormat::UNKNOWN;
		U16 VertexBufferIndex = 0; // 0 to 3 (which vertex buffer to use)
		U16 VertexBufferLocation = 0; // index of the attribute in the current vertex buffer (index). 0 to 31
	};

	VertexAttrib Attribs[32];
	
	U8 NumVertexBuffers = 0; // between 0 and 4
	U8 NumVertexAttribs = 0; // between 0 and 32
	
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
	
	String FragmentSh = ""; // fragment shader name
	String VertexSh = ""; // vertex shader name
	
	// FUNCTIONALITY
	
	RenderVertexState VertexState; // bindable buffer information (format)
	
	void Create(); // creates and sets hash.
	
	~RenderPipelineState(); // destroy if needed
	
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
	
	// bind a pipeline to this command list
	void BindPipeline(std::shared_ptr<RenderPipelineState>& state);
	
	// bind vertex buffers, (array). First is the first slot to bind to. Offsets can be NULL to mean all from start. Offsets is each offset.
	void BindVertexBuffers(std::shared_ptr<RenderBuffer>* buffers, U32* offsets, U32 first, U32 num);
	
	// bind an index buffer for a draw call. isHalf = true means U16 indices in the buffer, false = U32 indices. Pass in offset into buffer.
	void BindIndexBuffer(std::shared_ptr<RenderBuffer> indexBuffer, U32 offset, Bool isHalf);
	
	// Binds num textures along with samplers, starting at the given slot, to the pipeline fragment shader. num is 0 to 32.
	void BindTextures(U32 slot, U32 num, std::shared_ptr<RenderTexture>* pTextures, std::shared_ptr<RenderSampler>* pSamplers);
	
	// Binds num generic storage buffers (eg camera) to a specific shader in the pipeline.
	void BindGenericBuffers(U32 slot, U32 num, std::shared_ptr<RenderBuffer>* pBuffers, RenderShaderType shaderSlot);
	
	// Perform a buffer upload.
	void UploadBufferData(std::shared_ptr<RenderBuffer>& buffer, DataStreamRef srcStream, U64 srcOffset, U32 destOffset, U32 numBytes);
	
	// Perform a texture sub-image upload
	void UploadTextureData(std::shared_ptr<RenderTexture>& texture, DataStreamRef srcStream,
						   U64 srcOffset, U32 mip, U32 slice, U32 dataSize);
	
	// push a render pass to the stack. Pop the pass when needed. BindXXX calls must have a pass pushed.
	void StartPass(RenderPass&& pass, RenderTexture& swapchainTex);
	
	RenderPass EndPass(); // end pass, its invalid but its information is in the return value.
	
	// By default all memory uploads and other related operations create their own copy pass if this isn't called. Batch them ideally together
	void StartCopyPass();
	
	void Submit(); // submit. will not wait.
	
	void Wait(); // wait for commands to finish executing, if submitted. use in render thread.
	
	Bool Finished(); // Returns true if finished, or was never submitted.
	
};

/// A render frame. Two are active at one time, one is filled up my the main thread and then executed and run on the GPU in the render job which is done async.
struct RenderFrame
{

    U64 _FrameNumber = 0; // this frame number
    RenderScene* _SceneStack = nullptr; // ordered scene stack
	RenderTexture* _BackBuffer = nullptr; // set at begin render frame internally
	
	std::vector<RenderCommandBuffer*> _CommandBuffers; // command buffers which will be destroyed at frame end
	
	LinearHeap _Heap; // frame heap memory
    
    // Reset and clear the frame to new frame number
	void Reset(U64 newFrameNumber);
    
};

/// This render context represents a window in which a scene stack can be rendered to.
class RenderContext
{
public:
    
    RenderContext(String windowName); // creates window. start rendering by calling frame update each main thread frame
    ~RenderContext(); // closes window and exists the context, freeing memory
    
    // Call each frame on the main thread to wait for the renderer job (render last frame) and prepare next scene render
    // Pass in if you want to force this to be the last frame (destructor called after). Returns if still running
    Bool FrameUpdate(Bool isLastFrame);
    
    // Current frame index.
    inline U64 GetCurrentFrameNumber()
    {
        return _Frame[_MainFrameIndex]._FrameNumber;
    }
    
    // Current frame index of what is being rendered.
    inline U64 GetRenderFrameNumber()
    {
        return _Frame[_MainFrameIndex ^ 1]._FrameNumber;
    }
    
    // Call these before and after any instance is created. Initialises SDL for multiple contexts
    static Bool Initialise();
    static void Shutdown();
	
	// Create a vertex buffer which can be filled up later
	std::shared_ptr<RenderBuffer> CreateVertexBuffer(U64 sizeBytes);
	
	// Create an index buffer which can be filled up later
	std::shared_ptr<RenderBuffer> CreateIndexBuffer(U64 sizeBytes);
	
	// Create a texture. Call its create member function to create.
	inline std::shared_ptr<RenderTexture> AllocateTexture()
	{
		return TTE_NEW_PTR(RenderTexture, MEMORY_TAG_RENDERER);
	}
    
private:
	
	inline RenderFrame& GetFrame(Bool bGetPopulatingFrame)
	{
		return bGetPopulatingFrame ? _Frame[_MainFrameIndex^ 1] : _Frame[_MainFrameIndex];
	}
	
	// Checks and ensures are being called from the main thread.
	inline void AssertMainThread()
	{
		TTE_ASSERT(IsCallingFromMain(), "This function cannot be called from anywhere but the main thread!");
	}
	
	// =========== INTERNAL RENDERING FUNCTIONALITY
	
	// Perform rendering of high level instructions into command buffers.
	void _Render(Float dt, RenderCommandBuffer* pMainCommandBuffer);
	
	// Allocate, Create() must be called after. New render pipeline state.
	std::shared_ptr<RenderPipelineState> _AllocatePipelineState();
	
	// Find a sampler or allocate new if doesn't exist.
	std::shared_ptr<RenderSampler> _FindSampler(RenderSampler desc);
	
	// Create and initialise new command buffer to render commands to. submit if wanted, if not it is automatically at frame end.
	// swap chain slot is put into slot 0!
	RenderCommandBuffer* _NewCommandBuffer();
	
	SDL_GPUShader* _FindShader(String name, RenderShaderType); // find a shader, load if needed and not previously loaded.]

	// note the transfer buffers below are UPLOAD ONES. DOWNLOAD BUFFERS COULD BE DONE IN THE FUTURE, BUT NO NEED ANY TIME SOON.
	
	_RenderTransferBuffer _AcquireTransferBuffer(U32 size, RenderCommandBuffer& cmds); // find an available transfer buffer to use
	
	void _ReclaimTransferBuffers(RenderCommandBuffer& cmds); // called after a submit command list finishes
	
	// =========== GENERIC FUNCTIONALITY
    
    // async job to populate render instructions
    static Bool _Populate(const JobThread& jobThread, void* pRenderCtx, void* pFrame);
    
    RenderFrame _Frame[2]; // index swaps, main thread and render thread frame
    JobHandle _PopulateJob; // populate render instructions job
    U32 _MainFrameIndex; // 0 or 1. NOT of this value is the render frame index, into the _Frame array.
	U64 _StartTimeMicros = 0; // start time of current mt frame
    
    SDL_Window* _Window; // SDL3 window handle
	SDL_GPUDevice* _Device; // SDL3 graphics device (vulkan,d3d,metal)
    SDL_Surface* _BackBuffer; // SDL3 window surface
	
	std::mutex _Lock; // for below
	std::vector<std::shared_ptr<RenderPipelineState>> _Pipelines; // pipeline states
	std::vector<RenderShader> _LoadedShaders; // in future can replace certain ones and update pipelines for hot reloads.
	std::set<_RenderTransferBuffer> _AvailTransferBuffers;
	std::vector<std::shared_ptr<RenderSampler>> _Samplers;
    
	friend struct RenderPipelineState;
	friend struct RenderShader;
	friend struct RenderCommandBuffer;
	friend struct RenderTexture;
	friend struct RenderVertexState;
	friend struct RenderBuffer;
	
};
