#include "Device.h"

#include "DxgiConverter.h"
#include "Message.h"
#include "FVFVertexShader.h"
#include "FVFPixelShader.h"

void Device::createBuffer(
    D3D12_HEAP_TYPE type,
    uint32_t dataSize,
    D3D12_RESOURCE_FLAGS flags,
    D3D12_RESOURCE_STATES initialState,
    ComPtr<D3D12MA::Allocation>& allocation) const
{
    D3D12MA::ALLOCATION_DESC allocDesc{};
    allocDesc.HeapType = type;

    const auto resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(dataSize, flags);

    assert(allocation == nullptr);

    const HRESULT hr = m_allocator->CreateResource(
        &allocDesc,
        &resourceDesc,
        initialState,
        nullptr,
        allocation.GetAddressOf(),
        IID_ID3D12Resource,
        nullptr);

    assert(SUCCEEDED(hr) && allocation != nullptr);
}

void Device::writeBuffer(
    const uint8_t* memory, 
    uint32_t offset, 
    uint32_t dataSize, 
    ID3D12Resource* dstResource)
{
    if (m_uploadBufferOffset + dataSize <= UPLOAD_BUFFER_SIZE && m_uploadBuffers[m_frame].size() > m_uploadBufferIndex &&
        m_uploadBuffers[m_frame][m_uploadBufferIndex].allocation != nullptr)
    {
        const auto& uploadBuffer = m_uploadBuffers[m_frame][m_uploadBufferIndex];

        memcpy(uploadBuffer.memory + m_uploadBufferOffset, memory, dataSize);

        m_copyCommandLists[m_frame].open();
        m_copyCommandLists[m_frame].getUnderlyingCommandList()->CopyBufferRegion(
            dstResource,
            offset,
            uploadBuffer.allocation->GetResource(),
            m_uploadBufferOffset,
            dataSize);

        m_uploadBufferOffset += dataSize;
    }
    else
    {
        auto& uploadBuffer = m_tempBuffers[m_frame].emplace_back();
        createBuffer(D3D12_HEAP_TYPE_UPLOAD, dataSize, D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_GENERIC_READ, uploadBuffer);

        void* mappedData = nullptr;
        const HRESULT hr = uploadBuffer->GetResource()->Map(0, nullptr, &mappedData);

        assert(SUCCEEDED(hr) && mappedData != nullptr);

        memcpy(mappedData, memory, dataSize);

        uploadBuffer->GetResource()->Unmap(0, nullptr);

        m_copyCommandLists[m_frame].open();
        m_copyCommandLists[m_frame].getUnderlyingCommandList()->CopyBufferRegion(
            dstResource,
            offset,
            uploadBuffer->GetResource(),
            0,
            dataSize);
    }
}

D3D12_GPU_VIRTUAL_ADDRESS Device::makeBuffer(const void* memory, uint32_t dataSize, uint32_t dataAlignment)
{
    m_uploadBufferOffset = (m_uploadBufferOffset + dataAlignment - 1) & ~(dataAlignment - 1);

    if (m_uploadBufferOffset + dataSize > UPLOAD_BUFFER_SIZE)
    {
        m_uploadBufferOffset = 0;
        ++m_uploadBufferIndex;
    }

    if (m_uploadBuffers[m_frame].size() <= m_uploadBufferIndex)
        m_uploadBuffers[m_frame].resize(m_uploadBufferIndex + 1);

    auto& uploadBuffer = m_uploadBuffers[m_frame][m_uploadBufferIndex];
    if (uploadBuffer.allocation == nullptr)
    {
        D3D12MA::ALLOCATION_DESC allocDesc{};
        allocDesc.HeapType = D3D12_HEAP_TYPE_UPLOAD;

        const auto resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(UPLOAD_BUFFER_SIZE);

        HRESULT hr = m_allocator->CreateResource(
            &allocDesc,
            &resourceDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            uploadBuffer.allocation.GetAddressOf(),
            IID_ID3D12Resource,
            nullptr);

        assert(SUCCEEDED(hr) && uploadBuffer.allocation != nullptr);

        hr = uploadBuffer.allocation->GetResource()->Map(
            0,
            nullptr,
            reinterpret_cast<void**>(&uploadBuffer.memory));

        assert(SUCCEEDED(hr) && uploadBuffer.memory != nullptr);
    }

    memcpy(uploadBuffer.memory + m_uploadBufferOffset, memory, dataSize);
    const D3D12_GPU_VIRTUAL_ADDRESS result = uploadBuffer.allocation->GetResource()->GetGPUVirtualAddress() + m_uploadBufferOffset;
    m_uploadBufferOffset += dataSize;

    return result;
}

void Device::setPrimitiveType(D3DPRIMITIVETYPE primitiveType)
{
    D3D12_PRIMITIVE_TOPOLOGY primitiveTopology;
    D3D12_PRIMITIVE_TOPOLOGY_TYPE primitiveTopologyType;

    switch (primitiveType)
    {
    case D3DPT_POINTLIST:
        primitiveTopology = D3D_PRIMITIVE_TOPOLOGY_POINTLIST;
        primitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
        break;

    case D3DPT_LINELIST:
        primitiveTopology = D3D_PRIMITIVE_TOPOLOGY_LINELIST;
        primitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
        break;

    case D3DPT_LINESTRIP:
        primitiveTopology = D3D_PRIMITIVE_TOPOLOGY_LINESTRIP;
        primitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
        break;

    case D3DPT_TRIANGLELIST:
        primitiveTopology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
        primitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        break;

    case D3DPT_TRIANGLESTRIP:
        primitiveTopology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
        primitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        break;

    default:
        assert(false);
        break;
    }

    if (m_primitiveTopology != primitiveTopology)
        m_dirtyFlags |= DIRTY_FLAG_PRIMITIVE_TOPOLOGY;

    if (m_pipelineDesc.PrimitiveTopologyType != primitiveTopologyType)
        m_dirtyFlags |= DIRTY_FLAG_PIPELINE_DESC;

    m_primitiveTopology = primitiveTopology;
    m_pipelineDesc.PrimitiveTopologyType = primitiveTopologyType;
}

void Device::flushGraphicsState()
{
    if (m_dirtyFlags & DIRTY_FLAG_RENDER_TARGET_AND_DEPTH_STENCIL)
    {
        m_graphicsCommandLists[m_frame].getUnderlyingCommandList()->OMSetRenderTargets(
            m_renderTargetView.ptr != NULL ? 1 : 0,
            m_renderTargetView.ptr != NULL ? &m_renderTargetView : nullptr,
            FALSE,
            m_depthStencilView.ptr != NULL ? &m_depthStencilView : nullptr);
    }

    if (m_dirtyFlags & DIRTY_FLAG_VERTEX_DECLARATION)
    {
        const D3D12_INPUT_ELEMENT_DESC* inputElements = nullptr;
        uint32_t inputElementsSize = 0;

        if (m_vertexDeclarationId != NULL)
        {
            const auto& vertexDeclaration = m_vertexDeclarations[m_vertexDeclarationId];
            inputElements = &vertexDeclaration.inputElements[vertexDeclaration.inputElementsSize * m_instancing];
            inputElementsSize = vertexDeclaration.inputElementsSize;
        }

        if (m_pipelineDesc.InputLayout.pInputElementDescs != inputElements || 
            m_pipelineDesc.InputLayout.NumElements != inputElementsSize)
        {
            m_dirtyFlags |= DIRTY_FLAG_PIPELINE_DESC;
        }

        m_pipelineDesc.InputLayout.pInputElementDescs = inputElements;
        m_pipelineDesc.InputLayout.NumElements = inputElementsSize;
    }

    if (m_dirtyFlags & DIRTY_FLAG_PIPELINE_DESC)
    {
        const XXH64_hash_t pipelineHash = XXH3_64bits(&m_pipelineDesc, sizeof(m_pipelineDesc));
        auto& pipeline = m_pipelines[pipelineHash];
        if (!pipeline)
        {
            const HRESULT hr = m_device->CreateGraphicsPipelineState(&m_pipelineDesc, IID_PPV_ARGS(pipeline.GetAddressOf()));
            assert(SUCCEEDED(hr));
        }
        m_graphicsCommandLists[m_frame].getUnderlyingCommandList()->SetPipelineState(pipeline.Get());
    }

    if (m_dirtyFlags & DIRTY_FLAG_GLOBALS_VS)
    {
        m_graphicsCommandLists[m_frame].getUnderlyingCommandList()
            ->SetGraphicsRootConstantBufferView(0, makeBuffer(&m_globalsVS, sizeof(m_globalsVS), 0x100));
    }

    if (m_dirtyFlags & DIRTY_FLAG_SAMPLER_DESC)
    {
        for (uint32_t i = m_samplerDescsFirst; i <= m_samplerDescsLast; i++)
        {
            const XXH64_hash_t samplerHash = XXH3_64bits(&m_samplerDescs[i], sizeof(m_samplerDescs[i]));
            auto& sampler = m_samplers[samplerHash];
            if (sampler == NULL)
            {
                sampler = m_samplerDescriptorHeap.allocate();

                m_device->CreateSampler(
                    &m_samplerDescs[i],
                    m_samplerDescriptorHeap.getCpuHandle(sampler));
            }
            if (m_globalsPS.samplerIndices[i] != sampler)
                m_dirtyFlags |= DIRTY_FLAG_GLOBALS_PS;

            m_globalsPS.samplerIndices[i] = sampler;
        }
    }

    if (m_dirtyFlags & DIRTY_FLAG_GLOBALS_PS)
    {
        m_graphicsCommandLists[m_frame].getUnderlyingCommandList()
            ->SetGraphicsRootConstantBufferView(1, makeBuffer(&m_globalsPS, sizeof(m_globalsPS), 0x100));
    }

    if (m_dirtyFlags & DIRTY_FLAG_VIEWPORT)
        m_graphicsCommandLists[m_frame].getUnderlyingCommandList()->RSSetViewports(1, &m_viewport);

    if (m_dirtyFlags & DIRTY_FLAG_SCISSOR_RECT)
        m_graphicsCommandLists[m_frame].getUnderlyingCommandList()->RSSetScissorRects(1, &m_scissorRect);

    if (m_dirtyFlags & DIRTY_FLAG_VERTEX_BUFFER_VIEWS)
    {
        UINT startSlot;
        UINT numViews;

        if (m_vertexBufferViewsFirst > m_vertexBufferViewsLast)
        {
            startSlot = 0;
            numViews = _countof(m_vertexBufferViews);
        }
        else
        {
            startSlot = m_vertexBufferViewsFirst;
            numViews = m_vertexBufferViewsLast - m_vertexBufferViewsFirst + 1;
        }

        m_graphicsCommandLists[m_frame].getUnderlyingCommandList()->IASetVertexBuffers(startSlot, numViews, m_vertexBufferViews);
    }

    if (m_dirtyFlags & DIRTY_FLAG_INDEX_BUFFER_VIEW)
        m_graphicsCommandLists[m_frame].getUnderlyingCommandList()->IASetIndexBuffer(&m_indexBufferView);

    if (m_dirtyFlags & DIRTY_FLAG_PRIMITIVE_TOPOLOGY)
        m_graphicsCommandLists[m_frame].getUnderlyingCommandList()->IASetPrimitiveTopology(m_primitiveTopology);

    m_graphicsCommandLists[m_frame].commitBarriers();

    m_dirtyFlags = 0;

    m_vertexBufferViewsFirst = ~0;
    m_vertexBufferViewsLast = 0;

    m_samplerDescsFirst = ~0;
    m_samplerDescsLast = 0;
}

void Device::procMsgPadding()
{
    const auto& message = m_messageReceiver.getMessage<MsgPadding>();
}

void Device::procMsgCreateSwapChain()
{
    const auto& message = m_messageReceiver.getMessage<MsgCreateSwapChain>();

    m_swapChain.procMsgCreateSwapChain(*this, message);
    m_swapChainTextureId = message.textureId;

    if (m_textures.size() <= m_swapChainTextureId)
        m_textures.resize(m_swapChainTextureId + 1);

    m_textures[m_swapChainTextureId] = m_swapChain.getTexture();
    m_graphicsCommandLists[m_frame].transitionBarrier(m_swapChain.getResource(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_RENDER_TARGET);
}

void Device::procMsgSetRenderTarget()
{
    const auto& message = m_messageReceiver.getMessage<MsgSetRenderTarget>();

    assert(message.renderTargetIndex == 0);

    if (message.textureId != NULL)
    {
        auto& texture = m_textures[message.textureId];

        if (texture.allocation != nullptr)
            m_graphicsCommandLists[m_frame].transitionBarrier(texture.allocation->GetResource(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_RENDER_TARGET);

        if (texture.rtvIndex == NULL)
        {
            texture.rtvIndex = m_rtvDescriptorHeap.allocate();

            D3D12_RENDER_TARGET_VIEW_DESC rtvDesc;
            rtvDesc.Format = texture.format;
            rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
            rtvDesc.Texture2D.MipSlice = 0;
            rtvDesc.Texture2D.PlaneSlice = 0;

            m_device->CreateRenderTargetView(
                texture.allocation->GetResource(),
                &rtvDesc,
                m_rtvDescriptorHeap.getCpuHandle(texture.rtvIndex));
        }

        const auto renderTargetView = m_rtvDescriptorHeap.getCpuHandle(texture.rtvIndex);

        if (m_renderTargetView.ptr != renderTargetView.ptr)
            m_dirtyFlags |= DIRTY_FLAG_RENDER_TARGET_AND_DEPTH_STENCIL;

        if (m_pipelineDesc.NumRenderTargets != 1 || m_pipelineDesc.RTVFormats[0] != texture.format)
            m_dirtyFlags |= DIRTY_FLAG_PIPELINE_DESC;

        m_renderTargetView = renderTargetView;
        m_pipelineDesc.NumRenderTargets = 1;
        m_pipelineDesc.RTVFormats[0] = texture.format;
    }
    else
    {
        if (m_renderTargetView.ptr != NULL)
            m_dirtyFlags |= DIRTY_FLAG_RENDER_TARGET_AND_DEPTH_STENCIL;

        if (m_pipelineDesc.NumRenderTargets != 0 || m_pipelineDesc.RTVFormats[0] != DXGI_FORMAT_UNKNOWN)
            m_dirtyFlags |= DIRTY_FLAG_PIPELINE_DESC;

        m_renderTargetView.ptr = NULL;
        m_pipelineDesc.NumRenderTargets = 0;
        m_pipelineDesc.RTVFormats[0] = DXGI_FORMAT_UNKNOWN;
    }
}

static void addMissingInputElement(
    std::vector<D3D12_INPUT_ELEMENT_DESC>& inputElements,
    const char* semanticName, 
    uint32_t semanticIndex,
    DXGI_FORMAT format)
{
    bool foundAny = false;

    for (const auto& inputElement : inputElements)
    {
        if (strcmp(inputElement.SemanticName, semanticName) == 0 && 
            inputElement.SemanticIndex == semanticIndex)
        {
            foundAny = true;
            break;
        }
    }

    if (!foundAny)
    {
        inputElements.push_back({
            semanticName,
            semanticIndex,
            format,
            0,
            0,
            D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
            0 });
    }
}

void Device::procMsgCreateVertexDeclaration()
{
    const auto& message = m_messageReceiver.getMessage<MsgCreateVertexDeclaration>();

    std::vector<D3D12_INPUT_ELEMENT_DESC> inputElements;

    const D3DVERTEXELEMENT9* vertexElement = reinterpret_cast<const D3DVERTEXELEMENT9*>(message.data);

    while (vertexElement->Stream != 0xFF && vertexElement->Type != D3DDECLTYPE_UNUSED)
    {
        auto& inputElement = inputElements.emplace_back();

        inputElement.SemanticName = DxgiConverter::convertDeclUsage(static_cast<D3DDECLUSAGE>(vertexElement->Usage));
        inputElement.SemanticIndex = static_cast<UINT>(vertexElement->UsageIndex);
        inputElement.Format = DxgiConverter::convertDeclType(static_cast<D3DDECLTYPE>(vertexElement->Type));
        inputElement.InputSlot = static_cast<UINT>(vertexElement->Stream);
        inputElement.AlignedByteOffset = static_cast<UINT>(vertexElement->Offset);
        inputElement.InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
        inputElement.InstanceDataStepRate = 0;

        ++vertexElement;
    }

    addMissingInputElement(inputElements, "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT);
    addMissingInputElement(inputElements, "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT);
    addMissingInputElement(inputElements, "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT);
    addMissingInputElement(inputElements, "BINORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT);
    addMissingInputElement(inputElements, "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT);
    addMissingInputElement(inputElements, "TEXCOORD", 1, DXGI_FORMAT_R32G32_FLOAT);
    addMissingInputElement(inputElements, "TEXCOORD", 2, DXGI_FORMAT_R32G32_FLOAT);
    addMissingInputElement(inputElements, "TEXCOORD", 3, DXGI_FORMAT_R32G32_FLOAT);
    addMissingInputElement(inputElements, "COLOR", 0, DXGI_FORMAT_B8G8R8A8_UNORM);
    addMissingInputElement(inputElements, "BLENDWEIGHT", 0, DXGI_FORMAT_R8G8B8A8_UNORM);
    addMissingInputElement(inputElements, "BLENDINDICES", 0, DXGI_FORMAT_R8G8B8A8_UINT);

    assert(inputElements.size() <= 32);

    if (m_vertexDeclarations.size() <= message.vertexDeclarationId)
        m_vertexDeclarations.resize(message.vertexDeclarationId + 1);

    auto& vertexDeclaration = m_vertexDeclarations[message.vertexDeclarationId];

    vertexDeclaration.inputElements = std::make_unique<D3D12_INPUT_ELEMENT_DESC[]>(inputElements.size() * 2);
    vertexDeclaration.inputElementsSize = static_cast<uint32_t>(inputElements.size());

    memcpy(vertexDeclaration.inputElements.get(), 
        inputElements.data(), inputElements.size() * sizeof(D3D12_INPUT_ELEMENT_DESC));

    // create an instanced version in the second half
    memcpy(&vertexDeclaration.inputElements[inputElements.size()], 
        inputElements.data(), inputElements.size() * sizeof(D3D12_INPUT_ELEMENT_DESC));

    for (size_t i = 0; i < inputElements.size(); i++)
    {
        auto& inputElement = vertexDeclaration.inputElements[inputElements.size() + i];
        if (inputElement.InputSlot > 0)
        {
            inputElement.InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA;
            inputElement.InstanceDataStepRate = 1;
        }
    }
}

void Device::procMsgCreatePixelShader()
{
    const auto& message = m_messageReceiver.getMessage<MsgCreatePixelShader>();

    if (m_pixelShaders.size() <= message.pixelShaderId)
        m_pixelShaders.resize(message.pixelShaderId + 1);

    auto& pixelShader = m_pixelShaders[message.pixelShaderId];

    pixelShader.byteCode = std::make_unique<uint8_t[]>(message.dataSize);
    pixelShader.byteSize = message.dataSize;

    memcpy(pixelShader.byteCode.get(), message.data, message.dataSize);
}

void Device::procMsgCreateVertexShader()
{
    const auto& message = m_messageReceiver.getMessage<MsgCreateVertexShader>();

    if (m_vertexShaders.size() <= message.vertexShaderId)
        m_vertexShaders.resize(message.vertexShaderId + 1);

    auto& vertexShader = m_vertexShaders[message.vertexShaderId];

    vertexShader.byteCode = std::make_unique<uint8_t[]>(message.dataSize);
    vertexShader.byteSize = message.dataSize;

    memcpy(vertexShader.byteCode.get(), message.data, message.dataSize);
}

static constexpr size_t GLOBALS_VS_UNUSED_CONSTANT = 196;
static constexpr size_t GLOBALS_VS_BOOLEANS = 0;

static constexpr size_t GLOBALS_PS_UNUSED_CONSTANT = 148;
static constexpr size_t GLOBALS_PS_ENABLE_ALPHA_TEST = 0;
static constexpr size_t GLOBALS_PS_ALPHA_THRESHOLD = 1;
static constexpr size_t GLOBALS_PS_BOOLEANS = 2;

void Device::procMsgSetRenderState()
{
    const auto& message = m_messageReceiver.getMessage<MsgSetRenderState>();

#define SET_PIPELINE_DESC_VALUE(dst, src) \
    if (dst != src) m_dirtyFlags |= DIRTY_FLAG_PIPELINE_DESC; \
    dst = src

    switch (message.state)
    {
    case D3DRS_ZENABLE:
        SET_PIPELINE_DESC_VALUE(m_pipelineDesc.DepthStencilState.DepthEnable, static_cast<BOOL>(message.value));
        break;

    case D3DRS_FILLMODE:
        SET_PIPELINE_DESC_VALUE(m_pipelineDesc.RasterizerState.FillMode, static_cast<D3D12_FILL_MODE>(message.value));
        break;

    case D3DRS_ZWRITEENABLE:
        SET_PIPELINE_DESC_VALUE(m_pipelineDesc.DepthStencilState.DepthWriteMask, static_cast<D3D12_DEPTH_WRITE_MASK>(message.value));
        break;

    case D3DRS_ALPHATESTENABLE:
    {
        auto& enableAlphaTest = reinterpret_cast<uint32_t&>(
            m_globalsPS.floatConstants[GLOBALS_PS_UNUSED_CONSTANT][GLOBALS_PS_ENABLE_ALPHA_TEST]);

        if (enableAlphaTest != message.value)
            m_dirtyFlags |= DIRTY_FLAG_GLOBALS_PS;

        enableAlphaTest = message.value;
        break;
    }

    case D3DRS_SRCBLEND:
        SET_PIPELINE_DESC_VALUE(m_pipelineDesc.BlendState.RenderTarget[0].SrcBlend, static_cast<D3D12_BLEND>(message.value));
        break;

    case D3DRS_DESTBLEND:
        SET_PIPELINE_DESC_VALUE(m_pipelineDesc.BlendState.RenderTarget[0].DestBlend, static_cast<D3D12_BLEND>(message.value));
        break;

    case D3DRS_CULLMODE:
        SET_PIPELINE_DESC_VALUE(m_pipelineDesc.RasterizerState.CullMode, static_cast<D3D12_CULL_MODE>(message.value));
        break;

    case D3DRS_ZFUNC:
        SET_PIPELINE_DESC_VALUE(m_pipelineDesc.DepthStencilState.DepthFunc, static_cast<D3D12_COMPARISON_FUNC>(message.value));
        break;

    case D3DRS_ALPHAREF:
    {
        auto& alphaThreshold = m_globalsPS.floatConstants[GLOBALS_PS_UNUSED_CONSTANT][GLOBALS_PS_ALPHA_THRESHOLD];
        const float value = static_cast<float>(message.value) / 255.0f;

        if (std::abs(alphaThreshold - value) > 0.001f)
            m_dirtyFlags |= DIRTY_FLAG_GLOBALS_PS;

        alphaThreshold = value;

        break;
    }

    case D3DRS_ALPHABLENDENABLE:
        SET_PIPELINE_DESC_VALUE(m_pipelineDesc.BlendState.RenderTarget[0].BlendEnable, static_cast<BOOL>(message.value));
        break;

    case D3DRS_COLORWRITEENABLE:
        SET_PIPELINE_DESC_VALUE(m_pipelineDesc.BlendState.RenderTarget[0].RenderTargetWriteMask, static_cast<UINT8>(message.value));
        break;

    case D3DRS_BLENDOP:
        SET_PIPELINE_DESC_VALUE(m_pipelineDesc.BlendState.RenderTarget[0].BlendOp, static_cast<D3D12_BLEND_OP>(message.value));
        break;

    case D3DRS_SCISSORTESTENABLE:
        break;

    case D3DRS_SLOPESCALEDEPTHBIAS:
        SET_PIPELINE_DESC_VALUE(m_pipelineDesc.RasterizerState.SlopeScaledDepthBias, *reinterpret_cast<const float*>(&message.value));
        break;

    case D3DRS_COLORWRITEENABLE1:
        SET_PIPELINE_DESC_VALUE(m_pipelineDesc.BlendState.RenderTarget[1].RenderTargetWriteMask, static_cast<UINT8>(message.value));
        break;

    case D3DRS_COLORWRITEENABLE2:
        SET_PIPELINE_DESC_VALUE(m_pipelineDesc.BlendState.RenderTarget[2].RenderTargetWriteMask, static_cast<UINT8>(message.value));
        break;

    case D3DRS_COLORWRITEENABLE3:
        SET_PIPELINE_DESC_VALUE(m_pipelineDesc.BlendState.RenderTarget[3].RenderTargetWriteMask, static_cast<UINT8>(message.value));
        break;

    case D3DRS_DEPTHBIAS:
        SET_PIPELINE_DESC_VALUE(m_pipelineDesc.RasterizerState.DepthBias, static_cast<INT>(*reinterpret_cast<const float*>(&message.value) * (1 << 24)));
        break;

    case D3DRS_SRCBLENDALPHA:
        SET_PIPELINE_DESC_VALUE(m_pipelineDesc.BlendState.RenderTarget[0].SrcBlendAlpha, static_cast<D3D12_BLEND>(message.value));
        break;

    case D3DRS_DESTBLENDALPHA:
        SET_PIPELINE_DESC_VALUE(m_pipelineDesc.BlendState.RenderTarget[0].DestBlendAlpha, static_cast<D3D12_BLEND>(message.value));
        break;

    case D3DRS_BLENDOPALPHA:
        SET_PIPELINE_DESC_VALUE(m_pipelineDesc.BlendState.RenderTarget[0].BlendOpAlpha, static_cast<D3D12_BLEND_OP>(message.value));
        break;

    default:
        assert(false);
        break;
    }

#undef SET_PIPELINE_DESC_VALUE
}

void Device::procMsgCreateTexture()
{
    const auto& message = m_messageReceiver.getMessage<MsgCreateTexture>();

    D3D12MA::ALLOCATION_DESC allocDesc{};
    allocDesc.HeapType = D3D12_HEAP_TYPE_DEFAULT;

    if ((message.usage & (D3DUSAGE_RENDERTARGET | D3DUSAGE_DEPTHSTENCIL)) != 0)
        allocDesc.Flags = D3D12MA::ALLOCATION_FLAG_COMMITTED;

    D3D12_RESOURCE_DESC resourceDesc;
    resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    resourceDesc.Alignment = 0;
    resourceDesc.Width = static_cast<UINT64>(message.width);
    resourceDesc.Height = static_cast<UINT>(message.height);
    resourceDesc.DepthOrArraySize = 1;
    resourceDesc.MipLevels = static_cast<UINT16>(message.levels);
    resourceDesc.Format = DxgiConverter::convert(static_cast<D3DFORMAT>(message.format));
    resourceDesc.SampleDesc.Count = 1;
    resourceDesc.SampleDesc.Quality = 0;
    resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    resourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

    if ((message.usage & D3DUSAGE_RENDERTARGET) != 0)
        resourceDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

    if ((message.usage & D3DUSAGE_DEPTHSTENCIL) != 0)
        resourceDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

    if (m_textures.size() <= message.textureId)
        m_textures.resize(message.textureId + 1);

    auto& texture = m_textures[message.textureId];

    texture.format = resourceDesc.Format;
    texture.isShadowMap = message.format == D3DFMT_D24S8;

    constexpr float transparentColor[4]{};
    const CD3DX12_CLEAR_VALUE renderTargetClearValue(texture.format, transparentColor);
    const CD3DX12_CLEAR_VALUE depthStencilClearValue(texture.format, 1.0f, 0);

    assert(texture.allocation == nullptr);
    
    const HRESULT hr = m_allocator->CreateResource(
        &allocDesc,
        &resourceDesc,

        message.usage & D3DUSAGE_RENDERTARGET ? D3D12_RESOURCE_STATE_RENDER_TARGET :
        message.usage & D3DUSAGE_DEPTHSTENCIL ? D3D12_RESOURCE_STATE_DEPTH_WRITE : D3D12_RESOURCE_STATE_COMMON,

        message.usage & D3DUSAGE_RENDERTARGET ? &renderTargetClearValue : 
        message.usage & D3DUSAGE_DEPTHSTENCIL ? &depthStencilClearValue : nullptr,

        texture.allocation.GetAddressOf(),
        IID_ID3D12Resource,
        nullptr);

    assert(SUCCEEDED(hr) && texture.allocation != nullptr);

#ifdef _DEBUG
    wchar_t name[0x100];

    if ((message.usage & D3DUSAGE_RENDERTARGET) != 0)
        _swprintf(name, L"Render Target %d %dx%d", message.textureId, message.width, message.height);
    else if ((message.usage & D3DUSAGE_DEPTHSTENCIL) != 0)
        _swprintf(name, L"Depth Stencil %d %dx%d", message.textureId, message.width, message.height);
    else
        _swprintf(name, L"Texture %d %dx%d", message.textureId, message.width, message.height);

    texture.allocation->GetResource()->SetName(name);
#endif
}

void Device::procMsgSetTexture()
{
    const auto& message = m_messageReceiver.getMessage<MsgSetTexture>();

    bool isShadowMap = false;

    if (message.textureId != NULL)
    {
        auto& texture = m_textures[message.textureId];

        isShadowMap = texture.isShadowMap;

        if (texture.srvIndex == NULL)
        {
            texture.srvIndex = m_descriptorHeap.allocate();

            D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc;
            srvDesc.Format = DxgiConverter::makeColor(texture.format);
            srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
            srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

            srvDesc.Texture2D.MostDetailedMip = 0;
            srvDesc.Texture2D.MipLevels = static_cast<UINT>(-1);
            srvDesc.Texture2D.PlaneSlice = 0;
            srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;

            m_device->CreateShaderResourceView(
                texture.allocation->GetResource(),
                &srvDesc,
                m_descriptorHeap.getCpuHandle(texture.srvIndex));
        }

        // NOTE: texture corruption on nvidia with only pixel shader resource
        if (texture.rtvIndex != NULL)
        {
            m_graphicsCommandLists[m_frame].transitionBarrier(texture.allocation->GetResource(), 
                D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_COPY_SOURCE);
        }
        else if (texture.dsvIndex != NULL)
        {
            m_graphicsCommandLists[m_frame].transitionBarrier(texture.allocation->GetResource(),
                D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_COPY_SOURCE);
        }

        if (m_globalsPS.textureIndices[message.stage] != texture.srvIndex)
            m_dirtyFlags |= DIRTY_FLAG_GLOBALS_PS;

        m_globalsPS.textureIndices[message.stage] = texture.srvIndex;
    }
    else
    {
        if (m_globalsPS.textureIndices[message.stage] != NULL)
            m_dirtyFlags |= DIRTY_FLAG_GLOBALS_PS;

        m_globalsPS.textureIndices[message.stage] = NULL;
    }

    auto& filter = m_samplerDescs[message.stage].Filter;
    const auto value = (filter & ~0x80) | (isShadowMap ? 0x80 : 0x00); // comparison filter

    if (filter != value)
    {
        m_dirtyFlags |= DIRTY_FLAG_SAMPLER_DESC;
        m_samplerDescsFirst = std::min<uint32_t>(m_samplerDescsFirst, message.stage);
        m_samplerDescsLast = std::max<uint32_t>(m_samplerDescsLast, message.stage);
    }
    filter = static_cast<D3D12_FILTER>(value);
}

void Device::procMsgSetDepthStencilSurface()
{
    const auto& message = m_messageReceiver.getMessage<MsgSetDepthStencilSurface>();

    if (message.depthStencilSurfaceId != NULL)
    {
        auto& texture = m_textures[message.depthStencilSurfaceId];

        m_graphicsCommandLists[m_frame].transitionBarrier(texture.allocation->GetResource(),
            D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_DEPTH_WRITE);

        if (texture.dsvIndex == NULL)
        {
            texture.dsvIndex = m_dsvDescriptorHeap.allocate();

            D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc;
            dsvDesc.Format = texture.format;
            dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
            dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
            dsvDesc.Texture2D.MipSlice = 0;

            m_device->CreateDepthStencilView(
                texture.allocation->GetResource(),
                &dsvDesc,
                m_dsvDescriptorHeap.getCpuHandle(texture.dsvIndex));
        }

        const auto depthStencilView = m_dsvDescriptorHeap.getCpuHandle(texture.dsvIndex);

        if (m_depthStencilView.ptr != depthStencilView.ptr)
            m_dirtyFlags |= DIRTY_FLAG_RENDER_TARGET_AND_DEPTH_STENCIL;

        if (m_pipelineDesc.DSVFormat != texture.format)
            m_dirtyFlags |= DIRTY_FLAG_PIPELINE_DESC;

        m_depthStencilView = depthStencilView;
        m_pipelineDesc.DSVFormat = texture.format;
    }
    else
    {
        if (m_depthStencilView.ptr != NULL)
            m_dirtyFlags |= DIRTY_FLAG_RENDER_TARGET_AND_DEPTH_STENCIL;

        if (m_pipelineDesc.DSVFormat != DXGI_FORMAT_UNKNOWN)
            m_dirtyFlags |= DIRTY_FLAG_PIPELINE_DESC;

        m_depthStencilView.ptr = NULL;
        m_pipelineDesc.DSVFormat = DXGI_FORMAT_UNKNOWN;
    }
}

void Device::procMsgClear()
{
    const auto& message = m_messageReceiver.getMessage<MsgClear>();

    m_graphicsCommandLists[m_frame].commitBarriers();

    if ((message.flags & D3DCLEAR_TARGET) && m_renderTargetView.ptr != NULL)
    {
        FLOAT color[4];
        color[2] = static_cast<float>(message.color & 0xFF) / 255.0f;
        color[1] = static_cast<float>((message.color >> 8) & 0xFF) / 255.0f;
        color[0] = static_cast<float>((message.color >> 16) & 0xFF) / 255.0f;
        color[3] = static_cast<float>((message.color >> 24) & 0xFF) / 255.0f;

        m_graphicsCommandLists[m_frame].getUnderlyingCommandList()
            ->ClearRenderTargetView(m_renderTargetView, color, 0, nullptr);
    }

    if ((message.flags & (D3DCLEAR_ZBUFFER | D3DCLEAR_STENCIL)) && m_depthStencilView.ptr != NULL)
    {
        UINT clearFlag = 0;

        if (message.flags & D3DCLEAR_ZBUFFER)
            clearFlag |= D3D12_CLEAR_FLAG_DEPTH;

        if (message.flags & D3DCLEAR_STENCIL)
            clearFlag |= D3D12_CLEAR_FLAG_STENCIL;

        m_graphicsCommandLists[m_frame].getUnderlyingCommandList()
            ->ClearDepthStencilView(m_depthStencilView, static_cast<D3D12_CLEAR_FLAGS>(clearFlag), message.z, message.stencil, 0, nullptr);
    }
}

void Device::procMsgSetVertexShader()
{
    const auto& message = m_messageReceiver.getMessage<MsgSetVertexShader>();

    if (message.vertexShaderId != NULL)
    {
        const auto& vertexShader = m_vertexShaders[message.vertexShaderId];

        if (m_pipelineDesc.VS.pShaderBytecode != vertexShader.byteCode.get())
            m_dirtyFlags |= DIRTY_FLAG_PIPELINE_DESC;

        m_pipelineDesc.VS.pShaderBytecode = vertexShader.byteCode.get();
        m_pipelineDesc.VS.BytecodeLength = static_cast<SIZE_T>(vertexShader.byteSize);
    }
    else
    {
        if (m_pipelineDesc.VS.pShaderBytecode != FVFVertexShader)
            m_dirtyFlags |= DIRTY_FLAG_PIPELINE_DESC;

        m_pipelineDesc.VS.pShaderBytecode = FVFVertexShader;
        m_pipelineDesc.VS.BytecodeLength = sizeof(FVFVertexShader);
    }
}

void Device::procMsgSetPixelShader()
{
    const auto& message = m_messageReceiver.getMessage<MsgSetPixelShader>();

    if (message.pixelShaderId != NULL)
    {
        const auto& pixelShader = m_pixelShaders[message.pixelShaderId];

        if (m_pipelineDesc.PS.pShaderBytecode != pixelShader.byteCode.get())
            m_dirtyFlags |= DIRTY_FLAG_PIPELINE_DESC;

        m_pipelineDesc.PS.pShaderBytecode = pixelShader.byteCode.get();
        m_pipelineDesc.PS.BytecodeLength = static_cast<SIZE_T>(pixelShader.byteSize);
    }
    else
    {
        if (m_pipelineDesc.PS.pShaderBytecode != FVFPixelShader)
            m_dirtyFlags |= DIRTY_FLAG_PIPELINE_DESC;

        m_pipelineDesc.PS.pShaderBytecode = FVFPixelShader;
        m_pipelineDesc.PS.BytecodeLength = sizeof(FVFPixelShader);
    }
}

void Device::procMsgSetPixelShaderConstantF()
{
    const auto& message = m_messageReceiver.getMessage<MsgSetPixelShaderConstantF>();

    memcpy(m_globalsPS.floatConstants[message.startRegister], message.data, message.dataSize);
    m_dirtyFlags |= DIRTY_FLAG_GLOBALS_PS;
}

void Device::procMsgSetVertexShaderConstantF()
{
    const auto& message = m_messageReceiver.getMessage<MsgSetVertexShaderConstantF>();

    memcpy(m_globalsVS.floatConstants[message.startRegister], message.data, message.dataSize);
    m_dirtyFlags |= DIRTY_FLAG_GLOBALS_VS;
}

void Device::procMsgSetVertexShaderConstantB()
{
    const auto& message = m_messageReceiver.getMessage<MsgSetVertexShaderConstantB>();

    const uint32_t mask = 1 << message.startRegister;
    const uint32_t value = *reinterpret_cast<const BOOL*>(message.data) << message.startRegister;

    auto& booleans = reinterpret_cast<uint32_t&>(m_globalsVS.floatConstants[GLOBALS_VS_UNUSED_CONSTANT][GLOBALS_VS_BOOLEANS]);

    if ((booleans & mask) != value)
        m_dirtyFlags |= DIRTY_FLAG_GLOBALS_VS;

    booleans &= ~mask;
    booleans |= value;
}

void Device::procMsgSetSamplerState()
{
    const auto& message = m_messageReceiver.getMessage<MsgSetSamplerState>();

    bool isDirty = false;

#define SET_SAMPLER_DESC_VALUE(dst, src) \
    do { \
        if (dst != (src)) isDirty = true; \
        dst = src; \
    } \
    while (0)

    switch (message.type)
    {
    case D3DSAMP_ADDRESSU:
        SET_SAMPLER_DESC_VALUE(m_samplerDescs[message.sampler].AddressU, static_cast<D3D12_TEXTURE_ADDRESS_MODE>(message.value));
        break;

    case D3DSAMP_ADDRESSV:
        SET_SAMPLER_DESC_VALUE(m_samplerDescs[message.sampler].AddressV, static_cast<D3D12_TEXTURE_ADDRESS_MODE>(message.value));
        break;

    case D3DSAMP_ADDRESSW:
        SET_SAMPLER_DESC_VALUE(m_samplerDescs[message.sampler].AddressW, static_cast<D3D12_TEXTURE_ADDRESS_MODE>(message.value));
        break;

    case D3DSAMP_BORDERCOLOR:
        SET_SAMPLER_DESC_VALUE(m_samplerDescs[message.sampler].BorderColor[0], (static_cast<float>((message.value >> 16) & 0xFF)) / 255.0f);
        SET_SAMPLER_DESC_VALUE(m_samplerDescs[message.sampler].BorderColor[1], (static_cast<float>((message.value >> 8) & 0xFF)) / 255.0f);
        SET_SAMPLER_DESC_VALUE(m_samplerDescs[message.sampler].BorderColor[2], (static_cast<float>(message.value & 0xFF)) / 255.0f);
        SET_SAMPLER_DESC_VALUE(m_samplerDescs[message.sampler].BorderColor[3], (static_cast<float>((message.value >> 24) & 0xFF)) / 255.0f);
        break;

    case D3DSAMP_MAGFILTER:
        if (message.value == D3DTEXF_LINEAR)
            SET_SAMPLER_DESC_VALUE(m_samplerDescs[message.sampler].Filter, static_cast<D3D12_FILTER>(m_samplerDescs[message.sampler].Filter | 0x4));
        else
            SET_SAMPLER_DESC_VALUE(m_samplerDescs[message.sampler].Filter, static_cast<D3D12_FILTER>(m_samplerDescs[message.sampler].Filter & ~0x4));

        break;

    case D3DSAMP_MINFILTER:
        if (message.value == D3DTEXF_LINEAR)
            SET_SAMPLER_DESC_VALUE(m_samplerDescs[message.sampler].Filter, static_cast<D3D12_FILTER>(m_samplerDescs[message.sampler].Filter | 0x10));
        else
            SET_SAMPLER_DESC_VALUE(m_samplerDescs[message.sampler].Filter, static_cast<D3D12_FILTER>(m_samplerDescs[message.sampler].Filter & ~0x10));

        break;

    case D3DSAMP_MIPFILTER:
        if (message.value == D3DTEXF_LINEAR)
            SET_SAMPLER_DESC_VALUE(m_samplerDescs[message.sampler].Filter, static_cast<D3D12_FILTER>(m_samplerDescs[message.sampler].Filter | 0x1));
        else
            SET_SAMPLER_DESC_VALUE(m_samplerDescs[message.sampler].Filter, static_cast<D3D12_FILTER>(m_samplerDescs[message.sampler].Filter & ~0x1));

        break;
    }

#undef SET_SAMPLER_DESC_VALUE

    if (isDirty)
    {
        m_dirtyFlags |= DIRTY_FLAG_SAMPLER_DESC;
        m_samplerDescsFirst = std::min<uint32_t>(m_samplerDescsFirst, message.sampler);
        m_samplerDescsLast = std::max<uint32_t>(m_samplerDescsLast, message.sampler);
    }
}

void Device::procMsgSetViewport()
{
    const auto& message = m_messageReceiver.getMessage<MsgSetViewport>();

    D3D12_VIEWPORT viewport;
    viewport.TopLeftX = static_cast<float>(message.x);
    viewport.TopLeftY = static_cast<float>(message.y);
    viewport.Width = static_cast<float>(message.width);
    viewport.Height = static_cast<float>(message.height);
    viewport.MinDepth = message.minZ;
    viewport.MaxDepth = message.maxZ;

    if (m_viewport.TopLeftX != viewport.TopLeftX ||
        m_viewport.TopLeftY != viewport.TopLeftY ||
        m_viewport.Width != viewport.Width ||
        m_viewport.Height != viewport.Height ||
        m_viewport.MinDepth != viewport.MinDepth ||
        m_viewport.MaxDepth != viewport.MaxDepth)
    {
        m_dirtyFlags |= DIRTY_FLAG_VIEWPORT;
    }

    m_viewport = viewport;
}

void Device::procMsgSetScissorRect()
{
    const auto& message = m_messageReceiver.getMessage<MsgSetScissorRect>();

    RECT scissorRect;
    scissorRect.left = static_cast<LONG>(message.left);
    scissorRect.top = static_cast<LONG>(message.top);
    scissorRect.right = static_cast<LONG>(message.right);
    scissorRect.bottom = static_cast<LONG>(message.bottom);

    if (m_scissorRect.left != scissorRect.left ||
        m_scissorRect.top != scissorRect.top ||
        m_scissorRect.right != scissorRect.right ||
        m_scissorRect.bottom != scissorRect.bottom)
    {
        m_dirtyFlags |= DIRTY_FLAG_SCISSOR_RECT;
    }

    m_scissorRect = scissorRect;
}

void Device::procMsgSetVertexDeclaration()
{
    const auto& message = m_messageReceiver.getMessage<MsgSetVertexDeclaration>();

    if (m_vertexDeclarationId != message.vertexDeclarationId)
        m_dirtyFlags |= DIRTY_FLAG_VERTEX_DECLARATION;

    m_vertexDeclarationId = message.vertexDeclarationId;
}

void Device::procMsgDrawPrimitiveUP()
{
    const auto& message = m_messageReceiver.getMessage<MsgDrawPrimitiveUP>();

    m_vertexBufferViews[0].BufferLocation = makeBuffer(message.data, message.dataSize, 0x4);
    m_vertexBufferViews[0].SizeInBytes = message.dataSize;
    m_vertexBufferViews[0].StrideInBytes = message.vertexStreamZeroStride;

    m_dirtyFlags |= DIRTY_FLAG_VERTEX_BUFFER_VIEWS;
    m_vertexBufferViewsFirst = 0;

    setPrimitiveType(static_cast<D3DPRIMITIVETYPE>(message.primitiveType));
    flushGraphicsState();

    m_graphicsCommandLists[m_frame].getUnderlyingCommandList()
        ->DrawInstanced(message.vertexCount, m_instanceCount, 0, 0);
}

void Device::procMsgSetStreamSource()
{
    const auto& message = m_messageReceiver.getMessage<MsgSetStreamSource>();

    auto& vertexBufferView = m_vertexBufferViews[message.streamNumber];

    D3D12_GPU_VIRTUAL_ADDRESS bufferLocation;
    UINT sizeInBytes;
    UINT strideInBytes;

    if (message.streamDataId != NULL)
    {
        const auto& vertexBuffer = m_vertexBuffers[message.streamDataId];

        bufferLocation = vertexBuffer.allocation->GetResource()->GetGPUVirtualAddress() + message.offsetInBytes;
        sizeInBytes = static_cast<UINT>(vertexBuffer.byteSize - message.offsetInBytes);
        strideInBytes = static_cast<UINT>(message.stride);
    }
    else
    {
        bufferLocation = NULL;
        sizeInBytes = 0;
        strideInBytes = 0;
    }

    if (vertexBufferView.BufferLocation != bufferLocation ||
        vertexBufferView.SizeInBytes != sizeInBytes ||
        vertexBufferView.StrideInBytes != strideInBytes)
    {
        m_dirtyFlags |= DIRTY_FLAG_VERTEX_BUFFER_VIEWS;

        m_vertexBufferViewsFirst = std::min<uint32_t>(m_vertexBufferViewsFirst, message.streamNumber);
        m_vertexBufferViewsLast = std::max<uint32_t>(m_vertexBufferViewsLast, message.streamNumber);
    }

    vertexBufferView.BufferLocation = bufferLocation;
    vertexBufferView.SizeInBytes = sizeInBytes;
    vertexBufferView.StrideInBytes = strideInBytes;
}

void Device::procMsgSetIndices()
{
    const auto& message = m_messageReceiver.getMessage<MsgSetIndices>();

    D3D12_GPU_VIRTUAL_ADDRESS bufferLocation;
    UINT sizeInBytes;
    DXGI_FORMAT format;

    if (message.indexDataId != NULL)
    {
        const auto& indexBuffer = m_indexBuffers[message.indexDataId];

        bufferLocation = indexBuffer.allocation->GetResource()->GetGPUVirtualAddress();
        sizeInBytes = static_cast<UINT>(indexBuffer.byteSize);
        format = indexBuffer.format;
    }
    else
    {
        bufferLocation = NULL;
        sizeInBytes = 0;
        format = DXGI_FORMAT_UNKNOWN;
    }

    if (m_indexBufferView.BufferLocation != bufferLocation ||
        m_indexBufferView.SizeInBytes != sizeInBytes ||
        m_indexBufferView.Format != format)
    {
        m_dirtyFlags |= DIRTY_FLAG_INDEX_BUFFER_VIEW;
    }

    m_indexBufferView.BufferLocation = bufferLocation;
    m_indexBufferView.SizeInBytes = sizeInBytes;
    m_indexBufferView.Format = format;
}

void Device::procMsgPresent()
{
    const auto& message = m_messageReceiver.getMessage<MsgPresent>();
    m_shouldPresent = true;
}

void Device::procMsgCreateVertexBuffer()
{
    const auto& message = m_messageReceiver.getMessage<MsgCreateVertexBuffer>();

    if (m_vertexBuffers.size() <= message.vertexBufferId)
        m_vertexBuffers.resize(message.vertexBufferId + 1);

    auto& vertexBuffer = m_vertexBuffers[message.vertexBufferId];

    createBuffer(
        D3D12_HEAP_TYPE_DEFAULT,
        message.length,
        D3D12_RESOURCE_FLAG_NONE,
        D3D12_RESOURCE_STATE_COMMON, 
        vertexBuffer.allocation);

    vertexBuffer.byteSize = message.length;

#ifdef _DEBUG
    wchar_t name[0x100];
    _swprintf(name, L"Vertex Buffer %d", message.vertexBufferId);
    vertexBuffer.allocation->GetResource()->SetName(name);
#endif
}

void Device::procMsgWriteVertexBuffer()
{
    const auto& message = m_messageReceiver.getMessage<MsgWriteVertexBuffer>();

    writeBuffer(
        message.data,
        message.offset, 
        message.dataSize,
        m_vertexBuffers[message.vertexBufferId].allocation->GetResource());
}

void Device::procMsgCreateIndexBuffer()
{
    const auto& message = m_messageReceiver.getMessage<MsgCreateIndexBuffer>();

    if (m_indexBuffers.size() <= message.indexBufferId)
        m_indexBuffers.resize(message.indexBufferId + 1);

    auto& indexBuffer = m_indexBuffers[message.indexBufferId];

    createBuffer(
        D3D12_HEAP_TYPE_DEFAULT,
        message.length,
        D3D12_RESOURCE_FLAG_NONE,
        D3D12_RESOURCE_STATE_COMMON,
        indexBuffer.allocation);

    indexBuffer.format = DxgiConverter::convert(static_cast<D3DFORMAT>(message.format));
    indexBuffer.byteSize = message.length;

#ifdef _DEBUG
    wchar_t name[0x100];
    _swprintf(name, L"Index Buffer %d", message.indexBufferId);
    indexBuffer.allocation->GetResource()->SetName(name);
#endif
}

void Device::procMsgWriteIndexBuffer()
{
    const auto& message = m_messageReceiver.getMessage<MsgWriteIndexBuffer>();

    writeBuffer(
        message.data,
        message.offset,
        message.dataSize,
        m_indexBuffers[message.indexBufferId].allocation->GetResource());
}

void Device::procMsgWriteTexture()
{
    const auto& message = m_messageReceiver.getMessage<MsgWriteTexture>();
}

void Device::procMsgMakeTexture()
{
    const auto& message = m_messageReceiver.getMessage<MsgMakeTexture>();

    if (m_textures.size() <= message.textureId)
        m_textures.resize(message.textureId + 1);

    auto& texture = m_textures[message.textureId];

    std::vector<D3D12_SUBRESOURCE_DATA> subResources;
    bool isCubeMap = false;

    assert(texture.allocation == nullptr);

    const HRESULT hr = DirectX::LoadDDSTextureFromMemory(
        m_device.Get(),
        m_allocator.Get(),
        message.data,
        message.dataSize,
        nullptr,
        texture.allocation.GetAddressOf(),
        subResources,
        0,
        nullptr,
        &isCubeMap);

    assert(SUCCEEDED(hr) && texture.allocation != nullptr);

    auto& uploadBuffer = m_tempBuffers[m_frame].emplace_back();

    createBuffer(
        D3D12_HEAP_TYPE_UPLOAD,
        static_cast<uint32_t>(GetRequiredIntermediateSize(texture.allocation->GetResource(), 0, static_cast<UINT>(subResources.size()))),
        D3D12_RESOURCE_FLAG_NONE,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        uploadBuffer);

    m_copyCommandLists[m_frame].open();

    UpdateSubresources(
        m_copyCommandLists[m_frame].getUnderlyingCommandList(),
        texture.allocation->GetResource(),
        uploadBuffer->GetResource(),
        0,
        0,
        static_cast<UINT>(subResources.size()),
        subResources.data());

    const auto resourceDesc = texture.allocation->GetResource()->GetDesc();

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc;
    srvDesc.Format = resourceDesc.Format;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

    switch (resourceDesc.Dimension)
    {
    case D3D12_RESOURCE_DIMENSION_TEXTURE1D:
        assert(!isCubeMap);

        srvDesc.ViewDimension = resourceDesc.DepthOrArraySize > 1 ? D3D12_SRV_DIMENSION_TEXTURE1DARRAY : D3D12_SRV_DIMENSION_TEXTURE1D;

        if (srvDesc.ViewDimension == D3D12_SRV_DIMENSION_TEXTURE1DARRAY)
        {
            srvDesc.Texture1DArray.MostDetailedMip = 0;
            srvDesc.Texture1DArray.MipLevels = resourceDesc.MipLevels;
            srvDesc.Texture1DArray.FirstArraySlice = 0;
            srvDesc.Texture1DArray.ArraySize = resourceDesc.DepthOrArraySize;
            srvDesc.Texture1DArray.ResourceMinLODClamp = 0.0f;
        }
        else
        {
            srvDesc.Texture1D.MostDetailedMip = 0;
            srvDesc.Texture1D.MipLevels = resourceDesc.MipLevels;
            srvDesc.Texture1D.ResourceMinLODClamp = 0.0f;
        }

        break;

    case D3D12_RESOURCE_DIMENSION_TEXTURE2D:
        if (isCubeMap)
        {
            srvDesc.ViewDimension = resourceDesc.DepthOrArraySize > 6 ? D3D12_SRV_DIMENSION_TEXTURECUBEARRAY : D3D12_SRV_DIMENSION_TEXTURECUBE;

            if (srvDesc.ViewDimension == D3D12_SRV_DIMENSION_TEXTURECUBEARRAY)
            {
                srvDesc.TextureCubeArray.MostDetailedMip = 0;
                srvDesc.TextureCubeArray.MipLevels = resourceDesc.MipLevels;
                srvDesc.TextureCubeArray.First2DArrayFace = 0;
                srvDesc.TextureCubeArray.NumCubes = resourceDesc.DepthOrArraySize / 6;
                srvDesc.TextureCubeArray.ResourceMinLODClamp = 0.0f;
            }
            else
            {
                srvDesc.TextureCube.MostDetailedMip = 0;
                srvDesc.TextureCube.MipLevels = resourceDesc.MipLevels;
                srvDesc.TextureCube.ResourceMinLODClamp = 0.0f;
            }
        }
        else
        {
            srvDesc.ViewDimension = resourceDesc.DepthOrArraySize > 1 ? D3D12_SRV_DIMENSION_TEXTURE2DARRAY : D3D12_SRV_DIMENSION_TEXTURE2D;

            if (srvDesc.ViewDimension == D3D12_SRV_DIMENSION_TEXTURE2DARRAY)
            {
                srvDesc.Texture2DArray.MostDetailedMip = 0;
                srvDesc.Texture2DArray.MipLevels = resourceDesc.MipLevels;
                srvDesc.Texture2DArray.FirstArraySlice = 0;
                srvDesc.Texture2DArray.ArraySize = resourceDesc.DepthOrArraySize;
                srvDesc.Texture2DArray.PlaneSlice = 0;
                srvDesc.Texture2DArray.ResourceMinLODClamp = 0.0f;
            }
            else
            {
                srvDesc.Texture2D.MostDetailedMip = 0;
                srvDesc.Texture2D.MipLevels = resourceDesc.MipLevels;
                srvDesc.Texture2D.PlaneSlice = 0;
                srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
            }
        }

        break;

    case D3D12_RESOURCE_DIMENSION_TEXTURE3D:
        assert(!isCubeMap);

        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE3D;
        srvDesc.Texture3D.MostDetailedMip = 0;
        srvDesc.Texture3D.MipLevels = resourceDesc.MipLevels;
        srvDesc.Texture3D.ResourceMinLODClamp = 0.0f;
        break;

    default: 
        assert(false);
        break;
    }

    texture.srvIndex = m_descriptorHeap.allocate();

    m_device->CreateShaderResourceView(
        texture.allocation->GetResource(),
        &srvDesc,
        m_descriptorHeap.getCpuHandle(texture.srvIndex));

#ifdef _DEBUG
    wchar_t name[0x100];
    MultiByteToWideChar(CP_UTF8, 0, message.textureName, -1, name, _countof(name));
    texture.allocation->GetResource()->SetName(name);
#endif
}

void Device::procMsgDrawIndexedPrimitive()
{
    const auto& message = m_messageReceiver.getMessage<MsgDrawIndexedPrimitive>();

    setPrimitiveType(static_cast<D3DPRIMITIVETYPE>(message.primitiveType));
    flushGraphicsState();

    m_graphicsCommandLists[m_frame].getUnderlyingCommandList()
        ->DrawIndexedInstanced(
            message.indexCount,
            m_instanceCount,
            message.startIndex,
            message.baseVertexIndex,
            0);
}

void Device::procMsgSetStreamSourceFreq()
{
    const auto& message = m_messageReceiver.getMessage<MsgSetStreamSourceFreq>();

    assert(message.streamNumber == 0);

    const bool instancing = (message.setting & D3DSTREAMSOURCE_INDEXEDDATA) != 0;

    if (instancing != m_instancing)
        m_dirtyFlags |= DIRTY_FLAG_PIPELINE_DESC;
    
    m_instancing = instancing;
    m_instanceCount = instancing ? message.setting & 0x3FFFFFFF : 1;
}

void Device::procMsgReleaseResource()
{
    const auto& message = m_messageReceiver.getMessage<MsgReleaseResource>();

    switch (message.resourceType)
    {
    case MsgReleaseResource::ResourceType::Texture:
        m_tempTextures[m_frame].push_back(m_textures[message.resourceId]);
        m_textures[message.resourceId] = {};

        break;

    case MsgReleaseResource::ResourceType::IndexBuffer:
        m_tempBuffers[m_frame].emplace_back(std::move(m_indexBuffers[message.resourceId].allocation));
        break;

    case MsgReleaseResource::ResourceType::VertexBuffer:
        m_tempBuffers[m_frame].emplace_back(std::move(m_vertexBuffers[message.resourceId].allocation));
        break;

    default: 
        assert(false);
        break;
    }
}

void Device::procMsgDrawPrimitive()
{
    const auto& message = m_messageReceiver.getMessage<MsgDrawPrimitive>();

    setPrimitiveType(static_cast<D3DPRIMITIVETYPE>(message.primitiveType));
    flushGraphicsState();

    m_graphicsCommandLists[m_frame].getUnderlyingCommandList()
        ->DrawInstanced(
            message.vertexCount,
            m_instanceCount,
            message.startVertex,
            0);
}

Device::Device()
{
    HRESULT hr;

#ifdef _DEBUG
    ComPtr<ID3D12Debug> debugInterface;
    hr = D3D12GetDebugInterface(IID_PPV_ARGS(debugInterface.GetAddressOf()));

    assert(SUCCEEDED(hr) && debugInterface != nullptr);

    debugInterface->EnableDebugLayer();
#endif

    hr = D3D12CreateDevice(m_swapChain.getAdapter(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(m_device.GetAddressOf()));

    assert(SUCCEEDED(hr) && m_device != nullptr);

#ifdef _DEBUG
    ComPtr<ID3D12InfoQueue> infoQueue;
    m_device.As(&infoQueue);

    // Disable messages we're aware of and okay with
    D3D12_MESSAGE_ID ids[] =
    {
        D3D12_MESSAGE_ID_CREATEGRAPHICSPIPELINESTATE_RENDERTARGETVIEW_NOT_SET,
        D3D12_MESSAGE_ID_CLEARRENDERTARGETVIEW_MISMATCHINGCLEARVALUE
    };

    D3D12_INFO_QUEUE_FILTER filter{};
    filter.DenyList.NumIDs = _countof(ids);
    filter.DenyList.pIDList = ids;

    infoQueue->AddStorageFilterEntries(&filter);
#endif

    D3D12MA::ALLOCATOR_DESC desc{};

    desc.Flags = D3D12MA::ALLOCATOR_FLAG_SINGLETHREADED | D3D12MA::ALLOCATOR_FLAG_DEFAULT_POOLS_NOT_ZEROED;
    desc.pDevice = m_device.Get();
    desc.pAdapter = m_swapChain.getAdapter();

    hr = D3D12MA::CreateAllocator(&desc, m_allocator.GetAddressOf());

    assert(SUCCEEDED(hr) && m_allocator != nullptr);

    m_graphicsQueue.init(m_device.Get(), D3D12_COMMAND_LIST_TYPE_DIRECT);
    m_copyQueue.init(m_device.Get(), D3D12_COMMAND_LIST_TYPE_COPY);

    for (auto& graphicsCommandList : m_graphicsCommandLists)
        graphicsCommandList.init(m_device.Get(), m_graphicsQueue);

    for (auto& copyCommandList : m_copyCommandLists)
        copyCommandList.init(m_device.Get(), m_copyQueue);

    m_descriptorHeap.init(m_device.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    m_samplerDescriptorHeap.init(m_device.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
    m_rtvDescriptorHeap.init(m_device.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    m_dsvDescriptorHeap.init(m_device.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

    CD3DX12_ROOT_PARAMETER rootParameters[2];
    rootParameters[0].InitAsConstantBufferView(0, 0, D3D12_SHADER_VISIBILITY_VERTEX);
    rootParameters[1].InitAsConstantBufferView(1, 0, D3D12_SHADER_VISIBILITY_PIXEL);

    D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc{};
    rootSignatureDesc.NumParameters = _countof(rootParameters);
    rootSignatureDesc.pParameters = rootParameters;
    rootSignatureDesc.Flags = 
        D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
        D3D12_ROOT_SIGNATURE_FLAG_CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED |
        D3D12_ROOT_SIGNATURE_FLAG_SAMPLER_HEAP_DIRECTLY_INDEXED;

    ComPtr<ID3DBlob> blob;
    ComPtr<ID3DBlob> errorBlob;

    hr = D3D12SerializeRootSignature(
        &rootSignatureDesc,
        D3D_ROOT_SIGNATURE_VERSION_1, 
        blob.GetAddressOf(),
        errorBlob.GetAddressOf());

    assert(SUCCEEDED(hr) && blob != nullptr);

    hr = m_device->CreateRootSignature(
        0,
        blob->GetBufferPointer(),
        blob->GetBufferSize(),
        IID_PPV_ARGS(m_rootSignature.GetAddressOf()));

    assert(SUCCEEDED(hr) && m_rootSignature != nullptr);

    m_pipelineDesc.pRootSignature = m_rootSignature.Get();
    m_pipelineDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    m_pipelineDesc.SampleMask = ~0;
    m_pipelineDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    m_pipelineDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
    m_pipelineDesc.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_0xFFFF;
    m_pipelineDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    m_pipelineDesc.NumRenderTargets = 1;
    m_pipelineDesc.SampleDesc.Count = 1;

    for (auto& samplerDesc : m_samplerDescs)
    {
        samplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
        samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        samplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
        samplerDesc.MaxLOD = D3D12_FLOAT32_MAX;
    }
}

void Device::processMessages()
{
    m_graphicsCommandLists[m_frame].open();

    if (m_swapChainTextureId != 0)
    {
        m_textures[m_swapChainTextureId] = m_swapChain.getTexture();
        m_graphicsCommandLists[m_frame].transitionBarrier(m_swapChain.getResource(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_RENDER_TARGET);
    }

    ID3D12DescriptorHeap* descriptorHeaps[] =
    {
        m_descriptorHeap.getUnderlyingHeap(),
        m_samplerDescriptorHeap.getUnderlyingHeap()
    };

    m_graphicsCommandLists[m_frame].getUnderlyingCommandList()
        ->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

    m_graphicsCommandLists[m_frame].getUnderlyingCommandList()
        ->SetGraphicsRootSignature(m_rootSignature.Get());

    bool stop = false;

    // Wait for Generations to copy messages
    m_cpuEvent.wait();
    m_cpuEvent.reset();

    while (!stop && !m_swapChain.getWindow().m_shouldExit)
    {
        switch (m_messageReceiver.getId())
        {
        case MsgEof::s_id:
            stop = true;
            break;

        default:
            if (!processRaytracingMessage())
            {
                assert(!"Unknown message type");
                stop = true;
            }
            break;

        case MsgPadding::s_id: procMsgPadding(); break;
        case MsgCreateSwapChain::s_id: procMsgCreateSwapChain(); break;
        case MsgSetRenderTarget::s_id: procMsgSetRenderTarget(); break;
        case MsgCreateVertexDeclaration::s_id: procMsgCreateVertexDeclaration(); break;
        case MsgCreatePixelShader::s_id: procMsgCreatePixelShader(); break;
        case MsgCreateVertexShader::s_id: procMsgCreateVertexShader(); break;
        case MsgSetRenderState::s_id: procMsgSetRenderState(); break;
        case MsgCreateTexture::s_id: procMsgCreateTexture(); break;
        case MsgSetTexture::s_id: procMsgSetTexture(); break;
        case MsgSetDepthStencilSurface::s_id: procMsgSetDepthStencilSurface(); break;
        case MsgClear::s_id: procMsgClear(); break;
        case MsgSetVertexShader::s_id: procMsgSetVertexShader(); break;
        case MsgSetPixelShader::s_id: procMsgSetPixelShader(); break;
        case MsgSetPixelShaderConstantF::s_id: procMsgSetPixelShaderConstantF(); break;
        case MsgSetVertexShaderConstantF::s_id: procMsgSetVertexShaderConstantF(); break;
        case MsgSetVertexShaderConstantB::s_id: procMsgSetVertexShaderConstantB(); break;
        case MsgSetSamplerState::s_id: procMsgSetSamplerState(); break;
        case MsgSetViewport::s_id: procMsgSetViewport(); break;
        case MsgSetScissorRect::s_id: procMsgSetScissorRect(); break;
        case MsgSetVertexDeclaration::s_id: procMsgSetVertexDeclaration(); break;
        case MsgDrawPrimitiveUP::s_id: procMsgDrawPrimitiveUP(); break;
        case MsgSetStreamSource::s_id: procMsgSetStreamSource(); break;
        case MsgSetIndices::s_id: procMsgSetIndices(); break;
        case MsgPresent::s_id: procMsgPresent(); break;
        case MsgCreateVertexBuffer::s_id: procMsgCreateVertexBuffer(); break;
        case MsgWriteVertexBuffer::s_id: procMsgWriteVertexBuffer(); break;
        case MsgCreateIndexBuffer::s_id: procMsgCreateIndexBuffer(); break;
        case MsgWriteIndexBuffer::s_id: procMsgWriteIndexBuffer(); break;
        case MsgWriteTexture::s_id: procMsgWriteTexture(); break;
        case MsgMakeTexture::s_id: procMsgMakeTexture(); break;
        case MsgDrawIndexedPrimitive::s_id: procMsgDrawIndexedPrimitive(); break;
        case MsgSetStreamSourceFreq::s_id: procMsgSetStreamSourceFreq(); break;
        case MsgReleaseResource::s_id: procMsgReleaseResource(); break;
        case MsgDrawPrimitive::s_id: procMsgDrawPrimitive(); break;

        }
    }

    // Let Generations know we finished processing messages
    m_gpuEvent.set();
    m_messageReceiver.reset();

    if (m_copyCommandLists[m_frame].isOpen())
    {
        m_copyCommandLists[m_frame].close();
        m_copyQueue.executeCommandList(m_copyCommandLists[m_frame]);

        // Wait for copy command list to finish
        const uint64_t fenceValue = m_copyQueue.getNextFenceValue();
        m_copyQueue.signal(fenceValue);
        m_graphicsQueue.wait(fenceValue, m_copyQueue);
    }

    m_graphicsCommandLists[m_frame].close();
    m_graphicsQueue.executeCommandList(m_graphicsCommandLists[m_frame]);

    if (m_shouldPresent)
    {
        m_swapChain.present();
        m_shouldPresent = false;
    }

    m_fenceValues[m_frame] = m_graphicsQueue.getNextFenceValue();
    m_graphicsQueue.signal(m_fenceValues[m_frame]);

    m_uploadBufferIndex = 0;
    m_uploadBufferOffset = 0;

    m_renderTargetView = {};
    m_depthStencilView = {};

    memset(m_vertexBufferViews, 0, sizeof(m_vertexBufferViews));
    m_vertexBufferViewsFirst = 0;
    m_vertexBufferViewsLast = _countof(m_vertexBufferViews) - 1;

    m_indexBufferView = {};

    m_dirtyFlags = ~0;

    m_samplerDescsFirst = 0;
    m_samplerDescsLast = _countof(m_samplerDescs) - 1;

    m_frame = (m_frame + 1) % NUM_FRAMES;

    // Wait for graphics command list to finish
    m_graphicsQueue.wait(m_fenceValues[m_frame]);

    // Cleanup
    for (const auto& texture : m_tempTextures[m_frame])
    {
        if (texture.srvIndex != NULL)
            m_descriptorHeap.free(texture.srvIndex);

        if (texture.rtvIndex != NULL)
            m_rtvDescriptorHeap.free(texture.rtvIndex);

        if (texture.dsvIndex != NULL)
            m_dsvDescriptorHeap.free(texture.dsvIndex);
    }

    m_tempTextures[m_frame].clear();
    m_tempBuffers[m_frame].clear();
}

void Device::runLoop()
{
    while (!m_swapChain.getWindow().m_shouldExit)
    {
        processMessages();
        m_swapChain.getWindow().processMessages();
    }

    // Let the GPU finish its work before we quit
    const uint64_t fenceValue = m_graphicsQueue.getNextFenceValue();
    m_graphicsQueue.signal(fenceValue);
    m_graphicsQueue.wait(fenceValue);
}

void Device::setEvents()
{
    m_cpuEvent.set();
    m_gpuEvent.set();
}

void Device::setShouldExit()
{
    m_swapChain.getWindow().m_shouldExit = true;
    setEvents();
}

ID3D12Device* Device::getUnderlyingDevice() const
{
    return m_device.Get();
}

CommandQueue& Device::getGraphicsQueue()
{
    return m_graphicsQueue;
}

CommandQueue& Device::getCopyQueue()
{
    return m_copyQueue;
}

DescriptorHeap& Device::getDescriptorHeap()
{
    return m_descriptorHeap;
}

DescriptorHeap& Device::getSamplerDescriptorHeap()
{
    return m_samplerDescriptorHeap;
}

DescriptorHeap& Device::getRtvDescriptorHeap()
{
    return m_rtvDescriptorHeap;
}

DescriptorHeap& Device::getDsvDescriptorHeap()
{
    return m_dsvDescriptorHeap;
}
