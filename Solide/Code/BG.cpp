#include "BG.h"

#include "Donya/Useful.h" // Use SignBit().

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

#if USE_IMGUI
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

	ImGui::TreePop();
}
#endif // USE_IMGUI
