#include "TerrainComponent.h"
#include "GameEngine.h"
#include "Core/Object.h"
#include "Components/TransformComponent.h"
#include "DX_Graphics/ResourceUtils.h"

TerrainComponent::TerrainComponent() : Component()
{
}

TerrainComponent::~TerrainComponent()
{
}

void TerrainComponent::Initialize(std::string rawDataPath, float width, float depth, float maxHeight)
{
    mWidth = width;
    mDepth = depth;
    mMaxHeight = maxHeight;

    std::ifstream file(rawDataPath, std::ios::binary | std::ios::ate);
    if (!file.is_open())
    {
        return;
    }

    std::streamsize fileSize = file.tellg();
    file.seekg(0, std::ios::beg);

    UINT totalPixels = static_cast<UINT>(fileSize / 2);
    UINT resolution = static_cast<UINT>(std::sqrt(totalPixels));

    if (resolution * resolution != totalPixels)
    {
        return;
    }

    std::vector<uint16_t> rawData(totalPixels);
    file.read(reinterpret_cast<char*>(rawData.data()), fileSize);
    file.close();

    mHeightField = std::make_unique<TerrainHeightField>();
    mHeightField->BuildFromRawData(rawData, resolution, resolution, mWidth, mDepth, mMaxHeight);


    RendererContext rc = GameEngine::Get().Get_UploadContext();
    ComPtr<ID3D12Resource> uploadBuffer;

    ComPtr<ID3D12Resource> texResource = 
        ResourceUtils::CreateTextureFromMemory(rc, rawData.data(), resolution, resolution, DXGI_FORMAT_R16_UNORM, 2, uploadBuffer);

    if (texResource)
    {
        mHeightMap = std::make_shared<Texture>();
        mHeightMap->SetResource(texResource);
        mHeightMap->SetPath(rawDataPath);
        mHeightMap->SetAlias(rawDataPath + "_HeightMap");


        auto descriptorManager = GameEngine::Get().GetResourceHeapManager();
        mHeightMap->CreateSRV(descriptorManager);
    }

    mQuadTree = std::make_unique<TerrainQuadTree>();
    mQuadTree->Initialize(mWidth, mDepth, mMaxHeight, 5);

    mPatchMesh = std::make_shared<TerrainPatchMesh>();
}

void TerrainComponent::Update()
{
    if (mTransform.expired())
    {
        std::shared_ptr<TransformComponent> tr = GetOwner()->GetComponent<TransformComponent>();
        if (tr)
        {
            mTransform = tr;
        }
    }
}

void TerrainComponent::UpdateLOD(const XMFLOAT3& cameraPos)
{
    if (!mQuadTree) return;

    std::shared_ptr<TransformComponent> tr = mTransform.lock();
    if (!tr) return;

    XMMATRIX worldMat = XMLoadFloat4x4(&tr->GetWorldMatrix());
    mQuadTree->Update(cameraPos, worldMat);
}

const std::vector<TerrainInstanceData>& TerrainComponent::GetDrawList() const
{
    if (mQuadTree)
        return mQuadTree->GetDrawList();

    static const std::vector<TerrainInstanceData> empty;
    return empty;
}

float TerrainComponent::GetHeight(XMFLOAT3 worldPos)
{
    if (!mHeightField) return 0.0f;

    std::shared_ptr<TransformComponent> tr = mTransform.lock();
    if (!tr) return 0.0f;

    XMMATRIX worldInv = XMMatrixInverse(nullptr, XMLoadFloat4x4(&tr->GetWorldMatrix()));
    XMVECTOR localPosVec = XMVector3TransformCoord(XMLoadFloat3(&worldPos), worldInv);
    XMFLOAT3 localPos;
    XMStoreFloat3(&localPos, localPosVec);

    return mHeightField->GetHeight(localPos.x, localPos.z);
}