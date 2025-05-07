#include <Common/Mesh.hpp>
#include <EditorTasks.hpp>

// RENDERABLE

void SceneModule<SceneModuleType::RENDERABLE>::OnSetupAgent(SceneAgent* pAgentGettingCreated)
{
    // no need to check parent prop. different from actual engine, module is already initiantiated
    pAgentGettingCreated->AgentNode->AddObjDataRef("", Renderable);
}

// NORMALISATION AND SPECIALISATION OF MESH INTO COMMON FORMAT AND BACK
class MeshAPI
{
public:
    
    static inline Mesh::MeshInstance* Task(LuaManager& man)
    {
        return (Mesh::MeshInstance*)man.ToPointer(1);
    }
    
    // setname(commonInst, name)
    static U32 luaSetName(LuaManager& man)
    {
        TTE_ASSERT(man.GetTop() == 2, "Requires 2 arguments");
        TTE_ASSERT(man.Type(2) == LuaType::STRING, "Invalid usage");
        Mesh::MeshInstance* t = Task(man);
        t->Name = man.ToString(2);
        return 0;
    }
    
    // setdeformable(commonInst)
    static U32 luaSetDeformable(LuaManager& man)
    {
        TTE_ASSERT(man.GetTop() == 1, "Requires 1 arguments");
        Mesh::MeshInstance* t = Task(man);
        t->MeshFlags += Mesh::FLAG_DEFORMABLE;
        return 0;
    }
    
    // setvertexbuffer(commonInst, numverts, stride, buffer)
    static U32 luaSetNextVertexBuffer(LuaManager& man)
    {
        TTE_ASSERT(man.GetTop() == 4, "Requires 4 arguments");
        Mesh::MeshInstance* t = Task(man);
        
        if(t->VertexStates.size() == 0)
            t->VertexStates.emplace_back();
        
        U32 nVerts = (U32)man.ToInteger(2);
        U32 stride = (U32)man.ToInteger(3);
        
        Meta::ClassInstance buf = Meta::AcquireScriptInstance(man, 4);
        
        if(!buf)
        {
            TTE_ASSERT(false, "Vertex buffer not found");
            return 0;
        }
        
        Meta::BinaryBuffer* pBuffer = (Meta::BinaryBuffer*)buf._GetInternal();
        
        auto& vertexState = t->VertexStates.back();
        
        TTE_ASSERT(vertexState.Default.NumVertexBuffers != 32, "Too many vertex buffers pushed!");
        vertexState.Default.BufferPitches[vertexState.Default.NumVertexBuffers] = stride;
        vertexState.VertexBuffers[vertexState.Default.NumVertexBuffers++] = *pBuffer;
        
        return 0;
    }
    
    // setvertexbuffer(commonInst, numindices, isHalf, buffer)
    static U32 luaSetIndexBuffer(LuaManager& man)
    {
        TTE_ASSERT(man.GetTop() == 4, "Requires 4 arguments");
        Mesh::MeshInstance* t = Task(man);
        
        if(t->VertexStates.size() == 0)
            t->VertexStates.emplace_back();
        
        U32 nVerts = (U32)man.ToInteger(2);
        Bool isHalf = man.ToBool(3);
        
        Meta::ClassInstance buf = Meta::AcquireScriptInstance(man, 4);
        
        if(!buf)
        {
            TTE_ASSERT(false, "Vertex buffer not found");
            return 0;
        }
        
        Meta::BinaryBuffer* pBuffer = (Meta::BinaryBuffer*)buf._GetInternal();
        
        TTE_ASSERT(pBuffer && pBuffer->BufferSize >= nVerts * (isHalf?2:4), "Buffer invalid for index buffer: null or too small");
        
        auto& vertexState = t->VertexStates.back();
        
        TTE_ASSERT(vertexState.IndexBuffer.BufferSize == 0, "Index buffer already set");
        vertexState.IndexBuffer = *pBuffer;
        
        return 0;
    }
    
    //advance to next vertex state
    static U32 luaAdvanceVertexState(LuaManager& man)
    {
        TTE_ASSERT(man.GetTop() == 1, "Requires 1 argument");
        Mesh::MeshInstance* t = Task(man);
        t->VertexStates.emplace_back();
        return 0;
    }
    
    // Decode compressed float
    static Float DecodeCompressedFloat(U32 encodedValue, U32 bitwidth, Float min, Float max)
    {
        Float range = max - min;
        U32 mask = (1 << bitwidth) - 1;
        Float divisor = (Float)mask;
        Float normalised = (Float)(encodedValue & mask) / (Float)divisor;
        return min + (range * normalised);
    }
    
    // perform decompress of oldest vertices in current vertex buffer
    // NOTE THAT THIS WILL UPDATE THE META CLASS BUFFER VALUE. SET COMPRESSED TO FALSE IN THE SOURCE CLASS TO ENSURE CONSISTENCY
    // decompress(state, buffer, numVerts, compressionType)
    static U32 luaDecompressVertices(LuaManager& man)
    {
        TTE_ASSERT(man.GetTop() == 4, "Requires 4 arguments");
        Mesh::MeshInstance* t = Task(man);
        TTE_ASSERT(t->VertexStates.size() && t->VertexStates.back().Default.NumVertexBuffers, "No vertex buffers set yet");
        
        Meta::ClassInstance bb = Meta::AcquireScriptInstance(man, 2);
        Meta::BinaryBuffer* buf = (Meta::BinaryBuffer*)bb._GetInternal();
        I32 numVerts = man.ToInteger(3);
        I32 type = man.ToInteger(4);
        
        if(type == 0)
        {
            // 2x float compressed Unorm pair
            auto decompressed = TTE_ALLOC_PTR(8 * numVerts, MEMORY_TAG_RUNTIME_BUFFER);
            for(U32 i = 0; i < numVerts; i++)
            {
                const U16& compressedValue = *((U16*)(buf->BufferData.get() + (i<<1)));
                Float* decompressedUV = (Float*)(decompressed.get() + (i<<3));
                decompressedUV[0] = DecodeCompressedFloat(compressedValue & 0xFF, 8, 0.0f, 1.0f);
                decompressedUV[1] = DecodeCompressedFloat((compressedValue >> 8) & 0xFF, 8, 0.0f, 1.0f);
            }
            buf->BufferSize = (U32)numVerts << 3;
            buf->BufferData = std::move(decompressed); // swap out (will remove ref)
        }
        else if(type == 1)
        {
            // 3x float Snorm normal
            auto decompressed = TTE_ALLOC_PTR(12 * numVerts, MEMORY_TAG_RUNTIME_BUFFER);
            for(U32 i = 0; i < numVerts; i++)
            {
                const U16& compressedValue = *((U16*)(buf->BufferData.get() + (i<<1)));
                Float* decompressedNormal = (Float*)(decompressed.get() + (i*12));
                decompressedNormal[0] = DecodeCompressedFloat(compressedValue & 0x7F, 7, -1.0f, 1.0f);
                decompressedNormal[1] = DecodeCompressedFloat((compressedValue >> 7) & 0x7F, 7, -1.0f, 1.0f);
                decompressedNormal[2] = sqrtf(fmaxf(0.0f, 1.0f - (decompressedNormal[0]*decompressedNormal[0]) - (decompressedNormal[1]*decompressedNormal[1])));
                if(compressedValue & 0x4000)
                    decompressedNormal[2] *= -1.0f;
            }
            buf->BufferSize = (U32)numVerts * 12;
            buf->BufferData = std::move(decompressed); // swap out (will remove ref)
        }
        else if(type == 2)
        {
            // 3x float compressed Unorm nomal, approximated z
            auto decompressed = TTE_ALLOC_PTR(12 * numVerts, MEMORY_TAG_RUNTIME_BUFFER);
            for(U32 i = 0; i < numVerts; i++)
            {
                const U16& compressedValue = *((U16*)(buf->BufferData.get() + (i<<1)));
                Float* decompressedNormal = (Float*)(decompressed.get() + (i*12));
                decompressedNormal[0] = DecodeCompressedFloat(compressedValue & 0xFF, 8, 0.0f, 1.0f);
                decompressedNormal[1] = DecodeCompressedFloat((compressedValue >> 8) & 0xFF, 8, 0.0f, 1.0f);
                decompressedNormal[2] = 1 - decompressedNormal[0] - decompressedNormal[1];
            }
            buf->BufferSize = (U32)numVerts * 12;
            buf->BufferData = std::move(decompressed); // swap out (will remove ref)
        }
        else
        {
            TTE_ASSERT(false, "Unknown vertex compression");
        }
        
        return 0;
    }
    
    // pushlod(inst, vertexstateindex)
    static U32 luaPushLevelOfDetail(LuaManager& man)
    {
        TTE_ASSERT(man.GetTop() == 2, "Requires 2 arguments");
        
        Mesh::MeshInstance* t = Task(man);
        U32 sindex = (U32)man.ToInteger(2);
        
        t->LODs.emplace_back();
        t->LODs.back().VertexStateIndex = sindex;
        
        return 0;
    }
    
    static void _GetBoundings(LuaManager& man, Meta::ClassInstance& bb, Meta::ClassInstance& sph, BoundingBox& box, Sphere& bsph, Bool createSphere)
    {
        Meta::ClassInstance minmax = Meta::GetMember(bb, "mMin", true);
        box._Min.Set(Meta::GetMember<Float>(minmax, "x"), Meta::GetMember<Float>(minmax, "y"), Meta::GetMember<Float>(minmax, "z"));
        minmax = Meta::GetMember(bb, "mMax", true);
        box._Max.Set(Meta::GetMember<Float>(minmax, "x"), Meta::GetMember<Float>(minmax, "y"), Meta::GetMember<Float>(minmax, "z"));
        
        if(createSphere)
            bsph = ::Mesh::CreateSphereForBox(box);
        else
        {
            minmax = Meta::GetMember(bb, "mCenter", true);
            bsph._Center.Set(Meta::GetMember<Float>(minmax, "x"), Meta::GetMember<Float>(minmax, "y"), Meta::GetMember<Float>(minmax, "z"));
            bsph._Radius = Meta::GetMember<Float>(bb, "mRadius");
        }
    }
    
    // bounds(inst, meta bb, optional meta bsphere)
    static U32 luaSetLODBounds(LuaManager& man)
    {
        TTE_ASSERT(man.GetTop() == 2 || man.GetTop() == 3, "Required 2/3 arguments");
        
        Mesh::MeshInstance* t = Task(man);
        
        Meta::ClassInstance bb = Meta::AcquireScriptInstance(man, 2);
        TTE_ASSERT(bb, "Bounding box meta instance was not found or was invalid");
        Meta::ClassInstance sph{};
        if(man.GetTop() == 3)
        {
            sph = Meta::AcquireScriptInstance(man, 3);
            TTE_ASSERT(sph, "Bounding sphere meta instance invalid or was not found");
        }
        
        BoundingBox box{};
        Sphere bsph{};
        _GetBoundings(man, bb, sph, box, bsph, man.GetTop() == 2);
        
        t->LODs.back().BBox = box;
        t->LODs.back().BSphere = bsph;
        
        return 0;
    }
    
    // FOR DOCS: note that this will set bounds to parent LOD bounds if not set, so set them before calling any batch pushed (can override after)
    // pushbatch(inst, bShadow) - todo i know that twd michonne only exception has its own batch array?!
    static U32 luaPushBatch(LuaManager& man)
    {
        TTE_ASSERT(man.GetTop() == 2, "Requires 2 arguments");
        Mesh::MeshInstance* t = Task(man);
        TTE_ASSERT(t->LODs.size(), "No LODs");
        
        U32 batchIndex = man.ToBool(2) ? 1 : 0;
        
        t->LODs.back().Batches[batchIndex].emplace_back();
        
        auto& batch = t->LODs.back().Batches[batchIndex].back();
        
        batch.BBox = t->LODs.back().BBox; // default bounds
        batch.BSphere = t->LODs.back().BSphere;
        
        return 0;
    }
    
    // bounds(inst, bShadow, meta bb, optional meta bsphere)
    static U32 luaBatchSetBounds(LuaManager& man)
    {
        TTE_ASSERT(man.GetTop() == 3 || man.GetTop() == 4, "Required 3/4 arguments");
        
        Mesh::MeshInstance* t = Task(man);
        TTE_ASSERT(t->LODs.size(), "No LODs");
        
        U32 batchIndex = man.ToBool(2) ? 1 : 0;
        Meta::ClassInstance bb = Meta::AcquireScriptInstance(man, 3);
        TTE_ASSERT(bb, "Bounding box meta instance was not found or was invalid");
        Meta::ClassInstance sph{};
        if(man.GetTop() == 4)
        {
            sph = Meta::AcquireScriptInstance(man, 4);
            TTE_ASSERT(sph, "Bounding sphere meta instance invalid or was not found");
        }
        
        BoundingBox box{};
        Sphere bsph{};
        _GetBoundings(man, bb, sph, box, bsph, man.GetTop() == 3);
        
        t->LODs.back().Batches[batchIndex].back().BBox = box;
        t->LODs.back().Batches[batchIndex].back().BSphere = bsph;
        
        return 0;
    }
    
    // base index?
    // params(inst, bShadow, min vert, max vert, start index buffer index, num primitives, num indices, baseindex, materialIndex)
    static U32 luaSetBatchParameters(LuaManager& man)
    {
        TTE_ASSERT(man.GetTop() == 9, "Required 9 arguments");
        
        Mesh::MeshInstance* t = Task(man);
        
        U32 batchType = man.ToBool(2) ? 1 : 0;
        U32 minVert = (U32)man.ToInteger(3);
        U32 maxVert = (U32)man.ToInteger(4);
        U32 startInd = (U32)man.ToInteger(5);
        U32 numPrim = (U32)man.ToInteger(6);
        U32 numInd = (U32)man.ToInteger(7);
        I32 baseInd = man.ToInteger(8);
        U32 matInd = (U32)man.ToInteger(9);
        
        TTE_ASSERT(t->LODs.size() && t->LODs.back().Batches[batchType].size(), "No LODs/batches");
        
        auto& batch = t->LODs.back().Batches[batchType].back();
        batch.MinVertIndex = minVert;
        batch.MaxVertIndex = maxVert;
        batch.NumIndices = numInd;
        batch.NumPrimitives = numPrim;
        batch.StartIndex = startInd;
        batch.BaseIndex = baseInd;
        batch.MaterialIndex = matInd;
        
        return 0;
    }
    
    // add attrib(state, type, bufferIndex, format)
    static U32 luaAddVertexAttrib(LuaManager& man)
    {
        TTE_ASSERT(man.GetTop() == 4, "Requires 4 arguments");
        Mesh::MeshInstance* t = Task(man);
        TTE_ASSERT(t->VertexStates.size() && t->VertexStates.back().Default.NumVertexAttribs != 32,"Too many attributes!");
        
        auto& state = t->VertexStates.back().Default;
        state.Attribs[state.NumVertexAttribs].Attrib = (RenderAttributeType)man.ToInteger(2);
        state.Attribs[state.NumVertexAttribs].VertexBufferIndex = man.ToInteger(3);
        state.Attribs[state.NumVertexAttribs].Format = (RenderBufferAttributeFormat)man.ToInteger(4);
        state.NumVertexAttribs++;
        
        return 0;
    }
    
    // push mat(diffuse texture symbol)
    static U32 luaPushMaterial(LuaManager& man)
    {
        TTE_ASSERT(man.GetTop() == 2, "Requires 2 arguments");
        Mesh::MeshInstance* t = Task(man);
        Symbol diffuse = ScriptManager::ToSymbol(man, 2);
        Mesh::MeshMaterial material{};
        material.DiffuseTexture.SetObject(diffuse);
        t->Materials.push_back(std::move(material));
        return 0;
    }
    
    // resolve (state, buffer, palette table, vertIndexTab, fourBoneSkinning, indexDivisor, meshName)
    // palette table is table of tables. paletteTable[batch/whatever index] = table_u32_array[actual serialised buffer data index => bone name]
    // vertex index table maps 'whatever indices' (above) to an actual vertex buffer range. Each value in table must be have Min and Max keys as int indices
    // four bone skinning is a bool. true for 4 bone influence, else 3.
    // index divisor is what each bone index is divided by. used for old vec4 indices for each row (into matrix, div4) for old games. use 1 for normal.
    // ONLY ACCEPTS UBYTEx4 (ie 1x uint per vert!)
    static U32 luaResolveBonePalettes(LuaManager& man)
    {
        Memory::FastBufferAllocator allocator{};
        TTE_ASSERT(man.GetTop() == 7, "Incorrect usage");
        Mesh::MeshInstance* t = Task(man);
        Meta::ClassInstance buf = Meta::AcquireScriptInstance(man, 2);
        if(!buf)
        {
            TTE_ASSERT(false, "Vertex buffer not found");
            return 0;
        }
        Meta::BinaryBuffer* pBuffer = (Meta::BinaryBuffer*)buf._GetInternal();
        U8* pBoneIndices = pBuffer->BufferData.get();
        U8* pNewIndices = allocator.Alloc(pBuffer->BufferSize, 8);
        memset(pNewIndices, 0xFF, pBuffer->BufferSize);
        Bool bFourSkinning = man.ToBool(5);
        U32 divisor = (U32)man.ToInteger(6);
        TTE_ASSERT(divisor > 0, "Divisor invalid");
        
        // mesh name determines skeleton file name. in future if its different can find .prop of meshName and use Skeleton File key for skeleton file
        String skeleton = FileSetExtension(man.ToString(7), "skl");
        Ptr<ResourceRegistry> pRegistry = t->GetRegistry();
        Handle<Skeleton> hSkeleton = pRegistry->MakeHandle<Skeleton>(skeleton, true);
        Ptr<Skeleton> pSkeleton = hSkeleton.GetObject(pRegistry, true);
        TTE_ASSERT(pSkeleton, "Skeleton file %s was not found or could not be loaded when normalising its mesh file! This file is required "
                   "to be able to load meshes with a skeleton attachment!", skeleton.c_str());
        
        man.PushNil();
        while(man.TableNext(4))
        {
            ScriptManager::TableGet(man, "Min");
            U32 minIndex = ScriptManager::PopUnsignedInteger(man);
            ScriptManager::TableGet(man, "Max");
            U32 maxIndex = ScriptManager::PopUnsignedInteger(man);
            TTE_ASSERT(maxIndex >= minIndex, "Invalid indices range");
            man.Pop(1); // pop table
            man.PushCopy(-1); // dup top as will be erased in gettable
            man.GetTable(3); // key, batch index is at top. get from palette table
            man.PushNil();
            while(man.TableNext(-2))
            {
                U32 serialisedIndex = (U32)man.ToInteger(-2); // key
                String skeletonBone = ScriptManager::PopString(man);
                U32 skeletonIndex = 0;
                for(const auto& sklEntry: pSkeleton->GetEntries())
                {
                    if(CompareCaseInsensitive(sklEntry.JointName, skeletonBone))
                    {
                        break;
                    }
                    skeletonIndex++;
                }
                TTE_ASSERT(skeletonIndex < pSkeleton->GetEntries().size(), "Mesh bone palette entry '%s' was not found in the skeleton!", skeletonBone.c_str());
                TTE_ASSERT(skeletonIndex < 255 && serialisedIndex < 255, "Indices invalid"); // COULD change to 256, doubt ever get close to that many. 0xFF reserved here.
                for(U32 i = minIndex; i <= maxIndex; i++)
                {
                    U32 bufIndex = i << 2;
                    for(U32 j = 0; j < (bFourSkinning?4:3); j++)
                    {
                        if((pBoneIndices[bufIndex] / divisor) == serialisedIndex)
                        {
                            pNewIndices[bufIndex] = skeletonIndex;
                        }
                        bufIndex++;
                    }
                }
            }
            man.Pop(1); // pop key table
        }
        for(U32 i = 0; i < pBuffer->BufferSize; i++)
        {
            if(!bFourSkinning && ((i&3) == 3))
                pNewIndices[i] = 0;
            TTE_ASSERT(pNewIndices[i] != 0xFF, "When resolving bone indices, at vertex %d the bone indices were not all resolved!", i >> 2);
        }
        memcpy(pBoneIndices, pNewIndices, pBuffer->BufferSize);
        return 0;
    }
    
};

Sphere Mesh::CreateSphereForBox(BoundingBox bb)
{
    Vector3 center = (bb._Min + bb._Max) * 0.5f;
    Float radius = bb._Min.Distance(bb._Max) * 0.5f;
    return Sphere{center, radius};
}

// register all lua normalisation and specialisation API
void Mesh::RegisterScriptAPI(LuaFunctionCollection &Col)
{
    PUSH_FUNC(Col, "CommonMeshSetName", &MeshAPI::luaSetName, "nil CommonMeshSetName(state, name)", "Set the common mesh name");
    PUSH_FUNC(Col, "CommonMeshSetDeformable", &MeshAPI::luaSetDeformable, "nil CommonMeshSetDeformable(state, deformable)", "Set whether the mesh is deformable or not");
    PUSH_FUNC(Col, "CommonMeshPushVertexBuffer", &MeshAPI::luaSetNextVertexBuffer, "nil CommonMeshPushVertexBuffer(state, numVerts, vertexStride, binaryBuffer)",
              "Push a new vertex buffer to the common mesh");
    PUSH_FUNC(Col, "CommonMeshSetIndexBuffer", &MeshAPI::luaSetIndexBuffer
              , "nil CommonMeshSetIndexBuffer(state, numIndices, formatIsUnsignedShort, binaryBuffer", "Set the common mesh's index buffer");
    PUSH_FUNC(Col, "CommonMeshAdvanceVertexState", &MeshAPI::luaAdvanceVertexState, "nil CommonMeshAdvanceVertexState(state)", "Sequential attributes and buffers"
              " will be assigned to this new vertex state.");
    PUSH_FUNC(Col, "CommonMeshDecompressVertices", &MeshAPI::luaDecompressVertices,
              "nil CommonMeshDecompressVertices(state, binaryBuffer, numVerts, compressedType)", "Decompress vertices from a telltale mesh");
    PUSH_FUNC(Col, "CommonMeshPushLOD", &MeshAPI::luaPushLevelOfDetail, "nil CommonMeshPushLOD(state, vertexStateIndex)",
              "Push a level-of-detail to the given vertex state index. All sequential calls will append to this LOD.");
    PUSH_FUNC(Col, "CommonMeshSetLODBounds", &MeshAPI::luaSetLODBounds,
              "nil CommonMeshSetLODBounds(state, boundingBoxInst, [optional] boundingSphereInst)", "Set current LOD bounding information");
    PUSH_FUNC(Col, "CommonMeshPushBatch", &MeshAPI::luaPushBatch, "nil CommonMeshPushBatch(state, isShadowBatch)",
              "Push a mesh batch. Specify if its a shadow batch.");
    PUSH_FUNC(Col, "CommonMeshSetBatchBounds", &MeshAPI::luaBatchSetBounds,
              "nil CommonMeshSetBatchBounds(state, isShadowBatch, boundingBoxInst, [optional] boundingSphereInst)", "Set current batch bounding information");
    PUSH_FUNC(Col, "CommonMeshSetBatchParameters", &MeshAPI::luaSetBatchParameters,
              "nil CommonMeshSetBatchParameters(state, bIsShadowBatch, minVert, maxVert, startIndexBufferIndex, numPrimitives, numIndices, baseIndex, matIndex)",
              "Set batch parameter information");
    PUSH_FUNC(Col, "CommonMeshAddVertexAttribute", &MeshAPI::luaAddVertexAttrib,
              "nil CommonMeshAddVertexAttribute(state, attribType, vertexBufferIndex, attribFormat)", "Adds a new vertex attribute to the current vertex state");
    PUSH_FUNC(Col, "CommonMeshPushMaterial", &MeshAPI::luaPushMaterial,
              "nil CommonMeshPushMaterial(state, diffuseTexName)", "Push material information to the mesh");
    PUSH_FUNC(Col, "CommonMeshResolveBonePalettes", &MeshAPI::luaResolveBonePalettes,
              "nil CommonMeshResolveBonePalettes(state, buffer, palette table, vertIndexTab, fourBoneSkinning, indexDivisor, meshName)",
              "Resolve legacy mesh bone palettes. This will modify the bone indices buffer given. "
              "Palette table is table of tables. paletteTable[batch/whatever index] = table_u32_array[actual serialised buffer data index => bone name]"
              ". Vertex index table maps 'whatever indices' (above) to an actual vertex buffer range. Each value in table must be have Min and Max keys as int indices"
              " Four bone skinning is a bool: true for 4 bone influence, else 3."
              " Index divisor is what each bone index is divided by. Used for old Vec4 indices for each row (into matrix, div 4) for old games. Use 1 to ignore."
              " This function only accepts bone index buffer in the format of 4x ubytes! (or float unorm bytes x4)");
    
    // U16 => two Unorm floats. x and y 8 bits each normalised to 0.0 to 1.0
    PUSH_GLOBAL_I(Col, "kCommonMeshCompressedFormatUNormUV", 0, "Mesh compressed format involving unsigned normed UV values");
    // U16 => three Snorm floats, third determined by sqrt. x and y have 7 bits each, normed to -1 to 1. MSB unused. bit 15 of 16 is sign of z.
    PUSH_GLOBAL_I(Col, "kCommonMeshCompressedFormatSNormNormal", 1, "Mesh compressed format involving signed normed normal vector3 values");
    // U16 => three Unorm floats, third determined by 1 - x - y (no square, approx). x and y 8 bits each. normalised to 0.0 to 1.0
    PUSH_GLOBAL_I(Col, "kCommonMeshCompressedFormatUNormNormalAprox", 2, "Mesh compressed format involving unsigned approximated normed normal vector3 values");
    
}

void Mesh::AddMesh(Ptr<ResourceRegistry>& registry, Handle<Mesh::MeshInstance> handle)
{
    MeshList.push_back(handle.GetObject(registry, true));
}

// finish async normalisation, doing any stuff which wasnt set from lua
void Mesh::MeshInstance::FinaliseNormalisationAsync()
{
    // Iterate over all LODs
    for (LODInstance& lod : LODs)
    {
        if(lod.BBox.Volume() == 0.0f)
        {
            BoundingBox b{};
            for (MeshBatch& batch : lod.Batches[RenderViewType::DEFAULT]) // use default view and not shadow
            {
                b = b.Merge(batch.BBox);
            }
            lod.BBox = b;
            lod.BSphere = CreateSphereForBox(lod.BBox);
        }
        else
        {
            for (MeshBatch& batch : lod.Batches[RenderViewType::DEFAULT]) // use default view and not shadow
            {
                // If the batch has an empty bounding box, inherit from LOD
                if (batch.BBox.Volume() == 0.0f)
                {
                    batch.BBox = lod.BBox;
                    batch.BSphere = lod.BSphere;
                }
            }
        }
    }
    
    // If the MeshInstance has no bounding box, merge from LODs
    if (BBox.Volume() == 0.0f && !LODs.empty())
    {
        for (const LODInstance& lod : LODs)
        {
            if (lod.BBox.Volume() > 0.0f)
            {
                BBox = BBox.Merge(lod.BBox);
            }
        }
        BSphere = CreateSphereForBox(BBox);
    }
}

