#pragma once

#include <string>

#include "Donya/Collision.h"
#include "Donya/Serializer.h"
#include "Donya/Vector.h"
#include "Donya/UseImGui.h"

#include "ObjectBase.h"

/// <summary>
/// Provides some world position.
/// </summary>
class Section : protected Solid
{
public:
	Section();
	Section( const Donya::Vector3 &wsPos );
	Section( const Donya::Vector3 &wsPos, const Donya::AABB &wsHitBox );
private:
	friend class cereal::access;
	template<class Archive>
	void serialize( Archive &archive, std::uint32_t version )
	{
		archive
		(
			CEREAL_NVP( pos ),
			CEREAL_NVP( hitBox )
		);

		if ( 1 <= version )
		{
			// archive( CEREAL_NVP( x ) );
		}
	}
public:
	void DrawHitBox( const Donya::Vector4x4 &matVP, const Donya::Vector4 &color = { 1.0f, 1.0f, 1.0f, 1.0f } ) const
	{
		Solid::DrawHitBox( matVP, color );
	}
public:
	void SetPosition( const Donya::Vector3 &wsPos )
	{
		pos = wsPos;
	}
	Donya::Vector3 GetPosition() const
	{
		return Solid::GetPosition();
	}
	Donya::AABB GetHitBox() const
	{
		return Solid::GetHitBox();
	}
public:
#if USE_IMGUI
	void ShowImGuiNode( const std::string &nodeCaption, bool *pShouldErase = nullptr );
#endif // USE_IMGUI
};
CEREAL_CLASS_VERSION( Section, 0 )
