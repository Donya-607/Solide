#include "UI.h"

#include "Donya/Sprite.h"

bool UIObject::LoadSprite( const std::wstring &filePath, size_t maxCount )
{
	sprite = Donya::Sprite::Load( filePath, maxCount );
	return ( sprite == NULL ) ? false : true;
}

bool UIObject::Draw() const
{
	return Donya::Sprite::DrawExt
	(
		sprite,
		pos.x, pos.y,
		drawScale.x, drawScale.y,
		degree, alpha
	);
}
bool UIObject::DrawPart() const
{
	return Donya::Sprite::DrawPartExt
	(
		sprite,
		pos.x, pos.y,
		texPos.x, texPos.y,
		texSize.x, texSize.y,
		drawScale.x, drawScale.y,
		degree, alpha
	);
}

#if USE_IMGUI
void UIObject::ShowImGuiNode( const std::string &nodeCaption )
{
	if ( !ImGui::TreeNode( nodeCaption.c_str() ) ) { return; }
	// else

	ImGui::DragFloat2 ( u8"スクリーン座標",	&pos.x );

	ImGui::SliderFloat( u8"描画角度",		&degree,  -360.0f, 360.0f );
	ImGui::SliderFloat( u8"描画アルファ",		&alpha,   0.0f, 1.0f );
	ImGui::DragFloat2 ( u8"描画スケール",		&drawScale.x, 0.1f );

	ImGui::DragFloat2 ( u8"パート・左上座標",					&texPos.x  );
	ImGui::DragFloat2 ( u8"パート・切り取りサイズ（全体）",	&texSize.x );

	ImGui::TreePop();
}
#endif // USE_IMGUI
