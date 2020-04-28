#include "EffectAdmin.h"

#include <d3d11.h>
#include <unordered_map>

#include "Effekseer.h"
#include "EffekseerRendererDX11.h"

#include "Donya/Useful.h"

#include "EffectUtil.h"


namespace Fx			= Effekseer;
namespace FxRenderer	= EffekseerRendererDX11;


class EffectWrapper
{
private:
	Fx::Effect *pFx = nullptr;
public:
	EffectWrapper( Fx::Manager *pManager, const std::basic_string<EFK_CHAR> &filePath, float magnification = 1.0f, const std::basic_string<EFK_CHAR> &materialPath = {} )
	{
		const EFK_CHAR *materialPathOrNullptr = ( materialPath.empty() ) ? nullptr : materialPath.c_str();
		pFx = Fx::Effect::Create( pManager, filePath.c_str(), magnification, materialPathOrNullptr );
	}
	~EffectWrapper()
	{
		ES_SAFE_RELEASE( pFx );
	}
public:
	bool		IsValid() const
	{
		return ( pFx );
	}
	Fx::Effect	*GetEffectOrNullptr() const
	{
		return ( IsValid() ) ? pFx : nullptr;
	}
};


struct EffectAdmin::Impl
{
private:
	const int				maxInstanceCount	= 4096;
	const int32_t			maxSpriteCount		= 4096;
private:
	Fx::Manager				*pManager			= nullptr;
	FxRenderer::Renderer	*pRenderer			= nullptr;
	bool					wasInitialized		= false;
private:
	std::unordered_map<std::basic_string<EFK_CHAR>, std::shared_ptr<EffectWrapper>> fxMap;
public:
	bool Init( ID3D11Device *pDevice, ID3D11DeviceContext *pImmediateContext )
	{
		if ( wasInitialized ) { return true; }
		// else

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

		// Specidy coordinate system
		pManager->SetCoordinateSystem( Fx::CoordinateSystem::LH );

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

		fxMap.clear();
		wasInitialized = true;
		return true;
	}
	void Uninit()
	{
		fxMap.clear();

		pManager->Destroy();
		pRenderer->Destroy();

		wasInitialized = false;
	}

	void Update( float updateSpeedMagnification )
	{
		assert( wasInitialized );
		pManager->Update( updateSpeedMagnification );
	}
	
	void Draw()
	{
		assert( wasInitialized );
		pRenderer->BeginRendering();
		pManager->Draw();
		pRenderer->EndRendering();
	}
public:
	void SetViewMatrix( const Donya::Vector4x4 &m )
	{
		assert( wasInitialized );
		pRenderer->SetCameraMatrix( ToFxMatrix( m ) );
	}
	void SetProjectionMatrix( const Donya::Vector4x4 &m )
	{
		assert( wasInitialized );
		pRenderer->SetProjectionMatrix( ToFxMatrix( m ) );
	}
public:
	Fx::Manager *GetManagerOrNullptr() const
	{
		assert( wasInitialized );
		return pManager;
	}
public:
	bool LoadEffect( EffectAttribute attr )
	{
		assert( wasInitialized );
		const std::basic_string<EFK_CHAR> filePath = GetEffectPath( attr );

		// Has already loaded?
		const auto find = fxMap.find( filePath );
		if ( find != fxMap.end() ) { return true; }
		// else

		auto pointer = std::make_shared<EffectWrapper>( pManager, filePath );
		auto result  = fxMap.insert( std::make_pair( filePath, std::move( pointer ) ) );
		return result.second;
	}
	void UnloadEffect( EffectAttribute attr )
	{
		assert( wasInitialized );
		fxMap.erase( GetEffectPath( attr ) );
	}
	void UnloadEffectAll()
	{
		assert( wasInitialized );
		fxMap.clear();
	}
public:
	Effekseer::Effect *GetEffectOrNullptr( EffectAttribute attr )
	{
		assert( wasInitialized );
		
		const auto find = fxMap.find( GetEffectPath( attr ) );
		if ( find == fxMap.end() ) { return nullptr; }
		// else

		return find->second->GetEffectOrNullptr();
	}
};


EffectAdmin::EffectAdmin()  : Singleton(), pImpl( std::make_unique<EffectAdmin::Impl>() ) {}
EffectAdmin::~EffectAdmin() = default;

bool EffectAdmin::Init( ID3D11Device *pDevice, ID3D11DeviceContext *pImmediateContext )
							{ return pImpl->Init( pDevice, pImmediateContext );	}
void EffectAdmin::Uninit()
							{ pImpl->Uninit();									}
void EffectAdmin::Update( float updateSpeedMagni )
							{ pImpl->Update( updateSpeedMagni );				}
void EffectAdmin::Draw()
							{ pImpl->Draw();									}
void EffectAdmin::SetViewMatrix( const Donya::Vector4x4 &m )
							{ pImpl->SetViewMatrix( m );						}
void EffectAdmin::SetProjectionMatrix( const Donya::Vector4x4 &m )
							{ pImpl->SetProjectionMatrix( m );					}
Fx::Manager *EffectAdmin::GetManagerOrNullptr() const
							{ return pImpl->GetManagerOrNullptr();				}
bool EffectAdmin::LoadEffect( EffectAttribute attr )
							{ return pImpl->LoadEffect( attr );					}
void EffectAdmin::UnloadEffect( EffectAttribute attr )
							{ pImpl->UnloadEffect( attr );						}
void EffectAdmin::UnloadEffectAll()
							{ pImpl->UnloadEffectAll();							}
Effekseer::Effect *EffectAdmin::GetEffectOrNullptr( EffectAttribute attr )
							{ return pImpl->GetEffectOrNullptr( attr );			}
