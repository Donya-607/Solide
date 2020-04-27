#include "EffectAdmin.h"

#include <d3d11.h>

#include "Effekseer.h"
#include "EffekseerRendererDX11.h"

#include "Donya/Useful.h"

namespace Fx			= Effekseer;
namespace FxRenderer	= EffekseerRendererDX11;

namespace
{
	Fx::Vector2D		ToFxVector( const Donya::Vector2 &v )	{ return Fx::Vector2D{ v.x, v.y };			}
	Fx::Vector3D		ToFxVector( const Donya::Vector3 &v )	{ return Fx::Vector3D{ v.x, v.y, v.z };		}
	Fx::Matrix44		ToFxMatrix( const Donya::Vector4x4 &m )
	{
		Fx::Matrix44 fx{};

		for ( int row = 0; row < 4; ++row )
		{
			for ( int column = 0; column < 4; ++column )
			{
				fx.Values[row][column] = m.m[row][column];
			}
		}

		return fx;
	}
	Donya::Vector2		ToVector( const Fx::Vector2D &v )		{ return Donya::Vector2{ v.X, v.Y };		}
	Donya::Vector3		ToVector( const Fx::Vector3D &v )		{ return Donya::Vector3{ v.X, v.Y, v.Z };	}
	Donya::Vector4x4	ToMatrix( const Fx::Matrix44 &fx )
	{
		Donya::Vector4x4 m{};

		for ( int row = 0; row < 4; ++row )
		{
			for ( int column = 0; column < 4; ++column )
			{
				m.m[row][column] = fx.Values[row][column];
			}
		}

		return m;
	}
}

struct EffectAdmin::Impl
{
public:
	const int		maxInstanceCount	= 4096;
	const int32_t	maxSpriteCount		= 4096;
public:
	Fx::Manager				*pManager	= nullptr;
	FxRenderer::Renderer	*pRenderer	= nullptr;
public:
	bool Init( ID3D11Device *pDevice, ID3D11DeviceContext *pImmediateContext )
	{
		constexpr const wchar_t *ERR_MSG = L"Error: Effect initialize is failed.";

		if ( !pDevice || !pImmediateContext ) { _ASSERT_EXPR( 0, ERR_MSG ); return false; }
		// else

		// Create a renderer and manager
		{
			pRenderer = FxRenderer::Renderer::Create( pDevice, pImmediateContext, maxSpriteCount );
			pManager = Fx::Manager::Create( maxInstanceCount );
		}
		if ( !pRenderer || !pManager ) { _ASSERT_EXPR( 0, ERR_MSG ); return false; }
		// else

		// Sprcify rendering modules
		{
			pManager->SetSpriteRenderer	( pRenderer->CreateSpriteRenderer()	);
			pManager->SetRibbonRenderer	( pRenderer->CreateRibbonRenderer()	);
			pManager->SetRingRenderer	( pRenderer->CreateRingRenderer()	);
			pManager->SetTrackRenderer	( pRenderer->CreateTrackRenderer()	);
			pManager->SetModelRenderer	( pRenderer->CreateModelRenderer()	);
		}

		// Specify a texture, model and material loader
		{
			pManager->SetTextureLoader	( pRenderer->CreateTextureLoader()	);
			pManager->SetModelLoader	( pRenderer->CreateModelLoader()	);
			pManager->SetMaterialLoader	( pRenderer->CreateMaterialLoader()	);
		}

		// Set default camera matrix and projection matrix
		{
			constexpr Donya::Vector2 defaultWindowSize{ 800.0f, 608.0f };
			constexpr float defaultAspect	= defaultWindowSize.x / defaultWindowSize.y;
			constexpr float defaultNear		= 1.0f;
			constexpr float defaultFar		= 1.0f;

			SetProjectionMatrix
			(
				Donya::Vector4x4::MakePerspectiveFovLH
				(
					ToRadian( 90.0f ),
					defaultAspect,
					defaultNear,
					defaultFar
				)
			);

			SetViewMatrix
			(
				Donya::Vector4x4::MakeLookAtLH
				(
					-Donya::Vector3::Front(),
					Donya::Vector3::Zero(),
					Donya::Vector3::Up()
				)
			);
		}

		return true;
	}
	void Uninit()
	{
		pManager->Destroy();
		pRenderer->Destroy();
	}

	void Update( float elapsedTime )
	{

	}
	
	void Draw( float elapsedTime )
	{

	}
public:
	void SetViewMatrix( const Donya::Vector4x4 &m )
	{
		pRenderer->SetCameraMatrix( ToFxMatrix( m ) );
	}
	void SetProjectionMatrix( const Donya::Vector4x4 &m )
	{
		pRenderer->SetProjectionMatrix( ToFxMatrix( m ) );
	}
};


EffectAdmin::EffectAdmin() : Singleton(), pImpl( std::make_unique<EffectAdmin::Impl>() ) {}

bool EffectAdmin::Init( ID3D11Device *pDevice, ID3D11DeviceContext *pImmediateContext )
												{ return pImpl->Init( pDevice, pImmediateContext ); }
void EffectAdmin::Uninit()						{ pImpl->Uninit(); }
void EffectAdmin::Update( float elapsedTime )	{ pImpl->Update( elapsedTime ); }
void EffectAdmin::Draw( float elapsedTime )		{ pImpl->Draw( elapsedTime ); }

void EffectAdmin::SetViewMatrix( const Donya::Vector4x4 &m )		{ pImpl->SetViewMatrix( m ); }
void EffectAdmin::SetProjectionMatrix( const Donya::Vector4x4 &m )	{ pImpl->SetProjectionMatrix( m ); }
