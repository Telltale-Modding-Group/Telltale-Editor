#pragma once

// Render context synthesizes all of the render types into one scene runtime

#include <Core/Context.hpp>
#include <Renderer/RenderAPI.hpp>

#include <Resource/DataStream.hpp>

#include <Common/Scene.hpp>

#include <vector>
#include <set>

// Default capped frame rate to stop CPU usage going through the roof. Between 1 and 120.
#define DEFAULT_FRAME_RATE_CAP 60

// Threshold on the number of frames between last use and current frame at which a resource will be freed due to not being hot in use.
#define DEFAULT_HOT_RESOURCE_THRESHOLD 256

struct PendingDeletion
{
	Ptr<void> _Resource;
	U64 _LastUsedFrame;
};

enum RenderContextFlags
{
	RENDER_CONTEXT_NEEDS_PURGE,
};

/// Represents an execution environment for running scenes, which holds a window.
class RenderContext
{
public:
    
	// creates window. start rendering by calling frame update each main thread frame. frame rate cap from 1 to 120!
    RenderContext(String windowName, U32 frameRateCap = DEFAULT_FRAME_RATE_CAP);
    ~RenderContext(); // closes window and exists the context, freeing memory
    
    // Call each frame on the main thread to wait for the renderer job (render last frame) and prepare next scene render
    // Pass in if you want to force this to be the last frame (destructor called after). Returns if still running
    Bool FrameUpdate(Bool isLastFrame);
	
	// Returns if the current API uses a row major matrix ordering
	Bool IsRowMajor();
    
    // Current frame index.
    inline U64 GetCurrentFrameNumber()
    {
        return _Frame[_MainFrameIndex].FrameNumber;
    }
    
    // Current frame index of what is being rendered.
    inline U64 GetRenderFrameNumber()
    {
        return _Frame[_MainFrameIndex ^ 1].FrameNumber;
    }
    
    // Call these before and after any instance is created. Initialises SDL for multiple contexts
    static Bool Initialise();
    static void Shutdown();
	
	// Allocate, Create() must be called after. New render pipeline state. Note this should only be used in renderer internally.
	Ptr<RenderPipelineState> _AllocatePipelineState();
	
	// Create a vertex buffer which can be filled up later
	Ptr<RenderBuffer> CreateVertexBuffer(U64 sizeBytes);
	
	// Create an index buffer which can be filled up later
	Ptr<RenderBuffer> CreateIndexBuffer(U64 sizeBytes);
	
	// Create a generic buffer
	Ptr<RenderBuffer> CreateGenericBuffer(U64 szBytes);
	
	// Create a texture. Call its create member function to create.
	inline Ptr<RenderTexture> AllocateTexture()
	{
		return TTE_NEW_PTR(RenderTexture, MEMORY_TAG_RENDERER);
	}
	
	// Pushes a scene to the currently rendering scene stack. No accesses to this can be made directly after this (internally held).
	// Stop the scene using a stop message
	void PushScene(Scene&&);
	
	void* AllocateMessageArguments(U32 size); // freed automatically when executing the message (temp aloc)
	
	void SendMessage(SceneMessage message); // send a message to a scene to do something
	
	// Purge unused transfer buffers and other GPU resources to free up memory
	void PurgeResources();
	
	/**
	 Allocates a useable parameter stack for the current given frame
	 */
	ShaderParametersStack* AllocateParametersStack(RenderFrame& frame);
	
	// Pushes a render instance (call in populater) and executes it next render frame.
	// If a default mesh, param stack can be null (if it requires no other inputs). alloc param stack from the alloc function, not on stack!
	// But 9/10 a vertex buffer and index buffer are needed (not in defaults they are bound automatically) so parameters are needed.
	void PushRenderInst(RenderFrame& frame, ShaderParametersStack* paramStack, RenderInst&& inst);
	
	/**
	 Pushes another parameter stack to the given parameter stack, leaving 'stack' (being pushed) unaffected.
	 */
	void PushParameterStack(RenderFrame& frame, ShaderParametersStack* self, ShaderParametersStack* stack);
	
	/**
	 Pushes a parameter group to the given parameter stack
	 */
	void PushParameterGroup(RenderFrame& frame, ShaderParametersStack* self, ShaderParametersGroup* group);
	
	/**
	 Allocates parameters. Push it to the stack when needed.
	 */
	inline ShaderParametersGroup* AllocateParameters(RenderFrame& frame, ShaderParameterTypes types)
	{
		return _CreateParameterGroup(frame, types);
	}
	
	/**
	 Allocates parameters. Push it to the stack when needed.
	 */
	inline ShaderParametersGroup* AllocateParameter(RenderFrame& frame, ShaderParameterType type)
	{
		ShaderParameterTypes types{};
		types.Set(type, true);
		return AllocateParameters(frame, types);
	}
	
	// Sets a texture parameter. pSamplerDesc must be on the frame heap and not created. It will be resolved in render. Can be null for default.
	void SetParameterTexture(RenderFrame& frame, ShaderParametersGroup* group, ShaderParameterType type,
							 Ptr<RenderTexture> tex, RenderSampler* pSamplerDesc);
	
	// Sets a uniform parameter
	void SetParameterUniform(RenderFrame& frame, ShaderParametersGroup* group, ShaderParameterType type,
							 const void* uniformData, U32 size);
	
	// Sets a generic buffer parameter
	void SetParameterGenericBuffer(RenderFrame& frame, ShaderParametersGroup* group, ShaderParameterType type,
							 Ptr<RenderBuffer> buffer, U32 bufferOffset);
	
	// Set a vertex buffer parameter
	void SetParameterVertexBuffer(RenderFrame& frame, ShaderParametersGroup* group, ShaderParameterType type,
								  Ptr<RenderBuffer> buffer, U32 startOffset);
	
	// Set an index buffer parameter input.
	void SetParameterIndexBuffer(RenderFrame& frame, ShaderParametersGroup* group, ShaderParameterType type,
								  Ptr<RenderBuffer> buffer, U32 startIndex);
	
	// Sets a default texture binding. pSamplerDesc must be on the frame heap and not created. It will be resolved in render. Can be null for default.
	void SetParameterDefaultTexture(RenderFrame& frame, ShaderParametersGroup* group, ShaderParameterType type,
									DefaultRenderTextureType textype, RenderSampler* pSamplerDesc);
    
	// Unsafe call, ensure calling from the correct place!
	inline RenderFrame& GetFrame(Bool bGetPopulatingFrame)
	{
		return bGetPopulatingFrame ? _Frame[_MainFrameIndex^ 1] : _Frame[_MainFrameIndex];
	}
	
	// Call from main thread only (ie high level outside of this class). Default cap macro is above. Range from 1 to 120
	inline void CapFrameRate(U32 fps)
	{
		TTE_ASSERT(fps > 0 && fps <= 120, "Frame rate can only be from 1 to 120!");
		_MinFrameTimeMS = (U32)(1000.0f / ((Float)fps));
	}
	
	// Call from the main thread. See the default macro
	inline void AdjustHotResourceThreshold(U32 newValue)
	{
		_HotResourceThresh = newValue;
	}
	
private:
	
	// Checks and ensures are being called from the main thread.
	inline void AssertMainThread()
	{
		TTE_ASSERT(IsCallingFromMain(), "This function cannot be called from anywhere but the main thread!");
	}
	
	inline Ptr<RenderTexture> _GetDefaultTexture(DefaultRenderTextureType type)
	{
		TTE_ASSERT(type != DefaultRenderTextureType::NONE, "Invalid default texture type");
		return _DefaultTextures[(U32)type - 1].Texture;
	}
	
	inline DefaultRenderMesh& _GetDefaultMesh(DefaultRenderMeshType type)
	{
		TTE_ASSERT(type != DefaultRenderMeshType::NONE, "Invalid default mesh type");
		return _DefaultMeshes[(U32)type - 1];
	}
	
	// =========== INTERNAL RENDERING FUNCTIONALITY
	
	ShaderParametersGroup* _CreateParameterGroup(RenderFrame&, ShaderParameterTypes);
	
	// Perform rendering of high level instructions into command buffers.
	void _Render(Float dt, RenderCommandBuffer* pMainCommandBuffer);
	
	// Find a sampler or allocate new if doesn't exist.
	Ptr<RenderSampler> _FindSampler(RenderSampler desc);
	
	// Create and initialise new command buffer to render commands to. submit if wanted, if not it is automatically at frame end.
	// swap chain slot is put into slot 0!
	RenderCommandBuffer* _NewCommandBuffer();
	
	// find a shader, load if needed and not previously loaded.]
	Ptr<RenderShader> _FindShader(String name, RenderShaderType);
	
	Bool _FindProgram(String name, Ptr<RenderShader>& vert, Ptr<RenderShader>& frag);

	// note the transfer buffers below are UPLOAD ONES. DOWNLOAD BUFFERS COULD BE DONE IN THE FUTURE, BUT NO NEED ANY TIME SOON.
	
	_RenderTransferBuffer _AcquireTransferBuffer(U32 size, RenderCommandBuffer& cmds); // find an available transfer buffer to use
	
	void _ReclaimTransferBuffers(RenderCommandBuffer& cmds); // called after a submit command list finishes
	
	void _PurgeColdResources(RenderFrame*); // free resources kept for too long
	
	// Draws a render command
	void _Draw(RenderFrame& frame, RenderInst inst, RenderCommandBuffer& cmds);
	
	// dont create desc but fill in its params. finds by hash. will create one if not found - lazy
	Ptr<RenderPipelineState> _FindPipelineState(RenderPipelineState desc);
	
	// calculates pipeline state
	U64 _HashPipelineState(RenderPipelineState& state);
	
	void _FreePendingDeletions(U64 currentFrameNumber); // main thread call
	
	// =========== GENERIC FUNCTIONALITY
    
    // async job to populate render instructions
    static Bool _Populate(const JobThread& jobThread, void* pRenderCtx, void* pFrame);
    
    RenderFrame _Frame[2]; // index swaps, main thread and render thread frame
    JobHandle _PopulateJob; // populate render instructions job
    U32 _MainFrameIndex; // 0 or 1. NOT of this value is the render frame index, into the _Frame array.
	U64 _StartTimeMicros = 0; // start time of current mt frame
	U32 _MinFrameTimeMS; // min frame time (ie max frame rate)
	U32 _HotResourceThresh; // hot resource threshold
    
    SDL_Window* _Window; // SDL3 window handle
	SDL_GPUDevice* _Device; // SDL3 graphics device (vulkan,d3d,metal)
    SDL_Surface* _BackBuffer; // SDL3 window surface
	
	std::mutex _Lock; // for below
	std::vector<Ptr<RenderPipelineState>> _Pipelines; // pipeline states
	std::vector<Ptr<RenderShader>> _LoadedShaders; // in future can replace certain ones and update pipelines for hot reloads.
	std::set<_RenderTransferBuffer> _AvailTransferBuffers;
	std::vector<Ptr<RenderSampler>> _Samplers;
	std::vector<PendingDeletion> _PendingSDLResourceDeletions{};
	U32 _Flags = 0;
	
	// ACCESS ONLY THROUGH _GETDEFAULTTEXTURE AND _GETDEFAULTMESH
	std::vector<DefaultRenderMesh> _DefaultMeshes;
	std::vector<DefaultRenderTexture> _DefaultTextures;
	
	std::vector<Scene> _AsyncScenes; // populator job access ONLY (ensure one thread access at a time). list of active rendering scenes.
	
	std::mutex _MessagesLock; // for below
	std::priority_queue<SceneMessage> _AsyncMessages; // messages stack
    
	friend struct RenderPipelineState;
	friend struct RenderShader;
	friend struct RenderCommandBuffer;
	friend struct RenderTexture;
	friend struct RenderSampler;
	friend struct RenderVertexState;
	friend struct RenderBuffer;
	friend class RenderFrameUpdateList;
	friend class Scene;
	
	// Both defined in render defaults source.
	friend void RegisterDefaultTextures(RenderContext& context, RenderCommandBuffer* cmds, std::vector<DefaultRenderTexture>& textures);
	friend void RegisterDefaultMeshes(RenderContext& context, RenderCommandBuffer* cmds, std::vector<DefaultRenderMesh>& meshes);
	
};

/**
 Finalises the given matrix into the out vectors. These can then be passed into shaders
 */
inline void FinalisePlatformMatrix(RenderContext& context, Vector4 (&out)[4], const Matrix4& in)
{
	if(context.IsRowMajor())
	{
		out[0] = in.GetRow(0);
		out[1] = in.GetRow(1);
		out[2] = in.GetRow(2);
		out[3] = in.GetRow(3);
	}
	else
	{
		out[0] = in.GetColumn(0);
		out[1] = in.GetColumn(1);
		out[2] = in.GetColumn(2);
		out[3] = in.GetColumn(3);
	}
}
