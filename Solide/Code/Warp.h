#pragma once

#include <vector>

#include "Donya/Collision.h"
#include "Donya/Quaternion.h"
#include "Donya/Serializer.h"
#include "Donya/UseImGui.h"
#include "Donya/Vector.h"

#include "Renderer.h"
#include "Timer.h"

class Warp
{
private:
	bool unlocked = false;
private: // Serialize members.
	int					destStageNo	= 1;
	float				drawScale	= 1.0f;
	Donya::Vector3		wsPos;
	Donya::Vector4		drawColor{ 1.0f, 1.0f, 1.0f, 1.0f };
	Donya::AABB			hitBox;
	std::vector<Timer>	borderTimes;
	Donya::Vector3		returningPosOffset;		// Used for decision a returning position of the player that warped by myself.
	Donya::Quaternion	returningOrientation;	// Used for decision a returning orientation of the player that warped by myself.
private:
	friend class cereal::access;
	template<class Archive>
	void serialize( Archive &archive, std::uint32_t version )
	{
		archive
		(
			CEREAL_NVP( destStageNo ),
			CEREAL_NVP( drawScale	),
			CEREAL_NVP( wsPos		),
			CEREAL_NVP( drawColor	),
			CEREAL_NVP( hitBox		)
		);

		if ( 1 <= version )
		{
			archive( CEREAL_NVP( borderTimes ) );
		}
		if ( 2 <= version )
		{
			archive
			(
				CEREAL_NVP( returningPosOffset		),
				CEREAL_NVP( returningOrientation	)
			);
		}
		if ( 3 <= version )
		{
			// archive( CEREAL_NVP( x ) );
		}
	}
public:
	void Init();
	void Uninit();

	void Update( float elapsedTime );

	void Draw( RenderingHelper *pRenderer, const Donya::Vector4 &color );
	void DrawHitBox( RenderingHelper *pRenderer, const Donya::Vector4x4 &matVP, const Donya::Vector4 &color );
public:
	bool				IsUnlocked()				const;
	int					GetDestinationStageNo()		const;
	Donya::Vector3		GetPosition()				const;
	Donya::AABB			GetHitBox()					const;
	Donya::Vector4x4	CalcWorldMatrix( bool useForHitBox ) const;
	std::vector<Timer>	GetBorderTimes()			const;
	Donya::Vector3		GetReturningPosition()		const;
	Donya::Quaternion	GetReturningOrientation()	const;
private:
	Donya::Vector4		MakeDrawColor( const Donya::Vector4 &blendColor ) const;
#if USE_IMGUI
public:
	void ShowImGuiNode( const std::string &nodeCaption, bool *wantRemoveMe );
#endif // USE_IMGUI
};
CEREAL_CLASS_VERSION( Warp, 2 )


class WarpContainer
{
public:
	static bool LoadResource();
private: // Serializer member.
	int					stageNo	= 1;
	std::vector<Warp>	warps;
private:
	friend class cereal::access;
	template<class Archive>
	void serialize( Archive &archive, std::uint32_t version )
	{
		archive( CEREAL_NVP( warps ) );

		if ( 1 <= version )
		{
			// archive( CEREAL_NVP( x ) );
		}
	}
	static constexpr const char *ID = "Warp";
public:
	void Init( int stageNo );
	void Uninit();

	void Update( float elapsedTime );

	void Draw( RenderingHelper *pRenderer, const Donya::Vector4 &color );
	void DrawHitBoxes( RenderingHelper *pRenderer, const Donya::Vector4x4 &matVP, const Donya::Vector4 &color );
public:
	size_t		GetWarpCount() const;
	bool		IsOutOfRange( size_t warpIndex ) const;
	const Warp	*GetWarpPtrOrNullptr( size_t warpIndex ) const;
private:
	void LoadBin ( int stageNo );
	void LoadJson( int stageNo );
#if USE_IMGUI
	void SaveBin ( int stageNo );
	void SaveJson( int stageNo );
public:
	void ShowImGuiNode( const std::string &nodeCaption, int stageNo );
#endif // USE_IMGUI
};
CEREAL_CLASS_VERSION( WarpContainer, 0 )

