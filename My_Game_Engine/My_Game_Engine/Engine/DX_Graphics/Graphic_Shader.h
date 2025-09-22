#include "pch.h"
#include "PipelineDescFactory.h"
#include "Graphic_RootSignature.h"

enum class ShaderVariant
{
    Default = 0,
    Shadow,
    DepthOnly,
    Count
};

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


struct VariantConfig 
{
    ShaderVariant variant;
    ShaderSetting setting;
    PipelinePreset preset;
};

class Shader
{
public:
    Shader(RootSignature_Type rootType) : mRootType(rootType) {}
        
    void SetShader(ComPtr<ID3D12GraphicsCommandList> cmdList, ShaderVariant shadervariant = ShaderVariant::Default);

    ComPtr<ID3DBlob> CompileShader(const ShaderStage& stage);
    void CreateGraphicsPSO(ID3D12Device* device, RootSignature_Type rootType, ShaderVariant variant, const ShaderSetting& setting, const PipelinePreset& preset, D3D12_PRIMITIVE_TOPOLOGY_TYPE topologyType);
    void CreateAllGraphicsPSOs(ID3D12Device* device, RootSignature_Type rootType, const std::vector<VariantConfig>& configs, D3D12_PRIMITIVE_TOPOLOGY_TYPE topologyType);


    ID3D12PipelineState* GetPSO(ShaderVariant variant) const { return mPSOs[(int)variant].Get(); }
    RootSignature_Type GetRootType() const { return mRootType; }
    D3D12_PRIMITIVE_TOPOLOGY_TYPE GetPrimitiveTopologyType() { return mPrimitiveTopologyType; }

private:
    RootSignature_Type mRootType;
    D3D12_PRIMITIVE_TOPOLOGY_TYPE mPrimitiveTopologyType;
    std::array<ComPtr<ID3D12PipelineState>, (int)ShaderVariant::Count> mPSOs;
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


    std::shared_ptr<Shader> RegisterShader(const std::string& name, RootSignature_Type rootType, const std::vector<VariantConfig>& variants, D3D12_PRIMITIVE_TOPOLOGY_TYPE topologyType);
    std::shared_ptr<Shader> GetShader(const std::string& name);

    void BindShader(ComPtr<ID3D12GraphicsCommandList> cmdList, const std::string& name, ShaderVariant variant = ShaderVariant::Default);

private:
    ComPtr<ID3D12Device> mDevice;
    std::unordered_map<std::string, std::shared_ptr<Shader>> mShaders;

    std::shared_ptr<Shader> mCurrentShader;
    ShaderVariant mCurrentVariant = ShaderVariant::Count;
};
