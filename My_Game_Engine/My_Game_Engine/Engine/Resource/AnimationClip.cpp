#include "AnimationClip.h"
#include "DXMathUtils.h"


namespace
{
    struct KeyTimeCompare {
        bool operator()(const auto& key, float time) const { return key.time < time; }
        bool operator()(float time, const auto& key) const { return time < key.time; }
    };

    template<typename T>
    void WriteKeyVector(rapidjson::Value& obj, const char* name, const std::vector<TKey<T>>& vec, rapidjson::Document::AllocatorType& alloc);

    template<>
    void WriteKeyVector<XMFLOAT3>(rapidjson::Value& obj, const char* name, const std::vector<VectorKey>& vec, rapidjson::Document::AllocatorType& alloc)
    {
        rapidjson::Value keys(rapidjson::kArrayType);
        for (const auto& k : vec)
        {
            keys.PushBack(k.time, alloc);
            keys.PushBack(k.value.x, alloc);
            keys.PushBack(k.value.y, alloc);
            keys.PushBack(k.value.z, alloc);
        }
        obj.AddMember(rapidjson::Value(name, alloc), keys, alloc);
    }

    template<>
    void WriteKeyVector<XMFLOAT4>(rapidjson::Value& obj, const char* name, const std::vector<QuatKey>& vec, rapidjson::Document::AllocatorType& alloc)
    {
        rapidjson::Value keys(rapidjson::kArrayType);
        for (const auto& k : vec)
        {
            keys.PushBack(k.time, alloc);
            keys.PushBack(k.value.x, alloc);
            keys.PushBack(k.value.y, alloc);
            keys.PushBack(k.value.z, alloc);
            keys.PushBack(k.value.w, alloc);
        }
        obj.AddMember(rapidjson::Value(name, alloc), keys, alloc);
    }

    template<typename T>
    void ReadKeyVector(const rapidjson::Value& obj, const char* name, std::vector<TKey<T>>& vec);

    template<>
    void ReadKeyVector<XMFLOAT3>(const rapidjson::Value& obj, const char* name, std::vector<VectorKey>& vec)
    {
        if (obj.HasMember(name) && obj[name].IsArray())
        {
            const auto& arr = obj[name].GetArray();
            for (size_t i = 0; i < arr.Size(); i += 4)
            {
                vec.push_back({ arr[i].GetFloat(), { arr[i + 1].GetFloat(), arr[i + 2].GetFloat(), arr[i + 3].GetFloat() } });
            }
        }
    }

    template<>
    void ReadKeyVector<XMFLOAT4>(const rapidjson::Value& obj, const char* name, std::vector<QuatKey>& vec)
    {
        if (obj.HasMember(name) && obj[name].IsArray())
        {
            const auto& arr = obj[name].GetArray();
            for (size_t i = 0; i < arr.Size(); i += 5)
            {
                vec.push_back({ arr[i].GetFloat(), { arr[i + 1].GetFloat(), arr[i + 2].GetFloat(), arr[i + 3].GetFloat(), arr[i + 4].GetFloat() } });
            }
        }
    }
}

bool AnimationClip::SaveToFile(const std::string& path) const
{
    Document doc(kObjectType);
    Document::AllocatorType& alloc = doc.GetAllocator();

    doc.AddMember("DefinitionType", (int)mAvatarDefinitionType, alloc);
    doc.AddMember("Duration", mDuration, alloc);
    doc.AddMember("TicksPerSecond", mTicksPerSecond, alloc);

    Value tracks(kObjectType);
    for (const auto& [name, track] : mTracks)
    {
        Value trackObj(kObjectType);
        WriteKeyVector(trackObj, "PositionKeys", track.PositionKeys, alloc);
        WriteKeyVector(trackObj, "RotationKeys", track.RotationKeys, alloc);
        WriteKeyVector(trackObj, "ScaleKeys", track.ScaleKeys, alloc);
        tracks.AddMember(Value(name.c_str(), alloc), trackObj, alloc);
    }
    doc.AddMember("Tracks", tracks, alloc);

    StringBuffer buffer;
    PrettyWriter<StringBuffer> writer(buffer);
    doc.Accept(writer);

    std::ofstream ofs(path);
    if (!ofs.is_open()) return false;
    ofs << buffer.GetString();
    ofs.close();
    return true;
}

bool AnimationClip::LoadFromFile(std::string path, const RendererContext& ctx)
{
    std::ifstream ifs(path);
    if (!ifs.is_open()) return false;

    std::stringstream buffer;
    buffer << ifs.rdbuf();
    std::string json = buffer.str();
    ifs.close();

    Document doc;
    if (doc.Parse(json.c_str()).HasParseError()) return false;

    if (doc.HasMember("DefinitionType"))
        mAvatarDefinitionType = (DefinitionType)doc["DefinitionType"].GetInt();
    if (doc.HasMember("Duration"))
        mDuration = doc["Duration"].GetFloat();
    if (doc.HasMember("TicksPerSecond"))
        mTicksPerSecond = doc["TicksPerSecond"].GetFloat();

    mTracks.clear();
    mTracks.reserve(doc["Tracks"].MemberCount());

    for (auto it = doc["Tracks"].MemberBegin(); it != doc["Tracks"].MemberEnd(); ++it)
    {
        std::string boneName = it->name.GetString();
        const auto& trackObj = it->value;

        AnimationTrack track;
        ReadKeyVector(trackObj, "PositionKeys", track.PositionKeys);
        ReadKeyVector(trackObj, "RotationKeys", track.RotationKeys);
        ReadKeyVector(trackObj, "ScaleKeys", track.ScaleKeys);

        mTracks.emplace_back(std::move(boneName), std::move(track));
    }

    std::sort(mTracks.begin(), mTracks.end(), [](const auto& a, const auto& b) { return a.first < b.first; });

    return true;
}

XMFLOAT3 AnimationTrack::SamplePosition(float time) const
{
    if (PositionKeys.empty()) return XMFLOAT3(0, 0, 0);

    auto itNext = std::lower_bound(PositionKeys.begin(), PositionKeys.end(), time, KeyTimeCompare{});

    if (itNext == PositionKeys.begin())
        return PositionKeys.front().value;
    if (itNext == PositionKeys.end())
        return PositionKeys.back().value;

    auto itPrev = itNext - 1;

    float keyTimeDiff = itNext->time - itPrev->time;
    if (keyTimeDiff <= 0.0f)
        return itPrev->value;

    float t = (time - itPrev->time) / keyTimeDiff;
    return Vector3::Lerp(itPrev->value, itNext->value, t);
}

XMFLOAT4 AnimationTrack::SampleRotation(float time) const
{
    if (RotationKeys.empty()) return XMFLOAT4(0, 0, 0, 1);

    auto itNext = std::lower_bound(RotationKeys.begin(), RotationKeys.end(), time, KeyTimeCompare{});

    if (itNext == RotationKeys.begin())
        return RotationKeys.front().value;
    if (itNext == RotationKeys.end())
        return RotationKeys.back().value;

    auto itPrev = itNext - 1;

    float keyTimeDiff = itNext->time - itPrev->time;
    if (keyTimeDiff <= 0.0f)
        return itPrev->value;

    float t = (time - itPrev->time) / keyTimeDiff;
    return Matrix4x4::QuaternionSlerp(itPrev->value, itNext->value, t);
}

XMFLOAT3 AnimationTrack::SampleScale(float time) const
{
    if (ScaleKeys.empty()) return XMFLOAT3(1, 1, 1);

    auto itNext = std::lower_bound(ScaleKeys.begin(), ScaleKeys.end(), time, KeyTimeCompare{});

    if (itNext == ScaleKeys.begin())
        return ScaleKeys.front().value;
    if (itNext == ScaleKeys.end())
        return ScaleKeys.back().value;

    auto itPrev = itNext - 1;

    float keyTimeDiff = itNext->time - itPrev->time;
    if (keyTimeDiff <= 0.0f)
        return itPrev->value;

    float t = (time - itPrev->time) / keyTimeDiff;
    return Vector3::Lerp(itPrev->value, itNext->value, t);
}

XMMATRIX AnimationTrack::Sample(float time) const
{
    XMFLOAT3 s_val = SampleScale(time);
    XMFLOAT4 r_val = SampleRotation(time);
    XMFLOAT3 t_val = SamplePosition(time);

    XMVECTOR S = XMLoadFloat3(&s_val);
    XMVECTOR R = XMLoadFloat4(&r_val);
    XMVECTOR T = XMLoadFloat3(&t_val);

    return XMMatrixScalingFromVector(S) * XMMatrixRotationQuaternion(R) * XMMatrixTranslationFromVector(T);
}

void AnimationTrack::Sample(float time, XMVECTOR& outS, XMVECTOR& outR, XMVECTOR& outT) const
{
    XMFLOAT3 s = SampleScale(time);
    XMFLOAT4 r = SampleRotation(time);
    XMFLOAT3 t = SamplePosition(time);

    outS = XMLoadFloat3(&s);
    outR = XMLoadFloat4(&r);
    outT = XMLoadFloat3(&t);
}

AnimationClip::AnimationClip()
    : Game_Resource(ResourceType::AnimationClip)
{
}

UINT AnimationClip::GetTotalKeyframes() 
{
    if (!TotalKeyframe)
    {
        for (const auto& trackPair : mTracks)
        {
            const AnimationTrack& track = trackPair.second;
            TotalKeyframe += track.PositionKeys.size();
            TotalKeyframe += track.RotationKeys.size();
            TotalKeyframe += track.ScaleKeys.size();
        }
    }
    return TotalKeyframe;
}

const AnimationTrack* AnimationClip::GetTrack(const std::string& boneKey) const
{
    auto it = std::lower_bound(mTracks.begin(), mTracks.end(), boneKey,
        [](const std::pair<std::string, AnimationTrack>& element, const std::string& key) {
            return element.first < key;
        });

    if (it != mTracks.end() && it->first == boneKey)
    {
        return &it->second;
    }
    return nullptr;
}

const AnimationTrack* AnimationClip::GetRootTrack() const
{
    if (!mModelSkeleton)
        return nullptr;

    int rootIdx = mModelSkeleton->GetRootBoneIndex();
    if (rootIdx == -1)
        return nullptr;

    const std::string& rootName = mModelSkeleton->GetBoneName(rootIdx);

    return GetTrack("Hips");
}

