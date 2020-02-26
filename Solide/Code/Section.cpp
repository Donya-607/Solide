#include "Section.h"

namespace
{
	static const Donya::AABB DEFAULT_HIT_BOX{ {}, { 0.5f, 0.5f, 0.5f }, true };
}

Section::Section() : Solid()
{
	hitBox	= DEFAULT_HIT_BOX;
}
Section::Section( const Donya::Vector3 &wsPos ) : Solid()
{
	pos		= wsPos;
	hitBox	= DEFAULT_HIT_BOX;
}
Section::Section( const Donya::Vector3 &wsPos, const Donya::AABB &wsHitBox ) : Solid()
{
	pos		= wsPos;
	hitBox	= wsHitBox;
}

#if USE_IMGUI
void Section::ShowImGuiNode( const std::string &caption, bool &shouldErase )
{
	if ( !ImGui::TreeNode( caption.c_str() ) ) { return; }
	// else

	if ( ImGui::Button( u8"�폜" ) )
	{
		shouldErase = true;
	}

	ImGui::DragFloat3( u8"���[���h���W", &pos.x, 0.1f );

	ImGui::DragFloat3( u8"�����蔻��F�I�t�Z�b�g",			&hitBox.pos.x,	0.1f );
	ImGui::DragFloat3( u8"�����蔻��F�T�C�Y�i�������w��j",	&hitBox.size.x,	0.1f );
	ImGui::Checkbox( u8"�����蔻��F�L���ɂ���",				&hitBox.exist );

	ImGui::TreePop();
}
#endif // USE_IMGUI
