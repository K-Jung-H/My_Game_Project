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


using ShaderID = UINT;

class Shader
{
public:
    Shader(RootSignature_Type rootType, const ComPtr<ID3D12PipelineState>& shader_pso)
        : mRootType(rootType), pso(shader_pso) { }
        

    ID3D12PipelineState* GetPSO() const { return pso.Get(); }
    RootSignature_Type GetRootType() const { return mRootType; }

private:
    RootSignature_Type mRootType;
    ComPtr<ID3D12PipelineState> pso;
};

class PSO_Manager
{
public:
    static PSO_Manager& Instance()
    {
        static PSO_Manager instance;
        return instance;
    }

    void Init(ComPtr<ID3D12Device> device);
    
    PSO_Manager() = default;
    ~PSO_Manager() = default;


    ShaderID RegisterShader(RootSignature_Type rootType, const ShaderSetting& setting,
        const PipelinePreset& preset, D3D12_PRIMITIVE_TOPOLOGY_TYPE topologyType);

    Shader* GetShader(ShaderID id);


protected:
    ComPtr<ID3DBlob> CompileShader(const ShaderStage& stage);
    ComPtr<ID3D12PipelineState> CreateGraphicsPSO(ID3D12Device* device, RootSignature_Type rootType,
        const ShaderSetting& setting, const PipelinePreset& preset, D3D12_PRIMITIVE_TOPOLOGY_TYPE topologyType);

private:
    ComPtr<ID3D12Device> mDevice;
    ShaderID mNextId = 0;
    std::unordered_map<ShaderID, std::shared_ptr<Shader>> mShaders;
};

