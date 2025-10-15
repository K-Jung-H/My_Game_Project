
enum class GBufferType : UINT
{
    Albedo = 0,
    Normal,
    Material,
    Count
};


namespace ShaderRegister
{
    enum SRV : UINT
    {
        Diffuse = 0, // t0
        Normal = 1, // t1
        Roughness = 2, // t2
        Metallic = 3, // t3
        Count
    };
}

namespace RootParameter_Default
{
    enum Slot : UINT
    {
        SceneCBV = 0,
        CameraCBV = 1,
        ObjectCBV = 2,
        TextureTable = 3,
        Count
    };
}

namespace RootParameter_LightPass
{
    enum Slot : UINT
    {
        SceneCBV = 0,
        CameraCBV = 1,
        LightBufferSRV = 2,
        ClusterAreaSRV = 3,
        ClusterAreaUAV = 4,
        ClusterLightMetaUAV = 5,
        ClusterLightIndicesUAV = 6,
        GlobalCounterUAV = 7,
        Count
    };
}


namespace RootParameter_PostFX
{
    enum Slot : UINT
    {
        SceneCBV = 0,
        CameraCBV = 1,
        GBufferTable = 2,
        DepthTexture = 3,
        MergeTexture = 4,
        ClusterAreaSRV = 5,
        LightBufferSRV = 6,
        Count
    };
}

enum class RootSignature_Type : int
{
    Default,
    Skinned,
    Terrain,
    PostFX,
    UI,
	LightPass, // Compute shader for light culling
    Count
};

class RootSignatureFactory
{
public:
    static void Init(ID3D12Device* device);

    static ID3D12RootSignature* Get(RootSignature_Type type);
    static void Destroy();

private:
    static inline std::array<ComPtr<ID3D12RootSignature>, (int)RootSignature_Type::Count> cache;
    static inline bool initialized = false;

    static ComPtr<ID3D12RootSignature> CreateDefault(ID3D12Device* device);
    static ComPtr<ID3D12RootSignature> CreatePostFX(ID3D12Device* device);
    static ComPtr<ID3D12RootSignature> CreateTerrain(ID3D12Device* device);
    static ComPtr<ID3D12RootSignature> CreateSkinned(ID3D12Device* device);
    static ComPtr<ID3D12RootSignature> CreateUI(ID3D12Device* device);
    static ComPtr<ID3D12RootSignature> CreateLightPass(ID3D12Device* device);

};
