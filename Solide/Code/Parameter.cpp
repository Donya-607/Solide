#include "Parameter.h"

#if USE_IMGUI
namespace ParameterHelper
{
	void ShowAABBNode( const std::string &caption, Donya::AABB *p )
	{
		if ( !ImGui::TreeNode( caption.c_str() ) ) { return; }
		// else

		ImGui::DragFloat3( u8"：中心のオフセット"	,		&p->pos.x,	0.01f	);
		ImGui::DragFloat3( u8"：サイズ（半分を指定）",	&p->size.x,	0.01f	);
		ImGui::Checkbox  ( u8"：判定を有効にする"	,		&p->exist			);

		ImGui::TreePop();
	}
	void ShowSphereNode( const std::string &caption, Donya::Sphere *p )
	{
		if ( !ImGui::TreeNode( caption.c_str() ) ) { return; }
		// else

		ImGui::DragFloat3( u8"：中心のオフセット"	,		&p->pos.x,	0.01f	);
		ImGui::DragFloat ( u8"：半径",					&p->radius,	0.01f	);
		ImGui::Checkbox  ( u8"：判定を有効にする"	,		&p->exist			);

		ImGui::TreePop();
	}
}
#endif // USE_IMGUI
