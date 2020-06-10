#include "EffectAdmin.h"

#include <d3d11.h>
#include <unordered_map>
#include <vector>

#include <cereal/types/vector.hpp>

#include "Effekseer.h"
#include "EffekseerRendererDX11.h"

#include "Donya/Constant.h"		// Use scast macro.
#include "Donya/Serializer.h"
#include "Donya/Useful.h"
#include "Donya/UseImGui.h"

#include "EffectUtil.h"
#include "FilePath.h"
#include "Parameter.h"


namespace
{
	static constexpr size_t attrCount = scast<size_t>( EffectAttribute::AttributeCount );

	std::string GetEffectName( EffectAttribute attr )
	{
		switch ( attr )
		{
		case EffectAttribute::Fire:			return "Fire";
		case EffectAttribute::FlameCannon:	return "FlameCannon";
		case EffectAttribute::IceCannon:	return "IceCannon";
		case EffectAttribute::ColdSmoke:	return "ColdSmoke";
		case EffectAttribute::PlayerSliding:return "PlayerSliding";
		default: break;
		}

		_ASSERT_EXPR( 0, L"Error: Unexpected attribute!" );
		return "ERROR_ATTR";
	}

	struct Member
	{
		std::vector<float> effectScales;
	private:
		friend class cereal::access;
		template<class Archive>
		void serialize( Archive &archive, std::uint32_t version )
		{
			archive
			(
				CEREAL_NVP( effectScales )
			);

			if ( 1 <= version )
			{
				// archive( CEREAL_NVP( x ) );
			}
		}
	};
}
CEREAL_CLASS_VERSION( Member, 0 )

class ParamEffect : public ParameterBase<ParamEffect>
{
public:
	static constexpr const char *ID = "Effect";
private:
	Member m;
public:
	void Init() override
	{
	#if DEBUG_MODE
		constexpr bool fromBinary = false;
	#else
		constexpr bool fromBinary = true;
	#endif // DEBUG_MODE

		Load( m, fromBinary );

		ResizeVectorIfNeeded();
	}
	Member Data() const { return m; }
private:
	void ResizeVectorIfNeeded()
	{
		if ( m.effectScales.size() != attrCount )
		{
			m.effectScales.resize( attrCount, 1.0f );
		}
	}
private:
	std::string GetSerializeIdentifier()			override { return ID; }
	std::string GetSerializePath( bool isBinary )	override { return GenerateSerializePath( ID, isBinary ); }
public:
#if USE_IMGUI
	void UseImGui() override
	{
		if ( !ImGui::BeginIfAllowed() ) { return; }
		// else

		if ( ImGui::TreeNode( u8"エフェクトのパラメータ調整" ) )
		{
			if ( ImGui::TreeNode( u8"スケール調整" ) )
			{
				ImGui::Text( u8"生成時に適用されます" );
				ResizeVectorIfNeeded();

				std::string caption{};
				for ( size_t i = 0; i < attrCount; ++i )
				{
					caption = u8"[" + std::to_string( i ) + u8"]:";
					caption += GetEffectName( scast<EffectAttribute>( i ) );
					ImGui::DragFloat( caption.c_str(), &m.effectScales[i], 0.01f );
				}

				ImGui::TreePop();
			}

			ShowIONode( m );

			ImGui::TreePop();
		}

		ImGui::End();
	}
#endif // USE_IMGUI
};


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
			constexpr float defaultFar		= 1000.0f;

			// Now the "wasInitialized" is false, so I don't use SetXXXMatrix() method.

			pRenderer->SetProjectionMatrix
			(
				ToFxMatrix
				(
					Donya::Vector4x4::MakePerspectiveFovLH
					(
						ToRadian( 90.0f ),
						defaultAspect,
						defaultNear,
						defaultFar
					)
				)
			);

			pRenderer->SetCameraMatrix
			(
				ToFxMatrix
				(
					Donya::Vector4x4::MakeLookAtLH
					(
						-Donya::Vector3::Front(),
						Donya::Vector3::Zero(),
						Donya::Vector3::Up()
					)
				)
			);
		}

		fxMap.clear();
		wasInitialized = true;

		ParamEffect::Get().Init();
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
	#if USE_IMGUI
		ParamEffect::Get().UseImGui();
	#endif // USE_IMGUI

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
	bool IsOutOfRange( EffectAttribute attr )
	{
		return scast<size_t>( EffectAttribute::AttributeCount ) < scast<size_t>( attr );
	}
	float GetEffectScale( EffectAttribute attr )
	{
		if ( IsOutOfRange( attr ) ) { return 0.0f; }
		// else
		const auto scales = ParamEffect::Get().Data().effectScales;
		return scales[scast<size_t>( attr )];
	}
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
float EffectAdmin:: GetEffectScale( EffectAttribute attr )
							{ return pImpl->GetEffectScale( attr ); }
Effekseer::Effect *EffectAdmin::GetEffectOrNullptr( EffectAttribute attr )
							{ return pImpl->GetEffectOrNullptr( attr );			}
