#include "pch.h"
#include "PipelineDescFactory.h"
#include "Graphic_RootSignature.h"

struct ShaderStage
{
    std::wstring file;
    std::string  entry;
    std::string  target;

    bool IsValid() const { return !file.empty() && !entry.empty() && !target.empty(); }
};

struct ShaderSetting
{
    ShaderStage vs;
    ShaderStage ps;
    ShaderStage gs;
    ShaderStage hs;
    ShaderStage ds;
    ShaderStage cs;
};

class PSO_Manager
{
public:
    explicit PSO_Manager(ComPtr<ID3D12Device> device) : mDevice(device)
    {
        RootSignatureFactory::Init(mDevice.Get());
    }
    
    PSO_Manager() = delete;

    ComPtr<ID3D12PipelineState> CreateGraphicsPSO(ID3D12Device* device, RootSignature_Type rootType, 
        const ShaderSetting& setting, const PipelinePreset& preset, D3D12_PRIMITIVE_TOPOLOGY_TYPE topologyType);

protected:
    ComPtr<ID3DBlob> CompileShader(const ShaderStage& stage);

private:
    ComPtr<ID3D12Device> mDevice;

};