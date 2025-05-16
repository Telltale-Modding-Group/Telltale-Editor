#pragma once

// Render context synthesizes all of the render types into one scene runtime

#include <Core/Context.hpp>
#include <Renderer/RenderAPI.hpp>
#include <Common/Common.hpp>

#include <vector>
#include <set>
#include <type_traits>

// ============================================= MISC =============================================

// Default capped frame rate to stop CPU usage going through the roof. Between 1 and 120.
#define DEFAULT_FRAME_RATE_CAP 60

// Threshold on the number of frames between last use and current frame at which a resource will be freed due to not being hot in use.
#define DEFAULT_HOT_RESOURCE_THRESHOLD 256

// Threshold on the number of frames where a locked resource is not used and can now be unlocked. Larger as only useful for scene unloads (uncommon)
// At 512 / 30fps => every 15 ish seconds to check them
#define DEFAULT_LOCKED_HANDLE_THRESHOLD 512

struct DefaultRenderTexture
{
    
    DefaultRenderTextureType Type;
    Ptr<RenderTexture> Texture;
    
};

struct DefaultRenderMesh
{
    
    DefaultRenderMeshType Type;
    RenderPipelineState PipelineDesc;
    Ptr<RenderBuffer> IndexBuffer;
    Ptr<RenderBuffer> VertexBuffer; // only one vertex buffer for default ones, they are simple.
    U32 NumIndices = 0;
    
};

struct PendingDeletion
{
    Ptr<void> _Resource;
    U64 _LastUsedFrame;
};

enum RenderContextFlags
{
    RENDER_CONTEXT_NEEDS_PURGE = 1,
};

class RenderContext;

// ============================================= RENDER FRAME =============================================

class RenderFrameUpdateList;
class RenderSceneView;
struct RenderSceneViewParams;

/// A render frame. Two are active at one time, one is filled up my the main thread and then executed and run on the GPU in the render job which is done async.
struct RenderFrame
{
    
    U64 FrameNumber = 0; // this frame number
    RenderTexture* BackBuffer = nullptr; // set at begin render frame internally
    
    std::vector<RenderCommandBuffer*> CommandBuffers; // command buffers which will be destroyed at frame end (render execution)
    
    RenderSceneView* Views = nullptr;
    
    RenderFrameUpdateList* UpdateList; // list of updates for frame.
    
    LinearHeap Heap; // frame heap memory
    
    // Reset and clear the frame to new frame number
    void Reset(RenderContext& context, U64 newFrameNumber);
    
    // Shared pointer will be released at end of frame after render. Use to keep pointers alive for duration of frame
    template<typename T>
    inline void PushAutorelease(Ptr<T> val)
    {
        auto it = _Autorelease.find((U64)val.get());
        if(it == _Autorelease.end())
            _Autorelease[(U64)val.get()] = std::move(val);
    }
    
    RenderSceneView* PushView(RenderSceneViewParams parameters, Bool bPushFront);
    
    std::unordered_map<U64, Ptr<void>> _Autorelease; // released at end of frame.
    
    std::vector<RuntimeInputEvent> _Events;
    
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
        
        Ptr<RenderBuffer> Buffer;
        
        U64 DestPosition = 0; // dest position in Buffer
        
    };
    
    struct MetaTextureUpload
    {
        
        MetaTextureUpload* Next = nullptr;
        
        Meta::BinaryBuffer Data;
        
        Ptr<RenderTexture> Texture;
        
        U32 Mip;
        U32 Slice;
        U32 Face;
        
    };
    
    struct DataStreamBufferUpload
    {
        
        DataStreamBufferUpload* Next = nullptr;
        
        DataStreamRef Src;
        Ptr<RenderBuffer> Buffer;
        
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
    MetaTextureUpload* _MetaTexUploads = nullptr;
    
    // Begins the render frame internally. This does all uploading updates to GPU.
    void BeginRenderFrame(RenderCommandBuffer* pCopyCommands);
    
    void EndRenderFrame();
    
    // Remove the buffer from any previous uploads, as a new upload is present which would override it.
    void _DismissBuffer(const Ptr<RenderBuffer>& buf);
    
    void _DismissTexture(const Ptr<RenderTexture>& tex, U32 mip, U32 slice, U32 face);
    
public:
    
    inline RenderFrameUpdateList(RenderContext& context, RenderFrame& frame) : _Frame(frame), _Context(context) {}
    
    void UpdateBufferMeta(const Meta::BinaryBuffer& metaBuffer, const Ptr<RenderBuffer>& destBuffer, U64 destOffset);
    
    void UpdateBufferDataStream(const DataStreamRef& srcStream, const Ptr<RenderBuffer>& destBuffer, U64 srcPos, U64 nBytes, U64 destOffset);
    
    void UpdateTexture(const Ptr<RenderTexture>& destTexture, U32 mip, U32 slice, U32 face);
    
};

// ============================================= RENDER LAYER =============================================

// Layer inside the renderer. Layers are like abstractions to what is rendered. OnAttach & Detach not defined, use constructor / dtor for this.
class RenderLayer
{
    
    friend class RenderContext;
    
public:
    
    virtual ~RenderLayer() = default;
    
    inline String GetName() const { return _Name; } // layer name
    
    inline RenderContext& GetRenderContext() { return _Context; } // render context
    
    explicit RenderLayer(String name, RenderContext& context);
    
    void GetWindowSize(U32& width, U32& height); // get the current window size
    
private:
    
    RenderContext& _Context;
    
    const String _Name;
    
protected:
    
    // Render & update the layer. Make sure to apply scissor argument before rendering. Return the scissor to pass down to subsequent layers.
    virtual RenderNDCScissorRect AsyncUpdate(RenderFrame& frame, RenderNDCScissorRect parentScissor, Float deltaTime) = 0; // perform rendering
    
    // Events passed downwards to layers. This is executed *before* the update, every frame, if any events are polled.
    virtual void AsyncProcessEvents(const std::vector<RuntimeInputEvent>& events) = 0;
    
};

// ========================================== HIGH LEVEL RENDER VIEW & PASS ==========================================

// Code in this section is related to rendering instructions at a higher level which is passed down to the main thread renderer next frame.
// These don't explicitly reference render API structures but define what needs to be done by the renderer this frame, such as being able
// to resolve correct binds, render targets and draw calls for each pass.

// Render scene view type
enum class RenderViewType : U32
{
    MAIN,
    NUM,
};

struct RenderViewPassParams
{
    
    enum PassFl
    {
        DO_CLEAR_COLOUR = 1,
        DO_CLEAR_STENCIL = 2,
        DO_CLEAR_DEPTH = 4,
    };
    
    Colour ClearColour = Colour::Black;
    U32 ClearStencil = 0; // bottom 8 bits used
    Float ClearDepth = 1.0f; // assuming less than. set yourself
    Flags PassFlags{};
    
};

/*
 A high level render pass. Multiple inside a scene view. Treated as POD with no dependencies so no destructor, as this is created and destroyed lots each frame.
 This eventually will translate into a render pipeline state.
 */
class RenderViewPass
{
    
    friend class RenderContext;
    friend class RenderFrame;
    friend class RenderSceneView;
    
    RenderSceneView* SceneView;
    RenderViewPass* Next = nullptr, *Prev = nullptr;
    
    RenderViewPassParams Params;
    
    CString Name = "";
    
    PagedList<RenderInst, 32> DrawCalls;
    
    RenderTargetIDSet TargetRefs{};
    
    ShaderParametersStack Parameters; // base params
    
public:
    
    void PushRenderInst(RenderContext& context, RenderInst &&inst);
    void PushRenderInst(RenderContext& context, RenderInst &&inst, ShaderParametersGroup* params);
    
    void PushPassParameters(RenderContext& context, ShaderParametersGroup* pGroup); // push base parameters for all draws in this pass
    
    void SetName(CString format, ...);
    
    void SetRenderTarget(U32 index, RenderTargetID id, U32 mip, U32 slice);
    void SetDepthTarget(RenderTargetID id, U32 mip, U32 slice);
    
};

struct RenderSceneViewParams
{
    RenderViewType ViewType;
};

/*
 High level scene view. This could be default, light assigments or light baking.
 */
class RenderSceneView
{
    
    friend class RenderContext;
    friend class RenderFrame;
    friend class RenderViewPass;
    
    RenderFrame* Frame;
    RenderSceneView* Next, *Prev;
    
    CString Name;
    
    RenderSceneViewParams Params;
    
    ShaderParametersStack Parameters; // base params
    
    RenderViewPass* PassList, *PassListTail; // list of passes
    
public:
    
    RenderViewPass* PushPass(RenderViewPassParams params); // push a pass
    
    void PushViewParameters(RenderContext& context, ShaderParametersGroup* pGroup); // push base parameters for all passes and draws in this view.
    
    void SetName(CString format, ...);
    
};

// Internal scene drawing context
struct RenderSceneContext
{
    Float DeltaTime;
    RenderCommandBuffer* CommandBuf;
};

// ============================================= RENDER HIGH LEVEL CONTEXT =============================================

/// Represents an execution environment for running scenes, which holds a window.
class RenderContext : public HandleLockOwner
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
    
    Bool IsLeftHanded();
    
    // Pops a layer. This could stop a current scene rendering for example. Will update next frame. If you previously called push, this reverts this.
    void PopLayer();
    
    // Pushes a layer. T must derive from RenderLayer. Include extra arguments in args, not the render context as all require that as the first arg.
    template<typename T, typename... Args>
    inline WeakPtr<T> PushLayer(Args&&... args);
    
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
    Ptr<RenderBuffer> CreateVertexBuffer(U64 sizeBytes, String Name = "TTE Vertex Buffer");
    
    // Create an index buffer which can be filled up later
    Ptr<RenderBuffer> CreateIndexBuffer(U64 sizeBytes, String Name = "TTE Index Buffer");
    
    // Create a generic buffer
    Ptr<RenderBuffer> CreateGenericBuffer(U64 szBytes, String Name = "TTE Generic Buffer");
    
    // Create a texture. Call its create member function to create.
    inline Ptr<RenderTexture> AllocateRuntimeTexture()
    {
        return TTE_NEW_PTR(RenderTexture, MEMORY_TAG_RENDERER, nullptr);
    }
    
    // Purge unused transfer buffers and other GPU resources to free up memory
    void PurgeResources();
    
    /**
     Allocates a useable parameter stack for the current given frame
     */
    ShaderParametersStack* AllocateParametersStack(RenderFrame& frame);
    
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
        TTE_ASSERT(IsCallingFromMain(), "Main thread call");
        _HotResourceThresh = newValue;
    }
    
    // Call from the main thread. See the default macro
    inline void AdjustHotLockedHandleThreshold(U32 newValue)
    {
        TTE_ASSERT(IsCallingFromMain(), "Main thread call");
        _HotLockThresh = newValue;
    }
    
    Bool DbgCheckOwned(const Ptr<Handleable> handle) const; // thread safe to check if we own this resource. if not it was not locked and should have been
    
    Bool TouchResource(const Ptr<Handleable> handle); // thread safe. locks resource. MUST! be called if using a handle in the thread frame.
    
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
    
    void _PushLayer(Ptr<RenderLayer> pLayer);
    
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
    
    std::vector<Ptr<Handleable>> _PurgeColdLocks(); // main thread call. unlocks returned ones, and removes from locked array into return
    
    // Draws a render command
    void _Draw(RenderFrame& frame, RenderInst inst, RenderCommandBuffer& cmds);
    
    // dont create desc but fill in its params. finds by hash. will create one if not found - lazy
    Ptr<RenderPipelineState> _FindPipelineState(RenderPipelineState desc);
    
    // calculates pipeline state
    U64 _HashPipelineState(RenderPipelineState& state);
    
    void _FreePendingDeletions(U64 currentFrameNumber); // main thread call
    
    void _ExecuteFrame(RenderFrame& frame, RenderSceneContext& context); // execute render scene views
    void _ExecutePass(RenderFrame& frame, RenderSceneContext& context, RenderViewPass* pass);
    
    void _PrepareRenderPass(RenderFrame& frame, RenderPass& pass, const RenderViewPass& viewInfo);
    void _ResolveTargets(RenderFrame& frame, const RenderTargetIDSet& ids, RenderTargetSet& set);
    void _ResolveTarget(RenderFrame& frame, const RenderTargetIDSurface& surface, RenderTargetResolvedSurface& resolved, U32 screenW, U32 screenH);
    void _ResolveBackBuffers(RenderCommandBuffer& buf);
    
    // =========== GENERIC FUNCTIONALITY
    
    // async job to populate render instructions
    static Bool _Populate(const JobThread& jobThread, void* pRenderCtx, void* pFrame);
    
    RenderFrame _Frame[2]; // index swaps, main thread and render thread frame
    JobHandle _PopulateJob; // populate render instructions job
    U32 _MainFrameIndex; // 0 or 1. NOT of this value is the render frame index, into the _Frame array.
    U64 _StartTimeMicros = 0; // start time of current mt frame
    U32 _MinFrameTimeMS; // min frame time (ie max frame rate)
    U32 _HotResourceThresh; // hot resource threshold
    U32 _HotLockThresh;
    
    SDL_Window* _Window; // SDL3 window handle
    SDL_GPUDevice* _Device; // SDL3 graphics device (vulkan,d3d,metal)
    SDL_Surface* _BackBuffer; // SDL3 window surface
    
    std::mutex _Lock; // for below
    std::vector<Ptr<RenderPipelineState>> _Pipelines; // pipeline states
    std::vector<Ptr<RenderShader>> _LoadedShaders; // in future can replace certain ones and update pipelines for hot reloads.
    std::set<_RenderTransferBuffer> _AvailTransferBuffers;
    std::vector<Ptr<RenderSampler>> _Samplers;
    std::vector<PendingDeletion> _PendingSDLResourceDeletions{};
    std::vector<Ptr<Handleable>> _LockedResources; // locked resources which we use (eg textures, meshes etc). common to all layers.
    std::vector<Ptr<RenderLayer>> _DeltaLayers; // if nullptr means a pop. locked
    U32 _Flags = 0;
    
    std::vector<Ptr<RenderLayer>> _LayerStack; // NOT THREAD SAFE.
    
    // ACCESS ONLY THROUGH _GETDEFAULTTEXTURE AND _GETDEFAULTMESH
    std::vector<DefaultRenderMesh> _DefaultMeshes;
    std::vector<DefaultRenderTexture> _DefaultTextures;
    
    // ACCESS ONLY BY RENDER
    Ptr<RenderTexture> _ConstantTargets[(U32)RenderTargetConstantID::NUM];
    // Dynamic targets?
    
    friend struct RenderPipelineState;
    friend struct RenderShader;
    friend struct RenderCommandBuffer;
    friend struct RenderTexture;
    friend struct RenderSampler;
    friend struct RenderVertexState;
    friend struct RenderBuffer;
    friend class RenderFrameUpdateList;
    friend class RenderLayer;
    
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

template<typename T, typename... Args> // note we must have 1 or more arguments but they should require that (comma in macro)
inline WeakPtr<T> RenderContext::PushLayer(Args&&... args)
{
    static_assert(std::is_base_of<RenderLayer, T>::value, "T is not a render layer!");
    Ptr<T> pLayer = TTE_NEW_PTR(T, MEMORY_TAG_RENDER_LAYER, *this, std::forward<Args>(args)...);
    _PushLayer(pLayer);
    return pLayer; // as weak
}
