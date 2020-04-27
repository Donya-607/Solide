#pragma once

#include <memory>

#include "Donya/Template.h"
#include "Donya/Vector.h"

struct ID3D11Device;
struct ID3D11DeviceContext;
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
	bool Init( ID3D11Device *pDevice, ID3D11DeviceContext *pImmediateContext );
	void Uninit();

	void Update( float elapsedTime );

	void Draw( float elapsedTime );
public:
	void SetViewMatrix( const Donya::Vector4x4 &cameraMatrix );
	void SetProjectionMatrix( const Donya::Vector4x4 &projectionMatrix );
};
