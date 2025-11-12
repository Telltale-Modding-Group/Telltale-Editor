#pragma once

#include <Core/Config.hpp>
#include <Core/Math.hpp>
#include <Meta/Meta.hpp>
#include <Renderer/RenderAPI.hpp>
#include <Scripting/ScriptManager.hpp>
#include <Common/Texture.hpp>
#include <Resource/ResourceRegistry.hpp>

/**
 Group of meshes. The actual common mesh handleable format is MeshInstance, see below.
 */
struct Mesh : HandleLockOwner
{
    
    // separate triangle sets for default and shadow parts
    enum RenderViewType
    {
        DEFAULT = 0,
        SHADOW = 1,
        NUM = 2,
    };

    enum MeshBatchFlags
    {
        BATCH_BOUNDING_DIRTY = 1, // some games dont store the bounding box !
    };
    
    // A mesh batch. This is a optimised way telltale use to draw once per material. Most members TODO.
    struct MeshBatch
    {
        
        BoundingBox BBox;
        Sphere BSphere;
        Flags BatchFlags;
        U32 BatchUsage = 0; // not in use right now but in future. 1:deformable, 2:single deformable, 4:double sided, 8:triangle strip
        U32 MinVertIndex = 0; // in vertex array
        U32 MaxVertIndex = 0; // in vertex array
        I32 BaseIndex = 0; // value added to each index in the index buffer
        U32 StartIndex = 0; // start index buffer indicie (index) index
        U32 NumPrimitives = 0;
        U32 NumIndices = 0;
        //I32 TextureIndices[RenderViewType::NUM]{-1, -1}; // index into textures array for the bound texture.
        I32 MaterialIndex = -1; // material index
        //U32 AdjacencyStartIndex = 0;
        
    };
    
    // A material used by a mesh batch.
    struct MeshMaterial
    {
        
        Handle<RenderTexture> DiffuseTexture;
        
    };
    
    // May be extended. Vertex state.
    struct VertexState
    {

        RenderVertexState Default; // default. in future we can have more for skinning (compute etc). DO NOT CREATE. only set info members.
        Meta::BinaryBuffer VertexBuffers[32];
        Meta::BinaryBuffer IndexBuffer;
        
    };
    
    // LOD in mesh
    struct LODInstance
    {
        
        Sphere BSphere; // bounding sphere for this LOD
        BoundingBox BBox; // bounding box for this LOD
        
        U32 VertexStateIndex = 0; // index into MeshInstance::VertexState
        
        std::vector<MeshBatch> Batches[RenderViewType::NUM]; // batches for default and shadow
        
    };
    
    enum MeshFlags
    {
        FLAG_DEFORMABLE = 1,
    };
    
    // Renderable objects are a list of meshes (in props it has the 'D3D Mesh List' key or 'D3D Mesh'. base mesh + list
    // The common mesh format which we normalise and specialise telltale classes to.
    class MeshInstance : public HandleableRegistered<MeshInstance>
    {
    public:
        
        friend class RenderContext;
        friend class Scene;
        friend class MeshAPI;
        friend struct MeshNormalisationTask;
        
        String Name = "";
        Flags MeshFlags = {};
        
        Sphere BSphere = {}; // bounding sphere for total mesh
        BoundingBox BBox = {}; // bounding box for total mesh
        
        std::vector<LODInstance> LODs; // mesh LODs
        std::vector<VertexState> VertexStates; // vertex states (draw call bind sets), containing verts/inds/etc.
        std::vector<MeshMaterial> Materials;
        
        static constexpr CString ClassHandle = "Handle<D3DMesh>";
        static constexpr CString Class = "D3DMesh";
        static constexpr CString Extension = "d3dmesh";
        
        inline MeshInstance(Ptr<ResourceRegistry> reg) : HandleableRegistered<MeshInstance>(std::move(reg)) {}
        
        virtual void FinaliseNormalisationAsync() override;

        inline virtual CommonClass GetCommonClassType() override
        {
            return CommonClass::MESH;
        }

    private:

        void _GenerateBoundingBox(const LODInstance& lod, MeshBatch& batch);
        
    };
    
    // Registers mesh normalisers and specialisers
    static void RegisterScriptAPI(LuaFunctionCollection& Col);
    
    // creates bounding box for sphere
    static Sphere CreateSphereForBox(BoundingBox bb);
    
    
    
    // adds a mesh instance, which should have been previously normalised into.
    void AddMesh(Ptr<ResourceRegistry>& registry, Handle<Mesh::MeshInstance> handle);
    
    std::vector<Ptr<MeshInstance>> MeshList; // list of meshes
    
};
