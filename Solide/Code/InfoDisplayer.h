#pragma once

#include "Donya/Easing.h"
#include "Donya/Serializer.h"
#include "Donya/Vector.h"

#include "Numeric.h"
#include "Rank.h"
#include "SaveData.h"
#include "Timer.h"
#include "UI.h"

class StageInfoDisplayer
{
private:
	UIObject		sprFrame;
	UIObject		sprBossStage;
	UIObject		sprLockedStage;
	NumberDrawer	numberDrawer;
	Rank			rankDrawer;
private: // Serialize members.
	Donya::Easing::Kind	easeKind				= Donya::Easing::Kind::Linear;
	Donya::Easing::Type	easeType				= Donya::Easing::Type::In;
	float				targetRange				= 1.0f;	// Radius
	float				baseDrawDepth			= 0.5f;
	Donya::Vector3		basePosOffset;
	Donya::Vector2		baseDrawSize; // Whole size
	Donya::Vector2		ssDrawOffsetNumber;
	Donya::Vector2		ssDrawOffsetRank;
	float				drawScaleNumber			= 1.0f;
	float				drawScaleRank			= 1.0f;
	float				maxBaseDrawScale		= 1.0f;
	Donya::Vector2		ssDrawOffsetBossStage;
	Donya::Vector2		ssDrawOffsetLockedStage;
	float				drawScaleBossStage		= 1.0f;
	float				drawScaleLockedStage	= 1.0f;
	float				lockedDarkenAlpha		= 0.1f;
private:
	friend class cereal::access;
	template<class Archive>
	void serialize( Archive &archive, std::uint32_t version )
	{
		archive
		(
			CEREAL_NVP( easeKind			),
			CEREAL_NVP( easeType			),
			CEREAL_NVP( targetRange			),
			CEREAL_NVP( baseDrawDepth		),
			CEREAL_NVP( basePosOffset		),
			CEREAL_NVP( baseDrawSize		),
			CEREAL_NVP( ssDrawOffsetNumber	),
			CEREAL_NVP( ssDrawOffsetRank	)
		);

		if ( 1 <= version )
		{
			archive
			(
				CEREAL_NVP( drawScaleNumber	),
				CEREAL_NVP( drawScaleRank	)
			);
		}
		if ( 2 <= version )
		{
			archive( CEREAL_NVP( maxBaseDrawScale ) );
		}
		if ( 3 <= version )
		{
			archive
			(
				CEREAL_NVP( ssDrawOffsetBossStage	),
				CEREAL_NVP( ssDrawOffsetLockedStage	),
				CEREAL_NVP( drawScaleBossStage		),
				CEREAL_NVP( drawScaleLockedStage	)
			);
		}
		if ( 4 <= version )
		{
			archive( CEREAL_NVP( lockedDarkenAlpha ) );
		}
		if ( 5 <= version )
		{
			// archive( CEREAL_NVP( x ) );
		}
	}
	static constexpr const char *ID = "InfoDisplayer";
public:
	bool Init();
	void DrawInfo( const Donya::Vector4x4 &matWorldToScreen, const Donya::Vector3 &playerPos, const Donya::Vector3 &baseInfoPos, const SaveData::ClearData &drawData, int drawStageNo, bool isUnlockedStage, bool isBossStage );
private:
	float CalcDrawScale( const Donya::Vector3 &playerPos, const Donya::Vector3 &baseInfoPos );
	Donya::Vector2 CalcScreenPos( const Donya::Vector3 &baseInfoPos, const Donya::Vector4x4 &matWorldToScreen );
private:
	void DrawStageNumber( const Donya::Vector2 &ssPos, float drawScale, int stageNo );
	void DrawRank( const Donya::Vector2 &ssPos, float drawScale, const SaveData::ClearData &drawData );
	void DrawBossStage( const Donya::Vector2 &ssPos, float drawScale );
	void DrawLockedStage( const Donya::Vector2 &ssPos, float drawScale );
private:
	void LoadBin();
	void LoadJson();
#if USE_IMGUI
	void SaveBin();
	void SaveJson();
public:
	void ShowImGuiNode( const std::string &nodeCaption );
#endif // USE_IMGUI
};
CEREAL_CLASS_VERSION( StageInfoDisplayer, 4 )
