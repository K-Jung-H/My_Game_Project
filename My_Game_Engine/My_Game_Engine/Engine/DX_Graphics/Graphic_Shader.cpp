#include "pch.h"
#include "Graphic_Shader.h"

void PSO_Manager::Init(ComPtr<ID3D12Device> device)
{
    mDevice = device;
    RootSignatureFactory::Init(mDevice.Get());
}

ShaderID PSO_Manager::RegisterShader(RootSignature_Type rootType, const ShaderSetting& setting,
    const PipelinePreset& preset, D3D12_PRIMITIVE_TOPOLOGY_TYPE topologyType)
{
    ShaderID id = ++mNextId;
    auto pso = CreateGraphicsPSO(mDevice.Get(), rootType, setting, preset, topologyType);
    mShaders[id] = std::make_shared<Shader>(rootType, pso);
    return id;
}

Shader* PSO_Manager::GetShader(ShaderID id)
{
    auto it = mShaders.find(id);
    return (it != mShaders.end()) ? it->second.get() : nullptr;
}




ComPtr<ID3DBlob> PSO_Manager::CompileShader(const ShaderStage& stage)
{
    if (!stage.IsValid()) return nullptr;

    UINT compileFlags = 0;
#if defined(_DEBUG)
    compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

    ComPtr<ID3DBlob> shaderBlob;
    ComPtr<ID3DBlob> errorBlob;

    HRESULT hr = D3DCompileFromFile(stage.file.c_str(), nullptr,
        D3D_COMPILE_STANDARD_FILE_INCLUDE, stage.entry.c_str(), stage.target.c_str(), compileFlags, 0, &shaderBlob, &errorBlob);

    if (FAILED(hr))
    {
        if (errorBlob)
        {
            OutputDebugStringA("\n[Shader Compilation Error]\n");
            OutputDebugStringA("----------------------------------------\n");
            OutputDebugStringA((char*)errorBlob->GetBufferPointer());
            OutputDebugStringA("----------------------------------------\n");
        }
        throw std::runtime_error("Shader compilation failed: " + stage.entry);
    }
    else
    {
        std::string msg = "\n[Shader Compilation] Function: ";
        msg += stage.entry;
        msg += "\n";

        OutputDebugStringA(msg.c_str());
    }

    return shaderBlob;
}


ComPtr<ID3D12PipelineState> PSO_Manager::CreateGraphicsPSO(ID3D12Device* device, RootSignature_Type rootType, 
    const ShaderSetting& setting, const PipelinePreset& preset, D3D12_PRIMITIVE_TOPOLOGY_TYPE topologyType)
{
    D3D12_GRAPHICS_PIPELINE_STATE_DESC desc = {};
    ZeroMemory(&desc, sizeof(desc));

    desc.pRootSignature = RootSignatureFactory::Get(rootType);

    ComPtr<ID3DBlob> vsBlob, psBlob, gsBlob, hsBlob, dsBlob;

    if (setting.vs.IsValid()) 
    {
        vsBlob = CompileShader(setting.vs);
        desc.VS = { vsBlob->GetBufferPointer(), vsBlob->GetBufferSize() };
    }

    if (setting.ps.IsValid()) 
    {
        psBlob = CompileShader(setting.ps);
        desc.PS = { psBlob->GetBufferPointer(), psBlob->GetBufferSize() };
    }

    if (setting.gs.IsValid()) 
    {
        gsBlob = CompileShader(setting.gs);
        desc.GS = { gsBlob->GetBufferPointer(), gsBlob->GetBufferSize() };
    }

    if (setting.hs.IsValid()) 
    {
        hsBlob = CompileShader(setting.hs);
        desc.HS = { hsBlob->GetBufferPointer(), hsBlob->GetBufferSize() };
    }
    
    if (setting.ds.IsValid()) 
    {
        dsBlob = CompileShader(setting.ds);
        desc.DS = { dsBlob->GetBufferPointer(), dsBlob->GetBufferSize() };
    }

    desc.InputLayout = PipelineDescFactory::GetInputLayout(preset.inputlayout);
    desc.RasterizerState = PipelineDescFactory::GetRasterizer(preset.rasterizer);
    desc.BlendState = PipelineDescFactory::GetBlend(preset.blend);
    desc.DepthStencilState = PipelineDescFactory::GetDepth(preset.depth);
    desc.PrimitiveTopologyType = topologyType;

    RenderTargetDesc rtDesc = PipelineDescFactory::GetRenderTargetDesc(preset.RenderTarget);
    desc.NumRenderTargets = rtDesc.numRenderTargets;
    for (UINT i = 0; i < rtDesc.numRenderTargets; ++i)
        desc.RTVFormats[i] = rtDesc.rtvFormats[i];
    desc.DSVFormat = rtDesc.dsvFormat;
    desc.SampleDesc = rtDesc.sampleDesc;
    desc.SampleMask = rtDesc.sampleMask;


    ComPtr<ID3D12PipelineState> pso;
    HRESULT hr = device->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(&pso));
    if (FAILED(hr)) 
    {
        OutputDebugStringA("Failed to create Graphics PSO\n");
        throw std::runtime_error("CreateGraphicsPSO failed");
    }

}
