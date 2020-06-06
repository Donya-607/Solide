#include "Tutorial.h"

#include <algorithm>

#include "Donya/Color.h"
#include "Donya/Keyboard.h"
#include "Donya/Sprite.h"
#include "Donya/Useful.h"

#include "Common.h"
#include "FilePath.h"
#include "Parameter.h"
#include "SaveData.h"


namespace
{
	constexpr float depthDarken		= 0.12f;
	constexpr float depthFrame		= 0.1f;
	constexpr float depthSentence	= 0.08f;

#if USE_IMGUI
	static bool dontRemoveActivatedInstance = false;
#endif // USE_IMGUI
}


void Tutorial::Init() {}
void Tutorial::Uninit() {}
void Tutorial::Update( float elapsedTime, const Donya::XInput &controller )
{
	if ( !IsActive() ) { return; }
	// else

	timer++;

	if ( RequiredAdvance( controller ) )
	{
		shouldRemove = true;
	}
}
void Tutorial::Draw( const UIObject &sprite, const Donya::Vector2 &ssBasePos )
{
	UIObject drawer		= sprite;
	drawer.pos			= ssBasePos + ssRelatedPos;
	drawer.texPos		= texPartPos;
	drawer.texSize		= texPartSize;
	drawer.drawScale	= drawScale;
	
	drawer.DrawPart( depthSentence );
}
void Tutorial::DrawHitBox( RenderingHelper *pRenderer, const Donya::Vector4x4 &matVP, float alpha )
{
	if ( !Common::IsShowCollision() ) { return; }
	// else

	Donya::Color::Code color = Donya::Color::Code::FUCHSIA;
#if USE_IMGUI
	if ( dontRemoveActivatedInstance && ShouldRemove() )
	{
		// Make invalid color
		color = Donya::Color::Code::DARK_GRAY;
	}
#endif // USE_IMGUI

	Donya::Model::Cube::Constant constant;
	constant.matWorld		= CalcWorldMatrix();
	constant.matViewProj	= matVP;
	constant.drawColor		= Donya::Vector4{ Donya::Color::MakeColor( color ), alpha };
	constant.lightDirection	= -Donya::Vector3::Up();
	pRenderer->ProcessDrawingCube( constant );
}
void Tutorial::Start() { nowActive = true; }
bool				Tutorial::ShouldRemove()	const { return shouldRemove;	}
bool				Tutorial::IsActive()		const { return nowActive;		}
Donya::Vector3		Tutorial::GetPosition()		const { return wsHitBox.pos;	}
Donya::AABB			Tutorial::GetHitBox()		const { return wsHitBox;		}
Donya::Vector4x4	Tutorial::CalcWorldMatrix()	const
{
	const auto body = GetHitBox();

	Donya::Vector4x4 W{};
	W._11 = body.size.x * 2.0f;
	W._22 = body.size.y * 2.0f;
	W._33 = body.size.z * 2.0f;
	W._41 = body.pos.x;
	W._42 = body.pos.y;
	W._43 = body.pos.z;
	return W;
}
bool Tutorial::RequiredAdvance( const Donya::XInput &controller ) const
{
	if ( timer < ignoreInputFrame ) { return false; }
	// else
	return	( controller.IsConnected() )
			? controller.Trigger( Donya::Gamepad::A )
			: Donya::Keyboard::Trigger( 'Z' );
}
#if USE_IMGUI
void Tutorial::ShowImGuiNode( const std::string &nodeCaption )
{
	if ( !ImGui::TreeNode( nodeCaption.c_str() ) ) { return; }
	// else

	const std::string caption = nodeCaption + u8"を削除";
	if ( ImGui::Button( caption.c_str() ) )
	{
		shouldRemove = true;
	}

	if ( ImGui::Button( u8"状態をリセット" ) )
	{
		nowActive		= false;
		shouldRemove	= false;
	}

	ImGui::DragInt( u8"入力を無視する時間（フレーム）", &ignoreInputFrame );
	ignoreInputFrame = std::max( 0, ignoreInputFrame );

	ImGui::DragFloat ( u8"描画スケール",				&drawScale, 0.01f );
	drawScale = std::max( 0.0f, drawScale );

	ImGui::DragFloat2( u8"描画位置（相対）",			&ssRelatedPos.x );
	ImGui::DragFloat2( u8"テクスチャ切り取り位置",	&texPartPos.x   );
	ImGui::DragFloat2( u8"テクスチャ切り取りサイズ",	&texPartSize.x  );

	ParameterHelper::ShowAABBNode( u8"ワールド位置と当たり判定", &wsHitBox );

	ImGui::TreePop();
}
#endif // USE_IMGUI


bool TutorialContainer::Init( int stageNumber )
{
	stageNo = stageNumber;
#if DEBUG_MODE
	LoadJson( stageNumber );
#else
	LoadBin( stageNumber );
#endif // DEBUG_MODE

	for ( auto &it : instances )
	{
		it.Init();
	}

	using Spr = SpriteAttribute;
	bool succeeded = true;
	if ( !sprFrame.LoadSprite	( GetSpritePath( Spr::TutorialFrame		), 2U ) ) { succeeded = false; }
	if ( !sprSentence.LoadSprite( GetSpritePath( Spr::TutorialSentence	), 4U ) ) { succeeded = false; }

	return succeeded;
}
void TutorialContainer::Uninit()
{
	for ( auto &it : instances )
	{
		it.Uninit();
	}
}
void TutorialContainer::Update( float elapsedTime, const Donya::XInput &controller )
{
	auto ShouldRemove = []( Tutorial &element )
	{
		return element.ShouldRemove();
	};

	for ( auto &it : instances )
	{
		it.Update( elapsedTime, controller );

		if ( ShouldRemove( it ) )
		{
			it.Uninit();
		}
	}

#if USE_IMGUI
	if ( dontRemoveActivatedInstance ) { return; }
#endif // USE_IMGUI

	auto result = std::remove_if
	(
		instances.begin(), instances.end(),
		ShouldRemove
	);
	instances.erase( result, instances.end() );
}
void TutorialContainer::Draw()
{
	bool isThereActiveInstance = false;
	for ( const auto &it : instances )
	{
		if ( it.IsActive() )
		{
		#if USE_IMGUI
			if ( dontRemoveActivatedInstance && it.ShouldRemove() ) { continue; }
		#endif // USE_IMGUI
			isThereActiveInstance = true;
		}
	}

	if ( !isThereActiveInstance ) { return; }
	// else

	const float oldDepth = Donya::Sprite::GetDrawDepth();

	Donya::Sprite::SetDrawDepth( depthDarken );
	Donya::Sprite::DrawRect
	(
		Common::HalfScreenWidthF(),	Common::HalfScreenHeightF(),
		Common::ScreenWidthF(),		Common::ScreenHeightF(),
		Donya::Color::Code::BLACK,	darkenAlpha
	);

	sprFrame.Draw( depthFrame );

	for ( auto &it : instances )
	{
		if ( !it.IsActive() ) { continue; }
		// else
		it.Draw( sprSentence, sprFrame.pos );
	}

	Donya::Sprite::SetDrawDepth( oldDepth );
}
void TutorialContainer::DrawHitBoxes( RenderingHelper *pRenderer, const Donya::Vector4x4 &matVP, float alpha )
{
	if ( !Common::IsShowCollision() || !pRenderer ) { return; }
	// else

	for ( auto &it : instances )
	{
		it.DrawHitBox( pRenderer, matVP, alpha );
	}
}
size_t		TutorialContainer::GetTutorialCount() const { return instances.size(); }
bool		TutorialContainer::IsOutOfRange( size_t index ) const
{
	return ( GetTutorialCount() <= index ) ? true : false;
}
Tutorial	*TutorialContainer::GetTutorialPtrOrNullptr( size_t index )
{
	if ( IsOutOfRange( index ) ) { return nullptr; }
	// else
	return &instances[index];
}
bool		TutorialContainer::ShouldPauseGame() const
{
	for ( const auto &it : instances )
	{
		if ( it.IsActive() )
		{
		#if USE_IMGUI
			if ( dontRemoveActivatedInstance && it.ShouldRemove() )
			{
				// When debug dont-remove mode, I wanna ignore the instance.
				continue;
			}
		#endif // USE_IMGUI
			return true;
		}
	}
	// else
	return false;
}
void TutorialContainer::LoadBin ( int stageNo )
{
	constexpr bool fromBinary = true;
	Donya::Serializer::Load( *this, MakeStageParamPath( ID, stageNo, fromBinary ).c_str(), ID, fromBinary );
}
void TutorialContainer::LoadJson( int stageNo )
{
	constexpr bool fromBinary = false;
	Donya::Serializer::Load( *this, MakeStageParamPath( ID, stageNo, fromBinary ).c_str(), ID, fromBinary );
}
#if USE_IMGUI
void TutorialContainer::SaveBin ( int stageNo )
{
	constexpr bool fromBinary = true;

	const std::string filePath = MakeStageParamPath( ID, stageNo, fromBinary );
	MakeFileIfNotExists( filePath, fromBinary );

	Donya::Serializer::Save( *this, filePath.c_str(), ID, fromBinary );
}
void TutorialContainer::SaveJson( int stageNo )
{
	constexpr bool fromBinary = false;

	const std::string filePath = MakeStageParamPath( ID, stageNo, fromBinary );
	MakeFileIfNotExists( filePath, fromBinary );

	Donya::Serializer::Save( *this, filePath.c_str(), ID, fromBinary );
}
void TutorialContainer::ShowImGuiNode( const std::string &nodeCaption, int stageNo )
{
	if ( !ImGui::TreeNode( nodeCaption.c_str() ) ) { return; }
	// else

	ImGui::Checkbox( u8"触れた実体を取り除かない（デバッグ用）", &dontRemoveActivatedInstance );

	ImGui::DragFloat( u8"背景の黒アルファ", &darkenAlpha, 0.01f );

	sprFrame.ShowImGuiNode( u8"枠画像の設定" );
	ImGui::Text( u8"枠画像の描画位置は，文章画像の描画基本位置にもなります" );

	ImGui::Text( "" );

	ParameterHelper::ResizeByButton( &instances );
	if ( ImGui::TreeNode( u8"実体の設定" ) )
	{
		std::string caption{};
		const size_t tutorialCount = GetTutorialCount();
		for ( size_t i = 0; i < tutorialCount; ++i )
		{
			caption = u8"[" + std::to_string( i ) + u8"]番";

			instances[i].ShowImGuiNode( caption );
		}

		ImGui::TreePop();
	}
	
	auto ShowIONode = [&]()
	{
		if ( !ImGui::TreeNode( u8"ファイル I/O" ) ) { return; }
		// else

		const std::string strIndex = u8"[" + std::to_string( stageNo ) + u8"]";

		static bool isBinary = true;
		if ( ImGui::RadioButton( "Binary", isBinary ) ) { isBinary = true;  }
		if ( ImGui::RadioButton( "Json",  !isBinary ) ) { isBinary = false; }

		std::string loadStr = u8"ロード" + strIndex;
		loadStr += u8"(by:";
		loadStr += ( isBinary ) ? u8"Binary" : u8"Json";
		loadStr += u8")";

		if ( ImGui::Button( ( u8"セーブ" + strIndex ).c_str() ) )
		{
			SaveBin ( stageNo );
			SaveJson( stageNo );
		}
		if ( ImGui::Button( loadStr.c_str() ) )
		{
			( isBinary ) ? LoadBin( stageNo ) : LoadJson( stageNo );
		}

		ImGui::TreePop();
	};
	ShowIONode();

	ImGui::TreePop();
}
#endif // USE_IMGUI
