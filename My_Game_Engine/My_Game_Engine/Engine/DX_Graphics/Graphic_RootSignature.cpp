#include "Graphic_RootSignature.h"

void RootSignatureFactory::Init(ID3D12Device* device)
{
    if (initialized) return;

    cache[(int)RootSignature_Type::Default] = CreateDefault(device);
    cache[(int)RootSignature_Type::PostFX] = CreatePostFX(device);
    //cache[(int)RootSignature_Type::Terrain] = CreateTerrain(device);
    //cache[(int)RootSignature_Type::Skinned] = CreateSkinned(device);
    //cache[(int)RootSignature_Type::UI] = CreateUI(device);

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
    D3D12_DESCRIPTOR_RANGE srvRange = {};
    srvRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    srvRange.NumDescriptors = 2000;   
    srvRange.BaseShaderRegister = 0;        // t0
    srvRange.RegisterSpace = 0;
    srvRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    D3D12_ROOT_PARAMETER params[RootParameter_Default::Count] = {};

    params[RootParameter_Default::SceneCBV].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    params[RootParameter_Default::SceneCBV].Descriptor.ShaderRegister = 0;
    params[RootParameter_Default::SceneCBV].Descriptor.RegisterSpace = 0;
    params[RootParameter_Default::SceneCBV].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

    params[RootParameter_Default::CameraCBV].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    params[RootParameter_Default::CameraCBV].Descriptor.ShaderRegister = 2;
    params[RootParameter_Default::CameraCBV].Descriptor.RegisterSpace = 0;
    params[RootParameter_Default::CameraCBV].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

    params[RootParameter_Default::ObjectCBV].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    params[RootParameter_Default::ObjectCBV].Descriptor.ShaderRegister = 1;
    params[RootParameter_Default::ObjectCBV].Descriptor.RegisterSpace = 0;
    params[RootParameter_Default::ObjectCBV].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

    params[RootParameter_Default::TextureTable].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    params[RootParameter_Default::TextureTable].DescriptorTable.NumDescriptorRanges = 1;
    params[RootParameter_Default::TextureTable].DescriptorTable.pDescriptorRanges = &srvRange;
    params[RootParameter_Default::TextureTable].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;


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

    // === Root Parameters ===
    D3D12_ROOT_PARAMETER params[(UINT)RootParameter_PostFX::Count] = {};

    // b0 : SceneCBV
    params[RootParameter_PostFX::SceneCBV].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    params[RootParameter_PostFX::SceneCBV].Descriptor.ShaderRegister = 0;
    params[RootParameter_PostFX::SceneCBV].Descriptor.RegisterSpace = 0;
    params[RootParameter_PostFX::SceneCBV].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

    // b1 : CameraCBV
    params[RootParameter_PostFX::CameraCBV].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    params[RootParameter_PostFX::CameraCBV].Descriptor.ShaderRegister = 1;
    params[RootParameter_PostFX::CameraCBV].Descriptor.RegisterSpace = 0;
    params[RootParameter_PostFX::CameraCBV].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

    // t0~t(N-1) : GBuffer
    params[RootParameter_PostFX::GBufferTable].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    params[RootParameter_PostFX::GBufferTable].DescriptorTable.NumDescriptorRanges = 1;
    params[RootParameter_PostFX::GBufferTable].DescriptorTable.pDescriptorRanges = &gbufferRange;
    params[RootParameter_PostFX::GBufferTable].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    // tN : Depth
    params[RootParameter_PostFX::DepthTexture].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    params[RootParameter_PostFX::DepthTexture].DescriptorTable.NumDescriptorRanges = 1;
    params[RootParameter_PostFX::DepthTexture].DescriptorTable.pDescriptorRanges = &depthRange;
    params[RootParameter_PostFX::DepthTexture].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    // tN+1 : MergeRT
    params[RootParameter_PostFX::MergeTexture].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    params[RootParameter_PostFX::MergeTexture].DescriptorTable.NumDescriptorRanges = 1;
    params[RootParameter_PostFX::MergeTexture].DescriptorTable.pDescriptorRanges = &mergeRange;
    params[RootParameter_PostFX::MergeTexture].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;


    D3D12_STATIC_SAMPLER_DESC samplers[2] = {};
    samplers[0] = CD3DX12_STATIC_SAMPLER_DESC(0, D3D12_FILTER_MIN_MAG_MIP_LINEAR);
    samplers[1] = CD3DX12_STATIC_SAMPLER_DESC(1, D3D12_FILTER_MIN_MAG_MIP_POINT);


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
