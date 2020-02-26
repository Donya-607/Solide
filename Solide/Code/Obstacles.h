#pragma once

#include "ObjectBase.h"

class ObstacleBase : protected Solid
{
public:
	static bool LoadModels();
public:
	virtual void Init() {}
	virtual void Uninit() {}
	virtual void Update( float elapsedTime ) {}
	virtual void Draw( const Donya::Vector4x4 &VP, const Donya::Vector4 &lightDir, const Donya::Vector4 &color ) = 0;
};

class Stone : public ObstacleBase
{
public:
	void Draw( const Donya::Vector4x4 &VP, const Donya::Vector4 &lightDir, const Donya::Vector4 &color ) override;
};

class Log : public ObstacleBase
{
public:
	void Draw( const Donya::Vector4x4 &VP, const Donya::Vector4 &lightDir, const Donya::Vector4 &color ) override;
};