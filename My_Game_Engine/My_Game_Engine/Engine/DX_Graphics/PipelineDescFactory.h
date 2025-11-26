enum class InputLayoutPreset
{
	Default, // Hot/Cold
    PosNormTex,
    PosColor,
    PosOnly,
    None,
};

enum class RasterizerPreset 
{ 
    Default, 
    Wireframe, 
    Shadow,
};

enum class BlendPreset 
{ 
    Opaque, 
    AlphaBlend, 
    Additive,
};

enum class DepthPreset 
{ 
    Default, 
    ReadOnly, 
    Disabled,
};

enum class RenderTargetPreset
{
    OnePass,
    MRT,
    ShadowMap
};


struct RenderTargetDesc
{
    UINT numRenderTargets = 1;
    DXGI_FORMAT rtvFormats[8] = { DXGI_FORMAT_R8G8B8A8_UNORM };
    DXGI_FORMAT dsvFormat = DXGI_FORMAT_D32_FLOAT;
    DXGI_SAMPLE_DESC sampleDesc = { 1, 0 };
    UINT sampleMask = UINT_MAX;
};

struct PipelinePreset
{
    InputLayoutPreset inputlayout = InputLayoutPreset::Default;
    RasterizerPreset rasterizer = RasterizerPreset::Default;
    BlendPreset      blend = BlendPreset::AlphaBlend;
    DepthPreset      depth = DepthPreset::Default;
    RenderTargetPreset RenderTarget = RenderTargetPreset::MRT;
};

struct PipelineDescFactory
{
    static D3D12_INPUT_LAYOUT_DESC GetInputLayout(InputLayoutPreset preset);
    static D3D12_RASTERIZER_DESC GetRasterizer(RasterizerPreset preset);
    static D3D12_BLEND_DESC      GetBlend(BlendPreset preset);
    static D3D12_DEPTH_STENCIL_DESC GetDepth(DepthPreset preset);
    static RenderTargetDesc GetRenderTargetDesc(RenderTargetPreset preset);
};