#pragma once

#include "Donya/Collision.h"
#include "Donya/Quaternion.h"
#include "Donya/Serializer.h"
#include "Donya/UseImGui.h"
#include "Donya/Vector.h"

#include "Renderer.h"

class Goal
{
public:
	static bool LoadResource();
private:
	Donya::Quaternion	orientation;
private: // Serializer member.
	float				rotateAngle	= 1.5f;	// Per frame. Degree.
	float				drawScale	= 1.0f;
	Donya::Vector3		wsPos;
	Donya::Vector4		drawColor;
	Donya::AABB			hitBox;
private:
	friend class cereal::access;
	template<class Archive>
	void serialize( Archive &archive, std::uint32_t version )
	{
		archive
		(
			CEREAL_NVP( rotateAngle	),
			CEREAL_NVP( drawScale	),
			CEREAL_NVP( wsPos		),
			CEREAL_NVP( drawColor	),
			CEREAL_NVP( hitBox		)
		);

		if ( 1 <= version )
		{
			// archive( CEREAL_NVP( x ) );
		}
	}
	static constexpr const char *ID = "Goal";
public:
	void Init( int stageNo );
	void Uninit();

	void Update( float elapsedTime );

	void Draw( RenderingHelper *pRenderer, const Donya::Vector4 &color );
	void DrawHitBox( RenderingHelper *pRenderer, const Donya::Vector4x4 &matVP, const Donya::Vector4 &color );
public:
	Donya::Vector3		GetPosition()	const;
	Donya::AABB			GetHitBox()		const;
	Donya::Vector4x4	CalcWorldMatrix( bool useForHitBox ) const;
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
CEREAL_CLASS_VERSION( Goal, 0 )
