#include "InfoDisplayer.h"

#include "Donya/Useful.h"	// Use SeparateDigits()

#include "FilePath.h"
#include "Parameter.h"

bool StageInfoDisplayer::Init()
{
	bool succeeded = true;

	if ( !sprFrame.LoadSprite( GetSpritePath( SpriteAttribute::StageInfoFrame	), 32U	) ) { succeeded = false; }
	if ( !numberDrawer.Init  ( GetSpritePath( SpriteAttribute::Number			)		) ) { succeeded = false; }
	if ( !rankDrawer.Init    ( GetSpritePath( SpriteAttribute::ClearRank		)		) ) { succeeded = false; }

	return succeeded;
}
void StageInfoDisplayer::DrawInfo( const Donya::Vector4x4 &matScreen, const Donya::Vector3 &playerPos, const Donya::Vector3 &basePos, const SaveData::ClearData &drawData, int drawStageNo )
{
	const float drawScale = CalcDrawScale( playerPos, basePos );
	if ( drawScale <= 0.0f ) { return; }
	// else

	const Donya::Vector2 ssPos = CalcScreenPos( basePos, matScreen );

	sprFrame.pos		= ssPos;
	sprFrame.drawScale	= drawScale;
	sprFrame.texPos		= 0.0f;
	sprFrame.texSize	= baseDrawSize;
	sprFrame.DrawPart( baseDrawDepth );

	DrawStageNumber( ssPos, drawScale, drawStageNo );
	DrawRank( ssPos, drawScale, drawData );
}
float StageInfoDisplayer::CalcDrawScale( const Donya::Vector3 &playerPos, const Donya::Vector3 &basePos )
{
	const float distance = ( playerPos - basePos ).Length();
	if ( targetRange < distance ) { return 0.0f; }
	// else

	const float percent = distance / targetRange;
	return std::max( lowestScale, Donya::Easing::Ease( easeKind, easeType, percent ) );
}
Donya::Vector2 StageInfoDisplayer::CalcScreenPos( const Donya::Vector3 &basePos, const Donya::Vector4x4 &matScreen )
{
	auto WorldToScreen = [&matScreen]( const Donya::Vector3 &worldPos )
	{
		Donya::Vector4 tmp = matScreen.Mul( worldPos, 1.0f );
		tmp /= tmp.w;
		return tmp.XYZ();
	};
	return WorldToScreen( basePos + basePosOffset ).XY();
}
void StageInfoDisplayer::DrawStageNumber( const Donya::Vector2 &ssPos, float drawScale, int stageNo )
{
	const std::vector<unsigned int> uDigits = Donya::SeparateDigits( scast<unsigned int>( stageNo ), 2 );
	std::vector<int> digits; digits.resize( uDigits.size() );
	for ( const auto &it : uDigits )
	{
		digits.emplace_back( scast<int>( it ) );
	}

	numberDrawer.DrawNumbers
	(
		digits,
		NumberDrawer::Delimiter::Hyphen,
		ssPos + ssDrawOffsetNumber, drawScale,
		1.0f, Donya::Vector2{ 0.5f, 0.5f },
		baseDrawDepth
	);
}
void StageInfoDisplayer::DrawRank( const Donya::Vector2 &ssPos, float drawScale, const SaveData::ClearData &drawData )
{
	rankDrawer.Draw
	(
		drawData.clearRank,
		ssPos + ssDrawOffsetRank,
		drawScale,
		0.0f, 1.0f,
		Donya::Vector2{ 0.5f, 0.5f },
		baseDrawDepth
	);
}
void StageInfoDisplayer::LoadBin( int stageNo )
{
	constexpr bool fromBinary = true;
	Donya::Serializer::Load( *this, MakeStageParamPath( ID, stageNo, fromBinary ).c_str(), ID, fromBinary );
}
void StageInfoDisplayer::LoadJson( int stageNo )
{
	constexpr bool fromBinary = false;
	Donya::Serializer::Load( *this, MakeStageParamPath( ID, stageNo, fromBinary ).c_str(), ID, fromBinary );
}
#if USE_IMGUI
void StageInfoDisplayer::SaveBin( int stageNo )
{
	constexpr bool fromBinary = true;

	const std::string filePath = MakeStageParamPath( ID, stageNo, fromBinary );
	MakeFileIfNotExists( filePath, fromBinary );

	Donya::Serializer::Save( *this, filePath.c_str(), ID, fromBinary );
}
void StageInfoDisplayer::SaveJson( int stageNo )
{
	constexpr bool fromBinary = false;

	const std::string filePath = MakeStageParamPath( ID, stageNo, fromBinary );
	MakeFileIfNotExists( filePath, fromBinary );

	Donya::Serializer::Save( *this, filePath.c_str(), ID, fromBinary );
}
void StageInfoDisplayer::ShowImGuiNode( const std::string &nodeCaption )
{
	if ( !ImGui::TreeNode( nodeCaption.c_str() ) ) { return; }
	// else

	ParameterHelper::ShowEaseParam( u8"距離によるスケーリングに使うイージング", &easeKind, &easeType );

	ImGui::DragFloat( u8"描画しはじめる距離（半径）",	&targetRange, 0.1f  );
	ImGui::DragFloat( u8"最低描画スケール",			&lowestScale, 0.01f );
	targetRange = std::max( 0.001f, targetRange );
	lowestScale = std::max( 0.0f,   lowestScale );
	ImGui::SliderFloat( u8"描画Ｚ値",				&baseDrawDepth, 0.0f, 1.0f );

	ImGui::DragFloat3( u8"表示位置のオフセット",			&basePosOffset.x, 0.01f	);
	ImGui::DragFloat2( u8"基本描画サイズ",				&baseDrawSize.x			);
	ImGui::DragFloat2( u8"描画オフセット・ステージ番号",	&ssDrawOffsetNumber.x	);
	ImGui::DragFloat2( u8"描画オフセット・ランク",		&ssDrawOffsetRank.x		);

	ImGui::TreePop();
}
#endif // USE_IMGUI
