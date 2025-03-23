#include <Renderer/RenderContext.hpp>

#include <Core/Math.hpp>

#include <array>

static DataStreamRef Stream(U64 sz)
{
    return DataStreamManager::GetInstance()->CreateBufferStream({}, sz, nullptr, false);
}

void RegisterDefaultTextures(RenderContext& context, RenderCommandBuffer* upload, std::vector<DefaultRenderTexture>& textures)
{
    upload->StartCopyPass();
    
    // BLACK TEXTURE
    DefaultRenderTexture tex{};
    tex.Type = DefaultRenderTextureType::BLACK;
    tex.Texture = context.AllocateTexture();
    tex.Texture->Create2D(&context, 1, 1, RenderSurfaceFormat::RGBA8, 1);
    DataStreamRef texStream = Stream(4);
    texStream->Write((const U8*)"\x00\x00\x00\xFF", 4);
    upload->UploadTextureDataSlow(tex.Texture, std::move(texStream), 0, 0, 0, 4);
    textures.push_back(std::move(tex));
    
    // WHITE TEXTURE
    tex.Type = DefaultRenderTextureType::WHITE;
    tex.Texture = context.AllocateTexture();
    tex.Texture->Create2D(&context, 1, 1, RenderSurfaceFormat::RGBA8, 1);
    texStream = Stream(4);
    texStream->Write((const U8*)"\xFF\xFF\xFF\xFF", 4);
    upload->UploadTextureDataSlow(tex.Texture, std::move(texStream), 0, 0, 0, 4);
    textures.push_back(std::move(tex));
    
    upload->EndPass();
}

struct SampledVertex
{
    float x,y,z,u,v;
};

struct ColouredVertex
{
    float x,y,z;
};

static void _ColouredMesh(RenderContext& context, DefaultRenderMesh& mesh, DefaultRenderMeshType type, Bool bLines = false)
{
    mesh.Type = type;
    mesh.PipelineState = context._AllocatePipelineState();
    mesh.PipelineState->PrimitiveType = bLines ? RenderPrimitiveType::LINE_LIST : RenderPrimitiveType::TRIANGLE_LIST;
    mesh.PipelineState->ShaderProgram = "Flat";
    mesh.PipelineState->VertexState.BufferPitches[0] = sizeof(ColouredVertex);
    mesh.PipelineState->VertexState.NumVertexBuffers = 1;
    mesh.PipelineState->VertexState.NumVertexAttribs = 1;
    mesh.PipelineState->VertexState.Attribs[0] = {RenderAttributeType::POSITION, RenderBufferAttributeFormat::F32x3, 0};
    mesh.PipelineState->Create();
}

static void _GenerateSemicircle(Vector3 p1, Vector3 p2, Vector3 normal, U32 steps, std::vector<Vector3>& result) {
    
    Vector3 midpoint = (p1 + p2) * 0.5f;  // Midpoint of the segment
    Vector3 dir = Vector3::Normalize(p2 - p1);     // Direction vector
    
    Vector3 diff = p2 - p1;
    Float radius = sqrtf(Vector3::Dot(diff, diff)) * 0.5f;
    
    for (U32 i = 0; i <= steps; ++i) {
	    Float angle = PI_F * (Float(i) / Float(steps)); // Angle from 0 to pi
	    Float s = std::sin(angle);
	    Float c = std::cos(angle);
	    
	    Vector3 point = midpoint + normal * (s * radius) + dir * (c * radius);
	    result.push_back(point);
    }
}

void RegisterDefaultMeshes(RenderContext& context, RenderCommandBuffer* upload, std::vector<DefaultRenderMesh>& meshes)
{
    upload->StartCopyPass();
    
    // SIMPLE QUAD
    DefaultRenderMesh mesh{};
    DataStreamRef verts{};
    DataStreamRef inds{};
    SampledVertex vertices[4] = {
	    {-0.5f,1.0f,0.0f,0.0f,0.0f},
	    {0.5f,1.0f,0.0f,1.0f,0.0f},
	    {1.0f,-1.0f,0.0f,1.0f,1.0f},
	    {-1.0f,-1.0f,0.0f,0.0f,1.0f}};
    U16 indices[6] = {0,1,2,0,2,3};
    
    {
	    mesh.Type = DefaultRenderMeshType::QUAD;
	    mesh.PipelineState = context._AllocatePipelineState();
	    mesh.PipelineState->ShaderProgram = "Simple"; // sampled vertex shader (ie xyz and uv with camera simple)
	    mesh.PipelineState->VertexState.BufferPitches[0] = sizeof(SampledVertex);
	    mesh.PipelineState->VertexState.NumVertexBuffers = 1;
	    mesh.PipelineState->VertexState.NumVertexAttribs = 2;
	    mesh.PipelineState->VertexState.Attribs[0] = {RenderAttributeType::POSITION, RenderBufferAttributeFormat::F32x3, 0};
	    mesh.PipelineState->VertexState.Attribs[1] = {RenderAttributeType::UV_DIFFUSE, RenderBufferAttributeFormat::F32x2, 0};
	    mesh.PipelineState->Create();
	    mesh.NumIndices = 6;
	    mesh.VertexBuffer = context.CreateVertexBuffer(sizeof(SampledVertex) * 4);
	    mesh.IndexBuffer = context.CreateIndexBuffer(sizeof(U16) * 6);
	    verts = Stream(sizeof(SampledVertex) * 4);
	    inds = Stream((sizeof(U16) * 6));
	    verts->Write((const U8*)vertices, sizeof(SampledVertex) * 4);
	    inds->Write((const U8*)indices, sizeof(U16) * 6);
	    upload->UploadBufferDataSlow(mesh.VertexBuffer, std::move(verts), 0, 0, (sizeof(SampledVertex) * 4));
	    upload->UploadBufferDataSlow(mesh.IndexBuffer, std::move(inds), 0, 0, (sizeof(U16) * 6));
	    meshes.push_back(std::move(mesh));
    }
    
    // FULL SPHERE
    {
	    _ColouredMesh(context, mesh, DefaultRenderMeshType::FILLED_SPHERE);
	    
	    U32 SphereSlices = 64;
	    U32 SphereStacks = 64;
	    
	    verts = DataStreamManager::GetInstance()->CreateBufferStream({}, (SphereSlices+1)*(SphereStacks+1)*sizeof(ColouredVertex), 0, 0);
	    inds = DataStreamManager::GetInstance()->CreateBufferStream({}, 6*sizeof(U16)*SphereSlices*SphereStacks, 0, 0);
	    
	    for (U32 i = 0; i <= SphereStacks; ++i)
	    {
    	    Float lat = 3.14159265f * (-0.5f + static_cast<float>(i) / SphereStacks);
    	    Float sinLat = sinf(lat);
    	    Float cosLat = cosf(lat);
    	    
    	    for (U32 j = 0; j <= SphereSlices; ++j)
    	    {
	    	    Float lon = 2 * 3.14159265f * static_cast<float>(j) / SphereSlices;
	    	    Float sinLon = sinf(lon);
	    	    Float cosLon = cosf(lon);
	    	    
	    	    Float x = cosLon * cosLat;
	    	    Float y = sinLat;
	    	    Float z = sinLon * cosLat;
	    	    
	    	    ColouredVertex q{x,y,z};
	    	    verts->Write((const U8*)&q, sizeof(ColouredVertex));
	    	    
	    	    if(i != SphereStacks && j != SphereSlices)
	    	    {
    	    	    // add indices
    	    	    U16 first = (U16)(i * (SphereSlices + 1) + j);
    	    	    U16 second = (U16)(first + SphereSlices + 1);
    	    	    
    	    	    indices[0] = first;
    	    	    indices[1] = second;
    	    	    indices[2] = (U16)(first + 1);
    	    	    
    	    	    indices[3] = second;
    	    	    indices[4] = (U16)(second + 1);
    	    	    indices[5] = (U16)(first + 1);
    	    	    inds->Write((const U8*)indices, sizeof(U16) * 6);
	    	    }
	    	    
    	    }
	    }
	    
	    mesh.NumIndices = 6*SphereSlices*SphereStacks;
	    mesh.IndexBuffer = context.CreateIndexBuffer(inds->GetSize());
	    mesh.VertexBuffer = context.CreateVertexBuffer(verts->GetSize());
	    U32 vertsSize = (U32)verts->GetSize();
	    U32 indsSize = (U32)inds->GetSize();
	    upload->UploadBufferDataSlow(mesh.IndexBuffer, std::move(inds), 0, 0, indsSize);
	    upload->UploadBufferDataSlow(mesh.VertexBuffer, std::move(verts), 0, 0, vertsSize);
	    meshes.push_back(std::move(mesh));
    }
    
    // Modify the sphere rendering for wireframe view
    {
	    _ColouredMesh(context, mesh, DefaultRenderMeshType::WIREFRAME_SPHERE, true);
	    
	    U32 SphereSlices = 12;
	    U32 SphereStacks = 12;
	    
	    verts = DataStreamManager::GetInstance()->CreateBufferStream({}, (SphereSlices * (SphereStacks + 1) + SphereStacks * (SphereSlices + 1)) * sizeof(ColouredVertex), 0, 0);
	    inds = DataStreamManager::GetInstance()->CreateBufferStream({}, 2 * (6 * SphereSlices * SphereStacks) * sizeof(U16), 0, 0);
	    
	    // Vertices generation for latitude lines
	    for (U32 i = 0; i <= SphereStacks; ++i)
	    {
    	    Float lat = 3.14159265f * (-0.5f + static_cast<float>(i) / SphereStacks);
    	    Float sinLat = sinf(lat);
    	    Float cosLat = cosf(lat);
    	    
    	    for (U32 j = 0; j < SphereSlices; ++j)
    	    {
	    	    Float lon = 2 * 3.14159265f * static_cast<float>(j) / SphereSlices;
	    	    Float sinLon = sinf(lon);
	    	    Float cosLon = cosf(lon);
	    	    
	    	    Float x = cosLon * cosLat;
	    	    Float y = sinLat;
	    	    Float z = sinLon * cosLat;
	    	    
	    	    ColouredVertex q{x, y, z};
	    	    verts->Write((const U8*)&q, sizeof(ColouredVertex));
    	    }
	    }
	    
	    // Vertices generation for longitude lines (vertical circles)
	    for (U32 j = 0; j < SphereSlices; ++j)
	    {
    	    Float lon = 2 * 3.14159265f * static_cast<float>(j) / SphereSlices;
    	    Float sinLon = sinf(lon);
    	    Float cosLon = cosf(lon);
    	    
    	    for (U32 i = 0; i <= SphereStacks; ++i)
    	    {
	    	    Float lat = 3.14159265f * (-0.5f + static_cast<float>(i) / SphereStacks);
	    	    Float sinLat = sinf(lat);
	    	    Float cosLat = cosf(lat);
	    	    
	    	    Float x = cosLon * cosLat;
	    	    Float y = sinLat;
	    	    Float z = sinLon * cosLat;
	    	    
	    	    ColouredVertex q{x, y, z};
	    	    verts->Write((const U8*)&q, sizeof(ColouredVertex));
    	    }
	    }
	    
	    // Indices for latitude rings (connecting adjacent latitudes)
	    for (U32 i = 0; i < SphereStacks; ++i)
	    {
    	    for (U32 j = 0; j < SphereSlices; ++j)
    	    {
	    	    U16 first = (U16)(i * SphereSlices + j);
	    	    U16 second = (U16)(first + 1);
	    	    
	    	    if (j == SphereSlices - 1) second = (U16)(i * SphereSlices);
	    	    
	    	    // Horizontal wireframe (latitudes)
	    	    indices[0] = first;
	    	    indices[1] = second;
	    	    inds->Write((const U8*)indices, sizeof(U16) * 2);
    	    }
	    }
	    
	    // Indices for longitude rings (connecting adjacent longitudes)
	    for (U32 j = 0; j < SphereSlices; ++j)
	    {
    	    for (U32 i = 0; i < SphereStacks; ++i)
    	    {
	    	    U16 first = (U16)(i * SphereSlices + j);
	    	    U16 second = (U16)((i + 1) * SphereSlices + j);
	    	    
	    	    // Vertical wireframe (longitudes)
	    	    indices[0] = first;
	    	    indices[1] = second;
	    	    inds->Write((const U8*)indices, sizeof(U16) * 2);
    	    }
	    }
	    
	    mesh.NumIndices = 4 * SphereSlices * SphereStacks;
	    mesh.IndexBuffer = context.CreateIndexBuffer(inds->GetSize());
	    mesh.VertexBuffer = context.CreateVertexBuffer(verts->GetSize());
	    U32 vertsSize = (U32)verts->GetSize();
	    U32 indsSize = (U32)inds->GetSize();
	    upload->UploadBufferDataSlow(mesh.IndexBuffer, std::move(inds), 0, 0, indsSize);
	    upload->UploadBufferDataSlow(mesh.VertexBuffer, std::move(verts), 0, 0, vertsSize);
	    meshes.push_back(std::move(mesh));
    }
    
    // WIREFRAME BOX UNIT SIZE
    {
	    _ColouredMesh(context, mesh, DefaultRenderMeshType::WIREFRAME_BOX, true);
	    
	    // box verts
	    std::array<Vector3, 8> boxVertices = {{
    	    Vector3(-1, -1, -1), Vector3( 1, -1, -1), Vector3( 1,  1, -1), Vector3(-1,  1, -1),
    	    Vector3(-1, -1,  1), Vector3( 1, -1,  1), Vector3( 1,  1,  1), Vector3(-1,  1,  1)
	    }};
	    
	    // box indices
	    std::array<U16, 24> boxIndices = {{
    	    0, 1, 1, 2, 2, 3, 3, 0,  // Bottom square
    	    4, 5, 5, 6, 6, 7, 7, 4,  // Top square
    	    0, 4, 1, 5, 2, 6, 3, 7   // Vertical edges connecting top and bottom
	    }};
	    
	    // buffer
	    verts = DataStreamManager::GetInstance()->CreateBufferStream({}, boxVertices.size() * sizeof(ColouredVertex), 0, 0);
	    inds = DataStreamManager::GetInstance()->CreateBufferStream({}, boxIndices.size() * sizeof(U16), 0, 0);
	    
	    for (const auto& vertex : boxVertices)
	    {
    	    ColouredVertex q{vertex.x, vertex.y, vertex.z};
    	    verts->Write((const U8*)&q, sizeof(ColouredVertex));
	    }
	    
	    for (U32 i = 0; i < boxIndices.size(); i += 2)
	    {
    	    indices[0] = boxIndices[i];
    	    indices[1] = boxIndices[i + 1];
    	    inds->Write((const U8*)indices, sizeof(U16) * 2);
	    }
	    
	    mesh.NumIndices = (U32)boxIndices.size();
	    mesh.IndexBuffer = context.CreateIndexBuffer(inds->GetSize());
	    mesh.VertexBuffer = context.CreateVertexBuffer(verts->GetSize());
	    
	    U32 vertsSize = (U32)verts->GetSize();
	    U32 indsSize = (U32)inds->GetSize();
	    
	    upload->UploadBufferDataSlow(mesh.IndexBuffer, std::move(inds), 0, 0, indsSize);
	    upload->UploadBufferDataSlow(mesh.VertexBuffer, std::move(verts), 0, 0, vertsSize);
	    
	    meshes.push_back(std::move(mesh));
    }
    
    {
	    
	    // WIREFRAME CAPSULE UNIT SIZE
	    _ColouredMesh(context, mesh, DefaultRenderMeshType::WIREFRAME_CAPSULE, true);
	    
	    std::vector<Vector3> capsuleVertices{};
	    std::vector<U16> capsuleIndices{};
	    
	    // 1. two circles
	    for(U32 i = 0; i < 32; i++)
	    {
    	    Vector3 vert{};
    	    vert.x = 1.0 * cosf(3.14159f * 2 * (Float)i / 32.f);
    	    vert.z = 1.0 * sinf(3.14159f * 2 * (Float)i / 32.f);
    	    vert.y = -1.0f;
    	    capsuleVertices.push_back(vert);
    	    vert.y = 1.0f;
    	    capsuleVertices.push_back(vert);
    	    if(i!=31)
    	    {
	    	    capsuleIndices.push_back((U16)(i*2));
	    	    capsuleIndices.push_back((U16)(i*2+2));
	    	    capsuleIndices.push_back((U16)(i*2+1));
	    	    capsuleIndices.push_back((U16)(i*2+3));
    	    }
	    }
	    
	    // Close the loop (connect last to first)
	    capsuleIndices.push_back(62); // Bottom last (31*2)
	    capsuleIndices.push_back(0);  // Bottom first
	    capsuleIndices.push_back(63); // Top last (31*2+1)
	    capsuleIndices.push_back(1);  // Top first
	    
	    // 2. four connecting lines
	    capsuleIndices.push_back((U16)(0));
	    capsuleIndices.push_back((U16)(1));
	    capsuleIndices.push_back((U16)(30));
	    capsuleIndices.push_back((U16)(31));
	    
	    capsuleIndices.push_back((U16)(16));
	    capsuleIndices.push_back((U16)(17));
	    capsuleIndices.push_back((U16)(48));
	    capsuleIndices.push_back((U16)(49));
	    
	    // 3. semicircles
	    std::array<std::pair<U16, U16>, 4> startIndices = {{
    	    { (U16)0, (U16)30 },  // Semicircle 1
    	    { (U16)1, (U16)31 },  // Semicircle 2
    	    { (U16)16, (U16)48 }, // Semicircle 3
    	    { (U16)17, (U16)49 }  // Semicircle 4
	    }};
	    
	    for(U32 p = 0; p < 4; p++)
	    {
    	    U16 baseIndex = (U16)capsuleVertices.size();
    	    _GenerateSemicircle(capsuleVertices[startIndices[p].first], capsuleVertices[startIndices[p].second]
	    	    	    	    , p & 1 ? Vector3::Up : Vector3::Down, 16, capsuleVertices);
    	    for (U16 i = 0; i < 16; ++i) {
	    	    capsuleIndices.push_back(baseIndex + i);
	    	    capsuleIndices.push_back(baseIndex + i + 1);
    	    }
	    }
	    
	    verts = DataStreamManager::GetInstance()->CreateBufferStream({}, capsuleVertices.size() * sizeof(ColouredVertex), 0, 0);
	    inds = DataStreamManager::GetInstance()->CreateBufferStream({}, capsuleIndices.size() * sizeof(U16), 0, 0);
	    
	    for (const auto& vertex : capsuleVertices) {
    	    ColouredVertex q{vertex.x, vertex.y, vertex.z};
    	    verts->Write((const U8*)&q, sizeof(ColouredVertex));
	    }
	    
	    for (U32 i = 0; i < capsuleIndices.size(); i += 2) {
    	    indices[0] = capsuleIndices[i];
    	    indices[1] = capsuleIndices[i + 1];
    	    inds->Write((const U8*)indices, sizeof(U16) * 2);
	    }
	    
	    mesh.NumIndices = (U32)capsuleIndices.size();
	    mesh.IndexBuffer = context.CreateIndexBuffer(inds->GetSize());
	    mesh.VertexBuffer = context.CreateVertexBuffer(verts->GetSize());
	    
	    U32 vertsSize = (U32)verts->GetSize();
	    U32 indsSize = (U32)inds->GetSize();
	    
	    upload->UploadBufferDataSlow(mesh.IndexBuffer, std::move(inds), 0, 0, indsSize);
	    upload->UploadBufferDataSlow(mesh.VertexBuffer, std::move(verts), 0, 0, vertsSize);
	    
	    meshes.push_back(std::move(mesh));
	    
    }
    
    {
	    // FILLED BOX UNIT SIZE
	    _ColouredMesh(context, mesh, DefaultRenderMeshType::FILLED_BOX);
	    
	    std::array<Vector3, 8> fboxVertices = {{
    	    Vector3(-1, -1, -1), Vector3( 1, -1, -1), Vector3( 1,  1, -1), Vector3(-1,  1, -1), // Bottom
    	    Vector3(-1, -1,  1), Vector3( 1, -1,  1), Vector3( 1,  1,  1), Vector3(-1,  1,  1)  // Top
	    }};
	    
	    // Box indices for filled faces (triangles)
	    std::array<U16, 36> fboxIndices = {{
    	    // Bottom face
    	    0, 1, 2,  0, 2, 3,
    	    // Top face
    	    4, 6, 5,  4, 7, 6,
    	    // Front face
    	    0, 5, 1,  0, 4, 5,
    	    // Back face
    	    2, 7, 3,  2, 6, 7,
    	    // Left face
    	    0, 3, 7,  0, 7, 4,
    	    // Right face
    	    1, 6, 2,  1, 5, 6
	    }};
	    
	    // buffer
	    verts = DataStreamManager::GetInstance()->CreateBufferStream({}, fboxVertices.size() * sizeof(ColouredVertex), 0, 0);
	    inds = DataStreamManager::GetInstance()->CreateBufferStream({}, fboxIndices.size() * sizeof(U16), 0, 0);
	    
	    for (const auto& vertex : fboxVertices)
	    {
    	    ColouredVertex q{vertex.x, vertex.y, vertex.z};
    	    verts->Write((const U8*)&q, sizeof(ColouredVertex));
	    }
	    
	    for (U32 i = 0; i < fboxIndices.size(); i += 2)
	    {
    	    indices[0] = fboxIndices[i];
    	    indices[1] = fboxIndices[i + 1];
    	    inds->Write((const U8*)indices, sizeof(U16) * 2);
	    }
	    
	    mesh.NumIndices = (U32)fboxIndices.size();
	    mesh.IndexBuffer = context.CreateIndexBuffer(inds->GetSize());
	    mesh.VertexBuffer = context.CreateVertexBuffer(verts->GetSize());
	    
	    U32 vertsSize = (U32)verts->GetSize();
	    U32 indsSize = (U32)inds->GetSize();
	    
	    upload->UploadBufferDataSlow(mesh.IndexBuffer, std::move(inds), 0, 0, indsSize);
	    upload->UploadBufferDataSlow(mesh.VertexBuffer, std::move(verts), 0, 0, vertsSize);
	    
	    meshes.push_back(std::move(mesh));
    }
    
    // WIREFRAME CONE
    {
	    // WIREFRAME CONE UNIT SIZE
	    _ColouredMesh(context, mesh, DefaultRenderMeshType::WIREFRAME_CONE, true);
	    
	    std::vector<Vector3> coneVertices{};
	    std::vector<U16> coneIndices{};
	    
	    const U32 segments = 16;  // Number of base segments
    
	    // Generate base circle
	    for (U32 i = 0; i < segments; i++) {
    	    Vector3 vert{};
    	    vert.x = cosf(3.14159f * 2 * (Float)i / (Float)segments);
    	    vert.z = sinf(3.14159f * 2 * (Float)i / (Float)segments);
    	    vert.y = -1.0f; // Base of the cone
    	    coneVertices.push_back(vert);
	    }
	    
	    // Base center point
	    Vector3 baseCenter{};
	    baseCenter.x = 0.0f;
	    baseCenter.y = 1.0f;
	    baseCenter.z = 0.0f;
	    U16 baseCenterIndex = (U16)coneVertices.size();
	    coneVertices.push_back(baseCenter);
	    
	    // Apex point of the cone
	    Vector3 apex{};
	    apex.x = 0.0f;
	    apex.y = 1.0f; // Tip of the cone
	    apex.z = 0.0f;
	    U16 apexIndex = (U16)coneVertices.size();
	    coneVertices.push_back(apex);
	    
	    // Connect base vertices in a loop (wireframe base)
	    for (U32 i = 0; i < segments; i++) {
    	    coneIndices.push_back((U16)i);
    	    coneIndices.push_back((U16)((i + 1) % segments));
	    }
	    
	    // Connect base vertices to the apex (wireframe edges)
	    for (U32 i = 0; i < segments; i++) {
    	    coneIndices.push_back((U16)i);
    	    coneIndices.push_back(apexIndex);
	    }
	    
	    // Connect base vertices to the center of the base (radial lines)
	    for (U32 i = 0; i < segments>>1; i++) {
    	    coneIndices.push_back((U16)i);
    	    coneIndices.push_back((U16)((i + (segments>>1)) % segments));
	    }
	    
	    // Create buffers
	    verts = DataStreamManager::GetInstance()->CreateBufferStream({}, coneVertices.size() * sizeof(ColouredVertex), 0, 0);
	    inds = DataStreamManager::GetInstance()->CreateBufferStream({}, coneIndices.size() * sizeof(U16), 0, 0);
	    
	    // Write vertex data
	    for (const auto& vertex : coneVertices) {
    	    ColouredVertex q{vertex.x, vertex.y, vertex.z};
    	    verts->Write((const U8*)&q, sizeof(ColouredVertex));
	    }
	    
	    // Write index data
	    for (U32 i = 0; i < coneIndices.size(); i += 2) {
    	    indices[0] = coneIndices[i];
    	    indices[1] = coneIndices[i + 1];
    	    inds->Write((const U8*)indices, sizeof(U16) * 2);
	    }
	    
	    // Upload mesh data
	    mesh.NumIndices = (U32)coneIndices.size();
	    mesh.IndexBuffer = context.CreateIndexBuffer(inds->GetSize());
	    mesh.VertexBuffer = context.CreateVertexBuffer(verts->GetSize());
	    
	    U32 vertsSize = (U32)verts->GetSize();
	    U32 indsSize = (U32)inds->GetSize();
	    
	    upload->UploadBufferDataSlow(mesh.IndexBuffer, std::move(inds), 0, 0, indsSize);
	    upload->UploadBufferDataSlow(mesh.VertexBuffer, std::move(verts), 0, 0, vertsSize);
	    
	    meshes.push_back(std::move(mesh));

    }
    
    // FILLED CONE UNIT SIZE -1 TO 1
    {
	    _ColouredMesh(context, mesh, DefaultRenderMeshType::FILLED_CONE);
	    
	    std::vector<Vector3> coneVertices{};
	    std::vector<U16> coneIndices{};
	    
	    const U32 segments = 16;  // Number of base segments
	    
	    // Generate base circle
	    for (U32 i = 0; i < segments; i++) {
    	    Vector3 vert{};
    	    vert.x = cosf(3.14159f * 2 * (Float)i / (Float)segments);
    	    vert.z = sinf(3.14159f * 2 * (Float)i / (Float)segments);
    	    vert.y = -1.0f; // Base of the cone
    	    coneVertices.push_back(vert);
	    }
	    
	    // Base center point
	    Vector3 baseCenter{};
	    baseCenter.x = 0.0f;
	    baseCenter.y = -1.0f;
	    baseCenter.z = 0.0f;
	    U16 baseCenterIndex = (U16)coneVertices.size();
	    coneVertices.push_back(baseCenter);
	    
	    // Apex point of the cone
	    Vector3 apex{};
	    apex.x = 0.0f;
	    apex.y = 1.0f; // Tip of the cone
	    apex.z = 0.0f;
	    U16 apexIndex = (U16)coneVertices.size();
	    coneVertices.push_back(apex);
	    
	    // Create triangle fan for the base
	    for (U32 i = 0; i < segments; i++) {
    	    coneIndices.push_back(baseCenterIndex);
    	    coneIndices.push_back((U16)((i + 1) % segments));
    	    coneIndices.push_back((U16)i);
	    }
	    
	    // Create triangle fan for the sides
	    for (U32 i = 0; i < segments; i++) {
    	    coneIndices.push_back(apexIndex);
    	    coneIndices.push_back((U16)i);
    	    coneIndices.push_back((U16)((i + 1) % segments));
	    }
	    
	    // Create buffers
	    verts = DataStreamManager::GetInstance()->CreateBufferStream({}, coneVertices.size() * sizeof(ColouredVertex), 0, 0);
	    inds = DataStreamManager::GetInstance()->CreateBufferStream({}, coneIndices.size() * sizeof(U16), 0, 0);
	    
	    // Write vertex data
	    for (const auto& vertex : coneVertices) {
    	    ColouredVertex q{vertex.x, vertex.y, vertex.z};
    	    verts->Write((const U8*)&q, sizeof(ColouredVertex));
	    }
	    
	    // Write index data
	    for (U32 i = 0; i < coneIndices.size(); i += 3) {  // Changed step to 3 for triangles
    	    indices[0] = coneIndices[i];
    	    indices[1] = coneIndices[i + 1];
    	    indices[2] = coneIndices[i + 2];
    	    inds->Write((const U8*)indices, sizeof(U16) * 3);
	    }
	    
	    // Upload mesh data
	    mesh.NumIndices = (U32)coneIndices.size();
	    mesh.IndexBuffer = context.CreateIndexBuffer(inds->GetSize());
	    mesh.VertexBuffer = context.CreateVertexBuffer(verts->GetSize());
	    
	    U32 vertsSize = (U32)verts->GetSize();
	    U32 indsSize = (U32)inds->GetSize();
	    
	    upload->UploadBufferDataSlow(mesh.IndexBuffer, std::move(inds), 0, 0, indsSize);
	    upload->UploadBufferDataSlow(mesh.VertexBuffer, std::move(verts), 0, 0, vertsSize);
	    
	    meshes.push_back(std::move(mesh));
	    
    }
    
    // FILLED CYLINDER
    {
	    _ColouredMesh(context, mesh, DefaultRenderMeshType::FILLED_CYLINDER);
	    
	    std::vector<Vector3> cylinderVertices{};
	    std::vector<U16> cylinderIndices{};
	    
	    const U32 segments = 16;  // Number of segments for the circular base
	    
	    // --- Generate bottom circle vertices ---
	    for (U32 i = 0; i < segments; i++) {
    	    Vector3 vert{};
    	    vert.x = cosf(3.14159f * 2 * (Float)i / (Float)segments);
    	    vert.z = sinf(3.14159f * 2 * (Float)i / (Float)segments);
    	    vert.y = -1.0f; // Bottom of the cylinder
    	    cylinderVertices.push_back(vert);
	    }
	    
	    // --- Generate top circle vertices ---
	    for (U32 i = 0; i < segments; i++) {
    	    Vector3 vert{};
    	    vert.x = cosf(3.14159f * 2 * (Float)i / (Float)segments);
    	    vert.z = sinf(3.14159f * 2 * (Float)i / (Float)segments);
    	    vert.y = 1.0f; // Top of the cylinder
    	    cylinderVertices.push_back(vert);
	    }
	    
	    // --- Add center vertices for triangle fans ---
	    U16 bottomCenterIndex = (U16)cylinderVertices.size();
	    cylinderVertices.push_back(Vector3(0.0f, -1.0f, 0.0f));  // Bottom center
	    
	    U16 topCenterIndex = (U16)cylinderVertices.size();
	    cylinderVertices.push_back(Vector3(0.0f, 1.0f, 0.0f));  // Top center
	    
	    // --- Bottom circle (triangle fan) ---
	    for (U32 i = 0; i < segments; i++) {
    	    U16 next = (U16)((i + 1) % segments);
    	    cylinderIndices.push_back(bottomCenterIndex);
    	    cylinderIndices.push_back(next);
    	    cylinderIndices.push_back(i);
	    }
	    
	    // --- Top circle (triangle fan) ---
	    for (U32 i = 0; i < segments; i++) {
    	    U16 next = (U16)((i + 1) % segments);
    	    cylinderIndices.push_back(topCenterIndex);
    	    cylinderIndices.push_back((U16)(segments + i));
    	    cylinderIndices.push_back((U16)(segments + next));
	    }
	    
	    // --- Side faces (each quad as two triangles) ---
	    for (U32 i = 0; i < segments; i++) {
    	    U16 bottomIdx = (U16)i;
    	    U16 topIdx = (U16)(segments + i);
    	    U16 nextBottomIdx = (U16)((i + 1) % segments);
    	    U16 nextTopIdx = (U16)(segments + ((i + 1) % segments));
    	    
    	    // First triangle of the quad
    	    cylinderIndices.push_back(bottomIdx);
    	    cylinderIndices.push_back(topIdx);
    	    cylinderIndices.push_back(nextBottomIdx);
    	    
    	    // Second triangle of the quad
    	    cylinderIndices.push_back(nextBottomIdx);
    	    cylinderIndices.push_back(topIdx);
    	    cylinderIndices.push_back(nextTopIdx);
	    }
	    
	    // --- Create buffers ---
	    verts = DataStreamManager::GetInstance()->CreateBufferStream({}, cylinderVertices.size() * sizeof(ColouredVertex), 0, 0);
	    inds = DataStreamManager::GetInstance()->CreateBufferStream({}, cylinderIndices.size() * sizeof(U16), 0, 0);
	    
	    // --- Write vertex data ---
	    for (const auto& vertex : cylinderVertices) {
    	    ColouredVertex q{vertex.x, vertex.y, vertex.z};
    	    verts->Write((const U8*)&q, sizeof(ColouredVertex));
	    }
	    
	    // --- Write index data ---
	    for (U32 i = 0; i < cylinderIndices.size(); i += 3) {  // Writing triangles (sets of 3 vertices)
    	    indices[0] = cylinderIndices[i];
    	    indices[1] = cylinderIndices[i + 1];
    	    indices[2] = cylinderIndices[i + 2];
    	    inds->Write((const U8*)indices, sizeof(U16) * 3);
	    }
	    
	    // --- Upload mesh data ---
	    mesh.NumIndices = (U32)cylinderIndices.size();
	    mesh.IndexBuffer = context.CreateIndexBuffer(inds->GetSize());
	    mesh.VertexBuffer = context.CreateVertexBuffer(verts->GetSize());
	    
	    U32 vertsSize = (U32)verts->GetSize();
	    U32 indsSize = (U32)inds->GetSize();
	    
	    upload->UploadBufferDataSlow(mesh.IndexBuffer, std::move(inds), 0, 0, indsSize);
	    upload->UploadBufferDataSlow(mesh.VertexBuffer, std::move(verts), 0, 0, vertsSize);
	    
	    meshes.push_back(std::move(mesh));

    }
    
    // WIREFRAME CYLINDER
    {
	    _ColouredMesh(context, mesh, DefaultRenderMeshType::WIREFRAME_CYLINDER, true);
	    
	    std::vector<Vector3> cylinderVertices{};
	    std::vector<U16> cylinderIndices{};
	    
	    const U32 segments = 16;  // Number of base segments
	    
	    // Generate bottom circle vertices
	    for (U32 i = 0; i < segments; i++) {
    	    Vector3 vert{};
    	    vert.x = cosf(3.14159f * 2 * (Float)i / (Float)segments);
    	    vert.z = sinf(3.14159f * 2 * (Float)i / (Float)segments);
    	    vert.y = -1.0f; // Bottom of the cylinder
    	    cylinderVertices.push_back(vert);
	    }
	    
	    // Generate top circle vertices
	    for (U32 i = 0; i < segments; i++) {
    	    Vector3 vert{};
    	    vert.x = cosf(3.14159f * 2 * (Float)i / (Float)segments);
    	    vert.z = sinf(3.14159f * 2 * (Float)i / (Float)segments);
    	    vert.y = 1.0f; // Top of the cylinder
    	    cylinderVertices.push_back(vert);
	    }
	    
	    // --- Create bottom circle wireframe ---
	    for (U32 i = 0; i < segments; i++) {
    	    cylinderIndices.push_back((U16)i);
    	    cylinderIndices.push_back((U16)((i + 1) % segments));
	    }
	    
	    // --- Create top circle wireframe ---
	    for (U32 i = 0; i < segments; i++) {
    	    cylinderIndices.push_back((U16)(segments + i));
    	    cylinderIndices.push_back((U16)(segments + ((i + 1) % segments)));
	    }
	    
	    // --- Create vertical edges ---
	    for (U32 i = 0; i < segments; i++) {
    	    cylinderIndices.push_back((U16)i);
    	    cylinderIndices.push_back((U16)(segments + i));
	    }
	    
	    // connecting lines
	    for (U32 i = 0; i < segments>>1; i++) {
    	    cylinderIndices.push_back((U16)i);
    	    cylinderIndices.push_back((U16)((i + (segments>>1)) % segments));
    	    cylinderIndices.push_back((U16)(i+segments));
    	    cylinderIndices.push_back((U16)(segments + (i + (segments>>1)) % segments));
	    }
	    
	    // Create buffers
	    verts = DataStreamManager::GetInstance()->CreateBufferStream({}, cylinderVertices.size() * sizeof(ColouredVertex), 0, 0);
	    inds = DataStreamManager::GetInstance()->CreateBufferStream({}, cylinderIndices.size() * sizeof(U16), 0, 0);
	    
	    // Write vertex data
	    for (const auto& vertex : cylinderVertices) {
    	    ColouredVertex q{vertex.x, vertex.y, vertex.z};
    	    verts->Write((const U8*)&q, sizeof(ColouredVertex));
	    }
	    
	    // Write index data
	    for (U32 i = 0; i < cylinderIndices.size(); i += 2) {  // Writing lines (pairs of vertices)
    	    indices[0] = cylinderIndices[i];
    	    indices[1] = cylinderIndices[i + 1];
    	    inds->Write((const U8*)indices, sizeof(U16) * 2);
	    }
	    
	    // Upload mesh data
	    mesh.NumIndices = (U32)cylinderIndices.size();
	    mesh.IndexBuffer = context.CreateIndexBuffer(inds->GetSize());
	    mesh.VertexBuffer = context.CreateVertexBuffer(verts->GetSize());
	    
	    U32 vertsSize = (U32)verts->GetSize();
	    U32 indsSize = (U32)inds->GetSize();
	    
	    upload->UploadBufferDataSlow(mesh.IndexBuffer, std::move(inds), 0, 0, indsSize);
	    upload->UploadBufferDataSlow(mesh.VertexBuffer, std::move(verts), 0, 0, vertsSize);
	    
	    meshes.push_back(std::move(mesh));

    }
    
    upload->EndPass();
}
