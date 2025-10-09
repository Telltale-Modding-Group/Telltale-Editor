#include <Renderer/RenderContext.hpp>

#include <vector>
#include <unordered_map>

enum class SceneRendererFlag
{
    EDIT_MODE = 1, // edit mode
};

struct SceneFrameRenderParams
{
    Ptr<Scene> RenderScene;
    RenderTargetID Target;
    RenderNDCScissorRect Viewport;
    U32 TargetWidth, TargetHeight;
    void(*RenderPost)(void* UserData, const SceneFrameRenderParams& params, RenderSceneView* pMainSceneView) = nullptr;
    void* UserData = nullptr;
};

/**
 * The Scene Renderer is responsible for rendering the scene.
 */
class SceneRenderer
{
public:

    SceneRenderer(Ptr<RenderContext> pRenderContext);

    ~SceneRenderer();

    // Toggle editor mode
    void SetEditMode(Bool bOnOff);

    // Render the scene! Target is dest target (eg editor target, or main back buffer). Scissor is used if not 
    // default back buffer. Copies all of target into main back buffer within scissor inside the main back buffer
    void RenderScene(const SceneFrameRenderParams& frameRender);

    // Reset all scene render information
    void ResetScene(Ptr<Scene> pScene);
    
    inline Ptr<RenderContext> GetRenderer()
    {
        return _Renderer;
    }

private:

    void _RenderMeshInstance(Ptr<Scene> pScene, RenderFrame& frame, const Ptr<Mesh::MeshInstance> pMeshInstance, Transform model, RenderViewPass* pass, RenderStateBlob blob);
    void _RenderMeshLOD(Ptr<Scene> pScene, RenderFrame& frame, const Ptr<Mesh::MeshInstance> pMeshInstance, Transform model, Mesh::LODInstance& lod, RenderViewPass* pass, RenderStateBlob blob);
    void _RenderMeshBatch(Ptr<Scene> pScene, RenderFrame& frame, const Ptr<Mesh::MeshInstance> pMeshInstance, Transform model, Mesh::LODInstance& lod, Mesh::MeshBatch& batch, RenderViewPass* pass, RenderStateBlob blob);
    void _UpdateMeshBuffers(RenderFrame& frame, const Ptr<Mesh::MeshInstance> pMeshInstance);

    void _PostRenderCallback(RenderFrame* pFrame);

    // Persistent scene render state
    struct SceneState
    {

        struct VertexStateData
        {

            Ptr<RenderBuffer> GPUVertexBuffers[32];
            Ptr<RenderBuffer> GPUIndexBuffer;
            BitSet<U32, 33, 0> Dirty;

            VertexStateData();

        };

        struct MeshInstanceData
        {
            std::vector<VertexStateData> VertexState;
        };

        WeakPtr<Scene> MyScene;
        RenderNDCScissorRect Viewport; 
        Flags StateFlags;
        std::unordered_map<WeakPtr<Mesh::MeshInstance>, MeshInstanceData, WeakPtrHash, WeakPtrEqual> MeshData;
        Ptr<Camera> DefaultCam;

    };

    void _InitSceneState(SceneState& state, Ptr<Scene> pScene); // init the scene state
    void _SwitchSceneState(Ptr<Scene> pCurrentScene); // ensure that is the current scene

    Ptr<RenderContext> _Renderer;
    Flags _Flags;
    std::vector<SceneState> _PersistentScenes; 
    SceneState _CurrentScene;
    U64 _CallbackTag;

};