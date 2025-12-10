#include "GameEngine.h"
#include "Model.h"

Model::Model()
    : Game_Resource(ResourceType::Model)
{

}

bool Model::loadAndExport(const std::string& fbxPath, const std::string& outTxt) 
{
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(
        fbxPath,
        aiProcess_Triangulate | 
        aiProcess_JoinIdenticalVertices | 
        aiProcess_SortByPType
    );

    if (!scene) 
    {
        std::cerr << "Assimp: 파일 로드 실패 - " << importer.GetErrorString() << "\n";
        return false;
    }

    std::ofstream fout(outTxt);
    if (!fout.is_open()) 
    {
        std::cerr << "출력 파일을 열 수 없습니다: " << outTxt << "\n";
        return false;
    }


    for (unsigned int i = 0; i < scene->mNumMeshes; ++i) 
    {
        const aiMesh* mesh = scene->mMeshes[i];
        std::string meshName = mesh->mName.length > 0 ? mesh->mName.C_Str() : ("Mesh_" + std::to_string(i));
        fout << "Mesh: " << meshName << "\n";


        unsigned int matIndex = mesh->mMaterialIndex;
        if (matIndex < scene->mNumMaterials) 
        {
            const aiMaterial* material = scene->mMaterials[matIndex];

            aiString matName;
            if (material->Get(AI_MATKEY_NAME, matName) == AI_SUCCESS)
            {
                fout << "  Material: " << matName.C_Str() << "\n";
            }
            else 
            {
                fout << "  Material: Material_" << matIndex << "\n";
            }

            // 텍스처 타입별 출력
            const aiTextureType types[] = {
                aiTextureType_DIFFUSE,
                aiTextureType_SPECULAR,
                aiTextureType_NORMALS,
                aiTextureType_HEIGHT
            };
            const char* typeNames[] = { "Diffuse", "Specular", "Normal", "Height" };

            for (int t = 0; t < 4; ++t) {
                unsigned int texCount = material->GetTextureCount(types[t]);
                for (unsigned int j = 0; j < texCount; ++j) {
                    aiString texPath;
                    if (material->GetTexture(types[t], j, &texPath) == AI_SUCCESS) {
                        fout << "    " << typeNames[t] << " Texture: "
                            << texPath.C_Str() << "\n";
                    }
                }
            }
        }
    }

    fout.close();

    std::ostringstream oss;
    oss << "[완료] 결과가 '" << outTxt << "' 파일로 저장되었습니다.\n";
    OutputDebugStringA(oss.str().c_str());

    return true;
}

UINT Model::CountNodes(const std::shared_ptr<Model>& model)
{
    if (!model || !model->GetRoot()) return 0;

    std::function<size_t(const std::shared_ptr<Node>&)> dfs;
    dfs = [&](const std::shared_ptr<Node>& n) -> size_t
        {
            if (!n) return 0;
            size_t count = 1;
            for (auto& child : n->children)
                count += dfs(child);
            return count;
        };

    return static_cast<UINT>(dfs(model->GetRoot()));
}