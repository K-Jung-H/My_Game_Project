#include "Graphic_RootSignature.h"

void RootSignatureFactory::Init(ID3D12Device* device)
{
    if (initialized) return;

    cache[(int)RootSignature_Type::Default] = CreateDefault(device);
    cache[(int)RootSignature_Type::PostFX] = CreatePostFX(device);
    //cache[(int)RootSignature_Type::Terrain] = CreateTerrain(device);
    //cache[(int)RootSignature_Type::Skinned] = CreateSkinned(device);
    //cache[(int)RootSignature_Type::UI] = CreateUI(device);
    cache[(int)RootSignature_Type::ShadowPass] = CreateShadowPass(device);
    cache[(int)RootSignature_Type::LightPass] = CreateLightPass(device);

    initialized = true;
}

ID3D12RootSignature* RootSignatureFactory::Get(RootSignature_Type type)
{
    return cache[(int)type].Get();
}

void RootSignatureFactory::Destroy()
{
    for (auto& sig : cache)
        sig.Reset();
    initialized = false;
}


ComPtr<ID3D12RootSignature> RootSignatureFactory::CreateDefault(ID3D12Device* pd3dDevice)
{
    using namespace RootParameter_Default;

    D3D12_DESCRIPTOR_RANGE srvRange = {};
    srvRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    srvRange.NumDescriptors = 2000;   
    srvRange.BaseShaderRegister = 0;        // t0
    srvRange.RegisterSpace = 0;
    srvRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    D3D12_ROOT_PARAMETER params[Count] = {};

    params[SceneCBV].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    params[SceneCBV].Descriptor.ShaderRegister = 0;
    params[SceneCBV].Descriptor.RegisterSpace = 0;
    params[SceneCBV].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

    params[CameraCBV].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    params[CameraCBV].Descriptor.ShaderRegister = 1;
    params[CameraCBV].Descriptor.RegisterSpace = 0;
    params[CameraCBV].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

    params[ObjectCBV].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    params[ObjectCBV].Descriptor.ShaderRegister = 2;
    params[ObjectCBV].Descriptor.RegisterSpace = 0;
    params[ObjectCBV].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

    params[TextureTable].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    params[TextureTable].DescriptorTable.NumDescriptorRanges = 1;
    params[TextureTable].DescriptorTable.pDescriptorRanges = &srvRange;
    params[TextureTable].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;


    D3D12_STATIC_SAMPLER_DESC samplers[2] = {};
    samplers[0] = CD3DX12_STATIC_SAMPLER_DESC(0, D3D12_FILTER_MIN_MAG_MIP_LINEAR);
    samplers[1] = CD3DX12_STATIC_SAMPLER_DESC(1, D3D12_FILTER_MIN_MAG_MIP_POINT);

    D3D12_ROOT_SIGNATURE_DESC rsDesc = {};
    rsDesc.NumParameters = _countof(params);
    rsDesc.pParameters = params;
    rsDesc.NumStaticSamplers = _countof(samplers);
    rsDesc.pStaticSamplers = samplers;
    rsDesc.Flags =
        D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
        | D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS
        | D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS
        | D3D12_ROOT_SIGNATURE_FLAG_ALLOW_STREAM_OUTPUT;

    ComPtr<ID3DBlob> sigBlob, errorBlob;
    HRESULT hr = D3D12SerializeRootSignature(&rsDesc, D3D_ROOT_SIGNATURE_VERSION_1, sigBlob.GetAddressOf(), errorBlob.GetAddressOf());
    if (FAILED(hr))
    {
        if (errorBlob)
            OutputDebugStringA((char*)errorBlob->GetBufferPointer());
        return nullptr;
    }

    ComPtr<ID3D12RootSignature> rootSig;
    hr = pd3dDevice->CreateRootSignature(0, sigBlob->GetBufferPointer(), sigBlob->GetBufferSize(), IID_PPV_ARGS(&rootSig));
    if (FAILED(hr))
    {
        OutputDebugStringA("Failed to create root signature.\n");
        return nullptr;
    }

    return rootSig;
}

ComPtr<ID3D12RootSignature> RootSignatureFactory::CreatePostFX(ID3D12Device* pd3dDevice)
{
    using namespace RootParameter_PostFX;

    // === GBuffer Range ===
    D3D12_DESCRIPTOR_RANGE gbufferRange = {};
    gbufferRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    gbufferRange.NumDescriptors = (UINT)GBufferType::Count; 
    gbufferRange.BaseShaderRegister = 0; // t0 Ω√¿€
    gbufferRange.RegisterSpace = 0;
    gbufferRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    // === Depth Range ===
    D3D12_DESCRIPTOR_RANGE depthRange = {};
    depthRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    depthRange.NumDescriptors = 1;
    depthRange.BaseShaderRegister = (UINT)GBufferType::Count; 
    depthRange.RegisterSpace = 0;
    depthRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    // === Merge Range ===
    D3D12_DESCRIPTOR_RANGE mergeRange = {};
    mergeRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    mergeRange.NumDescriptors = 1;
    mergeRange.BaseShaderRegister = (UINT)GBufferType::Count + 1; 
    mergeRange.RegisterSpace = 0;
    mergeRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    D3D12_DESCRIPTOR_RANGE clusterLightRange[4] = {};
    clusterLightRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    clusterLightRange[0].NumDescriptors = 1;
    clusterLightRange[0].BaseShaderRegister = (UINT)GBufferType::Count + 2;
    clusterLightRange[0].RegisterSpace = 0;
    clusterLightRange[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    clusterLightRange[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    clusterLightRange[1].NumDescriptors = 1;
    clusterLightRange[1].BaseShaderRegister = (UINT)GBufferType::Count + 3;
    clusterLightRange[1].RegisterSpace = 0;
    clusterLightRange[1].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    clusterLightRange[2].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    clusterLightRange[2].NumDescriptors = 1;
    clusterLightRange[2].BaseShaderRegister = (UINT)GBufferType::Count + 4;
    clusterLightRange[2].RegisterSpace = 0;
    clusterLightRange[2].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    clusterLightRange[3].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    clusterLightRange[3].NumDescriptors = 1;
    clusterLightRange[3].BaseShaderRegister = (UINT)GBufferType::Count + 5;
    clusterLightRange[3].RegisterSpace = 0;
    clusterLightRange[3].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    D3D12_DESCRIPTOR_RANGE shadowRanges[3] = {};

    // CSM Lights (tN+6)
    shadowRanges[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    shadowRanges[0].NumDescriptors = 1; // Texture2DArray[]
    shadowRanges[0].BaseShaderRegister = (UINT)GBufferType::Count + 6;
    shadowRanges[0].RegisterSpace = 0;
    shadowRanges[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    // Point Lights (tN+7)
    shadowRanges[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    shadowRanges[1].NumDescriptors = 1; // TextureCube[]
    shadowRanges[1].BaseShaderRegister = (UINT)GBufferType::Count + 7;
    shadowRanges[1].RegisterSpace = 0;
    shadowRanges[1].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    // Spot Lights (tN+8)
    shadowRanges[2].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    shadowRanges[2].NumDescriptors = 1; // Texture2D[]
    shadowRanges[2].BaseShaderRegister = (UINT)GBufferType::Count + 8;
    shadowRanges[2].RegisterSpace = 0;
    shadowRanges[2].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    
    // === Root Parameters ===
    D3D12_ROOT_PARAMETER params[(UINT)Count] = {};

    // b0 : SceneCBV
    params[SceneCBV].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    params[SceneCBV].Descriptor.ShaderRegister = 0;
    params[SceneCBV].Descriptor.RegisterSpace = 0;
    params[SceneCBV].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

    // b1 : CameraCBV
    params[CameraCBV].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    params[CameraCBV].Descriptor.ShaderRegister = 1;
    params[CameraCBV].Descriptor.RegisterSpace = 0;
    params[CameraCBV].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

    // t0~t(N-1) : GBuffer
    params[GBufferTable].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    params[GBufferTable].DescriptorTable.NumDescriptorRanges = 1;
    params[GBufferTable].DescriptorTable.pDescriptorRanges = &gbufferRange;
    params[GBufferTable].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    // tN : Depth
    params[DepthTexture].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    params[DepthTexture].DescriptorTable.NumDescriptorRanges = 1;
    params[DepthTexture].DescriptorTable.pDescriptorRanges = &depthRange;
    params[DepthTexture].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    // tN+1 : MergeRT
    params[MergeTexture].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    params[MergeTexture].DescriptorTable.NumDescriptorRanges = 1;
    params[MergeTexture].DescriptorTable.pDescriptorRanges = &mergeRange;
    params[MergeTexture].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    // tN+2 : ClusterSRV
    params[ClusterAreaSRV].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    params[ClusterAreaSRV].DescriptorTable.NumDescriptorRanges = 1;
    params[ClusterAreaSRV].DescriptorTable.pDescriptorRanges = &clusterLightRange[0];
    params[ClusterAreaSRV].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

    // tN+3 : LightBufferSRV
    params[LightBufferSRV].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    params[LightBufferSRV].DescriptorTable.NumDescriptorRanges = 1;
    params[LightBufferSRV].DescriptorTable.pDescriptorRanges = &clusterLightRange[1];
    params[LightBufferSRV].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

    // tN+4 : ClusterLightMetaSRV
    params[ClusterLightMetaSRV].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    params[ClusterLightMetaSRV].DescriptorTable.NumDescriptorRanges = 1;
    params[ClusterLightMetaSRV].DescriptorTable.pDescriptorRanges = &clusterLightRange[2];
    params[ClusterLightMetaSRV].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

    // tN+5 : ClusterLightIndicesSRV
    params[ClusterLightIndicesSRV].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    params[ClusterLightIndicesSRV].DescriptorTable.NumDescriptorRanges = 1;
    params[ClusterLightIndicesSRV].DescriptorTable.pDescriptorRanges = &clusterLightRange[3];
    params[ClusterLightIndicesSRV].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

    // tN+6 : ShadowMapCSMTable
    params[ShadowMapCSMTable].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    params[ShadowMapCSMTable].DescriptorTable.NumDescriptorRanges = 1;
    params[ShadowMapCSMTable].DescriptorTable.pDescriptorRanges = &shadowRanges[0];
    params[ShadowMapCSMTable].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    // tN+7 : ShadowMapPointTable
    params[ShadowMapPointTable].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    params[ShadowMapPointTable].DescriptorTable.NumDescriptorRanges = 1;
    params[ShadowMapPointTable].DescriptorTable.pDescriptorRanges = &shadowRanges[1];
    params[ShadowMapPointTable].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    // tN+8 : ShadowMapSpotTable
    params[ShadowMapSpotTable].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    params[ShadowMapSpotTable].DescriptorTable.NumDescriptorRanges = 1;
    params[ShadowMapSpotTable].DescriptorTable.pDescriptorRanges = &shadowRanges[2];
    params[ShadowMapSpotTable].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    D3D12_STATIC_SAMPLER_DESC samplers[3] = {};
    samplers[0] = CD3DX12_STATIC_SAMPLER_DESC(0, D3D12_FILTER_MIN_MAG_MIP_LINEAR);
    samplers[1] = CD3DX12_STATIC_SAMPLER_DESC(1, D3D12_FILTER_MIN_MAG_MIP_POINT);
    samplers[2] = CD3DX12_STATIC_SAMPLER_DESC(2, D3D12_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT,
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
        0.0f,
        16,
        D3D12_COMPARISON_FUNC_GREATER_EQUAL,
        D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE
    );

    D3D12_ROOT_SIGNATURE_DESC rsDesc = {};
    rsDesc.NumParameters = _countof(params);
    rsDesc.pParameters = params;
    rsDesc.NumStaticSamplers = _countof(samplers);
    rsDesc.pStaticSamplers = samplers;
    rsDesc.Flags =
        D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS;

    ComPtr<ID3DBlob> sigBlob, errorBlob;
    HRESULT hr = D3D12SerializeRootSignature(&rsDesc, D3D_ROOT_SIGNATURE_VERSION_1,
        sigBlob.GetAddressOf(), errorBlob.GetAddressOf());
    if (FAILED(hr))
    {
        if (errorBlob) OutputDebugStringA((char*)errorBlob->GetBufferPointer());
        return nullptr;
    }

    ComPtr<ID3D12RootSignature> rootSig;
    hr = pd3dDevice->CreateRootSignature(0, sigBlob->GetBufferPointer(), sigBlob->GetBufferSize(),
        IID_PPV_ARGS(&rootSig));
    if (FAILED(hr))
    {
        OutputDebugStringA("Failed to create PostFX root signature.\n");
        return nullptr;
    }

    return rootSig;
}

ComPtr<ID3D12RootSignature> RootSignatureFactory::CreateTerrain(ID3D12Device* pd3dDevice)
{
    D3D12_DESCRIPTOR_RANGE ranges[ShaderRegister::Count];
    for (UINT i = 0; i < ShaderRegister::Count; i++)
    {
        ranges[i].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
        ranges[i].NumDescriptors = 1;
        ranges[i].BaseShaderRegister = i;
        ranges[i].RegisterSpace = 0;
        ranges[i].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
    }

    D3D12_ROOT_PARAMETER params[RootParameter_Default::Count] = {};

    D3D12_STATIC_SAMPLER_DESC samplers[1] = {};

    samplers[0].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
    samplers[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    samplers[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    samplers[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    samplers[0].MipLODBias = 0.0f;
    samplers[0].MaxAnisotropy = 1;
    samplers[0].ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
    samplers[0].MinLOD = 0.0f;
    samplers[0].MaxLOD = D3D12_FLOAT32_MAX;
    samplers[0].ShaderRegister = 0;
    samplers[0].RegisterSpace = 0;
    samplers[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;


    D3D12_ROOT_SIGNATURE_DESC rsDesc = {};
    rsDesc.NumParameters = _countof(params);
    rsDesc.pParameters = params;
    rsDesc.NumStaticSamplers = _countof(samplers);
    rsDesc.pStaticSamplers = samplers;
    rsDesc.Flags =
        D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
        | D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS
        | D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS
        | D3D12_ROOT_SIGNATURE_FLAG_ALLOW_STREAM_OUTPUT;

    ComPtr<ID3DBlob> sigBlob, errorBlob;
    HRESULT hr = D3D12SerializeRootSignature(&rsDesc, D3D_ROOT_SIGNATURE_VERSION_1, sigBlob.GetAddressOf(), errorBlob.GetAddressOf());
    if (FAILED(hr))
    {
        if (errorBlob)
            OutputDebugStringA((char*)errorBlob->GetBufferPointer());
        return nullptr;
    }

    ComPtr<ID3D12RootSignature> rootSig;
    hr = pd3dDevice->CreateRootSignature(0, sigBlob->GetBufferPointer(), sigBlob->GetBufferSize(), IID_PPV_ARGS(&rootSig));
    if (FAILED(hr))
    {
        OutputDebugStringA("Failed to create root signature.\n");
        return nullptr;
    }

    return rootSig;
}

ComPtr<ID3D12RootSignature> RootSignatureFactory::CreateSkinned(ID3D12Device* pd3dDevice)
{
    D3D12_DESCRIPTOR_RANGE ranges[ShaderRegister::Count];
    for (UINT i = 0; i < ShaderRegister::Count; i++)
    {
        ranges[i].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
        ranges[i].NumDescriptors = 1;
        ranges[i].BaseShaderRegister = i;
        ranges[i].RegisterSpace = 0;
        ranges[i].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
    }

    D3D12_ROOT_PARAMETER params[RootParameter_Default::Count] = {};

    D3D12_STATIC_SAMPLER_DESC samplers[1] = {};

    samplers[0].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
    samplers[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    samplers[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    samplers[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    samplers[0].MipLODBias = 0.0f;
    samplers[0].MaxAnisotropy = 1;
    samplers[0].ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
    samplers[0].MinLOD = 0.0f;
    samplers[0].MaxLOD = D3D12_FLOAT32_MAX;
    samplers[0].ShaderRegister = 0;
    samplers[0].RegisterSpace = 0;
    samplers[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;


    D3D12_ROOT_SIGNATURE_DESC rsDesc = {};
    rsDesc.NumParameters = _countof(params);
    rsDesc.pParameters = params;
    rsDesc.NumStaticSamplers = _countof(samplers);
    rsDesc.pStaticSamplers = samplers;
    rsDesc.Flags =
        D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
        | D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS
        | D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS
        | D3D12_ROOT_SIGNATURE_FLAG_ALLOW_STREAM_OUTPUT;

    ComPtr<ID3DBlob> sigBlob, errorBlob;
    HRESULT hr = D3D12SerializeRootSignature(&rsDesc, D3D_ROOT_SIGNATURE_VERSION_1, sigBlob.GetAddressOf(), errorBlob.GetAddressOf());
    if (FAILED(hr))
    {
        if (errorBlob)
            OutputDebugStringA((char*)errorBlob->GetBufferPointer());
        return nullptr;
    }

    ComPtr<ID3D12RootSignature> rootSig;
    hr = pd3dDevice->CreateRootSignature(0, sigBlob->GetBufferPointer(), sigBlob->GetBufferSize(), IID_PPV_ARGS(&rootSig));
    if (FAILED(hr))
    {
        OutputDebugStringA("Failed to create root signature.\n");
        return nullptr;
    }

    return rootSig;
}

ComPtr<ID3D12RootSignature> RootSignatureFactory::CreateUI(ID3D12Device* pd3dDevice)
{
    D3D12_DESCRIPTOR_RANGE ranges[ShaderRegister::Count];
    for (UINT i = 0; i < ShaderRegister::Count; i++)
    {
        ranges[i].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
        ranges[i].NumDescriptors = 1;
        ranges[i].BaseShaderRegister = i;
        ranges[i].RegisterSpace = 0;
        ranges[i].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
    }

    D3D12_ROOT_PARAMETER params[RootParameter_Default::Count] = {};

    D3D12_STATIC_SAMPLER_DESC samplers[1] = {};

    samplers[0].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
    samplers[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    samplers[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    samplers[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    samplers[0].MipLODBias = 0.0f;
    samplers[0].MaxAnisotropy = 1;
    samplers[0].ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
    samplers[0].MinLOD = 0.0f;
    samplers[0].MaxLOD = D3D12_FLOAT32_MAX;
    samplers[0].ShaderRegister = 0;
    samplers[0].RegisterSpace = 0;
    samplers[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;


    D3D12_ROOT_SIGNATURE_DESC rsDesc = {};
    rsDesc.NumParameters = _countof(params);
    rsDesc.pParameters = params;
    rsDesc.NumStaticSamplers = _countof(samplers);
    rsDesc.pStaticSamplers = samplers;
    rsDesc.Flags =
        D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
        | D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS
        | D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS
        | D3D12_ROOT_SIGNATURE_FLAG_ALLOW_STREAM_OUTPUT;

    ComPtr<ID3DBlob> sigBlob, errorBlob;
    HRESULT hr = D3D12SerializeRootSignature(&rsDesc, D3D_ROOT_SIGNATURE_VERSION_1, sigBlob.GetAddressOf(), errorBlob.GetAddressOf());
    if (FAILED(hr))
    {
        if (errorBlob)
            OutputDebugStringA((char*)errorBlob->GetBufferPointer());
        return nullptr;
    }

    ComPtr<ID3D12RootSignature> rootSig;
    hr = pd3dDevice->CreateRootSignature(0, sigBlob->GetBufferPointer(), sigBlob->GetBufferSize(), IID_PPV_ARGS(&rootSig));
    if (FAILED(hr))
    {
        OutputDebugStringA("Failed to create root signature.\n");
        return nullptr;
    }

    return rootSig;
}

ComPtr<ID3D12RootSignature> RootSignatureFactory::CreateLightPass(ID3D12Device* pd3dDevice)
{
    using namespace RootParameter_LightPass;

    // === SRV / UAV Range  ===
    D3D12_DESCRIPTOR_RANGE srvRanges[2] = {};
    srvRanges[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    srvRanges[0].NumDescriptors = 1;           // LightBuffer (t0)
    srvRanges[0].BaseShaderRegister = 0;
    srvRanges[0].RegisterSpace = 0;
    srvRanges[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    srvRanges[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    srvRanges[1].NumDescriptors = 1;           // ClusterBuffer (t1)
    srvRanges[1].BaseShaderRegister = 1;
    srvRanges[1].RegisterSpace = 0;
    srvRanges[1].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    D3D12_DESCRIPTOR_RANGE uavRanges[4] = {};
    uavRanges[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
    uavRanges[0].NumDescriptors = 1;
    uavRanges[0].BaseShaderRegister = 0;
    uavRanges[0].RegisterSpace = 0;
    uavRanges[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    uavRanges[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
    uavRanges[1].NumDescriptors = 1;
    uavRanges[1].BaseShaderRegister = 1;
    uavRanges[1].RegisterSpace = 0;
    uavRanges[1].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    uavRanges[2].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
    uavRanges[2].NumDescriptors = 1;
    uavRanges[2].BaseShaderRegister = 2;
    uavRanges[2].RegisterSpace = 0;
    uavRanges[2].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    uavRanges[3].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
    uavRanges[3].NumDescriptors = 1;
    uavRanges[3].BaseShaderRegister = 3;
    uavRanges[3].RegisterSpace = 0;
    uavRanges[3].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    // === Root Parameters ===
    D3D12_ROOT_PARAMETER params[(UINT)Count] = {};

    // b0 : SceneCBV
    params[SceneCBV].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    params[SceneCBV].Descriptor.ShaderRegister = 0;
    params[SceneCBV].Descriptor.RegisterSpace = 0;
    params[SceneCBV].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

    // b0 : CameraCBV
    params[CameraCBV].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    params[CameraCBV].Descriptor.ShaderRegister = 1;
    params[CameraCBV].Descriptor.RegisterSpace = 0;
    params[CameraCBV].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

    // t0 : LightBuffer SRV
    params[LightBufferSRV].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    params[LightBufferSRV].DescriptorTable.NumDescriptorRanges = 1;
    params[LightBufferSRV].DescriptorTable.pDescriptorRanges = &srvRanges[0];
    params[LightBufferSRV].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

    // t1 : Cluster SRV
    params[ClusterAreaSRV].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    params[ClusterAreaSRV].DescriptorTable.NumDescriptorRanges = 1;
    params[ClusterAreaSRV].DescriptorTable.pDescriptorRanges = &srvRanges[1];
    params[ClusterAreaSRV].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

    // u0 : Cluster UAV
    params[ClusterAreaUAV].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    params[ClusterAreaUAV].DescriptorTable.NumDescriptorRanges = 1;
    params[ClusterAreaUAV].DescriptorTable.pDescriptorRanges = &uavRanges[0];
    params[ClusterAreaUAV].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

    // u1 : ClusterLightMeta UAV
    params[ClusterLightMetaUAV].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    params[ClusterLightMetaUAV].DescriptorTable.NumDescriptorRanges = 1;
    params[ClusterLightMetaUAV].DescriptorTable.pDescriptorRanges = &uavRanges[1];
    params[ClusterLightMetaUAV].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

    // u2 : ClusterLightIndices UAV
    params[ClusterLightIndicesUAV].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    params[ClusterLightIndicesUAV].DescriptorTable.NumDescriptorRanges = 1;
    params[ClusterLightIndicesUAV].DescriptorTable.pDescriptorRanges = &uavRanges[2];
    params[ClusterLightIndicesUAV].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

    // u3 : GlobalCounter UAV
    params[GlobalCounterUAV].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    params[GlobalCounterUAV].DescriptorTable.NumDescriptorRanges = 1;
    params[GlobalCounterUAV].DescriptorTable.pDescriptorRanges = &uavRanges[3];
    params[GlobalCounterUAV].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
   

    // === Root Signature Desc ===
    D3D12_ROOT_SIGNATURE_DESC rsDesc = {};
    rsDesc.NumParameters = _countof(params);
    rsDesc.pParameters = params;
    rsDesc.NumStaticSamplers = 0;
    rsDesc.pStaticSamplers = nullptr;
    rsDesc.Flags =
        D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS;

    // === Serialize & Create ===
    ComPtr<ID3DBlob> sigBlob, errorBlob;
    HRESULT hr = D3D12SerializeRootSignature(&rsDesc, D3D_ROOT_SIGNATURE_VERSION_1,
        sigBlob.GetAddressOf(), errorBlob.GetAddressOf());
    if (FAILED(hr))
    {
        if (errorBlob) OutputDebugStringA((char*)errorBlob->GetBufferPointer());
        return nullptr;
    }

    ComPtr<ID3D12RootSignature> rootSig;
    hr = pd3dDevice->CreateRootSignature(0, sigBlob->GetBufferPointer(), sigBlob->GetBufferSize(),
        IID_PPV_ARGS(&rootSig));
    if (FAILED(hr))
    {
        OutputDebugStringA("Failed to create LightPass root signature.\n");
        return nullptr;
    }

    return rootSig;
}

ComPtr<ID3D12RootSignature> RootSignatureFactory::CreateShadowPass(ID3D12Device* pd3dDevice)
{
    using namespace RootParameter_Shadow;

    // t0: ShadowMatrix_SRV (StructuredBuffer)
    D3D12_DESCRIPTOR_RANGE srvRange = {};
    srvRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    srvRange.NumDescriptors = 1;
    srvRange.BaseShaderRegister = 0; // t0
    srvRange.RegisterSpace = 0;
    srvRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    D3D12_ROOT_PARAMETER params[Count] = {};

    // b0: IndexConstant
    params[ShadowMatrix_Index].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
    params[ShadowMatrix_Index].Constants.Num32BitValues = 1;
    params[ShadowMatrix_Index].Constants.ShaderRegister = 0; // b0
    params[ShadowMatrix_Index].Constants.RegisterSpace = 0;
    params[ShadowMatrix_Index].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

    // b1 : ObjectCBV
    params[ObjectCBV].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    params[ObjectCBV].Descriptor.ShaderRegister = 1; // b1
    params[ObjectCBV].Descriptor.RegisterSpace = 0;
    params[ObjectCBV].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

    // t0 : ShadowMatrix_SRV
    params[ShadowMatrix_SRV].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    params[ShadowMatrix_SRV].DescriptorTable.NumDescriptorRanges = 1;
    params[ShadowMatrix_SRV].DescriptorTable.pDescriptorRanges = &srvRange;
    params[ShadowMatrix_SRV].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

    D3D12_ROOT_SIGNATURE_DESC rsDesc = {};
    rsDesc.NumParameters = _countof(params);
    rsDesc.pParameters = params;
    rsDesc.NumStaticSamplers = 0;
    rsDesc.pStaticSamplers = nullptr;
    rsDesc.Flags =
        D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
        | D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS
        | D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS
        | D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS
        | D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS;

    ComPtr<ID3DBlob> sigBlob, errorBlob;
    HRESULT hr = D3D12SerializeRootSignature(&rsDesc, D3D_ROOT_SIGNATURE_VERSION_1,
        sigBlob.GetAddressOf(), errorBlob.GetAddressOf());
    if (FAILED(hr))
    {
        if (errorBlob) OutputDebugStringA((char*)errorBlob->GetBufferPointer());
        return nullptr;
    }

    ComPtr<ID3D12RootSignature> rootSig;
    hr = pd3dDevice->CreateRootSignature(0, sigBlob->GetBufferPointer(), sigBlob->GetBufferSize(),
        IID_PPV_ARGS(&rootSig));
    if (FAILED(hr))
    {
        OutputDebugStringA("Failed to create Shadow root signature.\n");
        return nullptr;
    }

    return rootSig;
}