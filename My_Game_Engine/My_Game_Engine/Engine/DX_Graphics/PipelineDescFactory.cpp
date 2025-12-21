#include "PipelineDescFactory.h"

D3D12_INPUT_LAYOUT_DESC PipelineDescFactory::GetInputLayout(InputLayoutPreset preset)
{
    static std::vector<D3D12_INPUT_ELEMENT_DESC> hotColdLayout =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0, 0,  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT,    0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TANGENT",  0, DXGI_FORMAT_R32G32B32_FLOAT,    0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,       1, 0,  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 1, DXGI_FORMAT_R32G32_FLOAT,       1, 8,  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 16, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
    };

    static std::vector<D3D12_INPUT_ELEMENT_DESC> posNormTexLayout = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,          D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12,          D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, 24,          D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    };

    static std::vector<D3D12_INPUT_ELEMENT_DESC> posColorLayout = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,          D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12,          D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    };

    static std::vector<D3D12_INPUT_ELEMENT_DESC> posOnlyLayout = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,          D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    };

    static std::vector<D3D12_INPUT_ELEMENT_DESC> terrainLayout = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0, 0,  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT,    0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TANGENT",  0, DXGI_FORMAT_R32G32B32_FLOAT,    0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,       1, 0,  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 1, DXGI_FORMAT_R32G32_FLOAT,       1, 8,  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 16, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },

        { "INST_POS",   0, DXGI_FORMAT_R32G32_FLOAT,    2, 0,  D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 },
        { "INST_SCALE", 0, DXGI_FORMAT_R32_FLOAT,       2, 8,  D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 },
        { "INST_LOD",   0, DXGI_FORMAT_R32_FLOAT,       2, 12, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 },
        { "INST_UV",    0, DXGI_FORMAT_R32G32B32_FLOAT, 2, 16, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 },
        { "INST_HEIGHT_SCALE", 0, DXGI_FORMAT_R32_FLOAT, 2, 28, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 }
    };

    switch (preset)
    {
    case InputLayoutPreset::Default:
        return { hotColdLayout.data(), (UINT)hotColdLayout.size() };
    case InputLayoutPreset::PosNormTex:
        return { posNormTexLayout.data(), (UINT)posNormTexLayout.size() };
    case InputLayoutPreset::PosColor:
        return { posColorLayout.data(), (UINT)posColorLayout.size() };
    case InputLayoutPreset::PosOnly:
        return { posOnlyLayout.data(), (UINT)posOnlyLayout.size() };
	case InputLayoutPreset::Terrain:
		return { terrainLayout.data(), (UINT)terrainLayout.size() };
    case InputLayoutPreset::None:
        return { nullptr, 0 };
    default:
        return { hotColdLayout.data(), (UINT)hotColdLayout.size() };
    }
}

D3D12_RASTERIZER_DESC PipelineDescFactory::GetRasterizer(RasterizerPreset preset)
{
    D3D12_RASTERIZER_DESC desc = {};

    desc.FillMode = D3D12_FILL_MODE_SOLID;
    desc.CullMode = D3D12_CULL_MODE_BACK;
    desc.FrontCounterClockwise = FALSE;
    desc.DepthBias = 0;
    desc.DepthBiasClamp = 0.0f;
    desc.SlopeScaledDepthBias = 0.0f;
    desc.DepthClipEnable = TRUE;
    desc.MultisampleEnable = TRUE;
    desc.AntialiasedLineEnable = FALSE;
    desc.ForcedSampleCount = 0;
    desc.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

    switch (preset)
    {
    case RasterizerPreset::Wireframe:
        desc.FillMode = D3D12_FILL_MODE_WIREFRAME;
        desc.CullMode = D3D12_CULL_MODE_NONE;
        break;

    case RasterizerPreset::Shadow:
        desc.CullMode = D3D12_CULL_MODE_FRONT;
        desc.DepthBias = -5000;   // Reverse-Z : 음수 bias
        desc.DepthBiasClamp = 0.0f;
        desc.SlopeScaledDepthBias = -1.5f;
        break;

    default:
        desc.CullMode = D3D12_CULL_MODE_BACK;
        break;
    }

    return desc;
}


D3D12_BLEND_DESC PipelineDescFactory::GetBlend(BlendPreset preset)
{
    D3D12_BLEND_DESC desc = {};
    desc.AlphaToCoverageEnable = FALSE;
    desc.IndependentBlendEnable = FALSE;

    for (int i = 0; i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; i++)
    {
        desc.RenderTarget[i].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
        desc.RenderTarget[i].BlendEnable = FALSE;

        // 기본값 (알파 채널)
        desc.RenderTarget[i].SrcBlendAlpha = D3D12_BLEND_ONE;
        desc.RenderTarget[i].DestBlendAlpha = D3D12_BLEND_ZERO;
        desc.RenderTarget[i].BlendOpAlpha = D3D12_BLEND_OP_ADD;
    }

    switch (preset)
    {
    case BlendPreset::Opaque:
        desc.RenderTarget[0].BlendEnable = FALSE;
        break;

    case BlendPreset::AlphaBlend:
        desc.RenderTarget[0].BlendEnable = TRUE;
        desc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
        desc.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
        desc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
        desc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
        desc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_INV_SRC_ALPHA;
        desc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
        break;

    case BlendPreset::Additive:
        desc.RenderTarget[0].BlendEnable = TRUE;
        desc.RenderTarget[0].SrcBlend = D3D12_BLEND_ONE;
        desc.RenderTarget[0].DestBlend = D3D12_BLEND_ONE;
        desc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
        desc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
        desc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
        desc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
        break;

    default:
        break;
    }

    return desc;
}

D3D12_DEPTH_STENCIL_DESC PipelineDescFactory::GetDepth(DepthPreset preset)
{
    D3D12_DEPTH_STENCIL_DESC desc = {};
    desc.DepthEnable = TRUE;
    desc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	desc.DepthFunc = D3D12_COMPARISON_FUNC_GREATER_EQUAL; // Use greater equal for reversed Z
//    desc.DepthFunc = D3D12_COMPARISON_FUNC_LESS;

    switch (preset)
    {
    case DepthPreset::ReadOnly:
        desc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
        break;
    case DepthPreset::Disabled:
        desc.DepthEnable = FALSE;
        desc.StencilEnable = FALSE;
        break;
    default: break;
    }
    return desc;
}

RenderTargetDesc PipelineDescFactory::GetRenderTargetDesc(RenderTargetPreset preset)
{
    RenderTargetDesc rt = {};

    switch (preset)
    {
    case RenderTargetPreset::OnePass:
        rt.numRenderTargets = 1;
        rt.rtvFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
        rt.dsvFormat = DXGI_FORMAT_D32_FLOAT;
        rt.sampleDesc = { 1, 0 };
        break;

    case RenderTargetPreset::MRT:
        rt.numRenderTargets = 3;
        rt.rtvFormats[0] = DXGI_FORMAT_R16G16B16A16_FLOAT; 
        rt.rtvFormats[1] = DXGI_FORMAT_R16G16B16A16_FLOAT; 
        rt.rtvFormats[2] = DXGI_FORMAT_R16G16B16A16_FLOAT;
        rt.dsvFormat = DXGI_FORMAT_D32_FLOAT;
        rt.sampleDesc = { 1, 0 };
        break;

    case RenderTargetPreset::ShadowMap:
        rt.numRenderTargets = 0;
        rt.dsvFormat = DXGI_FORMAT_D32_FLOAT;
        rt.sampleDesc = { 1, 0 };
        break;

    default: break;
    }

    return rt;
}