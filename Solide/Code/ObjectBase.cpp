#include "ObjectBase.h"

#include <memory>

#include "Donya/GeometricPrimitive.h"
#include "Donya/StaticMesh.h"
#include "Donya/Useful.h"

namespace
{
	Donya::Vector4x4 MakeWorldMatrix( const Donya::Vector3 &wsPos, const Donya::Vector3 &halfSize, const Donya::Quaternion &rotation = Donya::Quaternion::Identity() )
	{
		// The size is half.
		// But that will using as scale, so we should multiply to double.
		
		Donya::Vector4x4 m{};
		m._11 = halfSize.x * 2.0f;
		m._22 = halfSize.y * 2.0f;
		m._33 = halfSize.z * 2.0f;
		m    *= rotation.RequireRotationMatrix();
		m._41 = wsPos.x;
		m._42 = wsPos.y;
		m._43 = wsPos.z;
		return m;
	}
	void DrawCube( const Donya::Vector4x4 &W, const Donya::Vector4x4 &VP, const Donya::Vector4 &color )
	{
		static auto cube = Donya::Geometric::CreateCube();
		constexpr Donya::Vector4 lightDir{ 0.0f, -1.0f, 0.0f, 0.0f };

		cube.Render
		(
			nullptr,
			/* useDefaultShading	= */ true,
			/* isEnableFill			= */ true,
			W * VP, W,
			lightDir, color
		);
	}
}

void Solid::Move( const Donya::Vector3 &movement, const std::vector<Actor *> &actors )
{
	// TODO : Implement collision resolving process.
	pos += movement;
}

Donya::Vector3 Solid::GetPosition() const
{
	return pos;
}
Donya::AABB Solid::GetHitBox() const
{
	Donya::AABB tmp = hitBox;
	tmp.pos += pos;
	return tmp;
}
Donya::Vector4x4 Solid::GetWorldMatrix() const
{
	return MakeWorldMatrix( GetPosition(), { 0.5f, 0.5f, 0.5f } );
}

void Solid::DrawHitBox( const Donya::Vector4x4 &VP, const Donya::Vector4 &color ) const
{
	const auto body = GetHitBox();
	DrawCube( MakeWorldMatrix( body.pos, body.size ), VP, color );
}



void Actor::Move( const Donya::Vector3 &movement, const std::vector<Donya::Vector3> &wsRayOffsets, const Donya::StaticMesh *pTerrain, const Donya::Vector4x4 *pTerrainMatrix )
{
	if ( !pTerrain || !pTerrainMatrix )
	{
		pos += movement;
		return;
	}
	// else

	// First, move on XZ plane. Then correct my position by only XZ plane.
	// Second, move on Y plane.

	const Donya::Vector3 xzMovement{ movement.x,	0.0f,		movement.z	};
	const Donya::Vector3 yMovement { 0.0f,			movement.y,	0.0f		};

	if ( !xzMovement.IsZero() ) { MoveXZImpl( xzMovement, wsRayOffsets, 0, pTerrain, pTerrainMatrix ); }
	if ( !yMovement.IsZero()  ) { MoveYImpl ( yMovement,  pTerrain, pTerrainMatrix ); }
}
namespace
{
	Donya::Vector3 MakeSizeOffset( const Donya::AABB &hitBox, const Donya::Vector3 &sign )
	{
		const Donya::Vector3 sizeOffset
		{
			hitBox.size.x * Donya::SignBit( sign.x ),
			hitBox.size.y * Donya::SignBit( sign.y ),
			hitBox.size.z * Donya::SignBit( sign.z ),
		};
		return sizeOffset;
	}
	Donya::Vector3 ToVec3( const Donya::Vector4 &v )
	{
		return Donya::Vector3
		{
			v.x,	// / v.w,
			v.y,	// / v.w,
			v.z,	// / v.w,
		};
	}
	Donya::Vector3 Transform( const Donya::Vector4x4 &M, const Donya::Vector3 &V, float fourthParam )
	{
		return ToVec3( M.Mul( V, fourthParam ) );
	}
}
void Actor::MoveXZImpl( const Donya::Vector3 &xzMovement, const std::vector<Donya::Vector3> &wsRayOffsets, int recursionCount, const Donya::StaticMesh *pTerrain, const Donya::Vector4x4 *pTerrainMatrix )
{
	constexpr int RECURSIVE_LIMIT = 255;
	auto result = CalcCorrectVelocity( xzMovement, wsRayOffsets, pTerrain, pTerrainMatrix, {}, 0, RECURSIVE_LIMIT );

	pos += result.correctedVelocity;

	/*
	// If we can't resolve with very small movement, we give-up the moving.
	constexpr int RECURSIVE_LIMIT = 255;
	if ( RECURSIVE_LIMIT <= recursionCount ) { return; }
	if ( xzMovement.Length() < 0.001f ) { return; }
	// else

	const Donya::Vector4x4	&terrainMat		= *pTerrainMatrix;
	const Donya::Vector4x4	invTerrainMat	=  pTerrainMatrix->Inverse();
	const Donya::Vector3	xzSizeOffset	=  MakeSizeOffset( hitBox, xzMovement );
	const Donya::Vector3	wsRayStart		=  pos;
	const Donya::Vector3	wsRayEnd		=  wsRayStart + xzMovement + xzSizeOffset;

	// Terrain space.
	const Donya::Vector3 tsRayStart	= Transform( invTerrainMat, wsRayStart,	1.0f );
	const Donya::Vector3 tsRayEnd	= Transform( invTerrainMat, wsRayEnd,	1.0f );

	// Store nearest collided result of rays that be affected offsets, or invalid(wasHit == false).
	Donya::StaticMesh::RayPickResult result{};
	{
		result.wasHit = false;

		// Store a ray pick results of each ray offsets.

		Donya::Vector3 tsRayOffset{};
		std::vector<Donya::StaticMesh::RayPickResult> temporaryResults{};
		for ( const auto &offset : wsRayOffsets )
		{
			tsRayOffset = Transform( invTerrainMat, offset, 1.0f );

			auto tmp = pTerrain->RayPick( tsRayStart + tsRayOffset, tsRayEnd + tsRayOffset );
			temporaryResults.emplace_back( std::move( tmp ) );
		}

		// Choose the nearest collided result.

		float nearestDistance = FLT_MAX;
		Donya::Vector3 diff{};
		for ( const auto &it : temporaryResults )
		{
			if ( !it.wasHit ) { continue; }
			// else

			diff = it.intersectionPoint - tsRayStart;
			if ( diff.LengthSq() < nearestDistance )
			{
				nearestDistance = diff.LengthSq();
				result = it;
			}
		}
	}

	// The moving vector(ray) didn't collide to anything, so we can move directly.
	if ( !result.wasHit )
	{
		pos += xzMovement;
		return;
	}
	// else

	// Transform to World space from Terrains pace.
	const Donya::Vector3 wsIntersection	= Transform( terrainMat, result.intersectionPoint, 1.0f );
	const Donya::Vector3 wsWallNormal	= Transform( terrainMat, result.normal, 0.0f ).Normalized();

	const Donya::Vector3 internalVec	= wsRayEnd - wsIntersection;
	const Donya::Vector3 projVelocity	= -wsWallNormal * Dot( internalVec, -wsWallNormal );

	constexpr float ERROR_MAGNI = 1.0f + 0.1f;
	Donya::Vector3 correctedVelocity = xzMovement + ( -projVelocity * ERROR_MAGNI );

	// Move by corrected velocity.
	// This recursion will stop when the corrected velocity was not collided.
	MoveXZImpl( correctedVelocity, wsRayOffsets, recursionCount + 1, pTerrain, pTerrainMatrix );
	*/
}
void Actor::MoveYImpl ( const Donya::Vector3 &yMovement, const Donya::StaticMesh *pTerrain, const Donya::Vector4x4 *pTerrainMatrix )
{
	// Y moving does not need the correction recursively.
	constexpr int RECURSIVE_LIMIT = 1;
	auto result = CalcCorrectVelocity( yMovement, {}, pTerrain, pTerrainMatrix, {}, 0, RECURSIVE_LIMIT );

	pos += result.correctedVelocity;

	/*
	const Donya::Vector4x4	&terrainMat		= *pTerrainMatrix;
	const Donya::Vector4x4	invTerrainMat	=  pTerrainMatrix->Inverse();
	const Donya::Vector3	ySizeOffset		=  MakeSizeOffset( hitBox, yMovement );
	const Donya::Vector3	wsRayStart		=  pos - yMovement;
	const Donya::Vector3	wsRayEnd		=  pos + yMovement + ySizeOffset;

	// Terrain space.
	const Donya::Vector3 tsRayStart	= Transform( invTerrainMat, wsRayStart,	1.0f );
	const Donya::Vector3 tsRayEnd	= Transform( invTerrainMat, wsRayEnd,	1.0f );

	auto  result = pTerrain->RayPick( tsRayStart, tsRayEnd );
	// The moving vector(ray) didn't collide to anything, so we can move directly.
	if ( !result.wasHit )
	{
		pos += yMovement;
		return;
	}
	// else

	// Transform to World space from Terrain space.
	const Donya::Vector3 wsIntersection	= Transform( terrainMat, result.intersectionPoint, 1.0f );
	const Donya::Vector3 wsWallNormal	= Transform( terrainMat, result.normal, 0.0f ).Normalized();

	pos = wsIntersection - ySizeOffset;
	*/
}
Actor::CalcedRayResult Actor::CalcCorrectVelocity( const Donya::Vector3 &velocity, const std::vector<Donya::Vector3> &wsRayOffsets, const Donya::StaticMesh *pTerrain, const Donya::Vector4x4 *pTerrainMatrix, CalcedRayResult recursionResult, int recursionCount, int recursionLimit ) const
{
	constexpr float ERROR_ADJUST = 0.001f;

	// If we can't resolve with very small movement, we give-up the moving.
	if ( recursionLimit <= recursionCount ) { return recursionResult; }
	if ( velocity.Length() < ERROR_ADJUST ) { return recursionResult; }
	// else

	const Donya::Vector4x4	&terrainMat		= *pTerrainMatrix;
	const Donya::Vector4x4	invTerrainMat	=  pTerrainMatrix->Inverse();
	const Donya::Vector3	xzSizeOffset	=  MakeSizeOffset( hitBox, velocity );
	const Donya::Vector3	wsRayStart		=  pos;
	const Donya::Vector3	wsRayEnd		=  wsRayStart + velocity + xzSizeOffset;

	// Terrain space.
	const Donya::Vector3	tsRayStart		= Transform( invTerrainMat, wsRayStart,	1.0f );
	const Donya::Vector3	tsRayEnd		= Transform( invTerrainMat, wsRayEnd,	1.0f );

	// Store nearest collided result of rays that be affected offsets, or invalid(wasHit == false).
	Donya::StaticMesh::RayPickResult result{};
	{
		result.wasHit = false;

		if ( wsRayOffsets.empty() )
		{
			result = pTerrain->RayPick( tsRayStart, tsRayEnd );
		}
		else
		{
			// Store a ray pick results of each ray offsets.

			Donya::Vector3 tsRayOffset{};
			std::vector<Donya::StaticMesh::RayPickResult> temporaryResults{};
			for ( const auto &offset : wsRayOffsets )
			{
				tsRayOffset = Transform( invTerrainMat, offset, 1.0f );

				auto tmp = pTerrain->RayPick( tsRayStart + tsRayOffset, tsRayEnd + tsRayOffset );
				temporaryResults.emplace_back( std::move( tmp ) );
			}

			// Choose the nearest collided result.

			float nearestDistance = FLT_MAX;
			Donya::Vector3 diff{};
			for ( const auto &it : temporaryResults )
			{
				if ( !it.wasHit ) { continue; }
				// else

				diff = it.intersectionPoint - tsRayStart;
				if ( diff.LengthSq() < nearestDistance )
				{
					nearestDistance = diff.LengthSq();
					result = it;
				}
			}
		}
	}

	// The moving vector(ray) didn't collide to anything, so we can move directly.
	if ( !result.wasHit )
	{
		recursionResult.correctedVelocity = velocity;
		return recursionResult;
	}
	// else

	// Transform to World space from Terrains pace.
	const Donya::Vector3 wsIntersection	= Transform( terrainMat, result.intersectionPoint, 1.0f );
	const Donya::Vector3 wsWallNormal	= Transform( terrainMat, result.normal, 0.0f ).Normalized();

	const Donya::Vector3 internalVec	= wsRayEnd - wsIntersection;
	const Donya::Vector3 projVelocity	= -wsWallNormal * Dot( internalVec, -wsWallNormal );

	recursionResult.wsIntersection		= wsIntersection;
	recursionResult.wsWallNormal		= wsWallNormal;

	constexpr float ERROR_MAGNI = 1.0f + ERROR_ADJUST;
	Donya::Vector3 correctedVelocity = velocity + ( -projVelocity * ERROR_MAGNI );

	// Recurse by corrected velocity.
	// This recursion will stop when the corrected velocity was not collided.
	return CalcCorrectVelocity( correctedVelocity, wsRayOffsets, pTerrain, pTerrainMatrix, recursionResult, recursionCount + 1, recursionLimit );
}

bool Actor::IsRiding( const Solid &onto ) const
{
	constexpr Donya::Vector3 checkOffset{ 0.0f, 0.01f, 0.0f };

	Donya::AABB myself = hitBox;
	myself.pos += checkOffset;

	return Donya::AABB::IsHitAABB( myself, onto.GetHitBox() );
}

Donya::Vector3 Actor::GetPosition() const
{
	return pos;
}
Donya::AABB Actor::GetHitBox() const
{
	Donya::AABB tmp = hitBox;
	tmp.pos += pos;
	return tmp;
}
Donya::Vector4x4 Actor::GetWorldMatrix() const
{	
	return MakeWorldMatrix( GetPosition(), { 0.5f, 0.5f, 0.5f } );
}

void Actor::DrawHitBox( const Donya::Vector4x4 &VP, const Donya::Quaternion &rotation, const Donya::Vector4 &color ) const
{
	const auto body = GetHitBox();
	DrawCube( MakeWorldMatrix( body.pos, body.size, rotation ), VP, color );
}
