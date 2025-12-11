#pragma once
#include <DirectXMath.h>

struct TerrainInstanceData
{
    XMFLOAT3 LocalOffset;
    float             Scale;
    XMFLOAT2 UVOffset;
    XMFLOAT2 UVScale;
};