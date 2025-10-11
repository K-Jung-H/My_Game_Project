
class Scene;
class Object;

enum class SceneFileFormat
{
    JSON,
    Binary
};

class SceneArchive
{
public:
    bool Save(const std::shared_ptr<Scene>& scene, const std::string& file_name, SceneFileFormat format);
    std::shared_ptr<Scene> Load(const std::string& file_name, SceneFileFormat format);

private:
    bool SaveJSON(const std::shared_ptr<Scene>& scene, const std::string& file_name);
    bool SaveBinary(const std::shared_ptr<Scene>& scene, const std::string& file_name);

    std::shared_ptr<Scene> LoadJSON(const std::string& file_name);
    std::shared_ptr<Object> LoadObjectRecursive(const std::shared_ptr<Scene> scene, const rapidjson::Value& val);

    std::shared_ptr<Scene> LoadBinary(const std::string& file_name);
};