#include "Sentence.h"

#include <algorithm>	// For std::max(), std::min()

#include "Donya/Easing.h"
#include "Donya/Useful.h"

#include "FilePath.h"

#undef max
#undef min

#pragma region TitleSentence

void TitleSentence::FlusherBase::Init( TitleSentence &target )
{
	target.flushTimer = 0.0f;
	target.drawAlpha  = 1.0f;
}
void TitleSentence::FlusherBase::UpdateImpl( TitleSentence &target, float elapsedTime, float flushInterval, float flushRange )
{
	const float increaseAmount = 360.0f / ( 60.0f * flushInterval );
	target.flushTimer += increaseAmount;

	const float sin_01 = ( sinf( ToRadian( target.flushTimer ) ) + 1.0f ) * 0.5f;
	const float shake  = sin_01 * flushRange;

	target.drawAlpha = std::max( target.lowestAlpha, std::min( 1.0f, shake ) );
}
void TitleSentence::LateFlusher::Update( TitleSentence &target, float elapsedTime )
{
	FlusherBase::UpdateImpl( target, elapsedTime, target.flushIntervalLate, target.flushRangeLate );
}
void TitleSentence::FastFlusher::Update( TitleSentence &target, float elapsedTime )
{
	FlusherBase::UpdateImpl( target, elapsedTime, target.flushIntervalFast, target.flushRangeFast );
}

void TitleSentence::Init()
{
#if DEBUG_MODE
	LoadBin();
#else
	LoadJson();
#endif // DEBUG_MODE

	flushTimer	= 0.0f;
	drawAlpha	= 1.0f;
	ResetFlusher<LateFlusher>();
}
bool TitleSentence::LoadSprites( const std::wstring &logoName, const std::wstring &promptName )
{
	constexpr size_t INSTANCE_COUNT = 4U;

	bool result{};
	bool succeeded = true;

	result = uiLogo.LoadSprite( logoName, INSTANCE_COUNT );
	if ( !result ) { succeeded = false; }

	result = uiPrompt.LoadSprite( promptName, INSTANCE_COUNT );
	if ( !result ) { succeeded = false; }

	return succeeded;
}

void TitleSentence::Update( float elapsedTime )
{
	pFlusher->Update( *this, elapsedTime );

	uiPrompt.alpha = drawAlpha;
}

void TitleSentence::Draw( float elapsedTime ) const
{
	constexpr float DRAW_DEPTH = 0.0f;
	uiLogo.Draw( DRAW_DEPTH );
	uiPrompt.Draw( DRAW_DEPTH );
}

void TitleSentence::AdvanceState()
{
	ResetFlusher<FastFlusher>();
}

void TitleSentence::LoadBin()
{
	constexpr bool fromBinary = true;
	Donya::Serializer::Load( *this, GenerateSerializePath( ID, fromBinary ).c_str(), ID, fromBinary );
}
void TitleSentence::LoadJson()
{
	constexpr bool fromBinary = false;
	Donya::Serializer::Load( *this, GenerateSerializePath( ID, fromBinary ).c_str(), ID, fromBinary );
}
#if USE_IMGUI
void TitleSentence::SaveBin()
{
	constexpr bool fromBinary = true;
	Donya::Serializer::Save( *this, GenerateSerializePath( ID, fromBinary ).c_str(), ID, fromBinary );
}
void TitleSentence::SaveJson()
{
	constexpr bool fromBinary = false;
	Donya::Serializer::Save( *this, GenerateSerializePath( ID, fromBinary ).c_str(), ID, fromBinary );
}
void TitleSentence::ShowImGuiNode( const std::string &nodeCaption )
{
	if ( !ImGui::TreeNode( ( nodeCaption.c_str() ) ) ) { return; }
	// else

	uiLogo.ShowImGuiNode( u8"ロゴ" );
	uiPrompt.ShowImGuiNode( u8"スタート文字" );

	ImGui::DragFloat( u8"点滅間隔（秒）・ゆっくり",	&flushIntervalLate, 0.1f, 0.0f );
	ImGui::DragFloat( u8"点滅間隔（秒）・はやい",		&flushIntervalFast, 0.1f, 0.0f );
	ImGui::DragFloat( u8"点滅幅（秒）・ゆっくり",		&flushRangeLate, 0.1f, 0.0f );
	ImGui::DragFloat( u8"点滅幅（秒）・はやい",		&flushRangeFast, 0.1f, 0.0f );
	ImGui::SliderFloat( u8"アルファの最低値", &lowestAlpha, 0.0f, 1.0f );

	auto ShowIONode = [&]()
	{
		if ( !ImGui::TreeNode( u8"ファイル I/O" ) ) { return; }
		// else

		static bool isBinary = true;
		if ( ImGui::RadioButton( "Binary", isBinary ) ) { isBinary = true;  }
		if ( ImGui::RadioButton( "Json",  !isBinary ) ) { isBinary = false; }

		std::string loadStr = u8"ロード";
		loadStr += u8"（by:";
		loadStr += ( isBinary ) ? u8"Binary" : u8"Json";
		loadStr += u8"）";

		if ( ImGui::Button( u8"セーブ" ) )
		{
			SaveBin ();
			SaveJson();
		}
		if ( ImGui::Button( loadStr.c_str() ) )
		{
			( isBinary ) ? LoadBin() : LoadJson();
		}

		ImGui::TreePop();
	};
	ShowIONode();

	ImGui::TreePop();
}
#endif // USE_IMGUI

// region TitleSentence
#pragma endregion


#pragma region ClearSentence


void ClearSentence::Init()
{
#if DEBUG_MODE
	LoadBin();
#else
	LoadJson();
#endif // DEBUG_MODE

	Hide();
}
bool ClearSentence::LoadSprite( const std::wstring &clearName )
{
	constexpr size_t INSTANCE_COUNT = 4U;
	return uiClear.LoadSprite( clearName, INSTANCE_COUNT );
}

void ClearSentence::Update( float elapsedTime )
{
	if ( nowHidden ) { return; }
	// else

	{
		const float increment =  1.0f / ( 60.0f * easeSeconds );
		easingTimer += increment;
		easingTimer =  std::min( 1.0f, easingTimer );

		namespace E = Donya::Easing;
		const float easing = E::Ease( scast<E::Kind>( easeKind ), scast<E::Type>( easeType ), easingTimer );

		drawScale = easing * scalingSize;
	}

	uiClear.drawScale = drawScale;
}

void ClearSentence::Draw( float elapsedTime ) const
{
	if ( nowHidden ) { return; }
	// else

	constexpr float DRAW_DEPTH = 0.0f;
	uiClear.Draw( DRAW_DEPTH );;
}

void ClearSentence::Hide()
{
	easingTimer	= 0.0f;
	drawScale	= 0.0f;
	nowHidden	= true;
}
void ClearSentence::Appear()
{
	easingTimer	= 0.0f;
	drawScale	= 0.0f;
	nowHidden	= false;
}

void ClearSentence::LoadBin()
{
	constexpr bool fromBinary = true;
	Donya::Serializer::Load( *this, GenerateSerializePath( ID, fromBinary ).c_str(), ID, fromBinary );
}
void ClearSentence::LoadJson()
{
	constexpr bool fromBinary = false;
	Donya::Serializer::Load( *this, GenerateSerializePath( ID, fromBinary ).c_str(), ID, fromBinary );
}
#if USE_IMGUI
void ClearSentence::SaveBin()
{
	constexpr bool fromBinary = true;
	Donya::Serializer::Save( *this, GenerateSerializePath( ID, fromBinary ).c_str(), ID, fromBinary );
}
void ClearSentence::SaveJson()
{
	constexpr bool fromBinary = false;
	Donya::Serializer::Save( *this, GenerateSerializePath( ID, fromBinary ).c_str(), ID, fromBinary );
}
void ClearSentence::ShowImGuiNode( const std::string &nodeCaption )
{
	if ( !ImGui::TreeNode( ( nodeCaption.c_str() ) ) ) { return; }
	// else

	ImGui::Text( "Timer  : %5.3f", easingTimer );
	ImGui::Text( "Scale  : %5.3f", drawScale   );
	ImGui::Text( "Hidden : %s", ( nowHidden ) ? "True" : "False" );
	if ( ImGui::Button( u8"演出タイマーをリセット" ) )
	{
		easingTimer = 0.0f;
	}

	uiClear.ShowImGuiNode( u8"画像文字" );

	namespace E = Donya::Easing;
	constexpr int kindCount = E::GetKindCount();
	constexpr int typeCount = E::GetTypeCount();
	ImGui::SliderInt( u8"イージングの種類",	&easeKind, 0, kindCount - 1 );
	ImGui::SliderInt( u8"イージングのかけ方",	&easeType, 0, typeCount - 1 );
	ImGui::Text( u8"内容：%s・%s", E::KindName( easeKind ), E::TypeName( easeType ) );
	ImGui::Text( "" );

	ImGui::DragFloat( u8"イージングにかける秒数", &easeSeconds, 0.1f, 0.001f );
	ImGui::DragFloat( u8"スケーリングする幅", &scalingSize, 0.1f, 0.0f );

	auto ShowIONode = [&]()
	{
		if ( !ImGui::TreeNode( u8"ファイル I/O" ) ) { return; }
		// else

		static bool isBinary = true;
		if ( ImGui::RadioButton( "Binary", isBinary ) ) { isBinary = true;  }
		if ( ImGui::RadioButton( "Json",  !isBinary ) ) { isBinary = false; }

		std::string loadStr = u8"ロード";
		loadStr += u8"（by:";
		loadStr += ( isBinary ) ? u8"Binary" : u8"Json";
		loadStr += u8"）";

		if ( ImGui::Button( u8"セーブ" ) )
		{
			SaveBin ();
			SaveJson();
		}
		if ( ImGui::Button( loadStr.c_str() ) )
		{
			( isBinary ) ? LoadBin() : LoadJson();
		}

		ImGui::TreePop();
	};
	ShowIONode();

	ImGui::TreePop();
}
#endif // USE_IMGUI

// region ClearSentence
#pragma endregion
