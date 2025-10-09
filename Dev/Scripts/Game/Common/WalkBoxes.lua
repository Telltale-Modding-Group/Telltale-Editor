-- WalkBoxes WBOX implementation

-- earliest versions
function RegisterWalkBoxes0(MetaFlags, MetaVec3, collectionRegistrar)

    local wboxEdge = NewClass("class WalkBoxes::Edge", 0) 
    -- there about a billion different vers file for each of these. only doing the one used (at least here)
	wboxEdge.Members[1] = NewMember("mV1", kMetaInt)
	wboxEdge.Members[2] = NewMember("mV2", kMetaInt)
	wboxEdge.Members[3] = NewMember("mEdgeDest", kMetaInt)
	wboxEdge.Members[4] = NewMember("mEdgeDestEdge", kMetaInt)
	wboxEdge.Members[5] = NewMember("mEdgeDir", kMetaInt)
	wboxEdge.Members[6] = NewMember("mMaxRadius", kMetaFloat)
	MetaRegisterClass(wboxEdge)

	local array3Int = NewClass("class SArray<int,3>", 0)
	MetaRegisterCollection(array3Int, 3, kMetaInt)

	local array4Int = NewClass("class SArray<int,4>", 0)
	MetaRegisterCollection(array4Int, 4, kMetaInt)

	local array3Float = NewClass("class SArray<float,3>", 0)
	MetaRegisterCollection(array3Float, 3, kMetaFloat)

	local array3Edge = NewClass("class SArray<class WalkBoxes::Edge,3>", 0)
	MetaRegisterCollection(array3Edge, 3, wboxEdge)

	local wboxTri = NewClass("class WalkBoxes::Tri", 0)
	wboxTri.Members[1] = NewMember("mFlags", MetaFlags)
	wboxTri.Members[2] = NewMember("mNormal", kMetaInt)
	wboxTri.Members[3] = NewMember("mQuadBuddy", kMetaInt)
	wboxTri.Members[4] = NewMember("mMaxRadius", kMetaFloat)
	wboxTri.Members[5] = NewMember("mVerts", array3Int)
	wboxTri.Members[6] = NewMember("mEdgeInfo", array3Edge)
	wboxTri.Members[7] = NewMember("mVertOffsets", array3Int)
	wboxTri.Members[8] = NewMember("mVertScales", array3Float)
	MetaRegisterClass(wboxTri)

	local wboxVert = NewClass("class WalkBoxes::Vert", 0)
	wboxVert.Members[1] = NewMember("mFlags", MetaFlags)
	wboxVert.Members[2] = NewMember("mPos", MetaVec3)
	MetaRegisterClass(wboxVert)

	local wboxQuad = NewClass("class WalkBoxes::Quad", 0)
	wboxQuad.Members[1] = NewMember("mVerts", array4Int)
	MetaRegisterClass(wboxQuad)

	local arrayVec3 = collectionRegistrar("class DCArray<class Vector3>", nil, MetaVec3)
	local arrayQuad = collectionRegistrar("class DCArray<class WalkBoxes::Quad>", nil, wboxQuad)
	local arrayVert = collectionRegistrar("class DCArray<class WalkBoxes::Vert>", nil, wboxVert)
	local arrayTri = collectionRegistrar("class DCArray<class WalkBoxes::Tri>", nil, wboxTri)

	local wbox = NewClass("class WalkBoxes", 0)
	wbox.Extension = "wbox"
	wbox.Members[1] = NewMember("mName", kMetaClassString)
	wbox.Members[2] = NewMember("mTris", arrayTri)
	wbox.Members[3] = NewMember("mVerts", arrayVert)
	wbox.Members[4] = NewMember("mNormals", arrayVec3)
	wbox.Members[5] = NewMember("mQuads", arrayQuad)
	MetaRegisterClass(wbox)

    return wbox
end