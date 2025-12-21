#pragma once
#include <DirectXMath.h>

struct TerrainInstanceData
{
    XMFLOAT2 InstancePos;
    float Scale;
    float LOD;

    // xy: UV Offset 
    // z:  UV Scale
    XMFLOAT3 UVInfo;
    float HeightScale;
};