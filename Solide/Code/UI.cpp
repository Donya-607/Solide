#include "UI.h"

#include "Donya/Sprite.h"

bool UIObject::LoadSprite( const std::wstring &filePath, size_t maxCount )
{
	sprite = Donya::Sprite::Load( filePath, maxCount );
	return ( sprite == NULL ) ? false : true;
}

bool UIObject::Draw( float depth ) const
{
	const float prevDepth = Donya::Sprite::GetDrawDepth();
	Donya::Sprite::SetDrawDepth( depth );

	bool result = Donya::Sprite::DrawExt
	(
		sprite,
		pos.x, pos.y,
		drawScale.x, drawScale.y,
		degree, alpha
	);

	Donya::Sprite::SetDrawDepth( prevDepth );
	return result;
}
bool UIObject::DrawPart( float depth ) const
{
	const float prevDepth = Donya::Sprite::GetDrawDepth();
	Donya::Sprite::SetDrawDepth( depth );

	bool result = Donya::Sprite::DrawPartExt
	(
		sprite,
		pos.x, pos.y,
		texPos.x, texPos.y,
		texSize.x, texSize.y,
		drawScale.x, drawScale.y,
		degree, alpha
	);

	Donya::Sprite::SetDrawDepth( prevDepth );
	return result;
}

Donya::Int2 UIObject::GetSpriteSize() const
{
	return Donya::Sprite::GetTextureSize( sprite );
}

#if USE_IMGUI
void UIObject::ShowImGuiNode( const std::string &nodeCaption )
{
	if ( !ImGui::TreeNode( nodeCaption.c_str() ) ) { return; }
	// else

	ImGui::DragFloat2 ( u8"�X�N���[�����W",	&pos.x );

	ImGui::SliderFloat( u8"�`��p�x",		&degree,  -360.0f, 360.0f );
	ImGui::SliderFloat( u8"�`��A���t�@",		&alpha,   0.0f, 1.0f );
	ImGui::DragFloat2 ( u8"�`��X�P�[��",		&drawScale.x, 0.1f );

	ImGui::DragFloat2 ( u8"�p�[�g�E������W",					&texPos.x  );
	ImGui::DragFloat2 ( u8"�p�[�g�E�؂���T�C�Y�i�S�́j",	&texSize.x );

	ImGui::TreePop();
}
#endif // USE_IMGUI
