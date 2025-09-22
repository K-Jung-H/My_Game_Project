#include "pch.h"
#include "Graphic_RootSignature.h"

void RootSignatureFactory::Init(ID3D12Device* device)
{
    if (initialized) return;

    cache[(int)RootSignature_Type::Default] = CreateDefault(device);
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
    D3D12_DESCRIPTOR_RANGE ranges[ShaderRegister::Count];
    for (UINT i = 0; i < ShaderRegister::Count; i++)
    {
        ranges[i].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
        ranges[i].NumDescriptors = 1;
        ranges[i].BaseShaderRegister = i;
        ranges[i].RegisterSpace = 0;
        ranges[i].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
    }

    D3D12_ROOT_PARAMETER params[RootParameter::Count] = {};

    params[RootParameter::SceneCBV].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    params[RootParameter::SceneCBV].Descriptor.ShaderRegister = 0;
    params[RootParameter::SceneCBV].Descriptor.RegisterSpace = 0;
    params[RootParameter::SceneCBV].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;


    params[RootParameter::ObjectCBV].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    params[RootParameter::ObjectCBV].Descriptor.ShaderRegister = 1;
    params[RootParameter::ObjectCBV].Descriptor.RegisterSpace = 0;
    params[RootParameter::ObjectCBV].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

    params[RootParameter::CameraCBV].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    params[RootParameter::CameraCBV].Descriptor.ShaderRegister = 2;
    params[RootParameter::CameraCBV].Descriptor.RegisterSpace = 0;
    params[RootParameter::CameraCBV].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

    params[RootParameter::DiffuseTexture].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    params[RootParameter::DiffuseTexture].DescriptorTable.NumDescriptorRanges = 1;
    params[RootParameter::DiffuseTexture].DescriptorTable.pDescriptorRanges = &ranges[ShaderRegister::Diffuse];
    params[RootParameter::DiffuseTexture].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    params[RootParameter::NormalTexture].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    params[RootParameter::NormalTexture].DescriptorTable.NumDescriptorRanges = 1;
    params[RootParameter::NormalTexture].DescriptorTable.pDescriptorRanges = &ranges[ShaderRegister::Normal];
    params[RootParameter::NormalTexture].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    params[RootParameter::RoughnessTexture].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    params[RootParameter::RoughnessTexture].DescriptorTable.NumDescriptorRanges = 1;
    params[RootParameter::RoughnessTexture].DescriptorTable.pDescriptorRanges = &ranges[ShaderRegister::Roughness];
    params[RootParameter::RoughnessTexture].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    params[RootParameter::MetallicTexture].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    params[RootParameter::MetallicTexture].DescriptorTable.NumDescriptorRanges = 1;
    params[RootParameter::MetallicTexture].DescriptorTable.pDescriptorRanges = &ranges[ShaderRegister::Metallic];
    params[RootParameter::MetallicTexture].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    D3D12_STATIC_SAMPLER_DESC samplers[2] = {};
    samplers[0].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
    samplers[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    samplers[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    samplers[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    samplers[0].MipLODBias = 0;
    samplers[0].MaxAnisotropy = 1;
    samplers[0].ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
    samplers[0].MinLOD = 0;
    samplers[0].MaxLOD = D3D12_FLOAT32_MAX;
    samplers[0].ShaderRegister = 0;
    samplers[0].RegisterSpace = 0;
    samplers[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    samplers[1] = samplers[0];
    samplers[1].AddressU = samplers[1].AddressV = samplers[1].AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    samplers[1].ShaderRegister = 1;

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

    D3D12_ROOT_PARAMETER params[RootParameter::Count] = {};

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

    D3D12_ROOT_PARAMETER params[RootParameter::Count] = {};

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

    D3D12_ROOT_PARAMETER params[RootParameter::Count] = {};

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
