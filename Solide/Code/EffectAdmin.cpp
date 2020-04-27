#include "EffectAdmin.h"

#include <d3d11.h>

#include "Effekseer.h"
#include "EffekseerRendererDX11.h"

struct EffectAdmin::Impl
{
public:
	bool Init( ID3D11Device *pDevice )
	{

	}
	void Uninit()
	{

	}

	void Update( float elapsedTime )
	{

	}
	
	void Draw( float elapsedTime )
	{

	}
};


EffectAdmin::EffectAdmin() : Singleton(), pImpl( std::make_unique<EffectAdmin::Impl>() ) {}

bool EffectAdmin::Init( ID3D11Device *pDevice )	{ return pImpl->Init( pDevice ); }
void EffectAdmin::Uninit()						{ pImpl->Uninit(); }
void EffectAdmin::Update( float elapsedTime )	{ pImpl->Update( elapsedTime ); }
void EffectAdmin::Draw( float elapsedTime )		{ pImpl->Draw( elapsedTime ); }
