#include "Sentence.h"

#include <algorithm>	// For std::max(), std::min()

#undef max
#undef min

void TitleSentence::FlusherBase::Init( TitleSentence &target )
{
	target.flushTimer = 0.0f;
	target.drawAlpha  = 1.0f;
}
void TitleSentence::FlusherBase::UpdateImpl( TitleSentence &target, float elapsedTime, float flushInterval )
{
	const float increaseAmount = 360.0f / ( 60.0f * flushInterval );
	target.flushTimer += increaseAmount;

	const float sin  = sinf( scast<float>( target.flushTimer ) );
	target.drawAlpha = target.lowestAlpha + sin;
	target.drawAlpha = std::max( 0.0f, std::min( 1.0f, target.drawAlpha ) );
}
void TitleSentence::LateFlusher::Update( TitleSentence &target, float elapsedTime )
{
	FlusherBase::UpdateImpl( target, elapsedTime, target.flushIntervalLate );
}
void TitleSentence::FastFlusher::Update( TitleSentence &target, float elapsedTime )
{
	FlusherBase::UpdateImpl( target, elapsedTime, target.flushIntervalFast );
}

void TitleSentence::Init()
{
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

#if USE_IMGUI
void TitleSentence::ShowImGuiNode( const std::string &nodeCaption )
{
	if ( !ImGui::TreeNode( ( nodeCaption.c_str() ) ) ) { return; }
	// else

	uiLogo.ShowImGuiNode( u8"ロゴ" );
	uiPrompt.ShowImGuiNode( u8"スタート文字" );

	ImGui::DragFloat( u8"点滅間隔（秒）・ゆっくり",	&flushIntervalLate, 0.1f, 0.0f );
	ImGui::DragFloat( u8"点滅間隔（秒）・はやい",		&flushIntervalFast, 0.1f, 0.0f );
	ImGui::SliderFloat( u8"アルファの最低値", &lowestAlpha, 0.0f, 1.0f );

	ImGui::TreePop();
}
#endif // USE_IMGUI
