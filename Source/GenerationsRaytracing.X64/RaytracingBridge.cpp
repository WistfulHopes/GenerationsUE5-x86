﻿#include "RaytracingBridge.h"

#include "BlueNoise.h"
#include "Bridge.h"
#include "CopyPS.h"
#include "CopyVS.h"
#include "DLSS.h"
#include "EnvironmentColor.h"
#include "File.h"
#include "FSR.h"
#include "Message.h"
#include "Profiler.h"
#include "ShaderLibrary.h"
#include "ShaderMapping.h"
#include "Skinning.h"
#include "Utilities.h"

RaytracingBridge::RaytracingBridge(const Device& device, const std::string& directoryPath)
{
    const INIReader reader("GenerationsRaytracing.ini");

    if (reader.GetInteger("Mod", "Upscaler", 0))
        upscaler = std::make_unique<FSR>(device);
    else 
        upscaler = std::make_unique<DLSS>(device, directoryPath);

    shaderMapping.load(directoryPath + "/ShaderMapping.bin");

    bindingLayout = device.nvrhi->createBindingLayout(nvrhi::BindingLayoutDesc()
        .setVisibility(nvrhi::ShaderType::AllRayTracing)
        .addItem(nvrhi::BindingLayoutItem::VolatileConstantBuffer(0))
        .addItem(nvrhi::BindingLayoutItem::VolatileConstantBuffer(1))
        .addItem(nvrhi::BindingLayoutItem::VolatileConstantBuffer(2))
        .addItem(nvrhi::BindingLayoutItem::RayTracingAccelStruct(0))

        .addItem(nvrhi::BindingLayoutItem::StructuredBuffer_SRV(1))
        .addItem(nvrhi::BindingLayoutItem::StructuredBuffer_SRV(2))
        .addItem(nvrhi::BindingLayoutItem::StructuredBuffer_SRV(3))

        .addItem(nvrhi::BindingLayoutItem::Texture_UAV(0))
        .addItem(nvrhi::BindingLayoutItem::Texture_UAV(1))
        .addItem(nvrhi::BindingLayoutItem::Texture_UAV(2))
        .addItem(nvrhi::BindingLayoutItem::Texture_UAV(3))
        .addItem(nvrhi::BindingLayoutItem::Texture_UAV(4))
        .addItem(nvrhi::BindingLayoutItem::Texture_UAV(5))
        .addItem(nvrhi::BindingLayoutItem::Texture_UAV(6))
        .addItem(nvrhi::BindingLayoutItem::Texture_UAV(7))
        .addItem(nvrhi::BindingLayoutItem::Texture_UAV(8))
        .addItem(nvrhi::BindingLayoutItem::Texture_UAV(9))
        .addItem(nvrhi::BindingLayoutItem::Texture_UAV(10))
        .addItem(nvrhi::BindingLayoutItem::Texture_UAV(11))

        .addItem(nvrhi::BindingLayoutItem::Texture_SRV(4))
        .addItem(nvrhi::BindingLayoutItem::Texture_SRV(5))
        .addItem(nvrhi::BindingLayoutItem::Texture_SRV(6))
        .addItem(nvrhi::BindingLayoutItem::Texture_SRV(7))

        .addItem(nvrhi::BindingLayoutItem::Sampler(0)));

    bindlessLayout = device.nvrhi->createBindlessLayout(nvrhi::BindlessLayoutDesc()
        .setVisibility(nvrhi::ShaderType::AllRayTracing)
        .setMaxCapacity(65336)
        .addRegisterSpace(nvrhi::BindingLayoutItem::TypedBuffer_SRV(1))
        .addRegisterSpace(nvrhi::BindingLayoutItem::RawBuffer_SRV(2))
        .addRegisterSpace(nvrhi::BindingLayoutItem::Texture_SRV(3))
        .addRegisterSpace(nvrhi::BindingLayoutItem::Texture_SRV(4)));

    descriptorTable = device.nvrhi->createDescriptorTable(bindlessLayout);
    device.nvrhi->resizeDescriptorTable(descriptorTable, 65536, false);

    rtConstantBuffer = device.nvrhi->createBuffer(nvrhi::BufferDesc()
        .setByteSize(sizeof(RTConstants))
        .setIsConstantBuffer(true)
        .setIsVolatile(true)
        .setMaxVersions(1));

    linearRepeatSampler = device.nvrhi->createSampler(nvrhi::SamplerDesc()
        .setAllFilters(true)
        .setAllAddressModes(nvrhi::SamplerAddressMode::Repeat));

    skinningShader = device.nvrhi->createShader(nvrhi::ShaderDesc(nvrhi::ShaderType::Compute), SKINNING, sizeof(SKINNING));

    skinningBindingLayout = device.nvrhi->createBindingLayout(nvrhi::BindingLayoutDesc()
        .setVisibility(nvrhi::ShaderType::Compute)
        .addItem(nvrhi::BindingLayoutItem::VolatileConstantBuffer(0))
        .addItem(nvrhi::BindingLayoutItem::RawBuffer_SRV(0))
        .addItem(nvrhi::BindingLayoutItem::TypedBuffer_SRV(1))
        .addItem(nvrhi::BindingLayoutItem::StructuredBuffer_SRV(2))
        .addItem(nvrhi::BindingLayoutItem::RawBuffer_UAV(0)));

    skinningConstantBuffer = device.nvrhi->createBuffer(nvrhi::BufferDesc()
        .setByteSize(sizeof(Mesh::GPU))
        .setIsConstantBuffer(true)
        .setIsVolatile(true)
        .setMaxVersions(1));

    skinningPipeline = device.nvrhi->createComputePipeline(nvrhi::ComputePipelineDesc()
        .setComputeShader(skinningShader)
        .addBindingLayout(skinningBindingLayout));

    pointClampSampler = device.nvrhi->createSampler(nvrhi::SamplerDesc()
        .setAllFilters(false)
        .setAllAddressModes(nvrhi::SamplerAddressMode::Clamp));

    copyBindingLayout = device.nvrhi->createBindingLayout(nvrhi::BindingLayoutDesc()
        .setVisibility(nvrhi::ShaderType::Pixel)
        .addItem(nvrhi::BindingLayoutItem::Texture_SRV(0))
        .addItem(nvrhi::BindingLayoutItem::Texture_SRV(1))
        .addItem(nvrhi::BindingLayoutItem::Sampler(0)));

    copyVertexShader = device.nvrhi->createShader(nvrhi::ShaderDesc(nvrhi::ShaderType::Vertex), COPY_VS, sizeof(COPY_VS));
    copyPixelShader = device.nvrhi->createShader(nvrhi::ShaderDesc(nvrhi::ShaderType::Pixel), COPY_PS, sizeof(COPY_PS));

    copyDrawArguments.setVertexCount(6);
}

RaytracingBridge::~RaytracingBridge() = default;

void RaytracingBridge::procMsgCreateMesh(Bridge& bridge)
{
    const auto msg = bridge.msgReceiver.getMsgAndMoveNext<MsgCreateMesh>();
    const void* data = bridge.msgReceiver.getDataAndMoveNext(msg->nodeNum);

    auto& model = models[msg->model];
    auto& element = model.elements[msg->element];
    auto& group = element.groups[msg->group];

    auto& geometryDesc =
        group.desc.bottomLevelGeometries.size() > group.curMeshIndex ?
        group.desc.bottomLevelGeometries[group.curMeshIndex] :
        group.desc.bottomLevelGeometries.emplace_back();

    if (msg->opaque)
        geometryDesc.flags = nvrhi::rt::GeometryFlags::Opaque;

    auto& triangles = geometryDesc.geometryData.triangles;

    triangles.indexBuffer = nvrhi::checked_cast<nvrhi::IBuffer*>(bridge.resources[msg->indexBuffer].Get());
    triangles.vertexBuffer = nvrhi::checked_cast<nvrhi::IBuffer*>(bridge.resources[msg->vertexBuffer].Get());

    assert(triangles.indexBuffer && triangles.vertexBuffer);

    triangles.indexFormat = nvrhi::Format::R16_UINT;
    triangles.vertexFormat = nvrhi::Format::RGB32_FLOAT;
    triangles.indexOffset = msg->indexOffset;
    triangles.vertexOffset = msg->vertexOffset;
    triangles.indexCount = msg->indexCount;
    triangles.vertexCount = msg->vertexCount;
    triangles.vertexStride = msg->vertexStride;

    auto& mesh =
        group.meshes.size() > group.curMeshIndex ?
        group.meshes[group.curMeshIndex] :
        group.meshes.emplace_back(descriptorTableManager);

    if (mesh.vertexBuffer != triangles.vertexBuffer ||
        mesh.indexBuffer != triangles.indexBuffer)
    {
        mesh.skinningBuffer = nullptr;
        mesh.prevSkinningBuffer = nullptr;

        mesh.releaseDescriptors();

        mesh.skinningBufferId = GEOMETRY_SKINNING_BUFFER_ID(msg->vertexBuffer, msg->element);
        mesh.prevSkinningBufferId = GEOMETRY_PREV_SKINNING_BUFFER_ID(msg->vertexBuffer, msg->element);

        group.handle = nullptr;
    }

    mesh.gpu.vertexBuffer = msg->vertexBuffer;
    mesh.gpu.prevVertexBuffer = msg->vertexBuffer;

    mesh.gpu.vertexCount = msg->vertexCount;
    mesh.gpu.vertexStride = msg->vertexStride;
    mesh.gpu.normalOffset = msg->normalOffset;
    mesh.gpu.tangentOffset = msg->tangentOffset;
    mesh.gpu.binormalOffset = msg->binormalOffset;
    mesh.gpu.texCoordOffset = msg->texCoordOffset;
    mesh.gpu.colorOffset = msg->colorOffset;
    mesh.gpu.colorFormat = msg->colorFormat;
    mesh.gpu.blendWeightOffset = msg->blendWeightOffset;
    mesh.gpu.blendIndicesOffset = msg->blendIndicesOffset;

    mesh.gpu.indexBuffer = msg->indexBuffer;

    mesh.gpu.material = msg->material;
    mesh.gpu.punchThrough = msg->punchThrough;

    mesh.vertexBuffer = triangles.vertexBuffer;
    mesh.indexBuffer = triangles.indexBuffer;

    if (msg->nodeNum > 0)
    {
        auto& nodeIndicesBuffer = model.nodeIndicesBuffers[XXH64(data, msg->nodeNum, 0)];
        if (!nodeIndicesBuffer)
        {
            nodeIndicesBuffer = bridge.device.nvrhi->createBuffer(nvrhi::BufferDesc()
                .setByteSize(msg->nodeNum)
                .setFormat(nvrhi::Format::R8_UINT)
                .setCanHaveTypedViews(true)
                .setInitialState(nvrhi::ResourceStates::CopyDest)
                .setKeepInitialState(true));

            bridge.openCommandListForCopy();
            bridge.commandListForCopy->writeBuffer(nodeIndicesBuffer, data, msg->nodeNum);
            bridge.commandList->setPermanentBufferState(nodeIndicesBuffer, nvrhi::ResourceStates::ShaderResource);
        }

        mesh.nodeIndicesBuffer = nodeIndicesBuffer;
    }
    else
    {
        mesh.nodeIndicesBuffer = nullptr;
    }

    group.curMeshIndex += 1;
}

void RaytracingBridge::procMsgCreateModel(Bridge& bridge)
{
    const auto msg = bridge.msgReceiver.getMsgAndMoveNext<MsgCreateModel>();

    const size_t matrixByteSize = msg->matrixNum * 64;
    const void* data = bridge.msgReceiver.getDataAndMoveNext(matrixByteSize);

    const auto modelPair = models.find(msg->model);
    if (modelPair == models.end())
        return;

    const auto elementPair = modelPair->second.elements.find(msg->element);
    if (elementPair == modelPair->second.elements.end())
        return;

    if (msg->matrixNum > 0)
    {
        if (elementPair->second.matrixBuffer == nullptr ||
            elementPair->second.matrixBuffer->getDesc().byteSize != matrixByteSize)
        {
            elementPair->second.matrixBuffer = bridge.device.nvrhi->createBuffer(nvrhi::BufferDesc()
                .setByteSize(matrixByteSize)
                .setStructStride(64)
                .setCpuAccess(nvrhi::CpuAccessMode::Write));

            elementPair->second.matrixHash = 0;
        }

        const XXH64_hash_t matrixHash = XXH64(data, matrixByteSize, 0);
        elementPair->second.matrixBufferChanged = matrixHash != elementPair->second.matrixHash;

        if (elementPair->second.matrixBufferChanged)
        {
            elementPair->second.matrixHash = matrixHash;

            void* memory = bridge.device.nvrhi->mapBuffer(elementPair->second.matrixBuffer, nvrhi::CpuAccessMode::Write);
            memcpy(memory, data, matrixByteSize);
            bridge.device.nvrhi->unmapBuffer(elementPair->second.matrixBuffer);
        }
    }

    for (auto& groupPair : elementPair->second.groups)
    {
        groupPair.second.desc.bottomLevelGeometries.resize(groupPair.second.curMeshIndex);
        groupPair.second.meshes.resize(groupPair.second.curMeshIndex, { descriptorTableManager });
        groupPair.second.curMeshIndex = 0;
    }
}

void RaytracingBridge::procMsgCreateInstance(Bridge& bridge)
{
    const auto msg = bridge.msgReceiver.getMsgAndMoveNext<MsgCreateInstance>();

    auto& instance = instances.emplace_back();
    memcpy(instance.transform, msg->transform, sizeof(instance.transform));
    memcpy(instance.gpu.prevTransform, msg->prevTransform, sizeof(instance.gpu.prevTransform));
    instance.model = msg->model;
    instance.element = msg->element;
}

void RaytracingBridge::procMsgCreateMaterial(Bridge& bridge)
{
    const auto msg = bridge.msgReceiver.getMsgAndMoveNext<MsgCreateMaterial>();

    auto& material = materials[msg->material];

    const auto pair = shaderMapping.indices.find(msg->shader);

    if (pair != shaderMapping.indices.end())
        material.shader = pair->second;
    else
        material.shader = shaderMapping.indices["SysError"];

    memcpy(material.gpu.textures, msg->textures, sizeof(msg->textures));
    memcpy(material.gpu.parameters, msg->parameters, sizeof(msg->parameters));
}

void RaytracingBridge::procMsgReleaseSingleElement(Bridge& bridge)
{
    const auto msg = bridge.msgReceiver.getMsgAndMoveNext<MsgReleaseElement>();
    pendingReleases.emplace_back(msg->model, msg->element);
}

template<typename T>
static void createUploadBuffer(const Bridge& bridge, const std::vector<T>& vector, nvrhi::BufferHandle& buffer)
{
    if (buffer == nullptr || buffer->getDesc().byteSize < vectorByteSize(vector))
    {
        buffer = bridge.device.nvrhi->createBuffer(nvrhi::BufferDesc()
            .setByteSize(vectorByteSize(vector))
            .setStructStride((uint32_t)vectorByteStride(vector))
            .setCpuAccess(nvrhi::CpuAccessMode::Write));
    }

    void* copyDest = bridge.device.nvrhi->mapBuffer(buffer, nvrhi::CpuAccessMode::Write);
    memcpy(copyDest, vector.data(), vectorByteSize(vector));
    bridge.device.nvrhi->unmapBuffer(buffer);
}

void RaytracingBridge::procMsgNotifySceneTraversed(Bridge& bridge)
{
    const auto msg = bridge.msgReceiver.getMsgAndMoveNext<MsgNotifySceneTraversed>();

    if (instances.empty())
        return;

    auto& colorAttachment = bridge.framebufferDesc.colorAttachments[0].texture;
    if (!colorAttachment || colorAttachment->getDesc().format != nvrhi::Format::RGBA16_FLOAT)
        return;

    if (!pipeline)
    {
        shaderLibrary = bridge.device.nvrhi->createShaderLibrary(SHADER_LIBRARY, sizeof(SHADER_LIBRARY));

        size_t shaderLibraryByteSize = 0;
        auto shaderLibraryBytes = readAllBytes(bridge.directoryPath + "/ShaderLibrary.cso", shaderLibraryByteSize);

        shaderLibraryAutoGenerated = bridge.device.nvrhi->createShaderLibrary(shaderLibraryBytes.get(), shaderLibraryByteSize);

        auto pipelineDesc = nvrhi::rt::PipelineDesc()
            .addBindingLayout(bindingLayout)
            .addBindingLayout(bindlessLayout)

            .addShader({ "PrimaryRayGeneration", shaderLibrary->getShader("PrimaryRayGeneration", nvrhi::ShaderType::RayGeneration) })
            .addShader({ "GlobalIlluminationRayGeneration", shaderLibrary->getShader("GlobalIlluminationRayGeneration", nvrhi::ShaderType::RayGeneration) })
            .addShader({ "ShadowRayGeneration", shaderLibrary->getShader("ShadowRayGeneration", nvrhi::ShaderType::RayGeneration) })
            .addShader({ "ReflectionRayGeneration", shaderLibrary->getShader("ReflectionRayGeneration", nvrhi::ShaderType::RayGeneration) })
            .addShader({ "RefractionRayGeneration", shaderLibrary->getShader("RefractionRayGeneration", nvrhi::ShaderType::RayGeneration) })
            .addShader({ "CompositeRayGeneration", shaderLibrary->getShader("CompositeRayGeneration", nvrhi::ShaderType::RayGeneration) })

            .addShader({ "MissPrimary", shaderLibrary->getShader("MissPrimary", nvrhi::ShaderType::Miss) })
            .addShader({ "MissPrimarySky", shaderLibrary->getShader("MissPrimarySky", nvrhi::ShaderType::Miss) })
            .addShader({ "MissSecondary", shaderLibrary->getShader("MissSecondary", nvrhi::ShaderType::Miss) })
            .addShader({ "MissSecondarySky", shaderLibrary->getShader("MissSecondarySky", nvrhi::ShaderType::Miss) })

            .setMaxRecursionDepth(4)
            .setMaxPayloadSize(24);

        for (auto& shader : shaderMapping.shaders)
        {
            const auto primaryClosestHit = shaderLibraryAutoGenerated->getShader(shader.primaryClosestHit.c_str(), nvrhi::ShaderType::ClosestHit);
            const auto secondaryClosestHit = shaderLibraryAutoGenerated->getShader(shader.secondaryClosestHit.c_str(), nvrhi::ShaderType::ClosestHit);
            const auto anyHit = shaderLibraryAutoGenerated->getShader(shader.anyHit.c_str(), nvrhi::ShaderType::AnyHit);
            const auto callable = shaderLibraryAutoGenerated->getShader(shader.callable.c_str(), nvrhi::ShaderType::Callable);

            pipelineDesc.addHitGroup({ shader.primaryClosestHit, primaryClosestHit, anyHit });
            pipelineDesc.addHitGroup({ shader.secondaryClosestHit, secondaryClosestHit, anyHit });
            pipelineDesc.addShader({ shader.callable, callable });
        }

        pipeline = bridge.device.nvrhi->createRayTracingPipeline(pipelineDesc);

        shaderTable = pipeline->createShaderTable();

        for (auto& shader : shaderMapping.shaders)
            shaderTable->addCallableShader(shader.callable.c_str());
    }

    PUSH_PROFILER("Create GPU Materials");

    bindingSetItems.clear();
    materialBufferVec.clear();

    for (auto& [id, material] : materials)
    {
        material.indexInBuffer = (uint32_t)materialBufferVec.size();
        auto& gpuMaterial = materialBufferVec.emplace_back();

        for (size_t i = 0; i < 16; i++)
        {
            if (!material.gpu.textures[i])
                continue;

            const auto texPair = bridge.resources.find(material.gpu.textures[i]);
            if (texPair == bridge.resources.end() || !descriptorTableManager.put(material.gpu.textures[i], gpuMaterial.textures[i]))
                continue;

            bindingSetItems.push_back(nvrhi::BindingSetItem::Texture_SRV(gpuMaterial.textures[i], 
                nvrhi::checked_cast<nvrhi::ITexture*>(texPair->second.Get())));
        }

        memcpy(gpuMaterial.parameters, material.gpu.parameters, sizeof(material.gpu.parameters));
    }

    POP_PROFILER();
    PUSH_PROFILER("Create Shader Table and GPU Geometries");

    const bool hasEnvironmentColor = EnvironmentColor::get(bridge, rtConstants.environmentColor);

    shaderTable->clearMissShaders();
    shaderTable->addMissShader("MissPrimary"); // MISS
    shaderTable->addMissShader("MissPrimarySky"); // MISS_SKY
    shaderTable->addMissShader(hasEnvironmentColor ? "MissSecondary" : "MissSecondarySky"); // MISS_GLOBAL_ILLUMINATION
    shaderTable->addMissShader("MissSecondary"); // MISS_SHADOW
    shaderTable->addMissShader("MissSecondarySky"); // MISS_REFLECTION_REFRACTION

    meshBufferVec.clear();
    shaderTable->clearHitShaders();

    for (auto& [id, model] : models)
    {
        for (auto elementPair = model.elements.begin(); elementPair != model.elements.end();)
        {
            for (auto groupPair = elementPair->second.groups.begin(); groupPair != elementPair->second.groups.end();)
            {
                if (groupPair->second.meshes.empty())
                {
                    groupPair = elementPair->second.groups.erase(groupPair);
                    continue;
                }

                groupPair->second.indexInBuffer = (uint32_t)meshBufferVec.size();

                for (size_t i = 0; i < groupPair->second.meshes.size(); i++)
                {
                    auto& mesh = groupPair->second.meshes[i];
                    const auto pair = materials.find(mesh.gpu.material);
                    auto& gpuMesh = meshBufferVec.emplace_back(mesh.gpu);

                    if (pair != materials.end())
                    {
                        const auto& shader = shaderMapping.shaders[pair->second.shader];

                        shaderTable->addHitGroup(shader.primaryClosestHit.c_str());
                        shaderTable->addHitGroup(shader.secondaryClosestHit.c_str());

                        gpuMesh.material = pair->second.indexInBuffer;
                    }
                    else
                    {
                        shaderTable->addHitGroup("SysDefault_SysError_primary_closesthit");
                        shaderTable->addHitGroup("SysDefault_SysError_secondary_closesthit");

                        gpuMesh.material = NULL;
                    }

                    if ((groupPair->second.handle == nullptr || elementPair->second.matrixBufferChanged) && 
                        (mesh.nodeIndicesBuffer != nullptr && elementPair->second.matrixBuffer != nullptr))
                    {
                        std::swap(mesh.skinningBuffer, mesh.prevSkinningBuffer);
                        std::swap(mesh.skinningBufferId, mesh.prevSkinningBufferId);

                        if (mesh.skinningBuffer == nullptr)
                        {
                            auto desc = mesh.vertexBuffer->getDesc();
                            mesh.skinningBuffer = bridge.device.nvrhi->createBuffer(desc.setCanHaveUAVs(true));
                        }

                        bridge.commandList->writeBuffer(skinningConstantBuffer, &mesh.gpu, sizeof(Mesh::GPU));

                        auto skinningBindingSet = bridge.device.nvrhi->createBindingSet(nvrhi::BindingSetDesc()
                            .addItem(nvrhi::BindingSetItem::ConstantBuffer(0, skinningConstantBuffer))
                            .addItem(nvrhi::BindingSetItem::RawBuffer_SRV(0, mesh.vertexBuffer))
                            .addItem(nvrhi::BindingSetItem::TypedBuffer_SRV(1, mesh.nodeIndicesBuffer))
                            .addItem(nvrhi::BindingSetItem::StructuredBuffer_SRV(2, elementPair->second.matrixBuffer))
                            .addItem(nvrhi::BindingSetItem::RawBuffer_UAV(0, mesh.skinningBuffer)), skinningBindingLayout);

                        bridge.commandList->setComputeState(nvrhi::ComputeState()
                            .setPipeline(skinningPipeline)
                            .addBindingSet(skinningBindingSet));

                        bridge.commandList->dispatch((mesh.gpu.vertexCount + 63) / 64);

                        groupPair->second.desc.bottomLevelGeometries[i].geometryData.triangles.vertexBuffer = mesh.skinningBuffer;
                    }

                    if (descriptorTableManager.put(mesh.gpu.indexBuffer, gpuMesh.indexBuffer))
                        bindingSetItems.push_back(nvrhi::BindingSetItem::TypedBuffer_SRV(gpuMesh.indexBuffer, mesh.indexBuffer));

                    if (mesh.skinningBuffer != nullptr)
                    {
                        if (descriptorTableManager.put(mesh.skinningBufferId, gpuMesh.vertexBuffer))
                            bindingSetItems.push_back(nvrhi::BindingSetItem::RawBuffer_SRV(gpuMesh.vertexBuffer, mesh.skinningBuffer));

                        if (mesh.prevSkinningBuffer != nullptr && elementPair->second.matrixBufferChanged)
                        {
                            if (descriptorTableManager.put(mesh.prevSkinningBufferId, gpuMesh.prevVertexBuffer))
                                bindingSetItems.push_back(nvrhi::BindingSetItem::RawBuffer_SRV(gpuMesh.prevVertexBuffer, mesh.prevSkinningBuffer));
                        }
                        else
                        {
                            gpuMesh.prevVertexBuffer = gpuMesh.vertexBuffer;
                        }
                    }
                    else
                    {
                        if (descriptorTableManager.put(mesh.gpu.vertexBuffer, gpuMesh.vertexBuffer))
                            bindingSetItems.push_back(nvrhi::BindingSetItem::RawBuffer_SRV(gpuMesh.vertexBuffer, mesh.vertexBuffer));

                        gpuMesh.prevVertexBuffer = gpuMesh.vertexBuffer;
                    }
                }

                if (groupPair->second.handle == nullptr || elementPair->second.matrixBufferChanged)
                {
                    if (elementPair->second.matrixBuffer != nullptr)
                    {
                        groupPair->second.handle = bridge.device.nvrhi->createAccelStruct(groupPair->second.desc
                            .setBuildFlags(nvrhi::rt::AccelStructBuildFlags::PreferFastBuild));

                        nvrhi::utils::BuildBottomLevelAccelStruct(bridge.commandList, groupPair->second.handle, groupPair->second.desc);
                    }
                    else
                    {
                        auto& handle = model.accelStructs[groupPair->first];
                        if (!handle)
                        {
                            handle = bridge.device.nvrhi->createAccelStruct(groupPair->second.desc
                                .setBuildFlags(nvrhi::rt::AccelStructBuildFlags::PreferFastTrace));

                            nvrhi::utils::BuildBottomLevelAccelStruct(bridge.commandList, handle, groupPair->second.desc);
                        }

                        groupPair->second.handle = handle;
                    }
                }

                ++groupPair;
            }

            elementPair->second.matrixBufferChanged = false;

            if (elementPair->second.groups.empty())
                elementPair = model.elements.erase(elementPair);
            else
                ++elementPair;
        }
    }

    POP_PROFILER();
    PUSH_PROFILER("Write Descriptor Table");

    const uint32_t descriptorTableCapacity = nextPowerOfTwo(descriptorTableManager.getCapacity());

    if (descriptorTable->getCapacity() < descriptorTableCapacity)
        bridge.device.nvrhi->resizeDescriptorTable(descriptorTable, descriptorTableCapacity);

    for (auto& bindingSetItem : bindingSetItems)
        bridge.device.nvrhi->writeDescriptorTable(descriptorTable, bindingSetItem);

    POP_PROFILER();
    PUSH_PROFILER("Create GPU Instances");

    instanceDescs.clear();
    instanceBufferVec.clear();

    for (auto& instance : instances)
    {
        const auto modelPair = models.find(instance.model);
        if (modelPair == models.end())
            continue;

        const auto elementPair = modelPair->second.elements.find(instance.element);
        if (elementPair == modelPair->second.elements.end())
            continue;

        for (auto& groupPair : elementPair->second.groups)
        {
            auto& desc = instanceDescs.emplace_back();

            memcpy(desc.transform, instance.transform, sizeof(desc.transform));
            instanceBufferVec.push_back(instance.gpu);

            desc.instanceID = groupPair.second.indexInBuffer;
            desc.instanceMask = groupPair.first;
            desc.instanceContributionToHitGroupIndex = groupPair.second.indexInBuffer * 2;
            desc.bottomLevelAS = groupPair.second.handle;
        }
    }

    POP_PROFILER();

    if (instanceDescs.empty())
        return;

    PUSH_PROFILER("Build Top Level Acceleration Structure");

    auto topLevelAccelStruct = bridge.device.nvrhi->createAccelStruct(nvrhi::rt::AccelStructDesc()
        .setIsTopLevel(true)
        .setTrackLiveness(true)
        .setTopLevelMaxInstances(instanceDescs.size()));

    bridge.commandList->buildTopLevelAccelStruct(topLevelAccelStruct, instanceDescs.data(), instanceDescs.size());

    POP_PROFILER();

    if (!output)
    {
        auto textureDesc = colorAttachment->getDesc();
        output = bridge.device.nvrhi->createTexture(textureDesc
            .setFormat(nvrhi::Format::RGBA16_FLOAT)
            .setIsRenderTarget(false)
            .setInitialState(nvrhi::ResourceStates::UnorderedAccess)
            .setUseClearValue(false)
            .setIsUAV(true));

        upscaler->validate({ bridge, output });
    }

    PUSH_PROFILER("Create Binding Set");

    bridge.vsConstants.writeBuffer(bridge.commandList, bridge.vsConstantBuffer);
    bridge.psConstants.writeBuffer(bridge.commandList, bridge.psConstantBuffer);

    upscaler->getJitterOffset(rtConstants.currentFrame, rtConstants.jitterX, rtConstants.jitterY);

    bridge.commandList->writeBuffer(rtConstantBuffer, &rtConstants, sizeof(rtConstants));

    memcpy(rtConstants.prevProj, bridge.vsConstants.c[0], sizeof(rtConstants.prevProj));
    memcpy(rtConstants.prevView, bridge.vsConstants.c[4], sizeof(rtConstants.prevView));

    createUploadBuffer(bridge, meshBufferVec, meshBuffer);
    createUploadBuffer(bridge, materialBufferVec, materialBuffer);
    createUploadBuffer(bridge, instanceBufferVec, instanceBuffer);

    if (!blueNoise)
    {
        std::unique_ptr<uint8_t[]> ddsData;
        std::vector<D3D12_SUBRESOURCE_DATA> subResources;

        const HRESULT result = DirectX::LoadDDSTextureFromMemory(
            bridge.device.nvrhi, 
            BLUE_NOISE, 
            BLUE_NOISE_SIZE, 
            std::addressof(blueNoise),
            subResources);

        assert(result == S_OK);

        bridge.openCommandListForCopy();
        bridge.commandListForCopy->writeTexture(blueNoise, 0, 0, subResources[0].pData, subResources[0].RowPitch, subResources[0].SlicePitch);
    }

    auto bindingSet = bridge.device.nvrhi->createBindingSet(nvrhi::BindingSetDesc()
        .addItem(nvrhi::BindingSetItem::ConstantBuffer(0, bridge.vsConstantBuffer))
        .addItem(nvrhi::BindingSetItem::ConstantBuffer(1, bridge.psConstantBuffer))
        .addItem(nvrhi::BindingSetItem::ConstantBuffer(2, rtConstantBuffer))

        .addItem(nvrhi::BindingSetItem::RayTracingAccelStruct(0, topLevelAccelStruct))
        .addItem(nvrhi::BindingSetItem::StructuredBuffer_SRV(1, materialBuffer))
        .addItem(nvrhi::BindingSetItem::StructuredBuffer_SRV(2, meshBuffer))
        .addItem(nvrhi::BindingSetItem::StructuredBuffer_SRV(3, instanceBuffer))

        .addItem(nvrhi::BindingSetItem::Texture_UAV(0, upscaler->position))
        .addItem(nvrhi::BindingSetItem::Texture_UAV(1, upscaler->depth.getCurrent(rtConstants.currentFrame)))
        .addItem(nvrhi::BindingSetItem::Texture_UAV(2, upscaler->motionVector))
        .addItem(nvrhi::BindingSetItem::Texture_UAV(3, upscaler->normal.getCurrent(rtConstants.currentFrame)))
        .addItem(nvrhi::BindingSetItem::Texture_UAV(4, upscaler->texCoord))
        .addItem(nvrhi::BindingSetItem::Texture_UAV(5, upscaler->color))
        .addItem(nvrhi::BindingSetItem::Texture_UAV(6, upscaler->shader))
        .addItem(nvrhi::BindingSetItem::Texture_UAV(7, upscaler->globalIllumination.getCurrent(rtConstants.currentFrame)))
        .addItem(nvrhi::BindingSetItem::Texture_UAV(8, upscaler->shadow))
        .addItem(nvrhi::BindingSetItem::Texture_UAV(9, upscaler->reflection))
        .addItem(nvrhi::BindingSetItem::Texture_UAV(10, upscaler->refraction))
        .addItem(nvrhi::BindingSetItem::Texture_UAV(11, upscaler->composite))

        .addItem(nvrhi::BindingSetItem::Texture_SRV(4, blueNoise))
        .addItem(nvrhi::BindingSetItem::Texture_SRV(5, upscaler->depth.getPrevious(rtConstants.currentFrame)))
        .addItem(nvrhi::BindingSetItem::Texture_SRV(6, upscaler->normal.getPrevious(rtConstants.currentFrame)))
        .addItem(nvrhi::BindingSetItem::Texture_SRV(7, upscaler->globalIllumination.getPrevious(rtConstants.currentFrame)))

        .addItem(nvrhi::BindingSetItem::Sampler(0, linearRepeatSampler)), bindingLayout);

    POP_PROFILER();
    PUSH_PROFILER("Dispatch Rays");

    auto raytracingState = nvrhi::rt::State()
        .setShaderTable(shaderTable)
        .addBindingSet(bindingSet)
        .addBindingSet(descriptorTable);

    auto dispatchRaysArgs = nvrhi::rt::DispatchRaysArguments()
        .setWidth(upscaler->width)
        .setHeight(upscaler->height);

    auto dispatchRays = [&](const char* rayGenerationShader)
    {
        shaderTable->setRayGenerationShader(rayGenerationShader);
        bridge.commandList->setRayTracingState(raytracingState);
        bridge.commandList->dispatchRays(dispatchRaysArgs);
    };

    bridge.commandList->clearTextureFloat(upscaler->depth.getCurrent(rtConstants.currentFrame), nvrhi::TextureSubresourceSet(), nvrhi::Color(1));
    bridge.commandList->clearTextureFloat(upscaler->motionVector, nvrhi::TextureSubresourceSet(), nvrhi::Color(0));
    bridge.commandList->clearTextureUInt(upscaler->shader, nvrhi::TextureSubresourceSet(), 0);
    bridge.commandList->clearTextureFloat(upscaler->composite, nvrhi::TextureSubresourceSet(), nvrhi::Color(1));

    if (msg->resetAccumulation != 0)
        bridge.commandList->clearTextureFloat(upscaler->globalIllumination.getPrevious(rtConstants.currentFrame), nvrhi::TextureSubresourceSet(), nvrhi::Color(0));

    bridge.commandList->setTextureState(upscaler->depth.getCurrent(rtConstants.currentFrame), nvrhi::TextureSubresourceSet(), nvrhi::ResourceStates::UnorderedAccess);
    bridge.commandList->setTextureState(upscaler->motionVector, nvrhi::TextureSubresourceSet(), nvrhi::ResourceStates::UnorderedAccess);
    bridge.commandList->setTextureState(upscaler->shader, nvrhi::TextureSubresourceSet(), nvrhi::ResourceStates::UnorderedAccess);

    dispatchRays("PrimaryRayGeneration");

    bridge.commandList->setTextureState(upscaler->position, nvrhi::TextureSubresourceSet(), nvrhi::ResourceStates::UnorderedAccess);
    bridge.commandList->setTextureState(upscaler->depth.getCurrent(rtConstants.currentFrame), nvrhi::TextureSubresourceSet(), nvrhi::ResourceStates::UnorderedAccess);
    bridge.commandList->setTextureState(upscaler->motionVector, nvrhi::TextureSubresourceSet(), nvrhi::ResourceStates::UnorderedAccess);
    bridge.commandList->setTextureState(upscaler->normal.getCurrent(rtConstants.currentFrame), nvrhi::TextureSubresourceSet(), nvrhi::ResourceStates::UnorderedAccess);
    bridge.commandList->setTextureState(upscaler->texCoord, nvrhi::TextureSubresourceSet(), nvrhi::ResourceStates::UnorderedAccess);
    bridge.commandList->setTextureState(upscaler->color, nvrhi::TextureSubresourceSet(), nvrhi::ResourceStates::UnorderedAccess);
    bridge.commandList->setTextureState(upscaler->shader, nvrhi::TextureSubresourceSet(), nvrhi::ResourceStates::UnorderedAccess);

    dispatchRays("GlobalIlluminationRayGeneration");
    dispatchRays("ShadowRayGeneration");
    dispatchRays("ReflectionRayGeneration");
    dispatchRays("RefractionRayGeneration");

    bridge.commandList->setTextureState(upscaler->globalIllumination.getCurrent(rtConstants.currentFrame), nvrhi::TextureSubresourceSet(), nvrhi::ResourceStates::UnorderedAccess);
    bridge.commandList->setTextureState(upscaler->shadow, nvrhi::TextureSubresourceSet(), nvrhi::ResourceStates::UnorderedAccess);
    bridge.commandList->setTextureState(upscaler->reflection, nvrhi::TextureSubresourceSet(), nvrhi::ResourceStates::UnorderedAccess);
    bridge.commandList->setTextureState(upscaler->refraction, nvrhi::TextureSubresourceSet(), nvrhi::ResourceStates::UnorderedAccess);
    bridge.commandList->setTextureState(upscaler->composite, nvrhi::TextureSubresourceSet(), nvrhi::ResourceStates::UnorderedAccess);

    dispatchRays("CompositeRayGeneration");

    POP_PROFILER();
    PUSH_PROFILER("Copy Raytracing Composite");

    if (!copyPipeline || !copyFramebuffer || copyFramebuffer->getDesc().colorAttachments[0].texture != bridge.framebufferDesc.colorAttachments[0].texture)
    {
        copyFramebuffer = bridge.device.nvrhi->createFramebuffer(bridge.framebufferDesc);

        copyPipeline = bridge.device.nvrhi->createGraphicsPipeline(nvrhi::GraphicsPipelineDesc()
            .setVertexShader(copyVertexShader)
            .setPixelShader(copyPixelShader)
            .setPrimType(nvrhi::PrimitiveType::TriangleList)
            .setRenderState(nvrhi::RenderState()
                .setDepthStencilState(nvrhi::DepthStencilState()
                    .enableDepthTest()
                    .enableDepthWrite()
                    .setDepthFunc(nvrhi::ComparisonFunc::Always)
                    .disableStencil())
                .setRasterState(nvrhi::RasterState()
                    .setCullNone()))
            .addBindingLayout(copyBindingLayout), copyFramebuffer);
    }

    upscaler->evaluate({ bridge, (size_t)rtConstants.currentFrame, rtConstants.jitterX, rtConstants.jitterY, msg->resetAccumulation ? true : false });

    auto copyBindingSet = bridge.device.nvrhi->createBindingSet(nvrhi::BindingSetDesc()
        .addItem(nvrhi::BindingSetItem::Texture_SRV(0, output))
        .addItem(nvrhi::BindingSetItem::Texture_SRV(1, upscaler->depth.getCurrent(rtConstants.currentFrame)))
        .addItem(nvrhi::BindingSetItem::Sampler(0, pointClampSampler)), copyBindingLayout);

    bridge.commandList->setGraphicsState(nvrhi::GraphicsState()
        .setPipeline(copyPipeline)
        .addBindingSet(copyBindingSet)
        .setFramebuffer(copyFramebuffer)
        .setViewport(nvrhi::ViewportState()
            .addViewportAndScissorRect(nvrhi::Viewport(
                (float)output->getDesc().width,
                (float)output->getDesc().height))));

    bridge.commandList->draw(copyDrawArguments);

    rtConstants.currentFrame += 1;
    instances.clear();

    POP_PROFILER();
}