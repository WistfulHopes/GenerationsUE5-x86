#include "BottomLevelAccelStruct.h"

#include "FreeListAllocator.h"
#include "IndexBuffer.h"
#include "MaterialData.h"
#include "Message.h"
#include "MessageSender.h"
#include "VertexBuffer.h"
#include "VertexDeclaration.h"

class MeshDataEx : public Hedgehog::Mirage::CMeshData
{
public:
    DX_PATCH::IDirect3DIndexBuffer9* m_indices;
    uint32_t m_indexCount;
};

HOOK(MeshDataEx*, __fastcall, MeshDataConstructor, 0x722860, MeshDataEx* This)
{
    const auto result = originalMeshDataConstructor(This);

    This->m_indices = nullptr;
    This->m_indexCount = 0;

    return result;
}

HOOK(void, __fastcall, MeshDataDestructor, 0x7227A0, MeshDataEx* This)
{
    if (This->m_indices != nullptr)
        This->m_indices->Release();

    originalMeshDataDestructor(This);
}

struct MeshResource
{
    const char* materialName;
    uint32_t indexCount;
    uint16_t* indices;
    // ...don't need the rest
};

static void convertToTriangles(MeshDataEx& meshData, const MeshResource* data)
{
    if (meshData.m_VertexNum == 0 || meshData.m_IndexNum <= 2)
        return;

    std::vector<uint16_t> indices;
    indices.reserve((meshData.m_IndexNum - 2) * 3);

    size_t start = 0;

    for (size_t i = 0; i < _byteswap_ulong(data->indexCount); ++i)
    {
        if (data->indices[i] == 0xFFFF)
        {
            start = i + 1;
        }
        else if (i - start >= 2)
        {
            uint16_t a = _byteswap_ushort(data->indices[i - 2]);
            uint16_t b = _byteswap_ushort(data->indices[i - 1]);
            uint16_t c = _byteswap_ushort(data->indices[i]);

            if ((i - start) & 1)
                std::swap(a, b);

            if (a != b && a != c && b != c)
            {
                indices.push_back(a);
                indices.push_back(b);
                indices.push_back(c);
            }
        }
    }

    if (indices.empty())
        return;

    const size_t byteSize = indices.size() * sizeof(uint16_t);
    const auto indexBuffer = new IndexBuffer(byteSize);

    auto& createMessage = s_messageSender.makeMessage<MsgCreateIndexBuffer>();
    createMessage.length = byteSize;
    createMessage.format = D3DFMT_INDEX16;
    createMessage.indexBufferId = indexBuffer->getId();
    s_messageSender.endMessage();

    auto& copyMessage = s_messageSender.makeMessage<MsgWriteIndexBuffer>(byteSize);
    copyMessage.indexBufferId = indexBuffer->getId();
    copyMessage.offset = 0;
    memcpy(copyMessage.data, indices.data(), byteSize);
    s_messageSender.endMessage();

    meshData.m_indices = reinterpret_cast<DX_PATCH::IDirect3DIndexBuffer9*>(indexBuffer);
    meshData.m_indexCount = indices.size();
}

HOOK(void, __cdecl, MakeMeshData, 0x744A00,
    MeshDataEx& meshData,
    const MeshResource* data,
    const Hedgehog::Mirage::CMirageDatabaseWrapper& databaseWrapper,
    Hedgehog::Mirage::CRenderingInfrastructure& renderingInfrastructure)
{
    originalMakeMeshData(meshData, data, databaseWrapper, renderingInfrastructure);
    convertToTriangles(meshData, data);
}

HOOK(void, __cdecl, MakeMeshData2, 0x744CC0,
    MeshDataEx& meshData,
    const MeshResource* data,
    const Hedgehog::Mirage::CMirageDatabaseWrapper& databaseWrapper,
    Hedgehog::Mirage::CRenderingInfrastructure& renderingInfrastructure)
{
    originalMakeMeshData2(meshData, data, databaseWrapper, renderingInfrastructure);
    convertToTriangles(meshData, data);
}

static bool checkMeshValidity(const MeshDataEx& meshData)
{
    return meshData.m_indexCount > 2 && meshData.m_VertexNum > 2 &&
        meshData.m_indices != nullptr && meshData.m_pD3DVertexBuffer != nullptr;
}

static size_t getGeometryCount(const hh::vector<boost::shared_ptr<Hedgehog::Mirage::CMeshData>>& meshGroup)
{
    size_t geometryCount = 0;
    for (const auto& meshData : meshGroup)
    {
        if (checkMeshValidity(static_cast<MeshDataEx&>(*meshData)))
            ++geometryCount;
    }
    return geometryCount;
}

template<typename T>
static size_t getGeometryCount(const T& modelData)
{
    size_t geometryCount = 0;

    for (const auto& nodeGroupModelData : modelData.m_NodeGroupModels)
    {
        if (!nodeGroupModelData->m_Visible)
            continue;

        geometryCount += getGeometryCount(nodeGroupModelData->m_OpaqueMeshes);
        geometryCount += getGeometryCount(nodeGroupModelData->m_TransparentMeshes);
        geometryCount += getGeometryCount(nodeGroupModelData->m_PunchThroughMeshes);

        for (const auto& specialMeshGroup : nodeGroupModelData->m_SpecialMeshGroups)
            geometryCount += getGeometryCount(specialMeshGroup);
    }

    geometryCount += getGeometryCount(modelData.m_OpaqueMeshes);
    geometryCount += getGeometryCount(modelData.m_TransparentMeshes);
    geometryCount += getGeometryCount(modelData.m_PunchThroughMeshes);

    return geometryCount;
}

static MsgCreateBottomLevelAccelStruct::GeometryDesc* convertMeshGroupToGeometryDescs(
    const hh::vector<boost::shared_ptr<Hedgehog::Mirage::CMeshData>>& meshGroup,
    MsgCreateBottomLevelAccelStruct::GeometryDesc* geometryDesc)
{
    for (const auto& meshData : meshGroup)
    {
        auto& meshDataEx = static_cast<MeshDataEx&>(*meshData);

        if (!checkMeshValidity(meshDataEx))
            continue;

        geometryDesc->indexCount = meshDataEx.m_indexCount;
        geometryDesc->vertexCount = meshData->m_VertexNum;
        geometryDesc->indexBufferId = reinterpret_cast<const IndexBuffer*>(meshDataEx.m_indices)->getId();
        geometryDesc->vertexBufferId = reinterpret_cast<const VertexBuffer*>(meshData->m_pD3DVertexBuffer)->getId();
        geometryDesc->vertexStride = meshData->m_VertexSize;

        const auto vertexDeclaration = reinterpret_cast<const VertexDeclaration*>(
            meshData->m_VertexDeclarationPtr.m_pD3DVertexDeclaration);

        auto vertexElement = vertexDeclaration->getVertexElements();

        while (vertexElement->Stream != 0xFF && vertexElement->Type != D3DDECLTYPE_UNUSED)
        {
            const uint32_t offset = meshData->m_VertexOffset + vertexElement->Offset;

            switch (vertexElement->Usage)
            {
            case D3DDECLUSAGE_POSITION:
                geometryDesc->positionOffset = offset;
                break;

            case D3DDECLUSAGE_NORMAL: 
                geometryDesc->normalOffset = offset;
                break;

            case D3DDECLUSAGE_TANGENT: 
                geometryDesc->tangentOffset = offset;
                break;

            case D3DDECLUSAGE_BINORMAL: 
                geometryDesc->binormalOffset = offset;
                break;

            case D3DDECLUSAGE_TEXCOORD:
                geometryDesc->texCoordOffsets[vertexElement->UsageIndex] = offset;
                break;

            case D3DDECLUSAGE_COLOR:
                geometryDesc->colorOffset = offset;
                break;
            }

            ++vertexElement;
        }

        geometryDesc->materialId = reinterpret_cast<MaterialDataEx*>(
            meshData->m_spMaterial.get())->m_materialId;

        ++geometryDesc;
    }
    return geometryDesc;
}

template<typename T>
static MsgCreateBottomLevelAccelStruct::GeometryDesc* convertModelToGeometryDescs(
    const T& modelData,
    MsgCreateBottomLevelAccelStruct::GeometryDesc* geometryDesc)
{
    for (const auto& nodeGroupModelData : modelData.m_NodeGroupModels)
    {
        if (!nodeGroupModelData->m_Visible)
            continue;

        geometryDesc = convertMeshGroupToGeometryDescs(nodeGroupModelData->m_OpaqueMeshes, geometryDesc);
        geometryDesc = convertMeshGroupToGeometryDescs(nodeGroupModelData->m_TransparentMeshes, geometryDesc);
        geometryDesc = convertMeshGroupToGeometryDescs(nodeGroupModelData->m_PunchThroughMeshes, geometryDesc);

        for (const auto& specialMeshGroup : nodeGroupModelData->m_SpecialMeshGroups)
            geometryDesc = convertMeshGroupToGeometryDescs(specialMeshGroup, geometryDesc);
    }

    geometryDesc = convertMeshGroupToGeometryDescs(modelData.m_OpaqueMeshes, geometryDesc);
    geometryDesc = convertMeshGroupToGeometryDescs(modelData.m_TransparentMeshes, geometryDesc);
    geometryDesc = convertMeshGroupToGeometryDescs(modelData.m_PunchThroughMeshes, geometryDesc);

    return geometryDesc;
}

template<typename T>
static void createBottomLevelAccelStruct(const T& modelData)
{
    const size_t geometryCount = getGeometryCount(modelData);

    auto& message = s_messageSender.makeMessage<MsgCreateBottomLevelAccelStruct>(
        geometryCount * sizeof(MsgCreateBottomLevelAccelStruct::GeometryDesc));

    message.bottomLevelAccelStructId = modelData.m_bottomLevelAccelStructId;
    memset(message.data, 0, geometryCount * sizeof(MsgCreateBottomLevelAccelStruct::GeometryDesc));

    const auto geometryDesc = convertModelToGeometryDescs(modelData,
        reinterpret_cast<MsgCreateBottomLevelAccelStruct::GeometryDesc*>(message.data));

    assert(reinterpret_cast<uint8_t*>(geometryDesc - geometryCount) == message.data);

    s_messageSender.endMessage();
}

static FreeListAllocator s_freeListAllocator;

HOOK(TerrainModelDataEx*, __fastcall, TerrainModelDataConstructor, 0x717440, TerrainModelDataEx* This)
{
    const auto result = originalTerrainModelDataConstructor(This);

    This->m_bottomLevelAccelStructId = s_freeListAllocator.allocate();

    return result;
}

HOOK(void, __fastcall, TerrainModelDataDestructor, 0x717230, TerrainModelDataEx* This)
{
    auto& message = s_messageSender.makeMessage<MsgReleaseRaytracingResource>();

    message.resourceType = MsgReleaseRaytracingResource::ResourceType::BottomLevelAccelStruct;
    message.resourceId = This->m_bottomLevelAccelStructId;

    s_messageSender.endMessage();

    s_freeListAllocator.free(This->m_bottomLevelAccelStructId);

    originalTerrainModelDataDestructor(This);
}

static std::vector<TerrainModelDataEx*> s_terrainModelList;

static void __fastcall terrainModelDataSetMadeOne(TerrainModelDataEx* This)
{
    assert(This->m_bottomLevelAccelStructId != NULL);

    if (*reinterpret_cast<uint32_t*>(0x1B244D4) != NULL) // Share vertex buffer
    {
        s_terrainModelList.push_back(This);
    }
    else
    {
        createBottomLevelAccelStruct(*This);
    }

    This->SetMadeOne();
}

HOOK(void, __fastcall, CreateShareVertexBuffer, 0x72EBD0, void* This)
{
    originalCreateShareVertexBuffer(This);

    for (const auto terrainModelData : s_terrainModelList)
        createBottomLevelAccelStruct(*terrainModelData);

    s_terrainModelList.clear();
}

void BottomLevelAccelStruct::init()
{
    WRITE_MEMORY(0x72294D, uint32_t, sizeof(MeshDataEx));
    WRITE_MEMORY(0x739511, uint32_t, sizeof(MeshDataEx));
    WRITE_MEMORY(0x739641, uint32_t, sizeof(MeshDataEx));
    WRITE_MEMORY(0x7397D1, uint32_t, sizeof(MeshDataEx));
    WRITE_MEMORY(0x73C763, uint32_t, sizeof(MeshDataEx));
    WRITE_MEMORY(0x73C873, uint32_t, sizeof(MeshDataEx));
    WRITE_MEMORY(0x73C9EA, uint32_t, sizeof(MeshDataEx));
    WRITE_MEMORY(0x73D063, uint32_t, sizeof(MeshDataEx));
    WRITE_MEMORY(0x73D173, uint32_t, sizeof(MeshDataEx));
    WRITE_MEMORY(0x73D2EA, uint32_t, sizeof(MeshDataEx));
    WRITE_MEMORY(0x73D971, uint32_t, sizeof(MeshDataEx));
    WRITE_MEMORY(0x73DA86, uint32_t, sizeof(MeshDataEx));
    WRITE_MEMORY(0x73DBFE, uint32_t, sizeof(MeshDataEx));
    WRITE_MEMORY(0x73E383, uint32_t, sizeof(MeshDataEx));
    WRITE_MEMORY(0x73E493, uint32_t, sizeof(MeshDataEx));
    WRITE_MEMORY(0x73E606, uint32_t, sizeof(MeshDataEx));
    WRITE_MEMORY(0x73EF23, uint32_t, sizeof(MeshDataEx));
    WRITE_MEMORY(0x73F033, uint32_t, sizeof(MeshDataEx));
    WRITE_MEMORY(0x73F1A6, uint32_t, sizeof(MeshDataEx));
    WRITE_MEMORY(0x745661, uint32_t, sizeof(MeshDataEx));
    WRITE_MEMORY(0x745771, uint32_t, sizeof(MeshDataEx));
    WRITE_MEMORY(0x7458E4, uint32_t, sizeof(MeshDataEx));
    WRITE_MEMORY(0x745D01, uint32_t, sizeof(MeshDataEx));
    WRITE_MEMORY(0xCD9A99, uint32_t, sizeof(MeshDataEx));

    INSTALL_HOOK(MeshDataConstructor);
    INSTALL_HOOK(MeshDataDestructor);

    INSTALL_HOOK(MakeMeshData);
    INSTALL_HOOK(MakeMeshData2);

    WRITE_MEMORY(0x72FCDC, uint8_t, sizeof(TerrainModelDataEx));

    INSTALL_HOOK(TerrainModelDataConstructor);
    INSTALL_HOOK(TerrainModelDataDestructor);

    WRITE_CALL(0x73A6D3, terrainModelDataSetMadeOne);
    WRITE_CALL(0x739AB4, terrainModelDataSetMadeOne);

    INSTALL_HOOK(CreateShareVertexBuffer);
}
