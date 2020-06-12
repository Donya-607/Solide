#include "InfoDisplayer.h"

#include "Donya/Useful.h"	// Use SeparateDigits()

#include "FilePath.h"
#include "Parameter.h"

bool StageInfoDisplayer::Init()
{
#if DEBUG_MODE
	LoadJson();
#else
	LoadBin();
#endif // DEBUG_MODE

	bool succeeded = true;

	using Spr = SpriteAttribute;
	if ( !sprFrame.LoadSprite		( GetSpritePath( Spr::StageInfoFrame	), 32U	) ) { succeeded = false; }
	if ( !sprBossStage.LoadSprite	( GetSpritePath( Spr::BossStage			), 32U	) ) { succeeded = false; }
	if ( !sprLockedStage.LoadSprite	( GetSpritePath( Spr::LockedStage		), 64U	) ) { succeeded = false; }
	if ( !numberDrawer.Init			( GetSpritePath( Spr::Number			)		) ) { succeeded = false; }
	if ( !rankDrawer.Init			( GetSpritePath( Spr::ClearRank			)		) ) { succeeded = false; }

	return succeeded;
}
void StageInfoDisplayer::DrawInfo( const Donya::Vector4x4 &matScreen, const Donya::Vector3 &playerPos, const Donya::Vector3 &basePos, const SaveData::ClearData &drawData, int drawStageNo, bool isUnlockedStage, bool isBossStage )
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

	if ( isBossStage )		{ DrawBossStage( ssPos, drawScale );				}
	else					{ DrawStageNumber( ssPos, drawScale, drawStageNo );	}

	DrawRank( ssPos, drawScale, drawData );

	if ( !isUnlockedStage )
	{
		DrawLockedStage( ssPos, drawScale );
	}
}
float StageInfoDisplayer::CalcDrawScale( const Donya::Vector3 &playerPos, const Donya::Vector3 &basePos )
{
	const float distance = ( playerPos - basePos ).Length();
	if ( targetRange < distance ) { return 0.0f; }
	// else

	const float percent = 1.0f - ( distance / targetRange );
	return std::min( maxBaseDrawScale, Donya::Easing::Ease( easeKind, easeType, percent ) );
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
	std::vector<unsigned int> uDigits = Donya::SeparateDigits( scast<unsigned int>( stageNo ), 2 ); // Store '12' to [0:2][1:1], '1' to [0:1][1:0]
	std::reverse( uDigits.begin(), uDigits.end() ); // [0:2][1:1] to [0:1][1:2], [0:1][1:0] to [0:0][1:1]

	std::vector<int> digits;
	for ( const auto &it : uDigits )
	{
		digits.emplace_back( scast<int>( it ) );
	}

	const float scale = drawScale * drawScaleNumber;
	numberDrawer.DrawNumbers
	(
		digits,
		NumberDrawer::Delimiter::Hyphen,
		ssPos + ( ssDrawOffsetNumber * scale ),
		scale,
		1.0f, Donya::Vector2{ 0.5f, 0.5f },
		baseDrawDepth,
		1
	);
}
void StageInfoDisplayer::DrawRank( const Donya::Vector2 &ssPos, float drawScale, const SaveData::ClearData &drawData )
{
	const float scale = drawScale * drawScaleRank;
	rankDrawer.Draw
	(
		drawData.clearRank,
		ssPos + ( ssDrawOffsetRank * scale ),
		scale,
		0.0f, 1.0f,
		Donya::Vector2{ 0.5f, 0.5f },
		baseDrawDepth
	);
}
void StageInfoDisplayer::DrawBossStage( const Donya::Vector2 &ssPos, float drawScale )
{
	const float scale = drawScale * drawScaleBossStage;
	sprBossStage.pos		= ssPos + ( ssDrawOffsetBossStage * scale );
	sprBossStage.drawScale	= scale;
	sprBossStage.Draw( baseDrawDepth );
}
void StageInfoDisplayer::DrawLockedStage( const Donya::Vector2 &ssPos, float drawScale )
{
	const float scale = drawScale * drawScaleLockedStage;
	sprLockedStage.pos			= ssPos + ( ssDrawOffsetLockedStage * scale );
	sprLockedStage.drawScale	= scale;
	sprLockedStage.Draw( baseDrawDepth );
}
void StageInfoDisplayer::LoadBin()
{
	constexpr bool fromBinary = true;
	Donya::Serializer::Load( *this, GenerateSerializePath( ID, fromBinary ).c_str(), ID, fromBinary );
}
void StageInfoDisplayer::LoadJson()
{
	constexpr bool fromBinary = false;
	Donya::Serializer::Load( *this, GenerateSerializePath( ID, fromBinary ).c_str(), ID, fromBinary );
}
#if USE_IMGUI
void StageInfoDisplayer::SaveBin()
{
	constexpr bool fromBinary = true;
	Donya::Serializer::Save( *this, GenerateSerializePath( ID, fromBinary ).c_str(), ID, fromBinary );
}
void StageInfoDisplayer::SaveJson()
{
	constexpr bool fromBinary = false;
	Donya::Serializer::Save( *this, GenerateSerializePath( ID, fromBinary ).c_str(), ID, fromBinary );
}
void StageInfoDisplayer::ShowImGuiNode( const std::string &nodeCaption )
{
	if ( !ImGui::TreeNode( nodeCaption.c_str() ) ) { return; }
	// else

	ParameterHelper::ShowEaseParam( u8"�����ɂ��X�P�[�����O�Ɏg���C�[�W���O", &easeKind, &easeType );

	ImGui::DragFloat ( u8"�`�悵�͂��߂鋗���i���a�j",	&targetRange,		0.1f  );
	ImGui::DragFloat ( u8"�ő��{�`��X�P�[��",		&maxBaseDrawScale,	0.1f  );
	targetRange			= std::max( 0.001f,	targetRange			);
	maxBaseDrawScale	= std::max( 0.0f,	maxBaseDrawScale	);
	ImGui::SliderFloat( u8"�`��y�l",				&baseDrawDepth, 0.0f, 1.0f );

	ImGui::DragFloat3( u8"�\���ʒu�̃I�t�Z�b�g",			&basePosOffset.x,	0.01f	);
	ImGui::DragFloat2( u8"�`��X�v���C�g�T�C�Y",			&baseDrawSize.x				);

	auto ShowOffsetAndScale = []( const std::string &itemName, Donya::Vector2 *pOffset, float *pScale )
	{
		ImGui::DragFloat2( ( itemName + u8"�E�`��I�t�Z�b�g"	).c_str(), &pOffset->x );
		ImGui::DragFloat ( ( itemName + u8"�E�`��X�P�[��"	).c_str(), pScale, 0.01f );
	};
	ShowOffsetAndScale( u8"�X�e�[�W�ԍ�",		&ssDrawOffsetNumber,		&drawScaleNumber		);
	ShowOffsetAndScale( u8"�����N",			&ssDrawOffsetRank,			&drawScaleRank			);
	ShowOffsetAndScale( u8"�{�X�X�e�[�W",		&ssDrawOffsetBossStage,		&drawScaleBossStage		);
	ShowOffsetAndScale( u8"���J���X�e�[�W",	&ssDrawOffsetLockedStage,	&drawScaleLockedStage	);

	auto ShowIONode = [&]()
	{
		if ( !ImGui::TreeNode( u8"�t�@�C�� I/O" ) ) { return; }
		// else

		static bool isBinary = true;
		if ( ImGui::RadioButton( "Binary", isBinary ) ) { isBinary = true;  }
		if ( ImGui::RadioButton( "Json",  !isBinary ) ) { isBinary = false; }

		std::string loadStr = u8"���[�h";
		loadStr += u8"(by:";
		loadStr += ( isBinary ) ? u8"Binary" : u8"Json";
		loadStr += u8")";

		if ( ImGui::Button( u8"�Z�[�u" ) )
		{
			SaveBin ();
			SaveJson();
		}
		if ( ImGui::Button( loadStr.c_str() ) )
		{
			( isBinary ) ? LoadBin() : LoadJson();
		}

		ImGui::TreePop();
	};
	ShowIONode();

	ImGui::TreePop();
}
#endif // USE_IMGUI
