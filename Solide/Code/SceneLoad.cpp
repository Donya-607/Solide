#include "SceneLoad.h"

#include <vector>

#undef max
#undef min
#include <cereal/types/vector.hpp>

#include "Donya/Blend.h"
#include "Donya/Color.h"
#include "Donya/Constant.h"
#include "Donya/Serializer.h"
#include "Donya/Sound.h"
#include "Donya/Useful.h"
#include "Donya/Vector.h"
#if DEBUG_MODE
#include "Donya/GeometricPrimitive.h"
#include "Donya/Mouse.h"
#include "Donya/Random.h"
#endif // DEBUG_MODE

#include "Bullet.h"
#include "Common.h"
#include "EffectAdmin.h"
#include "Fader.h"
#include "FilePath.h"
#include "Music.h"
#include "Obstacles.h"
#include "Parameter.h"
#include "SaveData.h"
#include "StageNumberDefine.h"


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

	const auto data = FetchMember();

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

	assert( succeeded );
}
void SceneLoad::Uninit()
{
	ParamLoad::Get().Uninit();
}

Scene::Result SceneLoad::Update( float elapsedTime )
{
#if USE_IMGUI
	ParamLoad::Get().UseImGui();
	UseImGui();
#endif // USE_IMGUI

	SpritesUpdate( elapsedTime );

	return ReturnResult();
}

void SceneLoad::Draw( float elapsedTime )
{
	ClearBackGround();

	sprIcon.Draw();
	sprNowLoading.Draw();
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
	}

	const float sin_01 = ( sinf( ToRadian( flushingTimer ) ) + 1.0f ) * 0.5f;
	const float shake  = sin_01 * data.sprLoadFlushingRange;

	sprNowLoading.alpha = std::max( data.sprLoadMinAlpha, std::min( 1.0f, shake ) );
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
		change.sceneType = Scene::Type::Title;
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
			sprIcon.ShowImGuiNode		( u8"画像調整・アイコン" );
			sprNowLoading.ShowImGuiNode	( u8"画像調整・ロード中" );

			ImGui::TreePop();
		}

		ImGui::End();
	}
}
#endif // USE_IMGUI
