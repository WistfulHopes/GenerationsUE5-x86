#include "ModelData.h"

#include "GeometryFlags.h"
#include "IndexBuffer.h"
#include "MaterialData.h"
#include "Message.h"
#include "MessageSender.h"
#include "InstanceData.h"
#include "RaytracingRendering.h"
#include "RaytracingUtil.h"
#include "Texture.h"
#include "VertexBuffer.h"
#include "VertexDeclaration.h"

class MeshDataEx : public Hedgehog::Mirage::CMeshData
{
public:
    ComPtr<IndexBuffer> m_indices;
    uint32_t m_indexCount;
};

HOOK(MeshDataEx*, __fastcall, MeshDataConstructor, 0x722860, MeshDataEx* This)
{
    const auto result = originalMeshDataConstructor(This);

    new (std::addressof(This->m_indices)) ComPtr<IndexBuffer>();
    This->m_indexCount = 0;

    return result;
}

HOOK(void, __fastcall, MeshDataDestructor, 0x7227A0, MeshDataEx* This)
{
    This->m_indices.~ComPtr();
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
    assert(meshData.m_indices == nullptr);

    const uint32_t indexNum = _byteswap_ulong(data->indexCount);
    if (indexNum <= 2)
        return;

    std::vector<uint16_t> indices;
    indices.reserve((indexNum - 2) * 3);

    uint16_t a = _byteswap_ushort(data->indices[0]);
    uint16_t b = _byteswap_ushort(data->indices[1]);
    bool direction = false;

    for (uint32_t i = 2; i < indexNum; i++)
    {
        uint16_t c = _byteswap_ushort(data->indices[i]);

        if (c == 0xFFFF)
        {
            a = _byteswap_ushort(data->indices[++i]);
            b = _byteswap_ushort(data->indices[++i]);
            direction = false;
        }
        else
        {
            direction = !direction;
            if (a != b && b != c && c != a)
            {
                if (direction)
                {
                    indices.push_back(c);
                    indices.push_back(b);
                    indices.push_back(a);
                }
                else
                {
                    indices.push_back(a);
                    indices.push_back(b);
                    indices.push_back(c);
                }
            }

            a = b;
            b = c;
        }
    }

    if (indices.empty())
        return;

    const size_t byteSize = indices.size() * sizeof(uint16_t);

    meshData.m_indices.Attach(new IndexBuffer(byteSize));
    meshData.m_indexCount = indices.size();

    auto& createMessage = s_messageSender.makeMessage<MsgCreateIndexBuffer>();
    createMessage.length = byteSize;
    createMessage.format = D3DFMT_INDEX16;
    createMessage.indexBufferId = meshData.m_indices->getId();
    s_messageSender.endMessage();

    auto& copyMessage = s_messageSender.makeMessage<MsgWriteIndexBuffer>(byteSize);
    copyMessage.indexBufferId = meshData.m_indices->getId();
    copyMessage.offset = 0;
    memcpy(copyMessage.data, indices.data(), byteSize);
    s_messageSender.endMessage();
}

HOOK(void, __cdecl, MakeMeshData, 0x744A00,
    MeshDataEx& meshData,
    const MeshResource* data,
    const Hedgehog::Mirage::CMirageDatabaseWrapper& databaseWrapper,
    Hedgehog::Mirage::CRenderingInfrastructure& renderingInfrastructure)
{
    if (!meshData.IsMadeOne())
        convertToTriangles(meshData, data);
    originalMakeMeshData(meshData, data, databaseWrapper, renderingInfrastructure);
}

HOOK(void, __cdecl, MakeMeshData2, 0x744CC0,
    MeshDataEx& meshData,
    const MeshResource* data,
    const Hedgehog::Mirage::CMirageDatabaseWrapper& databaseWrapper,
    Hedgehog::Mirage::CRenderingInfrastructure& renderingInfrastructure)
{
    if (!meshData.IsMadeOne())
        convertToTriangles(meshData, data);
    originalMakeMeshData2(meshData, data, databaseWrapper, renderingInfrastructure);
}

template<typename T>
static void traverseMeshGroup(const hh::vector<boost::shared_ptr<Hedgehog::Mirage::CMeshData>>& meshGroup, uint32_t flags, const T& function)
{
    for (const auto& meshData : meshGroup)
    {
        const auto& meshDataEx = *reinterpret_cast<const MeshDataEx*>(meshData.get());

        if (meshDataEx.m_spMaterial != nullptr && meshDataEx.m_indexCount > 2 && meshDataEx.m_VertexNum > 2 &&
            meshDataEx.m_indices != nullptr && meshDataEx.m_pD3DVertexBuffer != nullptr)
        {
            function(meshDataEx, flags);
        }
    }
}

template<typename TModelData, typename TFunction>
static void traverseModelData(const TModelData& modelData, const TFunction& function)
{
    traverseMeshGroup(modelData.m_OpaqueMeshes, NULL, function);
    for (const auto& nodeGroupModelData : modelData.m_NodeGroupModels)
    {
        if (nodeGroupModelData->m_Visible)
            traverseMeshGroup(nodeGroupModelData->m_OpaqueMeshes, NULL, function);
    }

    traverseMeshGroup(modelData.m_TransparentMeshes, GEOMETRY_FLAG_TRANSPARENT, function);
    for (const auto& nodeGroupModelData : modelData.m_NodeGroupModels)
    {
        if (nodeGroupModelData->m_Visible)
            traverseMeshGroup(nodeGroupModelData->m_TransparentMeshes, GEOMETRY_FLAG_TRANSPARENT, function);
    }

    traverseMeshGroup(modelData.m_PunchThroughMeshes, GEOMETRY_FLAG_PUNCH_THROUGH, function);
    for (const auto& nodeGroupModelData : modelData.m_NodeGroupModels)
    {
        if (nodeGroupModelData->m_Visible)
            traverseMeshGroup(nodeGroupModelData->m_PunchThroughMeshes, GEOMETRY_FLAG_PUNCH_THROUGH, function);
    }

    for (const auto& nodeGroupModelData : modelData.m_NodeGroupModels)
    {
        if (!nodeGroupModelData->m_Visible)
            continue;

        for (const auto& specialMeshGroup : nodeGroupModelData->m_SpecialMeshGroups)
            traverseMeshGroup(specialMeshGroup, GEOMETRY_FLAG_TRANSPARENT, function);
    }
}

template<typename T>
static void createBottomLevelAccelStruct(const T& modelData, uint32_t& bottomLevelAccelStructId, uint32_t poseVertexBufferId)
{
    assert(bottomLevelAccelStructId == NULL);

    size_t geometryCount = 0;

    traverseModelData(modelData, [&](const MeshDataEx&, uint32_t) { ++geometryCount; });

    if (geometryCount == 0)
        return;

    auto& message = s_messageSender.makeMessage<MsgCreateBottomLevelAccelStruct>(
        geometryCount * sizeof(MsgCreateBottomLevelAccelStruct::GeometryDesc));

    message.bottomLevelAccelStructId = (bottomLevelAccelStructId = ModelData::s_idAllocator.allocate());
    message.preferFastBuild = poseVertexBufferId != NULL;
    memset(message.data, 0, geometryCount * sizeof(MsgCreateBottomLevelAccelStruct::GeometryDesc));

    auto geometryDesc = reinterpret_cast<MsgCreateBottomLevelAccelStruct::GeometryDesc*>(message.data);
    uint32_t poseVertexOffset = 0;

    traverseModelData(modelData, [&](const MeshDataEx& meshDataEx, uint32_t flags)
    {
        if (poseVertexBufferId != NULL)
            flags |= GEOMETRY_FLAG_POSE;

        geometryDesc->flags = flags;
        geometryDesc->indexCount = meshDataEx.m_indexCount;
        geometryDesc->vertexCount = meshDataEx.m_VertexNum;
        geometryDesc->indexBufferId = meshDataEx.m_indices->getId();

        uint32_t vertexOffset = meshDataEx.m_VertexOffset;
        if (poseVertexBufferId != NULL)
        {
            geometryDesc->vertexBufferId = poseVertexBufferId;
            vertexOffset = poseVertexOffset;
            poseVertexOffset += meshDataEx.m_VertexNum * (meshDataEx.m_VertexSize + 0xC); // Extra 12 bytes for previous position
        }
        else
        {
            geometryDesc->vertexBufferId = reinterpret_cast<const VertexBuffer*>(meshDataEx.m_pD3DVertexBuffer)->getId();
        }
        geometryDesc->vertexStride = meshDataEx.m_VertexSize;

        const auto vertexDeclaration = reinterpret_cast<const VertexDeclaration*>(
            meshDataEx.m_VertexDeclarationPtr.m_pD3DVertexDeclaration);

        auto vertexElement = vertexDeclaration->getVertexElements();
        uint32_t texCoordOffsets[4]{};

        while (vertexElement->Stream != 0xFF && vertexElement->Type != D3DDECLTYPE_UNUSED)
        {
            const uint32_t offset = vertexOffset + vertexElement->Offset;

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
                assert(vertexElement->UsageIndex < 4);
                texCoordOffsets[vertexElement->UsageIndex] = offset;
                break;

            case D3DDECLUSAGE_COLOR:
                geometryDesc->colorOffset = offset;
                if (vertexElement->Type != D3DDECLTYPE_FLOAT4)
                    geometryDesc->flags |= GEOMETRY_FLAG_D3DCOLOR;
                break;
            }

            ++vertexElement;
        }

        for (size_t i = 0; i < 4; i++)
            geometryDesc->texCoordOffsets[i] = texCoordOffsets[i] != 0 ? texCoordOffsets[i] : texCoordOffsets[0];

        const auto materialDataEx = reinterpret_cast<MaterialDataEx*>(
            meshDataEx.m_spMaterial.get());

        if (materialDataEx->m_materialId == NULL)
            materialDataEx->m_materialId = MaterialData::s_idAllocator.allocate();

        geometryDesc->materialId = materialDataEx->m_materialId;

        ++geometryDesc;
    });

    assert(reinterpret_cast<uint8_t*>(geometryDesc - geometryCount) == message.data);

    s_messageSender.endMessage();
}

HOOK(TerrainModelDataEx*, __fastcall, TerrainModelDataConstructor, 0x717440, TerrainModelDataEx* This)
{
    const auto result = originalTerrainModelDataConstructor(This);

    This->m_bottomLevelAccelStructId = NULL;

    return result;
}

HOOK(void, __fastcall, TerrainModelDataDestructor, 0x717230, TerrainModelDataEx* This)
{
    RaytracingUtil::releaseResource(RaytracingResourceType::BottomLevelAccelStruct, This->m_bottomLevelAccelStructId);
    originalTerrainModelDataDestructor(This);
}

HOOK(ModelDataEx*, __fastcall, ModelDataConstructor, 0x4FA400, ModelDataEx* This)
{
    const auto result = originalModelDataConstructor(This);

    This->m_bottomLevelAccelStructId = NULL;
    new (&This->m_noAoModel) boost::shared_ptr<Hedgehog::Mirage::CModelData>();

    return result;
}

HOOK(void, __fastcall, ModelDataDestructor, 0x4FA520, ModelDataEx* This)
{
    This->m_noAoModel.~shared_ptr();
    RaytracingUtil::releaseResource(RaytracingResourceType::BottomLevelAccelStruct, This->m_bottomLevelAccelStructId);

    originalModelDataDestructor(This);
}

HOOK(void, __cdecl, ModelDataMake, 0x7337A0,
    const Hedgehog::Base::CSharedString& name,
    const void* data,
    uint32_t dataSize,
    const boost::shared_ptr<Hedgehog::Database::CDatabase>& database,
    Hedgehog::Mirage::CRenderingInfrastructure* renderingInfrastructure)
{
    if (name == "chr_Sonic_HD" && data != nullptr)
    {
        Hedgehog::Mirage::CMirageDatabaseWrapper wrapper(database.get());

        const auto modelData = wrapper.GetModelData(name);
        if (!modelData->IsMadeOne())
        {
            auto& modelDataEx = *reinterpret_cast<ModelDataEx*>(modelData.get());
            if (XXH32(data, dataSize, 0) == 0x33CB76CD)
                modelDataEx.m_noAoModel = wrapper.GetModelData("chr_Sonic_HD_noao");
        }
    }

    originalModelDataMake(name, data, dataSize, database, renderingInfrastructure);
}

void ModelData::createBottomLevelAccelStruct(TerrainModelDataEx& terrainModelDataEx)
{
    if (terrainModelDataEx.m_bottomLevelAccelStructId == NULL)
        ::createBottomLevelAccelStruct(terrainModelDataEx, terrainModelDataEx.m_bottomLevelAccelStructId, NULL);
}

void ModelData::processEyeMaterials(ModelDataEx& modelDataEx, InstanceInfoEx& instanceInfoEx, MaterialMap& materialMap)
{
    if (!instanceInfoEx.m_handledEyeMaterials)
    {
        if (modelDataEx.m_spNodes != nullptr)
        {
            for (size_t i = 0; i < modelDataEx.m_NodeNum; i++)
            {
                if (modelDataEx.m_spNodes[i].m_Name == "Head")
                {
                    instanceInfoEx.m_headNodeIndex = i;
                    break;
                }
            }
        }

        static Hedgehog::Base::CStringSymbol s_sonicEyeHighLightPositionSymbol("g_SonicEyeHighLightPosition");
        static Hedgehog::Base::CStringSymbol s_sonicEyeHighLightPositionRaytracingSymbol("g_SonicEyeHighLightPosition_Raytracing");

        traverseModelData(modelDataEx, [&](const MeshDataEx& meshDataEx, uint32_t)
        {
            if (meshDataEx.m_spMaterial == nullptr || meshDataEx.m_spMaterial->m_spShaderListData == nullptr || 
                strstr(meshDataEx.m_spMaterial->m_spShaderListData->m_TypeAndName.c_str(), "ChrEye") == nullptr)
            {
                return;
            }

            auto& materialOverride = materialMap[meshDataEx.m_spMaterial.get()];
            if (materialOverride == nullptr)
                materialOverride = meshDataEx.m_spMaterial; // TODO: make a clone instead of assigning to itself

            auto& materialDataEx = *reinterpret_cast<MaterialDataEx*>(materialOverride.get());

            if (materialDataEx.m_raytracingHighLightParamValue == nullptr)
            {
                for (const auto& float4Param : materialDataEx.m_Float4Params)
                {
                    if (float4Param->m_Name == s_sonicEyeHighLightPositionSymbol)
                    {
                        materialDataEx.m_originalHighLightParamValue = float4Param->m_spValue;
                        break;
                    }
                }

                const auto float4Param = boost::make_shared<Hedgehog::Mirage::CParameterFloat4Element>();
                float4Param->m_Name = s_sonicEyeHighLightPositionRaytracingSymbol;
                float4Param->m_spValue = boost::make_shared<float[]>(4, 0.0f);
                float4Param->m_ValueNum = 1;

                materialDataEx.m_Float4Params.push_back(float4Param);
                materialDataEx.m_raytracingHighLightParamValue = float4Param->m_spValue;
            }
        });

        instanceInfoEx.m_handledEyeMaterials = true;
    }
}

void ModelData::createBottomLevelAccelStruct(ModelDataEx& modelDataEx, InstanceInfoEx& instanceInfoEx, const MaterialMap& materialMap, bool isEnabled)
{
    if (!isEnabled)
    {
        RaytracingUtil::releaseResource(RaytracingResourceType::Instance, instanceInfoEx.m_instanceId);
        return;
    }

    for (auto& [key, value] : materialMap)
    {
        auto& materialDataEx = *reinterpret_cast<MaterialDataEx*>(value.get());
        if (materialDataEx.m_raytracingHighLightParamValue != nullptr)
        {
            auto& originalHighLightPosition = *reinterpret_cast<Eigen::Vector3f*>(materialDataEx.m_originalHighLightParamValue.get());
            auto& raytracingHighLightPosition = *reinterpret_cast<Eigen::Vector3f*>(materialDataEx.m_raytracingHighLightParamValue.get());

            if (instanceInfoEx.m_spPose != nullptr && instanceInfoEx.m_spPose->GetMatrixNum() > instanceInfoEx.m_headNodeIndex)
                raytracingHighLightPosition = instanceInfoEx.m_Transform * (instanceInfoEx.m_spPose->GetMatrixList()[instanceInfoEx.m_headNodeIndex] * originalHighLightPosition);
            else
                raytracingHighLightPosition = instanceInfoEx.m_Transform * originalHighLightPosition;
        }

        MaterialData::create(*value, true);
    }

    const bool shouldCheckForHash = modelDataEx.m_hashFrame != RaytracingRendering::s_frame;

    if (shouldCheckForHash)
    {
        const XXH32_hash_t modelHash = XXH32(modelDataEx.m_NodeGroupModels.data(),
            modelDataEx.m_NodeGroupModels.size() * sizeof(modelDataEx.m_NodeGroupModels[0]), 0);

        uint32_t visibilityBits = 0;
        for (size_t i = 0; i < modelDataEx.m_NodeGroupModels.size(); i++)
        {
            if (modelDataEx.m_NodeGroupModels[i]->m_Visible)
                visibilityBits |= 1 << i;
        }

        if (modelDataEx.m_modelHash != modelHash || modelDataEx.m_visibilityBits != visibilityBits)
            RaytracingUtil::releaseResource(RaytracingResourceType::BottomLevelAccelStruct, modelDataEx.m_bottomLevelAccelStructId);

        modelDataEx.m_modelHash = modelHash;
        modelDataEx.m_visibilityBits = visibilityBits;
        modelDataEx.m_hashFrame = RaytracingRendering::s_frame;
    }

    if (instanceInfoEx.m_modelHash != modelDataEx.m_modelHash || instanceInfoEx.m_visibilityBits != modelDataEx.m_visibilityBits)
    {
        RaytracingUtil::releaseResource(RaytracingResourceType::Instance, instanceInfoEx.m_instanceId);
        RaytracingUtil::releaseResource(RaytracingResourceType::BottomLevelAccelStruct, instanceInfoEx.m_bottomLevelAccelStructId);
        instanceInfoEx.m_poseVertexBuffer = nullptr;
    }

    instanceInfoEx.m_modelHash = modelDataEx.m_modelHash;
    instanceInfoEx.m_visibilityBits = modelDataEx.m_visibilityBits;

    auto transform = instanceInfoEx.m_Transform;
    uint32_t bottomLevelAccelStructId;

    if (instanceInfoEx.m_spPose != nullptr && instanceInfoEx.m_spPose->GetMatrixNum() > 1)
    {
        if (instanceInfoEx.m_poseVertexBuffer == nullptr)
        {
            uint32_t length = 0;
            traverseModelData(modelDataEx, [&](const MeshDataEx& meshDataEx, uint32_t)
            {
                length += meshDataEx.m_VertexNum * (meshDataEx.m_VertexSize + 0xC); // Extra 12 bytes for previous position
            });

            if (length == 0)
                return;

            instanceInfoEx.m_poseVertexBuffer.Attach(new VertexBuffer(length));

            auto& message = s_messageSender.makeMessage<MsgCreateVertexBuffer>();
            message.vertexBufferId = instanceInfoEx.m_poseVertexBuffer->getId();
            message.allowUnorderedAccess = true;
            message.length = length;
            s_messageSender.endMessage();

            uint32_t offset = 0;
            traverseModelData(modelDataEx, [&](const MeshDataEx& meshDataEx, uint32_t)
            {
                auto& copyMessage = s_messageSender.makeMessage<MsgCopyVertexBuffer>();
                copyMessage.dstVertexBufferId = instanceInfoEx.m_poseVertexBuffer->getId();
                copyMessage.dstOffset = offset;
                copyMessage.srcVertexBufferId = reinterpret_cast<const VertexBuffer*>(meshDataEx.m_pD3DVertexBuffer)->getId();
                copyMessage.srcOffset = meshDataEx.m_VertexOffset;
                copyMessage.numBytes = meshDataEx.m_VertexNum * meshDataEx.m_VertexSize;
                s_messageSender.endMessage();

                offset += meshDataEx.m_VertexNum * (meshDataEx.m_VertexSize + 0xC); // Extra 12 bytes for previous position

            });
        }

        uint32_t geometryCount = 0;
        traverseModelData(modelDataEx, [&](const MeshDataEx&, uint32_t) { ++geometryCount; });

        auto& message = s_messageSender.makeMessage<MsgComputePose>(
            instanceInfoEx.m_spPose->GetMatrixNum() * sizeof(Hedgehog::Math::CMatrix) +
            geometryCount * sizeof(MsgComputePose::GeometryDesc));

        message.vertexBufferId = instanceInfoEx.m_poseVertexBuffer->getId();
        message.nodeCount = static_cast<uint8_t>(instanceInfoEx.m_spPose->GetMatrixNum());
        message.geometryCount = geometryCount;

        memcpy(message.data, instanceInfoEx.m_spPose->GetMatrixList(),
            instanceInfoEx.m_spPose->GetMatrixNum() * sizeof(Hedgehog::Math::CMatrix));

        auto geometryDesc = reinterpret_cast<MsgComputePose::GeometryDesc*>(message.data +
            instanceInfoEx.m_spPose->GetMatrixNum() * sizeof(Hedgehog::Math::CMatrix));

        memset(geometryDesc, 0, geometryCount * sizeof(MsgComputePose::GeometryDesc));

        traverseModelData(modelDataEx, [&](const MeshDataEx& meshDataEx, uint32_t)
        {
            assert(meshDataEx.m_VertexOffset == 0);

            geometryDesc->vertexCount = meshDataEx.m_VertexNum;
            geometryDesc->vertexBufferId = reinterpret_cast<const VertexBuffer*>(meshDataEx.m_pD3DVertexBuffer)->getId();
            geometryDesc->vertexStride = static_cast<uint8_t>(meshDataEx.m_VertexSize);

            if (meshDataEx.m_pNodeIndices != nullptr)
                memcpy(geometryDesc->nodePalette, meshDataEx.m_pNodeIndices, std::min(25u, meshDataEx.m_NodeNum));

            const auto vertexDeclaration = reinterpret_cast<const VertexDeclaration*>(
                meshDataEx.m_VertexDeclarationPtr.m_pD3DVertexDeclaration);

            auto vertexElement = vertexDeclaration->getVertexElements();

            while (vertexElement->Stream != 0xFF && vertexElement->Type != D3DDECLTYPE_UNUSED)
            {
                const uint8_t offset = static_cast<uint8_t>(vertexElement->Offset);

                switch (vertexElement->Usage)
                {
                case D3DDECLUSAGE_NORMAL:
                    geometryDesc->normalOffset = offset;
                    break;

                case D3DDECLUSAGE_TANGENT:
                    geometryDesc->tangentOffset = offset;
                    break;

                case D3DDECLUSAGE_BINORMAL:
                    geometryDesc->binormalOffset = offset;
                    break;

                case D3DDECLUSAGE_BLENDWEIGHT:
                    geometryDesc->blendWeightOffset = offset;
                    break;

                case D3DDECLUSAGE_BLENDINDICES:
                    geometryDesc->blendIndicesOffset = offset;
                    break;
                }

                ++vertexElement;
            }

            if (shouldCheckForHash)
                MaterialData::create(*meshDataEx.m_spMaterial, true);

            ++geometryDesc;
        });

        s_messageSender.endMessage();

        if (instanceInfoEx.m_bottomLevelAccelStructId == NULL)
        {
            ::createBottomLevelAccelStruct(modelDataEx, instanceInfoEx.m_bottomLevelAccelStructId, 
                instanceInfoEx.m_poseVertexBuffer->getId());

            if (instanceInfoEx.m_bottomLevelAccelStructId == NULL)
                return;
        }
        else
        {
            auto& buildMessage = s_messageSender.makeMessage<MsgBuildBottomLevelAccelStruct>();
            buildMessage.bottomLevelAccelStructId = instanceInfoEx.m_bottomLevelAccelStructId;
            s_messageSender.endMessage();
        }
        bottomLevelAccelStructId = instanceInfoEx.m_bottomLevelAccelStructId;
    }
    else
    {
        if (shouldCheckForHash)
        {
            traverseModelData(modelDataEx, [&](const MeshDataEx& meshDataEx, uint32_t)
            {
                MaterialData::create(*meshDataEx.m_spMaterial, true);
            });
        }

        if (instanceInfoEx.m_spPose != nullptr)
            transform = transform * (*instanceInfoEx.m_spPose->GetMatrixList());

        if (modelDataEx.m_bottomLevelAccelStructId == NULL)
        {
            ::createBottomLevelAccelStruct(modelDataEx, modelDataEx.m_bottomLevelAccelStructId, NULL);

            if (modelDataEx.m_bottomLevelAccelStructId == NULL)
                return;
        }

        bottomLevelAccelStructId = modelDataEx.m_bottomLevelAccelStructId;
    }

    auto& message = s_messageSender.makeMessage<MsgCreateInstance>(
        materialMap.size() * sizeof(uint32_t) * 2);

    for (size_t i = 0; i < 3; i++)
    {
        for (size_t j = 0; j < 4; j++)
            message.transform[i][j] = transform(i, j);
    }

    if (instanceInfoEx.m_instanceId == NULL)
    {
        instanceInfoEx.m_instanceId = InstanceData::s_idAllocator.allocate();
        message.storePrevTransform = false;
    }
    else
    {
        message.storePrevTransform = true;
    }

    message.instanceId = instanceInfoEx.m_instanceId;
    message.bottomLevelAccelStructId = bottomLevelAccelStructId;

    auto materialIds = reinterpret_cast<uint32_t*>(message.data);

    for (auto& [key, value] : materialMap)
    {
        *materialIds = reinterpret_cast<MaterialDataEx*>(key)->m_materialId;
        ++materialIds;

        *materialIds = reinterpret_cast<MaterialDataEx*>(value.get())->m_materialId;
        ++materialIds;
    }

    s_messageSender.endMessage();
}

void ModelData::renderSky(Hedgehog::Mirage::CModelData& modelData)
{
    size_t geometryCount = 0;
    traverseModelData(modelData, [&](const MeshDataEx&, uint32_t) { ++geometryCount; });

    if (geometryCount == 0)
        return;

    auto& message = s_messageSender.makeMessage<MsgRenderSky>(
        geometryCount * sizeof(MsgRenderSky::GeometryDesc));

    message.backgroundScale = *reinterpret_cast<const float*>(0x1A489EC);
    memset(message.data, 0, geometryCount * sizeof(MsgRenderSky::GeometryDesc));

    auto geometryDesc = reinterpret_cast<MsgRenderSky::GeometryDesc*>(message.data);

    static Hedgehog::Base::CStringSymbol s_diffuseSymbol("diffuse");
    static Hedgehog::Base::CStringSymbol s_opacitySymbol("opacity");
    static Hedgehog::Base::CStringSymbol s_displacementSymbol("displacement");
    static Hedgehog::Base::CStringSymbol s_ambientSymbol("ambient");

    DX_PATCH::IDirect3DBaseTexture9* diffuseTexture = nullptr;

    traverseModelData(modelData, [&](const MeshDataEx& meshDataEx, uint32_t flags)
    {
        geometryDesc->flags = flags;
        geometryDesc->vertexBufferId = reinterpret_cast<const VertexBuffer*>(meshDataEx.m_pD3DVertexBuffer)->getId();
        geometryDesc->vertexStride = meshDataEx.m_VertexSize;
        geometryDesc->vertexCount = meshDataEx.m_VertexNum;
        geometryDesc->indexBufferId = reinterpret_cast<const IndexBuffer*>(meshDataEx.m_pD3DIndexBuffer)->getId();
        geometryDesc->indexCount = meshDataEx.m_IndexNum;
        geometryDesc->vertexDeclarationId = reinterpret_cast<const VertexDeclaration*>(meshDataEx.m_VertexDeclarationPtr.m_pD3DVertexDeclaration)->getId();
    
        if (meshDataEx.m_spMaterial != nullptr)
        {
            geometryDesc->isAdditive = meshDataEx.m_spMaterial->m_Additive;

            for (const auto& texture : meshDataEx.m_spMaterial->m_spTexsetData->m_TextureList)
            {
                if (!texture->IsMadeOne() || texture->m_spPictureData == nullptr ||
                    texture->m_spPictureData->m_pD3DTexture == nullptr)
                {
                    continue;
                }
    
                MsgCreateMaterial::Texture* textureDesc = nullptr;
    
                if (texture->m_Type == s_diffuseSymbol)
                {
                    textureDesc = &geometryDesc->diffuseTexture;
                    diffuseTexture = texture->m_spPictureData->m_pD3DTexture;
                }
                else if (texture->m_Type == s_opacitySymbol)
                {
                    if (diffuseTexture == texture->m_spPictureData->m_pD3DTexture)
                        continue;

                    textureDesc = &geometryDesc->alphaTexture;
                }
                else if (texture->m_Type == s_displacementSymbol)
                {
                    textureDesc = &geometryDesc->emissionTexture;
                }
    
                if (textureDesc != nullptr)
                {
                    textureDesc->id = reinterpret_cast<const Texture*>(texture->m_spPictureData->m_pD3DTexture)->getId();
                    textureDesc->addressModeU = std::max(D3DTADDRESS_WRAP, texture->m_SamplerState.AddressU);
                    textureDesc->addressModeV = std::max(D3DTADDRESS_WRAP, texture->m_SamplerState.AddressV);
                    textureDesc->texCoordIndex = texture->m_TexcoordIndex;
                }
            }

            for (const auto& float4Param : meshDataEx.m_spMaterial->m_Float4Params)
            {
                if (float4Param->m_Name == s_ambientSymbol)
                    memcpy(geometryDesc->ambient, float4Param->m_spValue.get(), sizeof(geometryDesc->ambient));
            }
        }
        else
        {
            geometryDesc->isAdditive = false;
        }
    
        ++geometryDesc;
    });

    s_messageSender.endMessage();
}

void ModelData::init()
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

    WRITE_MEMORY(0x4FA1FC, uint32_t, sizeof(ModelDataEx));
    WRITE_MEMORY(0xE993F1, uint32_t, sizeof(ModelDataEx));

    INSTALL_HOOK(ModelDataConstructor);
    INSTALL_HOOK(ModelDataDestructor);

    INSTALL_HOOK(ModelDataMake);
}
