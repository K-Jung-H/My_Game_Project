
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

namespace RootParameter_Shadow
{
    enum Slot : UINT
    {
        ShadowMatrix_Index = 0,
        ObjectCBV = 1,
        ShadowMatrix_SRV = 2, 
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

namespace RootParameter_Skinning
{
    enum Slot : UINT
    {
        SkinningConstants = 0,
        SkinDataSRV = 1,
        HotInputSRV = 2,
        BoneMatricesSRV = 3,
        SkinnedOutputUAV = 4,
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
        ClusterLightMetaSRV = 7,
        ClusterLightIndicesSRV = 8,
        ShadowMatrix_SRV = 9,
        ShadowMapCSMTable = 10,
        ShadowMapSpotTable = 11,
        ShadowMapPointTable = 12,
        Count
    };
}

enum class RootSignature_Type : int
{
    Default,
    Skinning,
    Terrain,
    PostFX,
    UI,
    ShadowPass,
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
    static ComPtr<ID3D12RootSignature> CreateSkinning(ID3D12Device* device);
    static ComPtr<ID3D12RootSignature> CreateUI(ID3D12Device* device);
    static ComPtr<ID3D12RootSignature> CreateShadowPass(ID3D12Device* device);
    static ComPtr<ID3D12RootSignature> CreateLightPass(ID3D12Device* device);

};
