#include "pch.h"

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

namespace RootParameter
{
    enum Slot : UINT
    {
        SceneCBV = 0,

        ObjectCBV = 1,
        CameraCBV = 2,

        DiffuseTexture = 3,
        NormalTexture,
        RoughnessTexture,
        MetallicTexture,

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
    static ComPtr<ID3D12RootSignature> CreateTerrain(ID3D12Device* device);
    static ComPtr<ID3D12RootSignature> CreateSkinned(ID3D12Device* device);
    static ComPtr<ID3D12RootSignature> CreateUI(ID3D12Device* device);
};
