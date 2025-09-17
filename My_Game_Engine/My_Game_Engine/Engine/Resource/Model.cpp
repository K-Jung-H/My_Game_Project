#include "pch.h"
#include "GameEngine.h"
#include "Model.h"

Model::Model() : Game_Resource()
{

}

bool Model::LoadFromFile(std::string_view path, const RendererContext& ctx)
{
	return false;
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