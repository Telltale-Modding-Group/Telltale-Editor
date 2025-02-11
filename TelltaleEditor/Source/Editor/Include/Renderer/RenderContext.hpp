#pragma once

// Low level render API. Use RenderContext and then everything else from the higher level API.

#include <Core/Context.hpp>
#include <Renderer/RenderAPI.hpp>

#include <vector>

class RenderContext;

struct RenderScene;
struct RenderTexture;
struct RenderFrame;

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
	
	void Create(); // creates and sets hash.
	
	~RenderPipelineState(); // destroy if needed
	
};

/// A render pass
struct RenderPass
{
	
	RenderPass* _Next = nullptr;
	
	SDL_GPURenderPass* _Handle = nullptr;
	
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
	RenderPass* _PassStack = nullptr; // pass stack for command buffer
	Bool _Submitted = false;
	
	// bind a pipeline to this command list
	void BindPipeline(std::shared_ptr<RenderPipelineState>& state);
	
	// push a render pass to the stack. Pop the pass when needed. BindXXX calls must have a pass pushed.
	void PushPass(RenderPass&& pass, RenderTexture& swapchainTex);
	
	RenderPass PopPass(); // pop last pass, its invalid but its information is in the return value.
	
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
    // Pass in if you want to force this to be the last frame (destructor called after). Returns if user wants to quit.
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
    
private:
	
	inline RenderFrame& GetFrame(Bool bGetRenderFrame)
	{
		return bGetRenderFrame ? _Frame[_MainFrameIndex^ 1] : _Frame[_MainFrameIndex];
	}
	
	// Checks and ensures are being called from not the main thread.
	inline void AssertRenderThread()
	{
		TTE_ASSERT(!IsCallingFromMain(), "This function cannot be called from anywhere but the render job!");
	}
	
	// Checks and ensures are being called from not the main thread.
	inline void AssertMainThread()
	{
		TTE_ASSERT(IsCallingFromMain(), "This function cannot be called from anywhere but the main thread!");
	}
	
	// =========== THIS GROUP OF FUNCTIONS MUST BE CALLED BY THE RENDER FRAME, NOT THE MAIN THREAD.
	
	// Allocate, Create() must be called after. New render pipeline state.
	std::shared_ptr<RenderPipelineState> _AllocatePipelineState();
	
	// Create and initialise new command buffer to render commands to. submit if wanted, if not it is automatically at frame end.
	// swap chain slot is put into slot 0!
	RenderCommandBuffer* _NewCommandBuffer();
	
	SDL_GPUShader* _FindShader(String name, RenderShaderType); // find a shader, load if needed and not previously loaded.
	
	// =========== GENERIC FUNCTIONALITY
    
    // Perform main thread render
    void _RenderMain(Float deltaTimeSeconds);
    
    // async render job to perform render of last frame
    static Bool _Render(const JobThread& jobThread, void* pRenderCtx, void* pFrame);
    
    RenderFrame _Frame[2]; // index swaps, main thread and render thread frame
    JobHandle _RenderJob; // actual gpu render job
    U32 _MainFrameIndex; // 0 or 1. NOT of this value is the render frame index, into the _Frame array.
	U64 _StartTimeMicros = 0; // start time of current mt frame
    
    SDL_Window* _Window; // SDL3 window handle
	SDL_GPUDevice* _Device; // SDL3 graphics device (vulkan,d3d,metal)
    SDL_Surface* _BackBuffer; // SDL3 window surface
	
	std::mutex _Lock; // for below
	std::vector<std::shared_ptr<RenderPipelineState>> _Pipelines; // pipeline states
	std::vector<RenderShader> _LoadedShaders; // in future can replace certain ones and update pipelines for hot reloads.
    
	friend struct RenderPipelineState;
	friend struct RenderShader;
	friend struct RenderCommandBuffer;
	friend struct RenderTexture;
	
};
