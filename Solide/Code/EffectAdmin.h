#pragma once

#include <memory>

#include "Donya/Template.h"

struct ID3D11Device;
class EffectAdmin : public Donya::Singleton<EffectAdmin>
{
	friend Donya::Singleton<EffectAdmin>;
private:
	struct Impl;
public:
	std::unique_ptr<Impl> pImpl;
private:
	EffectAdmin();
public:
	bool Init( ID3D11Device *pDevice );
	void Uninit();

	void Update( float elapsedTime );

	void Draw( float elapsedTime );
};
