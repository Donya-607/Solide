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

	uiLogo.ShowImGuiNode( u8"���S" );
	uiPrompt.ShowImGuiNode( u8"�X�^�[�g����" );

	ImGui::DragFloat( u8"�_�ŊԊu�i�b�j�E�������",	&flushIntervalLate, 0.1f, 0.0f );
	ImGui::DragFloat( u8"�_�ŊԊu�i�b�j�E�͂₢",		&flushIntervalFast, 0.1f, 0.0f );
	ImGui::DragFloat( u8"�_�ŕ��i�b�j�E�������",		&flushRangeLate, 0.1f, 0.0f );
	ImGui::DragFloat( u8"�_�ŕ��i�b�j�E�͂₢",		&flushRangeFast, 0.1f, 0.0f );
	ImGui::SliderFloat( u8"�A���t�@�̍Œ�l", &lowestAlpha, 0.0f, 1.0f );

	auto ShowIONode = [&]()
	{
		if ( !ImGui::TreeNode( u8"�t�@�C�� I/O" ) ) { return; }
		// else

		static bool isBinary = true;
		if ( ImGui::RadioButton( "Binary", isBinary ) ) { isBinary = true;  }
		if ( ImGui::RadioButton( "Json",  !isBinary ) ) { isBinary = false; }

		std::string loadStr = u8"���[�h";
		loadStr += u8"�iby:";
		loadStr += ( isBinary ) ? u8"Binary" : u8"Json";
		loadStr += u8"�j";

		if ( ImGui::Button( u8"�Z�[�u" ) )
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
	if ( ImGui::Button( u8"���o�^�C�}�[�����Z�b�g" ) )
	{
		easingTimer = 0.0f;
	}

	uiClear.ShowImGuiNode( u8"�摜����" );

	namespace E = Donya::Easing;
	constexpr int kindCount = E::GetKindCount();
	constexpr int typeCount = E::GetTypeCount();
	ImGui::SliderInt( u8"�C�[�W���O�̎��",	&easeKind, 0, kindCount - 1 );
	ImGui::SliderInt( u8"�C�[�W���O�̂�����",	&easeType, 0, typeCount - 1 );
	ImGui::Text( u8"���e�F%s�E%s", E::KindName( easeKind ), E::TypeName( easeType ) );
	ImGui::Text( "" );

	ImGui::DragFloat( u8"�C�[�W���O�ɂ�����b��", &easeSeconds, 0.1f, 0.001f );
	ImGui::DragFloat( u8"�X�P�[�����O���镝", &scalingSize, 0.1f, 0.0f );

	auto ShowIONode = [&]()
	{
		if ( !ImGui::TreeNode( u8"�t�@�C�� I/O" ) ) { return; }
		// else

		static bool isBinary = true;
		if ( ImGui::RadioButton( "Binary", isBinary ) ) { isBinary = true;  }
		if ( ImGui::RadioButton( "Json",  !isBinary ) ) { isBinary = false; }

		std::string loadStr = u8"���[�h";
		loadStr += u8"�iby:";
		loadStr += ( isBinary ) ? u8"Binary" : u8"Json";
		loadStr += u8"�j";

		if ( ImGui::Button( u8"�Z�[�u" ) )
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
