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
	NumberDrawer	numberDrawer;
	Rank			rankDrawer;
private: // Serialize members.
	Donya::Easing::Kind	easeKind	= Donya::Easing::Kind::Linear;
	Donya::Easing::Type	easeType	= Donya::Easing::Type::In;
	float				targetRange	= 1.0f;	// Radius
	float				lowestScale	= 0.2f;
	float				baseDrawDepth = 0.5f;
	Donya::Vector3		basePosOffset;
	Donya::Vector2		baseDrawSize;		// Whole size
	Donya::Vector2		ssDrawOffsetNumber;
	Donya::Vector2		ssDrawOffsetRank;
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
			CEREAL_NVP( lowestScale			),
			CEREAL_NVP( baseDrawDepth		),
			CEREAL_NVP( basePosOffset		),
			CEREAL_NVP( baseDrawSize		),
			CEREAL_NVP( ssDrawOffsetNumber	),
			CEREAL_NVP( ssDrawOffsetRank	)
		);

		if ( 1 <= version )
		{
			// archive( CEREAL_NVP( x ) );
		}
	}
	static constexpr const char *ID = "InfoDisplayer";
public:
	bool Init();
	void DrawInfo( const Donya::Vector4x4 &matWorldToScreen, const Donya::Vector3 &playerPos, const Donya::Vector3 &baseInfoPos, const SaveData::ClearData &drawData, int drawStageNo );
private:
	float CalcDrawScale( const Donya::Vector3 &playerPos, const Donya::Vector3 &baseInfoPos );
	Donya::Vector2 CalcScreenPos( const Donya::Vector3 &baseInfoPos, const Donya::Vector4x4 &matWorldToScreen );
private:
	void DrawStageNumber( const Donya::Vector2 &ssPos, float drawScale, int stageNo );
	void DrawRank( const Donya::Vector2 &ssPos, float drawScale, const SaveData::ClearData &drawData );
private:
	void LoadBin( int stageNo );
	void LoadJson( int stageNo );
#if USE_IMGUI
	void SaveBin( int stageNo );
	void SaveJson( int stageNo );
public:
	void ShowImGuiNode( const std::string &nodeCaption );
#endif // USE_IMGUI
};
CEREAL_CLASS_VERSION( StageInfoDisplayer, 0 )
