#include "SceneLoad.h"

#include <vector>

#undef max
#undef min
#include <cereal/types/vector.hpp>

#include "Donya/Blend.h"
#include "Donya/Color.h"
#include "Donya/Constant.h"
#include "Donya/Donya.h"
#include "Donya/Serializer.h"
#include "Donya/Sound.h"
#include "Donya/Sprite.h"
#include "Donya/Useful.h"
#include "Donya/Vector.h"
#if DEBUG_MODE
#include "Donya/GeometricPrimitive.h"
#include "Donya/Mouse.h"
#include "Donya/Random.h"
#endif // DEBUG_MODE

#include "Boss.h"
#include "Bullet.h"
#include "Common.h"
#include "ClearPerformance.h"
#include "EffectAdmin.h"
#include "Enemy.h"
#include "Fader.h"
#include "FilePath.h"
#include "Goal.h"
#include "Music.h"
#include "Obstacles.h"
#include "Parameter.h"
#include "Player.h"
#include "Warp.h"


#undef max
#undef min

namespace
{
	struct Member
	{
		float sprIconScale				= 1.0f;
		float sprIconRotateSpeed		= -7.0f;
		float sprLoadScale				= 1.0f;
		float sprLoadFlushingInterval	= 1.0f;
		float sprLoadFlushingRange		= 1.0f;
		float sprLoadMinAlpha			= 0.0f;
		Donya::Vector2 sprIconPos{ 960.0f, 540.0f };
		Donya::Vector2 sprLoadPos{ 960.0f, 540.0f };
	private:
		friend class cereal::access;
		template<class Archive>
		void serialize( Archive &archive, std::uint32_t version )
		{
			archive
			(
				CEREAL_NVP( sprIconScale			),
				CEREAL_NVP( sprIconRotateSpeed		),
				CEREAL_NVP( sprLoadScale			),
				CEREAL_NVP( sprLoadFlushingInterval	),
				CEREAL_NVP( sprLoadFlushingRange	),
				CEREAL_NVP( sprLoadMinAlpha			),
				CEREAL_NVP( sprIconPos				),
				CEREAL_NVP( sprLoadPos				)
			);

			if ( 1 <= version )
			{
				// archive( CEREAL_NVP( x ) );
			}
		}
	};
}
CEREAL_CLASS_VERSION( Member, 0 )

class ParamLoad : public ParameterBase<ParamLoad>
{
public:
	static constexpr const char *ID = "Loading";
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
	}
	Member Data() const { return m; }
private:
	std::string GetSerializeIdentifier()			override { return ID; }
	std::string GetSerializePath( bool isBinary )	override { return GenerateSerializePath( ID, isBinary ); }
public:
#if USE_IMGUI
	void UseImGui() override
	{
		if ( !ImGui::BeginIfAllowed() ) { return; }
		// else

		if ( ImGui::TreeNode( u8"ロード画面のパラメータ調整" ) )
		{
			auto Clamp = []( auto *v, const auto &min, const auto &max )
			{
				*v = std::max( min, std::max( min, *v ) );
			};

			if ( ImGui::TreeNode( u8"スプライトの調整" ) )
			{
				if ( ImGui::TreeNode( u8"アイコン" ) )
				{
					ImGui::DragFloat( u8"スケール", &m.sprIconScale, 0.1f );
					ImGui::DragFloat( u8"回転角度（Degree）", &m.sprIconRotateSpeed, 0.1f );
					ImGui::DragFloat2( u8"スクリーン座標", &m.sprIconPos.x );

					Clamp( &m.sprIconScale, 0.0f, m.sprIconScale );

					ImGui::TreePop();
				}
				if ( ImGui::TreeNode( u8"ロード中" ) )
				{
					ImGui::DragFloat( u8"スケール",			&m.sprLoadScale,			0.1f );
					ImGui::DragFloat( u8"点滅周期（秒）",		&m.sprLoadFlushingInterval,	0.1f );
					ImGui::DragFloat( u8"点滅範囲",			&m.sprLoadFlushingRange,	0.1f );
					ImGui::DragFloat( u8"最低アルファ値",		&m.sprLoadMinAlpha,			0.1f );
					ImGui::DragFloat2( u8"スクリーン座標",	&m.sprLoadPos.x );

					Clamp( &m.sprLoadMinAlpha, 0.0f, m.sprLoadMinAlpha );

					ImGui::TreePop();
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

namespace
{
	Member FetchMember()
	{
		return ParamLoad::Get().Data();
	}
}


void SceneLoad::Init()
{
	ParamLoad::Get().Init();

	constexpr auto CoInitValue = COINIT_MULTITHREADED | COINIT_DISABLE_OLE1DDE;
	auto LoadingEffects	= [&CoInitValue]( bool *pFinishFlag, bool *pSucceedFlag, std::mutex *pSucceedMutex )
	{
		if ( !pFinishFlag || !pSucceedFlag ) { assert( !"Error: Flag ptr is null!" ); return; }
		// else

		HRESULT hr = CoInitializeEx( NULL, CoInitValue );
		if ( FAILED( hr ) )
		{
			std::lock_guard<std::mutex> lock( *pSucceedMutex );

			*pFinishFlag  = true;
			*pSucceedFlag = false;
			return;
		}
		// else

		constexpr size_t attrCount = scast<size_t>( EffectAttribute::AttributeCount );
		constexpr std::array<EffectAttribute, attrCount> attributes
		{
			EffectAttribute::Fire,
			EffectAttribute::FlameCannon,
			EffectAttribute::IceCannon,
			EffectAttribute::ColdSmoke,
			EffectAttribute::PlayerSliding,
		};

		bool succeeded = true;
		for ( const auto &it : attributes )
		{
			if ( !EffectAdmin::Get().LoadEffect( it ) )
			{
				succeeded = false;
			}
		}
		
		_ASSERT_EXPR( succeeded, L"Failed: Effects load is failed." );

		std::lock_guard<std::mutex> lock( *pSucceedMutex );
		*pFinishFlag  = true;
		*pSucceedFlag = succeeded;

		CoUninitialize();
	};
	auto LoadingModels	= [&CoInitValue]( bool *pFinishFlag, bool *pSucceedFlag, std::mutex *pSucceedMutex )
	{
		if ( !pFinishFlag || !pSucceedFlag ) { assert( !"Error: Flag ptr is null!" ); return; }
		// else

		HRESULT hr = CoInitializeEx( NULL, CoInitValue );
		if ( FAILED( hr ) )
		{
			std::lock_guard<std::mutex> lock( *pSucceedMutex );

			*pFinishFlag  = true;
			*pSucceedFlag = false;
			return;
		}
		// else

		bool succeeded = true;

		if ( !Bullet::LoadBulletsResource()	) { succeeded = false; }
		if ( !Enemy::LoadResources()		) { succeeded = false; }
		if ( !Goal::LoadResource()			) { succeeded = false; }
		if ( !ObstacleBase::LoadModels()	) { succeeded = false; }
		if ( !Player::LoadModels()			) { succeeded = false; }
		if ( !BossBase::LoadModels()		) { succeeded = false; }
		if ( !WarpContainer::LoadResource()	) { succeeded = false; }
		ClearPerformance::LoadParameter();

		_ASSERT_EXPR( succeeded, L"Failed: Models load is failed." );

		std::lock_guard<std::mutex> lock( *pSucceedMutex );
		*pFinishFlag  = true;
		*pSucceedFlag = succeeded;

		CoUninitialize();
	};
	auto LoadingSprites	= [&CoInitValue]( bool *pFinishFlag, bool *pSucceedFlag, std::mutex *pSucceedMutex )
	{
		if ( !pFinishFlag || !pSucceedFlag ) { assert( !"Error: Flag ptr is null!" ); return; }
		// else

		HRESULT hr = CoInitializeEx( NULL, CoInitValue );
		if ( FAILED( hr ) )
		{
			std::lock_guard<std::mutex> lock( *pSucceedMutex );

			*pFinishFlag  = true;
			*pSucceedFlag = false;
			return;
		}
		// else

		using Attr = SpriteAttribute;

		auto MakeTextureCache = []( Attr attr, size_t maxInstanceCount )->bool
		{
			const auto handle = Donya::Sprite::Load( GetSpritePath( attr ), maxInstanceCount );
			return  (  handle == NULL ) ? false : true;
		};

		bool succeeded = true;
		if ( !MakeTextureCache( Attr::BackGround,		2U ) ) { succeeded = false; }
		if ( !MakeTextureCache( Attr::CircleShadow,		1U ) ) { succeeded = false; }
		if ( !MakeTextureCache( Attr::ClearDescription,	8U ) ) { succeeded = false; }
		if ( !MakeTextureCache( Attr::ClearFrame,		2U ) ) { succeeded = false; }
		if ( !MakeTextureCache( Attr::ClearRank,	  128U ) ) { succeeded = false; }
		if ( !MakeTextureCache( Attr::ClearSentence,	4U ) ) { succeeded = false; }
		if ( !MakeTextureCache( Attr::Cloud,			2U ) ) { succeeded = false; }
		if ( !MakeTextureCache( Attr::LockedStage,	   64U ) ) { succeeded = false; }
		if ( !MakeTextureCache( Attr::Number,		 1024U ) ) { succeeded = false; }
		if ( !MakeTextureCache( Attr::Pause,			8U ) ) { succeeded = false; }
		if ( !MakeTextureCache( Attr::PlayerRemains,	4U ) ) { succeeded = false; }
		if ( !MakeTextureCache( Attr::StageInfoFrame,  32U ) ) { succeeded = false; }
		if ( !MakeTextureCache( Attr::TutorialFrame,	2U ) ) { succeeded = false; }
		if ( !MakeTextureCache( Attr::TutorialSentence,	4U ) ) { succeeded = false; }
		if ( !MakeTextureCache( Attr::TitleItems,	   16U ) ) { succeeded = false; }
		if ( !MakeTextureCache( Attr::TitleLogo,		2U ) ) { succeeded = false; }
		if ( !MakeTextureCache( Attr::TitlePrompt,		2U ) ) { succeeded = false; }

		_ASSERT_EXPR( succeeded, L"Failed: Sprites load is failed." );

		std::lock_guard<std::mutex> lock( *pSucceedMutex );
		*pFinishFlag  = true;
		*pSucceedFlag = succeeded;

		CoUninitialize();
	};
	auto LoadingSounds	= [&CoInitValue]( bool *pFinishFlag, bool *pSucceedFlag, std::mutex *pSucceedMutex )
	{
		if ( !pFinishFlag || !pSucceedFlag ) { assert( !"Error: Flag ptr is null!" ); return; }
		// else

		HRESULT hr = CoInitializeEx( NULL, CoInitValue );
		if ( FAILED( hr ) )
		{
			std::lock_guard<std::mutex> lock( *pSucceedMutex );

			*pFinishFlag  = true;
			*pSucceedFlag = false;
			return;
		}
		// else

		using Music::ID;

		struct Bundle
		{
			ID			id;
			std::string	filePath;
			bool		isEnableLoop;
		public:
			Bundle( ID id, const char *filePath, bool isEnableLoop )
				: id( id ), filePath( filePath ), isEnableLoop( isEnableLoop ) {}
		};

		const std::array<Bundle, ID::MUSIC_COUNT> bundles
		{
			// ID, FilePath, isEnableLoop
			Bundle{ ID::BGM_Title,			u8"./Data/Sounds/BGM/アニマル・スマイル.wav",		true	},
			Bundle{ ID::BGM_Stage1,			"./Data/Sounds/BGM/Bouncy.wav",					true	},
			Bundle{ ID::BGM_Stage2,			u8"./Data/Sounds/BGM/アフリカ探検隊.wav",			true	},
			Bundle{ ID::BGM_Stage3,			"./Data/Sounds/BGM/powdery_snow.wav",			true	},
			Bundle{ ID::BGM_Stage4,			"./Data/Sounds/BGM/Sword_dance.wav",			true	},
			Bundle{ ID::BGM_Boss,			"./Data/Sounds/BGM/Burning-Cavern_loop.wav",	true	},
			Bundle{ ID::BGM_Clear,			u8"./Data/Sounds/BGM/うきうき.wav",				true	},

			Bundle{ ID::UI_StartGame,		"./Data/Sounds/SE/Title/Start.wav",				false	},
			Bundle{ ID::UI_Goal,			"./Data/Sounds/SE/Game/Goal.wav",				false	},

			Bundle{ ID::PlayerJump,			"./Data/Sounds/SE/Player/Jump.wav",				false	},
			Bundle{ ID::PlayerLanding,		"./Data/Sounds/SE/Player/Landing.wav",			false	},
			Bundle{ ID::PlayerShot,			"./Data/Sounds/SE/Player/OilShot.wav",			false	},
			Bundle{ ID::PlayerSliding,		"./Data/Sounds/SE/Player/Sliding.wav",			true	},
			Bundle{ ID::PlayerTrans,		"./Data/Sounds/SE/Player/Trans.wav",			false	},
			
			Bundle{ ID::SprayCold,			"./Data/Sounds/SE/Obstacle/ColdSpray.wav",		false	},
			Bundle{ ID::SprayFlame,			"./Data/Sounds/SE/Obstacle/FlameSpray.wav",		false	},
			
			Bundle{ ID::BossImpact,			"./Data/Sounds/SE/Boss/Impact.wav",				false	},
			Bundle{ ID::BossStep,			"./Data/Sounds/SE/Boss/Step.wav",				false	},

			Bundle{ ID::ItemChoose,			"./Data/Sounds/SE/UI/ChooseItem.wav",			false	},
			Bundle{ ID::ItemDecision,		"./Data/Sounds/SE/UI/DecisionItem.wav",			false	},
		};

		bool succeeded = true;
		for ( size_t i = 0; i < ID::MUSIC_COUNT; ++i )
		{
			bool result = Donya::Sound::Load
			(
				bundles[i].id,
				bundles[i].filePath,
				bundles[i].isEnableLoop
			);
			if ( !result ) { succeeded = false; }
		}

		_ASSERT_EXPR( succeeded, L"Failed: Sounds load is failed." );

		std::lock_guard<std::mutex> lock( *pSucceedMutex );
		*pFinishFlag  = true;
		*pSucceedFlag = succeeded;

		CoUninitialize();
	};

	finishEffects	= false;
	finishModels	= false;
	finishSprites	= false;
	finishSounds	= false;
	allSucceeded	= true;

	pThreadModels	= std::make_unique<std::thread>( LoadingModels,		&finishModels,	&allSucceeded, &succeedMutex );
	pThreadSounds	= std::make_unique<std::thread>( LoadingSounds,		&finishSounds,	&allSucceeded, &succeedMutex );
	pThreadSprites	= std::make_unique<std::thread>( LoadingSprites,	&finishSprites,	&allSucceeded, &succeedMutex );

	// Note: Maybe Effekseer is not supported to async load? I can not found this way.
	// pThreadEffects = std::make_unique<std::thread>( LoadingEffects,	&finishEffects,	&allSucceeded, &succeedMutex );
	LoadingEffects( &finishEffects,	&allSucceeded, &succeedMutex );

	if ( !SpritesInit() )
	{
		_ASSERT_EXPR( 0, L"Error: Loading sprites does not works!" );
	}
}
void SceneLoad::Uninit()
{
	ReleaseAllThread();

	ParamLoad::Get().Uninit();
}

Scene::Result SceneLoad::Update( float elapsedTime )
{
#if USE_IMGUI
	ParamLoad::Get().UseImGui();
	UseImGui();
#endif // USE_IMGUI

#if DEBUG_MODE
	elapsedTimer += elapsedTime;
#endif // DEBUG_MODE

	SpritesUpdate( elapsedTime );

	if ( !Fader::Get().IsExist() && IsFinished() )
	{
		if ( allSucceeded )
		{
			StartFade();
		}
		else
		{
			const HWND hWnd = Donya::GetHWnd();
			MessageBox
			(
				hWnd,
				TEXT( "リソースの読み込みが失敗しました。アプリを終了します。" ),
				TEXT( "リソース読み込みエラー" ),
				MB_ICONERROR | MB_OK
			);

			PostMessage( hWnd, WM_CLOSE, 0, 0 );

			ReleaseAllThread();

			// Prevent a true being returned by IsFinished().
			finishEffects	= false;
			finishModels	= false;
			finishSprites	= false;
			finishSounds	= false;
		}
	}

	return ReturnResult();
}

void SceneLoad::Draw( float elapsedTime )
{
	ClearBackGround();

	sprIcon.Draw();
	sprNowLoading.Draw();
}

void SceneLoad::ReleaseAllThread()
{
	auto JoinThenRelease = []( std::unique_ptr<std::thread> &p )
	{
		if ( p )
		{ 
			if ( p->joinable() )
			{
				p->join();
			}

			p.reset();
		}
	};

	JoinThenRelease( pThreadEffects	);
	JoinThenRelease( pThreadModels	);
	JoinThenRelease( pThreadSprites	);
	JoinThenRelease( pThreadSounds	);
}

bool SceneLoad::SpritesInit()
{
	const auto data = FetchMember();

	bool succeeded = true;

	constexpr size_t MAX_INSTANCE_COUNT = 1U;
	if ( !sprIcon.LoadSprite( GetSpritePath( SpriteAttribute::LoadingIcon ), MAX_INSTANCE_COUNT ) )
	{ succeeded = false; }
	if ( !sprNowLoading.LoadSprite( GetSpritePath( SpriteAttribute::LoadingSentence ), MAX_INSTANCE_COUNT ) )
	{ succeeded = false; }

	sprIcon.pos				= data.sprIconPos;
	sprIcon.drawScale		= data.sprIconScale;
	sprIcon.alpha			= 1.0f;
	sprNowLoading.pos		= data.sprLoadPos;
	sprNowLoading.drawScale	= data.sprLoadScale;
	sprNowLoading.alpha		= 1.0f;

	flushingTimer			= 0.0f;

	return succeeded;
}
void SceneLoad::SpritesUpdate( float elapsedTime )
{
	const auto data = FetchMember();

	sprIcon.degree += data.sprIconRotateSpeed * elapsedTime;

	const float cycle = data.sprLoadFlushingInterval * elapsedTime;
	if ( !ZeroEqual( cycle ) )
	{
		const float sinIncrement = 360.0f / ( 60.0f * cycle );
		flushingTimer += sinIncrement;

		const float sin_01 = ( sinf( ToRadian( flushingTimer ) ) + 1.0f ) * 0.5f;
		const float shake  = sin_01 * data.sprLoadFlushingRange;

		sprNowLoading.alpha = std::max( data.sprLoadMinAlpha, std::min( 1.0f, shake ) );
	}
}

bool SceneLoad::IsFinished() const
{
	return ( finishEffects && finishModels && finishSprites && finishSounds );
}

void SceneLoad::ClearBackGround() const
{
	constexpr Donya::Vector3 gray{ Donya::Color::MakeColor( Donya::Color::Code::GRAY ) };
	constexpr FLOAT BG_COLOR[4]{ gray.x, gray.y, gray.z, 1.0f };
	Donya::ClearViews( BG_COLOR );
}

void SceneLoad::StartFade() const
{
	Fader::Configuration config{};
	config.type			= Fader::Type::Gradually;
	config.closeFrame	= Fader::GetDefaultCloseFrame();;
	config.SetColor( Donya::Color::Code::BLACK );
	Fader::Get().StartFadeOut( config );
}

Scene::Result SceneLoad::ReturnResult()
{
	if ( Fader::Get().IsClosed() )
	{
		Scene::Result change{};
		change.AddRequest( Scene::Request::ADD_SCENE, Scene::Request::REMOVE_ME );
	#if DEBUG_MODE
		change.sceneType = Scene::Type::Game;
	#else
		change.sceneType = Scene::Type::Title;
	#endif // DEBUG_MODE
		return change;
	}
	// else

	Scene::Result noop{ Scene::Request::NONE, Scene::Type::Null };
	return noop;
}

#if USE_IMGUI
void SceneLoad::UseImGui()
{
	if ( ImGui::BeginIfAllowed() )
	{
		if ( ImGui::TreeNode( u8"ロード画面のメンバ" ) )
		{
			auto GetBoolStr = []( bool v )->std::string
			{
				return ( v ) ? "True" : "False";
			};

			ImGui::Text( u8"終了フラグ・エフェクト[%s]",	GetBoolStr( finishEffects	).c_str() );
			ImGui::Text( u8"終了フラグ・モデル[%s]",		GetBoolStr( finishModels	).c_str() );
			ImGui::Text( u8"終了フラグ・スプライト[%s]",	GetBoolStr( finishSprites	).c_str() );
			ImGui::Text( u8"終了フラグ・サウンド[%s]",	GetBoolStr( finishSounds	).c_str() );
			
			ImGui::Text( u8"経過時間：[%6.3f]", elapsedTimer );

			sprIcon.ShowImGuiNode		( u8"画像調整・アイコン" );
			sprNowLoading.ShowImGuiNode	( u8"画像調整・ロード中" );

			ImGui::TreePop();
		}

		ImGui::End();
	}
}
#endif // USE_IMGUI
