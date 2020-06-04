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
		name =  u8"現在：";
		name += u8"[";
		name += Easing::KindName( intKind );
		name += u8":";
		name += Easing::TypeName( intType );
		name += u8"]";
		ImGui::Text( name.c_str() );

		ImGui::SliderInt( u8"種類",		&intKind, 0, Easing::GetKindCount() - 1 );
		ImGui::SliderInt( u8"タイプ",	&intType, 0, Easing::GetTypeCount() - 1 );

		*pKind = scast<Easing::Kind>( intKind );
		*pType = scast<Easing::Type>( intType );

		ImGui::TreePop();
	}

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

	void ShowConstantNode( const std::string &nodeCaption, RenderingHelper::TransConstant *pConstant )
	{
		if ( !ImGui::TreeNode( nodeCaption.c_str() ) ) { return; }
		// else

		ImGui::DragFloat( u8"範囲・手前側",	&pConstant->zNear,		0.01f, 0.0f );
		ImGui::DragFloat( u8"範囲・奥側",	&pConstant->zFar,		0.01f, 0.0f );
		ImGui::SliderFloat( u8"最低透明度",	&pConstant->lowerAlpha,	0.0f,  1.0f );
		ImGui::DragFloat( u8"透明を適用する高さ（自機からの相対）",		&pConstant->heightThreshold, 0.01f );

		ImGui::TreePop();
	}
	void ShowConstantNode( const std::string &nodeCaption, RenderingHelper::AdjustColorConstant *pConstant )
	{
		if ( !ImGui::TreeNode( nodeCaption.c_str() ) ) { return; }
		// else

		ImGui::ColorEdit4( u8"加算するスペキュラ値", &pConstant->addSpecular.x );

		ImGui::TreePop();
	}
}
#endif // USE_IMGUI
