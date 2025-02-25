#pragma once

// Render context synthesizes all of the render types into one scene runtime

#include <Core/Context.hpp>
#include <Renderer/RenderAPI.hpp>

#include <Resource/DataStream.hpp>

#include <Scene.hpp>

#include <vector>
#include <set>

/// Represents an execution environment for running scenes, which holds a window.
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
	
	// Create a generic buffer
	std::shared_ptr<RenderBuffer> CreateGenericBuffer(U64 szBytes);
	
	// Create a texture. Call its create member function to create.
	inline std::shared_ptr<RenderTexture> AllocateTexture()
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
	 Pushes another parameter stack to the given parameter stack
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
	
	// Sets a texture parameter
	void SetParameterTexture(RenderFrame& frame, ShaderParametersGroup* group, ShaderParameterType type,
							 std::shared_ptr<RenderTexture> tex, std::shared_ptr<RenderSampler> sampler);
	
	// Sets a uniform parameter
	void SetParameterUniform(RenderFrame& frame, ShaderParametersGroup* group, ShaderParameterType type,
							 const void* uniformData, U32 size);
	
	// Sets a generic buffer parameter
	void SetParameterGenericBuffer(RenderFrame& frame, ShaderParametersGroup* group, ShaderParameterType type,
							 std::shared_ptr<RenderBuffer> buffer, U32 bufferOffset);
	
	// Set a vertex buffer parameter
	void SetParameterVertexBuffer(RenderFrame& frame, ShaderParametersGroup* group, ShaderParameterType type,
								  std::shared_ptr<RenderBuffer> buffer, U32 startOffset);
	
	// Set an index buffer parameter input.
	void SetParameterIndexBuffer(RenderFrame& frame, ShaderParametersGroup* group, ShaderParameterType type,
								  std::shared_ptr<RenderBuffer> buffer, U32 startIndex);
    
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
	
	ShaderParametersGroup* _CreateParameterGroup(RenderFrame&, ShaderParameterTypes);
	
	// Perform rendering of high level instructions into command buffers.
	void _Render(Float dt, RenderCommandBuffer* pMainCommandBuffer);
	
	// Allocate, Create() must be called after. New render pipeline state.
	std::shared_ptr<RenderPipelineState> _AllocatePipelineState();
	
	// Find a sampler or allocate new if doesn't exist.
	std::shared_ptr<RenderSampler> _FindSampler(RenderSampler desc);
	
	// Create and initialise new command buffer to render commands to. submit if wanted, if not it is automatically at frame end.
	// swap chain slot is put into slot 0!
	RenderCommandBuffer* _NewCommandBuffer();
	
	std::shared_ptr<RenderShader> _FindShader(String name, RenderShaderType); // find a shader, load if needed and not previously loaded.]

	// note the transfer buffers below are UPLOAD ONES. DOWNLOAD BUFFERS COULD BE DONE IN THE FUTURE, BUT NO NEED ANY TIME SOON.
	
	_RenderTransferBuffer _AcquireTransferBuffer(U32 size, RenderCommandBuffer& cmds); // find an available transfer buffer to use
	
	void _ReclaimTransferBuffers(RenderCommandBuffer& cmds); // called after a submit command list finishes
	
	// Draws a render command
	void _Draw(RenderFrame& frame, RenderInst inst, RenderCommandBuffer& cmds);
	
	// dont create desc but fill in its params. finds by hash. will create one if not found - lazy
	std::shared_ptr<RenderPipelineState> _FindPipelineState(RenderPipelineState desc);
	
	// calculates pipeline state
	U64 _HashPipelineState(RenderPipelineState& state);
	
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
	std::vector<std::shared_ptr<RenderShader>> _LoadedShaders; // in future can replace certain ones and update pipelines for hot reloads.
	std::set<_RenderTransferBuffer> _AvailTransferBuffers;
	std::vector<std::shared_ptr<RenderSampler>> _Samplers;
	
	std::vector<DefaultRenderMesh> _DefaultMeshes;
	std::vector<DefaultRenderTexture> _DefaultTextures;
	
	std::vector<Scene> _AsyncScenes; // populator job access ONLY (ensure one thread access at a time). list of active rendering scenes.
	
	std::mutex _MessagesLock; // for below
	std::priority_queue<SceneMessage> _AsyncMessages; // messages stack
    
	friend struct RenderPipelineState;
	friend struct RenderShader;
	friend struct RenderCommandBuffer;
	friend struct RenderTexture;
	friend struct RenderVertexState;
	friend struct RenderBuffer;
	friend class Scene;
	
	// Both defined in render defaults source.
	friend void RegisterDefaultTextures(RenderContext& context, RenderCommandBuffer* cmds, std::vector<DefaultRenderTexture>& textures);
	friend void RegisterDefaultMeshes(RenderContext& context, RenderCommandBuffer* cmds, std::vector<DefaultRenderMesh>& meshes);
	
};
