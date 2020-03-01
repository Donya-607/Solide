#include "BG.h"

#include "Donya/Useful.h" // Use SignBit().

#include "Common.h"
#include "FilePath.h"

bool BG::LoadSprites( const std::wstring &BGName, const std::wstring &cloudName )
{
	constexpr size_t INSTANCE_COUNT = 4U;

	bool result{};
	bool succeeded = true;

	result = sprBG.LoadSprite( BGName, INSTANCE_COUNT );
	if ( !result ) { succeeded = false; }

	result = sprCloud.LoadSprite( cloudName, INSTANCE_COUNT );
	if ( !result ) { succeeded = false; }

	return succeeded;
}

void BG::Update( float elapsedTime )
{
	const Donya::Vector2 halfScreenSize
	{
		Common::HalfScreenWidthF(),
		Common::HalfScreenHeightF()
	};
	sprBG.pos = halfScreenSize;
	sprCloud.pos = halfScreenSize;

	horizonPos += scrollSpeed;

	sprCloud.pos.x = horizonPos;
}

void BG::Draw( float elapsedTime )
{
	constexpr float MOST_FAR = 1.0f;
	sprBG.Draw( MOST_FAR );

	sprCloud.Draw( MOST_FAR - 0.001f );

	const int scrollSign = Donya::SignBit( scrollSpeed );
	sprCloud.pos.x += cloudWidth * -scrollSign;
	sprCloud.Draw( MOST_FAR - 0.001f );
	sprCloud.pos.x -= cloudWidth * -scrollSign;
}

void BG::LoadBin()
{
	constexpr bool fromBinary = true;
	Donya::Serializer::Load( *this, GenerateSerializePath( ID, fromBinary ).c_str(), ID, fromBinary );
}
void BG::LoadJson()
{
	constexpr bool fromBinary = false;
	Donya::Serializer::Load( *this, GenerateSerializePath( ID, fromBinary ).c_str(), ID, fromBinary );
}
#if USE_IMGUI
void BG::SaveBin()
{
	constexpr bool fromBinary = true;
	Donya::Serializer::Save( *this, GenerateSerializePath( ID, fromBinary ).c_str(), ID, fromBinary );
}
void BG::SaveJson()
{
	constexpr bool fromBinary = false;
	Donya::Serializer::Save( *this, GenerateSerializePath( ID, fromBinary ).c_str(), ID, fromBinary );
}
void BG::ShowImGuiNode( const std::string &nodeCaption )
{
	if ( !ImGui::TreeNode( nodeCaption.c_str() ) ) { return; }
	// else

	ImGui::DragFloat( u8"���݂̂w���W", &horizonPos );
	ImGui::Text( "" );

	sprBG.ShowImGuiNode( u8"�w�i" );
	sprCloud.ShowImGuiNode( u8"�_" );

	ImGui::DragFloat( u8"�_�̃X�N���[�����x", &scrollSpeed, 0.1f );
	ImGui::DragFloat( u8"�_�̉���", &cloudWidth, 1.0f );
	
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
