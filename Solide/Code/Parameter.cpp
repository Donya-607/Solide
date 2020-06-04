#include "Parameter.h"

#if USE_IMGUI
namespace ParameterHelper
{
	void ShowEaseParam( const std::string &nodeCaption, Donya::Easing::Kind *pKind, Donya::Easing::Type *pType )
	{
		if ( !ImGui::TreeNode( nodeCaption.c_str() ) ) { return; }
		// else

		using namespace Donya;

		int intKind = scast<int>( *pKind );
		int intType = scast<int>( *pType );

		std::string name{};
		name =  u8"���݁F";
		name += u8"[";
		name += Easing::KindName( intKind );
		name += u8":";
		name += Easing::TypeName( intType );
		name += u8"]";
		ImGui::Text( name.c_str() );

		ImGui::SliderInt( u8"���",		&intKind, 0, Easing::GetKindCount() - 1 );
		ImGui::SliderInt( u8"�^�C�v",	&intType, 0, Easing::GetTypeCount() - 1 );

		*pKind = scast<Easing::Kind>( intKind );
		*pType = scast<Easing::Type>( intType );

		ImGui::TreePop();
	}

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

	void ShowConstantNode( const std::string &nodeCaption, RenderingHelper::TransConstant *pConstant )
	{
		if ( !ImGui::TreeNode( nodeCaption.c_str() ) ) { return; }
		// else

		ImGui::DragFloat( u8"�͈́E��O��",	&pConstant->zNear,		0.01f, 0.0f );
		ImGui::DragFloat( u8"�͈́E����",	&pConstant->zFar,		0.01f, 0.0f );
		ImGui::SliderFloat( u8"�Œᓧ���x",	&pConstant->lowerAlpha,	0.0f,  1.0f );
		ImGui::DragFloat( u8"������K�p���鍂���i���@����̑��΁j",		&pConstant->heightThreshold, 0.01f );

		ImGui::TreePop();
	}
	void ShowConstantNode( const std::string &nodeCaption, RenderingHelper::AdjustColorConstant *pConstant )
	{
		if ( !ImGui::TreeNode( nodeCaption.c_str() ) ) { return; }
		// else

		ImGui::ColorEdit4( u8"���Z����X�y�L�����l", &pConstant->addSpecular.x );

		ImGui::TreePop();
	}
}
#endif // USE_IMGUI
