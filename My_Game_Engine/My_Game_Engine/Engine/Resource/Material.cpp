#include "pch.h"
#include "Material.h"

Material::Material() : Game_Resource()
{

}

bool Material::LoadFromFile(std::string_view path)
{
	return true;
}

void Material::FromAssimp(const aiMaterial* material)
{

}
