#include "Graphic_Shader.h"

ComPtr<ID3DBlob> Shader::CompileShader(const ShaderStage& stage)
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

void Shader::CreateGraphicsPSO(ID3D12Device* device, RootSignature_Type rootType, ShaderVariant variant, const ShaderSetting& setting, const PipelinePreset& preset, D3D12_PRIMITIVE_TOPOLOGY_TYPE topologyType)
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

    HRESULT hr = device->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(&mPSOs[(int)variant]));
    if (FAILED(hr)) 
        throw std::runtime_error("Failed to create PSO for ShaderVariant");
}

void Shader::CreateAllGraphicsPSOs(ID3D12Device* device, RootSignature_Type rootType, const std::vector<VariantConfig>& configs, D3D12_PRIMITIVE_TOPOLOGY_TYPE topologyType)
{
    mPrimitiveTopologyType = topologyType;
    mRootType = rootType;

    for (const auto& cfg : configs) 
    {
        CreateGraphicsPSO(device, rootType, cfg.variant, cfg.setting, cfg.preset, topologyType);
    }
}

void Shader::SetShader(ComPtr<ID3D12GraphicsCommandList> cmdList, ShaderVariant shadervariant)
{
    ID3D12PipelineState* pso = mPSOs[(int)shadervariant].Get();
    if (!pso)
    {
        OutputDebugStringA("Shader::SetShader - PSO is null\n");
        return;
    }

    cmdList->SetPipelineState(pso);


    ID3D12RootSignature* rootSig = RootSignatureFactory::Get(mRootType);

    if (rootSig)
        cmdList->SetGraphicsRootSignature(rootSig);
}


void PSO_Manager::Init(ComPtr<ID3D12Device> device)
{
    mDevice = device;
    RootSignatureFactory::Init(mDevice.Get());
}

std::shared_ptr<Shader> PSO_Manager::RegisterShader(const std::string& name, RootSignature_Type rootType, const std::vector<VariantConfig>& variants, D3D12_PRIMITIVE_TOPOLOGY_TYPE topologyType)
{
    auto it = mShaders.find(name);
    if (it != mShaders.end()) 
    {
        OutputDebugStringA(("Shader already registered: " + name + "\n").c_str());
        return it->second;
    }

    auto shader = std::make_shared<Shader>(rootType);
    shader->CreateAllGraphicsPSOs(mDevice.Get(), rootType, variants, topologyType);
    mShaders[name] = shader;
    return shader;
}


std::shared_ptr<Shader> PSO_Manager::GetShader(const std::string& name)
{
    auto it = mShaders.find(name);
    return (it != mShaders.end()) ? it->second : nullptr;
}

void PSO_Manager::BindShader(ComPtr<ID3D12GraphicsCommandList> cmdList, const std::string& name, ShaderVariant variant)
{
    auto shader = GetShader(name);
    if (!shader) 
    {
        OutputDebugStringA(("PSO_Manager::BindShader - Shader not found: " + name + "\n").c_str());
        return;
    }

    shader->SetShader(cmdList, variant);

    mCurrentShader = shader;
    mCurrentVariant = variant;
}