#include "Parameter.h"

#if USE_IMGUI
namespace ParameterHelper
{
	void ShowAABBNode( const std::string &caption, Donya::AABB *p )
	{
		if ( !ImGui::TreeNode( caption.c_str() ) ) { return; }
		// else

		ImGui::DragFloat3( u8"�F���S�̃I�t�Z�b�g"	,		&p->pos.x,	0.01f	);
		ImGui::DragFloat3( u8"�F�T�C�Y�i�������w��j",	&p->size.x,	0.01f	);
		ImGui::Checkbox  ( u8"�F�����L���ɂ���"	,		&p->exist			);

		ImGui::TreePop();
	}
	void ShowSphereNode( const std::string &caption, Donya::Sphere *p )
	{
		if ( !ImGui::TreeNode( caption.c_str() ) ) { return; }
		// else

		ImGui::DragFloat3( u8"�F���S�̃I�t�Z�b�g"	,		&p->pos.x,	0.01f	);
		ImGui::DragFloat ( u8"�F���a",					&p->radius,	0.01f	);
		ImGui::Checkbox  ( u8"�F�����L���ɂ���"	,		&p->exist			);

		ImGui::TreePop();
	}
}
#endif // USE_IMGUI
