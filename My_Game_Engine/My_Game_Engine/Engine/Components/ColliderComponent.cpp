#include "ColliderComponent.h"

ColliderComponent::ColliderComponent(float radius) : mRadius(radius) 
{
}

rapidjson::Value ColliderComponent::ToJSON(rapidjson::Document::AllocatorType& alloc) const
{
    Value v(kObjectType);
    v.AddMember("type", "ColliderComponent", alloc);
    v.AddMember("Radius", mRadius, alloc);

    return v;
}
void ColliderComponent::FromJSON(const rapidjson::Value& val)
{
	if (val.HasMember("Radius"))
	{
		mRadius = val["Radius"].GetFloat();
	}

}