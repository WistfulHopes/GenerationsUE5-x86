﻿#pragma once

#include "BottomLevelAS.h"
#include "Instance.h"
#include "Material.h"

class Upscaler;
struct Device;
struct Bridge;

struct RTConstants
{
    float prevProj[4][4];
    float prevView[4][4];

    float jitterX = 0.0f;
    float jitterY = 0.0f;

    int currentFrame = 0;
};

struct RaytracingBridge
{
    nvrhi::ShaderLibraryHandle shaderLibrary;

    nvrhi::BindingLayoutHandle bindingLayout;
    nvrhi::BindingLayoutHandle geometryBindlessLayout;
    nvrhi::BindingLayoutHandle textureBindlessLayout;

    RTConstants rtConstants{};
    nvrhi::BufferHandle rtConstantBuffer;

    nvrhi::BufferHandle materialBuffer;
    nvrhi::BufferHandle geometryBuffer;

    nvrhi::TextureHandle output;
    std::unique_ptr<Upscaler> upscaler;

    nvrhi::DescriptorTableHandle geometryDescriptorTable;
    nvrhi::DescriptorTableHandle textureDescriptorTable;

    nvrhi::SamplerHandle linearRepeatSampler;

    nvrhi::rt::PipelineHandle pipeline;
    nvrhi::rt::ShaderTableHandle shaderTable;

    std::unordered_map<unsigned int, Material> materials;
    std::unordered_map<unsigned int, BottomLevelAS> bottomLevelAccelStructs;
    std::vector<Instance> instances;

    nvrhi::ShaderHandle skinningShader;
    nvrhi::BindingLayoutHandle skinningBindingLayout;
    nvrhi::BufferHandle skinningConstantBuffer;
    nvrhi::ComputePipelineHandle skinningPipeline;

    nvrhi::SamplerHandle pointClampSampler;
    nvrhi::BindingLayoutHandle copyBindingLayout;
    nvrhi::ShaderHandle copyVertexShader;
    nvrhi::ShaderHandle copyPixelShader;
    nvrhi::FramebufferHandle copyFramebuffer;
    nvrhi::GraphicsPipelineHandle copyPipeline;
    nvrhi::DrawArguments copyDrawArguments;

    std::vector<std::pair<unsigned int, unsigned int>> pendingReleases;

    RaytracingBridge(const Device& device, const std::string& directoryPath);
    ~RaytracingBridge();

    void procMsgCreateGeometry(Bridge& bridge);
    void procMsgCreateBottomLevelAS(Bridge& bridge);
    void procMsgCreateInstance(Bridge& bridge);
    void procMsgCreateMaterial(Bridge& bridge);
    void procMsgReleaseInstanceInfo(Bridge& bridge);
    void procMsgNotifySceneTraversed(Bridge& bridge);
};
