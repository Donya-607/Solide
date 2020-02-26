#include "Section.h"

#include "Parameter.h"

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
void Section::ShowImGuiNode( const std::string &caption, bool *pShouldErase )
{
	if ( !ImGui::TreeNode( caption.c_str() ) ) { return; }
	// else

	if ( pShouldErase && ImGui::Button( u8"çÌèú" ) )
	{
		*pShouldErase = true;
	}

	ImGui::DragFloat3( u8"ÉèÅ[ÉãÉhç¿ïW", &pos.x, 0.1f );
	ParameterHelper::ShowAABBNode( u8"ìñÇΩÇËîªíË", &hitBox );

	ImGui::TreePop();
}
#endif // USE_IMGUI
