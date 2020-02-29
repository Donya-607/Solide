#include "Sentence.h"

#include <algorithm>	// For std::max(), std::min()

#include "Donya/Useful.h"

#include "FilePath.h"

#undef max
#undef min

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
