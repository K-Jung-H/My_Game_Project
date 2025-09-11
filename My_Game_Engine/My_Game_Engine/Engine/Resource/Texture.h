#pragma once
#include "Game_Resource.h"

class Texture : public Game_Resource
{
public:
	Texture() {};
	virtual ~Texture() = default;
	virtual bool LoadFromFile(std::string_view path);
};

