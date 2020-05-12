#include "ScenePause.h"

#include <algorithm>
#include <string>
#include <vector>

#include <cereal/types/vector.hpp>

#include "Donya/Constant.h"
#include "Donya/Keyboard.h"
#include "Donya/Serializer.h"
#include "Donya/Sprite.h"
#include "Donya/Sound.h"
#include "Donya/Template.h"
#include "Donya/Vector.h"

#include "Common.h"
#include "Fader.h"
#include "FilePath.h"
#include "Music.h"
#include "Parameter.h"
#include "SaveData.h"

#undef max
#undef min

namespace
{
	static constexpr int choiceItemCount = scast<int>( ScenePause::Choice::ItemCount );

	std::string GetItemName( ScenePause::Choice kind )
	{
		switch ( kind )
		{
		case ScenePause::Resume:		return u8"再開";
		case ScenePause::Retry:			return u8"最初からやり直す";
		case ScenePause::ExitStage:		return u8"ステージから出る";
		case ScenePause::BackToTitle:	return u8"タイトルへ戻る";
		default: break;
		}

		_ASSERT_EXPR( 0, L"Error: Unexpected Kind!" );
		return u8"ERROR";
	}

	struct Member
	{
		struct Item
		{
			float			drawScale = 1.0f;
			Donya::Vector2	ssDrawPos{};	// Center
			Donya::Vector2	texPartPos{};	// Left-Top
			Donya::Vector2	texPartSize{};	// Whole size
		private:
			friend class cereal::access;
			template<class Archive>
			void serialize( Archive &archive, std::uint32_t version )
			{
				archive
				(
					CEREAL_NVP( drawScale	),
					CEREAL_NVP( ssDrawPos	),
					CEREAL_NVP( texPartPos	),
					CEREAL_NVP( texPartSize	)
				);

				if ( 1 <= version )
				{
					// archive( CEREAL_NVP( x ) );
				}
			}
		};
		float				darkenAlpha = 0.5f;
		float				choiceMagni = 1.2f;
		Item				pause{};
		std::vector<Item>	items; // size() == ChoiceItemCount
	private:
		friend class cereal::access;
		template<class Archive>
		void serialize( Archive &archive, std::uint32_t version )
		{
			archive
			(
				CEREAL_NVP( darkenAlpha	),
				CEREAL_NVP( choiceMagni	),
				CEREAL_NVP( pause		),
				CEREAL_NVP( items		)
			);

			if ( 1 <= version )
			{
				// archive( CEREAL_NVP( x ) );
			}
		}
	};
}
CEREAL_CLASS_VERSION( Member,		0 )
CEREAL_CLASS_VERSION( Member::Item,	0 )

class ParamPause : public ParameterBase<ParamPause>
{
public:
	static constexpr const char *ID = "Pause";
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
		if ( m.items.size() != choiceItemCount )
		{
			m.items.resize( choiceItemCount );
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

		if ( ImGui::TreeNode( u8"ポーズ画面のパラメータ調整" ) )
		{
			ImGui::DragFloat( u8"バックを暗くする強さ",	&m.darkenAlpha, 0.01f );
			ImGui::DragFloat( u8"選択項目の拡大率",		&m.choiceMagni, 0.01f );
			m.darkenAlpha = std::max( 0.0f, m.darkenAlpha );
			m.choiceMagni = std::max( 0.0f, m.choiceMagni );

			if ( ImGui::TreeNode( u8"項目の位置" ) )
			{
				ResizeVectorIfNeeded();

				auto ShowItem = []( const std::string &nodeCaption, Member::Item *p )
				{
					if ( !ImGui::TreeNode( nodeCaption.c_str() ) ) { return; }
					// else

					ImGui::DragFloat ( u8"描画スケール",				&p->drawScale		);
					ImGui::DragFloat2( u8"描画位置（中心点）",		&p->ssDrawPos.x		);
					ImGui::DragFloat2( u8"テクスチャ原点（左上）",	&p->texPartPos.x	);
					ImGui::DragFloat2( u8"テクスチャサイズ（全体）",	&p->texPartSize.x	);

					ImGui::TreePop();
				};

				ImGui::Text( u8"セレクト画面では，「再開」が「タイトルへ戻る」になります" );

				ShowItem( u8"[ポーズ]", &m.pause );

				std::string caption{};
				for ( size_t i = 0; i < choiceItemCount; ++i )
				{
					caption = u8"[" + GetItemName( scast<ScenePause::Choice>( i ) ) + u8"]";
					ShowItem( caption, &m.items[i] );
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
		return ParamPause::Get().Data();
	}
}

void ScenePause::Init()
{
	constexpr size_t maxInstanceCount = 8U;
	sprite.LoadSprite( GetSpritePath( SpriteAttribute::Pause ), maxInstanceCount );
	sprite.alpha	= 1.0f;
	sprite.degree	= 0.0f;

	currentStageNo	= SaveDataAdmin::Get().GetNowData().currentStageNumber;

	controller.Update();
}

void ScenePause::Uninit() {}

Scene::Result ScenePause::Update( float elapsedTime )
{
	controller.Update();

	UpdateChooseItem();

	return ReturnResult();
}

void ScenePause::Draw( float elapsedTime )
{
	DrawBackGround();

	auto DrawItem = [&]( const Member::Item &source, float scaleMagni, float drawDepth )
	{
		sprite.drawScale	= source.drawScale * scaleMagni;
		sprite.pos			= source.ssDrawPos;
		sprite.texPos		= source.texPartPos;
		sprite.texSize		= source.texPartSize;
		
		sprite.DrawPart( drawDepth );
	};

	// These number is anything okay that satisfies a very small value and near than a darken rectangle.
	constexpr float defaultDepth = 0.08f;
	constexpr float chosenDepth  = defaultDepth * 0.5f;

	const auto data = FetchMember();

	DrawItem( data.pause, 1.0f, defaultDepth );

	const size_t chosenIndex = scast<size_t>( choice );
	for ( size_t i = 0; i < choiceItemCount; ++i )
	{
		const float magni = ( i == chosenIndex ) ? data.choiceMagni	: 1.0f;
		const float depth = ( i == chosenIndex ) ? chosenDepth		: defaultDepth;
		DrawItem( data.items[i], magni, depth );
	}
}

void ScenePause::UpdateChooseItem()
{
	bool up{}, down{};
	if ( controller.IsConnected() )
	{
		up		= controller.Trigger( Donya::Gamepad::Button::UP	) || controller.TriggerStick( Donya::Gamepad::StickDirection::UP	);
		down	= controller.Trigger( Donya::Gamepad::Button::DOWN	) || controller.TriggerStick( Donya::Gamepad::StickDirection::DOWN	);
	}
	else
	{
		up		= Donya::Keyboard::Trigger( VK_UP   );
		down	= Donya::Keyboard::Trigger( VK_DOWN );
	}

	if ( up == down ) { return; } // Update is unnecessary when (!U && !D) or (U && D).
	// else

	int index = scast<int>( choice );
	const int oldIndex = index;

	if ( up		) { index--; }
	if ( down	) { index++; }

	index = std::max( 0, std::min( scast<int>( Choice::ItemCount ) - 1, index ) );

	if ( index != oldIndex )
	{
		Donya::Sound::Play( Music::ItemChoose );
	}

	choice = scast<Choice>( index );
}

void ScenePause::DrawBackGround() const
{
	// Darken.

	Donya::Sprite::SetDrawDepth( 0.1f );

	Donya::Sprite::DrawRect
	(
		Common::HalfScreenWidthF(),	Common::HalfScreenHeightF(),
		Common::ScreenWidthF(),		Common::ScreenHeightF(),
		Donya::Color::Code::BLACK,
		FetchMember().darkenAlpha, 0.0f
	);
}

Scene::Result ScenePause::ReturnResult()
{
	const bool requestPause	= Donya::Keyboard::Trigger( 'P' ) || controller.Trigger( Donya::Gamepad::Button::START ) || controller.Trigger( Donya::Gamepad::Button::SELECT );
	const bool allowPause	= !Fader::Get().IsExist();
	if ( requestPause && allowPause )
	{
		Donya::Sound::Play( Music::ItemDecision );

		Scene::Result change{};
		change.AddRequest( Scene::Request::REMOVE_ME );
		return change;
	}

	Scene::Result noop{ Scene::Request::NONE, Scene::Type::Null };
	return noop;
}
