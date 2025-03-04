#include <Common/Mesh.hpp>
#include <EditorTasks.hpp>

// NORMALISATION AND SPECIALISATION OF MESH INTO COMMON FORMAT AND BACK
namespace MeshAPI
{
	
	static inline MeshNormalisationTask* Task(LuaManager& man)
	{
		return (MeshNormalisationTask*)man.ToPointer(1);
	}
	
	// setname(commonInst, name)
	static U32 luaSetName(LuaManager& man)
	{
		TTE_ASSERT(man.GetTop() == 2, "Requires 2 arguments");
		TTE_ASSERT(man.Type(2) == LuaType::STRING, "Invalid usage");
		MeshNormalisationTask* t = Task(man);
		t->Renderable.Name = man.ToString(2);
		return 0;
	}
	
	// setdeformable(commonInst)
	static U32 luaSetDeformable(LuaManager& man)
	{
		TTE_ASSERT(man.GetTop() == 1, "Requires 1 arguments");
		MeshNormalisationTask* t = Task(man);
		t->Renderable.Flags += Mesh::FLAG_DEFORMABLE;
		return 0;
	}
	
	// setvertexbuffer(commonInst, numverts, stride, buffer)
	static U32 luaSetNextVertexBuffer(LuaManager& man)
	{
		TTE_ASSERT(man.GetTop() == 4, "Requires 4 arguments");
		MeshNormalisationTask* t = Task(man);
		
		if(t->Renderable.VertexStates.size() == 0)
			t->Renderable.VertexStates.emplace_back();
		
		U32 nVerts = (U32)man.ToInteger(2);
		U32 stride = (U32)man.ToInteger(3);
		
		Meta::ClassInstance buf = Meta::AcquireScriptInstance(man, 4);
		
		if(!buf)
		{
			TTE_ASSERT(false, "Vertex buffer not found");
			return 0;
		}
		
		Meta::BinaryBuffer* pBuffer = (Meta::BinaryBuffer*)buf._GetInternal();
		
		auto& vertexState = t->Renderable.VertexStates.back();
		
		TTE_ASSERT(vertexState.Default.NumVertexBuffers != 32, "Too many vertex buffers pushed!");
		vertexState.Default.BufferPitches[vertexState.Default.NumVertexBuffers] = stride;
		vertexState.VertexBuffers[vertexState.Default.NumVertexBuffers++] = *pBuffer;
		
		return 0;
	}
	
	// setvertexbuffer(commonInst, numindices, isHalf, buffer)
	static U32 luaSetIndexBuffer(LuaManager& man)
	{
		TTE_ASSERT(man.GetTop() == 4, "Requires 4 arguments");
		MeshNormalisationTask* t = Task(man);
		
		if(t->Renderable.VertexStates.size() == 0)
			t->Renderable.VertexStates.emplace_back();
		
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
		
		auto& vertexState = t->Renderable.VertexStates.back();
		
		TTE_ASSERT(vertexState.IndexBuffer.BufferSize == 0, "Index buffer already set");
		vertexState.IndexBuffer = *pBuffer;
		
		return 0;
	}
	
	//advance to next vertex state
	static U32 luaAdvanceVertexState(LuaManager& man)
	{
		TTE_ASSERT(man.GetTop() == 1, "Requires 1 argument");
		MeshNormalisationTask* t = Task(man);
		t->Renderable.VertexStates.emplace_back();
		return 0;
	}
	
	// perform decompress of oldest vertices in current vertex buffer
	// NOTE THAT THIS WILL UPDATE THE META CLASS BUFFER VALUE. SET COMPRESSED TO FALSE IN THE SOURCE CLASS TO ENSURE CONSISTENCY
	static U32 luaDecompressLegacyVertices(LuaManager& man)
	{
		TTE_ASSERT(man.GetTop() == 1, "Requires 1 argument");
		MeshNormalisationTask* t = Task(man);
		TTE_ASSERT(t->Renderable.VertexStates.size() && t->Renderable.VertexStates.back().Default.NumVertexBuffers, "No vertex buffers set yet");
		
		TTE_ASSERT(false, "Implement me!"); // todo
		
		return 0;
	}
	
	// pushlod(inst, vertexstateindex)
	static U32 luaPushLevelOfDetail(LuaManager& man)
	{
		TTE_ASSERT(man.GetTop() == 2, "Requires 2 arguments");
		
		MeshNormalisationTask* t = Task(man);
		U32 sindex = (U32)man.ToInteger(2);
		
		t->Renderable.LODs.emplace_back();
		t->Renderable.LODs.back().VertexStateIndex = sindex;
		
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
		
		MeshNormalisationTask* t = Task(man);
		
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
		
		t->Renderable.LODs.back().BBox = box;
		t->Renderable.LODs.back().BSphere = bsph;
			
		return 0;
	}
	
	// FOR DOCS: note that this will set bounds to parent LOD bounds if not set, so set them before calling any batch pushed (can override after)
	// pushbatch(inst, bShadow) - todo i know that twd michonne only exception has its own batch array?!
	static U32 luaPushBatch(LuaManager& man)
	{
		TTE_ASSERT(man.GetTop() == 2, "Requires 2 arguments");
		MeshNormalisationTask* t = Task(man);
		TTE_ASSERT(t->Renderable.LODs.size(), "No LODs");
		
		U32 batchIndex = man.ToBool(2) ? 1 : 0;
		
		t->Renderable.LODs.back().Batches[batchIndex].emplace_back();
		
		auto& batch = t->Renderable.LODs.back().Batches[batchIndex].back();
		
		batch.BBox = t->Renderable.LODs.back().BBox; // default bounds
		batch.BSphere = t->Renderable.LODs.back().BSphere;
		
		return 0;
	}
	
	// bounds(inst, bShadow, meta bb, optional meta bsphere)
	static U32 luaBatchSetBounds(LuaManager& man)
	{
		TTE_ASSERT(man.GetTop() == 3 || man.GetTop() == 4, "Required 3/4 arguments");
		
		MeshNormalisationTask* t = Task(man);
		TTE_ASSERT(t->Renderable.LODs.size(), "No LODs");
		
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
		
		t->Renderable.LODs.back().Batches[batchIndex].back().BBox = box;
		t->Renderable.LODs.back().Batches[batchIndex].back().BSphere = bsph;
		
		return 0;
	}
	
	// base index?
	// params(inst, bShadow, min vert, max vert, start index buffer index, num primitives, num indices, baseindex ???)
	static U32 luaSetBatchParameters(LuaManager& man)
	{
		TTE_ASSERT(man.GetTop() == 8, "Required 8 arguments");
		
		MeshNormalisationTask* t = Task(man);
		
		U32 batchType = man.ToBool(2) ? 1 : 0;
		U32 minVert = (U32)man.ToInteger(3);
		U32 maxVert = (U32)man.ToInteger(4);
		U32 startInd = (U32)man.ToInteger(5);
		U32 numPrim = (U32)man.ToInteger(6);
		U32 numInd = (U32)man.ToInteger(7);
		U32 baseInd = (U32)man.ToInteger(8);
		
		TTE_ASSERT(baseInd == 0, "Base index implementation needed!"); // wtf is this
		
		TTE_ASSERT(t->Renderable.LODs.size() && t->Renderable.LODs.back().Batches[batchType].size(), "No LODs/batches");
		
		auto& batch = t->Renderable.LODs.back().Batches[batchType].back();
		batch.MinVertIndex = minVert;
		batch.MaxVertIndex = maxVert;
		batch.NumIndices = numInd;
		batch.NumPrimitives = numPrim;
		batch.StartIndex = startInd;
		batch.BaseIndex = baseInd;
		
		return 0;
	}
	
	// add attrib(state, type, bufferIndex, attribCounter, format)
	static U32 luaAddVertexAttrib(LuaManager& man)
	{
		TTE_ASSERT(man.GetTop() == 5, "Requires 5 arguments");
		MeshNormalisationTask* t = Task(man);
		TTE_ASSERT(t->Renderable.VertexStates.size() && t->Renderable.VertexStates.back().Default.NumVertexAttribs != 32,"Too many attributes!");
		
		auto& state = t->Renderable.VertexStates.back().Default;
		state.Attribs[state.NumVertexAttribs].Attrib = (RenderAttributeType)man.ToInteger(2);
		state.Attribs[state.NumVertexAttribs].VertexBufferIndex = man.ToInteger(3);
		state.Attribs[state.NumVertexAttribs].VertexBufferLocation = man.ToInteger(4);
		state.Attribs[state.NumVertexAttribs].Format = (RenderBufferAttributeFormat)man.ToInteger(5);
		state.NumVertexAttribs++;
		
		return 0;
	}
	
}

Sphere Mesh::CreateSphereForBox(BoundingBox bb)
{
	Vector3 center = (bb._Min + bb._Max) * 0.5f;
	Float radius = bb._Min.Distance(bb._Max) * 0.5f;
	return Sphere{center, radius};
}

// register all lua normalisation and specialisation API
void Mesh::RegisterScriptAPI(LuaFunctionCollection &Col)
{
	PUSH_FUNC(Col, "CommonMeshSetName", &MeshAPI::luaSetName);
	PUSH_FUNC(Col, "CommonMeshSetDeformable", &MeshAPI::luaSetDeformable);
	PUSH_FUNC(Col, "CommonMeshPushVertexBuffer", &MeshAPI::luaSetNextVertexBuffer);
	PUSH_FUNC(Col, "CommonMeshSetIndexBuffer", &MeshAPI::luaSetIndexBuffer);
	PUSH_FUNC(Col, "CommonMeshAdvanceVertexState", &MeshAPI::luaAdvanceVertexState);
	PUSH_FUNC(Col, "CommonMeshDecompressLegacyVertices", &MeshAPI::luaDecompressLegacyVertices);
	PUSH_FUNC(Col, "CommonMeshPushLOD", &MeshAPI::luaPushLevelOfDetail);
	PUSH_FUNC(Col, "CommonMeshSetLODBounds", &MeshAPI::luaSetLODBounds);
	PUSH_FUNC(Col, "CommonMeshPushBatch", &MeshAPI::luaPushBatch);
	PUSH_FUNC(Col, "CommonMeshSetBatchBounds", &MeshAPI::luaBatchSetBounds);
	PUSH_FUNC(Col, "CommonMeshSetBatchParameters", &MeshAPI::luaSetBatchParameters);
	
	PUSH_FUNC(Col, "CommonMeshAddVertexAttribute", &MeshAPI::luaAddVertexAttrib);
	
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

