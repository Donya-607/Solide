#include "ObjectBase.h"

#include <memory>

#include "Donya/Constant.h"
#include "Donya/GeometricPrimitive.h"
#include "Donya/StaticMesh.h"
#include "Donya/Useful.h"

#undef max
#undef min

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



Donya::Vector3 Actor::Move( const Donya::Vector3 &movement, const std::vector<Donya::Vector3> &wsRayOffsets, const std::vector<Donya::AABB> &solids, const Donya::StaticMesh *pTerrain, const Donya::Vector4x4 *pTerrainMatrix )
{
	if ( !pTerrain || !pTerrainMatrix )
	{
		pos += movement;
		return Donya::Vector3::Zero();
	}
	// else

	// First, move on XZ plane. Then correct my position by only XZ plane.
	// Second, move on Y plane.

	const Donya::Vector3 xzMovement{ movement.x,	0.0f,		movement.z	};
	const Donya::Vector3 yMovement { 0.0f,			movement.y,	0.0f		};

	Donya::Vector3 lastNormal{};
	if ( !xzMovement.IsZero() ) { MoveXZImpl( xzMovement, wsRayOffsets, 0, solids, pTerrain, pTerrainMatrix ); }
	if ( !yMovement.IsZero()  ) { lastNormal = MoveYImpl ( yMovement,  solids, pTerrain, pTerrainMatrix ); }

	// If a face is there under from my foot, I correct onto the face for considering a slope.
	// But this correction is not necessary if the vertical movement is up.
	if ( yMovement.y <= 0.0f )
	{
		// The CalcCorrectVelocity() will extend the input velocity by the size of hitBox to the direction of input velocity.
		// This input velocity length should be serialized.
		// Because if this is lower, the slope correction is not function,
		// if this is larger, an actor will landing too hurry.
		const Donya::Vector3 checkUnderLength{ 0.0f, -hitBox.size.y * 0.5f, 0.0f };
		auto resultV = CalcCorrectVelocity( checkUnderLength, {}, pTerrain, pTerrainMatrix, {}, 0, 1 );
		if ( resultV.wasHit )
		{
			const Donya::Vector3 verticalSize{ 0.0f, hitBox.size.y, 0.0f };
			const Donya::Vector3 footPos = pos - verticalSize;
			const Donya::Vector3 diff = resultV.wsLastIntersection - footPos;
			pos.y += diff.y;
			lastNormal = resultV.wsLastWallNormal;
		}
	}

	return lastNormal;
}
namespace
{
	Donya::Vector3 MakeSizeOffset( const Donya::AABB &hitBox, const Donya::Vector3 &movement )
	{
		const float avgSpeed = ( hitBox.size.x + hitBox.size.y + hitBox.size.z ) / 3.0f;
		return movement.Normalized() * avgSpeed;

		/*
		const Donya::Vector3 sizeOffset
		{
			hitBox.size.x * Donya::SignBit( sign.x ),
			hitBox.size.y * Donya::SignBit( sign.y ),
			hitBox.size.z * Donya::SignBit( sign.z ),
		};
		return sizeOffset;
		*/
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
void Actor::MoveXZImpl( const Donya::Vector3 &xzMovement, const std::vector<Donya::Vector3> &wsRayOffsets, int recursionCount, const std::vector<Donya::AABB> &solids, const Donya::StaticMesh *pTerrain, const Donya::Vector4x4 *pTerrainMatrix )
{
	constexpr int RECURSIVE_LIMIT = 4;
	auto result = CalcCorrectVelocity( xzMovement, wsRayOffsets, pTerrain, pTerrainMatrix, {}, 0, RECURSIVE_LIMIT );

	MoveInAABB( result.correctedVelocity, solids );
}
Donya::Vector3 Actor::MoveYImpl ( const Donya::Vector3 &yMovement, const std::vector<Donya::AABB> &solids, const Donya::StaticMesh *pTerrain, const Donya::Vector4x4 *pTerrainMatrix )
{
	// Y moving does not need the correction recursively.
	constexpr int RECURSIVE_LIMIT = 1;
	auto result = CalcCorrectVelocity( yMovement, {}, pTerrain, pTerrainMatrix, {}, 0, RECURSIVE_LIMIT );

	const Donya::Vector3 sizeOffset	= Donya::Vector3::Up() * scast<float>( Donya::SignBit( yMovement.y ) ) * hitBox.size.y;
	const Donya::Vector3 destPos	= ( result.wasHit )
		? result.wsLastIntersection - sizeOffset
		: pos + yMovement;
	const Donya::Vector3 actualMovement = destPos - pos;
	
	const Donya::Vector3 directlyMovedPos = pos + actualMovement;
	MoveInAABB( actualMovement, solids );

	// If the position corrected by AABB, We should store the AABB's normal.
	if ( pos != directlyMovedPos )
	{
		const Donya::Vector3 diff = pos - directlyMovedPos;

		result.wsLastWallNormal = ( 0.0f < diff.y )
			? Donya::Vector3::Up()
			: -Donya::Vector3::Up();
	}

	return result.wsLastWallNormal;
}

Donya::AABB Actor::CalcCollidingBox( const Donya::AABB &myself, const std::vector<Donya::AABB> &solids ) const
{
	for ( const auto &it : solids )
	{
		if ( Donya::AABB::IsHitAABB( myself, it ) )
		{
			return it;
		}
	}

	return Donya::AABB::Nil();
}
void Actor::MoveInAABB( Donya::Vector3 moveVelocity, const std::vector<Donya::AABB> &solids )
{
	Donya::Vector3 moveSign // The moving direction of myself. Take a value of +1.0f or -1.0f.
	{
		scast<float>( Donya::SignBit( moveVelocity.x ) ),
		scast<float>( Donya::SignBit( moveVelocity.y ) ),
		scast<float>( Donya::SignBit( moveVelocity.z ) )
	};
	if ( moveSign.IsZero() ) { return; }
	// else

	Donya::AABB movedBody = GetHitBox();
	movedBody.pos += moveVelocity;

	Donya::AABB other{};
	
	constexpr unsigned int MAX_LOOP_COUNT = 1000U;
	unsigned int loopCount{};
	while ( ++loopCount < MAX_LOOP_COUNT )
	{
		other = CalcCollidingBox( movedBody, solids );
		if ( other == Donya::AABB::Nil() ) { break; } // Does not detected a collision.
		// else

		auto CalcPenetration	= []( const Donya::AABB &myself, const Donya::Vector3 &myMoveSign, const Donya::AABB &other )
		{
			Donya::Vector3 plusPenetration
			{
				fabsf( ( myself.pos.x + myself.size.x ) - ( other.pos.x - other.size.x ) ),
				fabsf( ( myself.pos.y + myself.size.y ) - ( other.pos.y - other.size.y ) ),
				fabsf( ( myself.pos.z + myself.size.z ) - ( other.pos.z - other.size.z ) )
			};
			Donya::Vector3 minusPenetration
			{
				fabsf( ( myself.pos.x - myself.size.x ) - ( other.pos.x + other.size.x ) ),
				fabsf( ( myself.pos.y - myself.size.y ) - ( other.pos.y + other.size.y ) ),
				fabsf( ( myself.pos.z - myself.size.z ) - ( other.pos.z + other.size.z ) )
			};
			Donya::Vector3 penetration{}; // Store absolute value.
			penetration.x
				= ( myMoveSign.x < 0.0f ) ? minusPenetration.x
				: ( myMoveSign.x > 0.0f ) ? plusPenetration.x
				: 0.0f;
			penetration.y
				= ( myMoveSign.y < 0.0f ) ? minusPenetration.y
				: ( myMoveSign.y > 0.0f ) ? plusPenetration.y
				: 0.0f;
			penetration.z
				= ( myMoveSign.z < 0.0f ) ? minusPenetration.z
				: ( myMoveSign.z > 0.0f ) ? plusPenetration.z
				: 0.0f;
			return penetration;
		};
		auto CalcResolver		= []( const Donya::Vector3 &penetration, const Donya::Vector3 &myMoveSign )
		{
			// Prevent the two edges onto same place(the collision detective allows same(equal) value).
			constexpr float ERROR_MARGIN = 0.0001f;

			Donya::Vector3 resolver
			{
				( penetration.x + ERROR_MARGIN ) * -myMoveSign.x,
				( penetration.y + ERROR_MARGIN ) * -myMoveSign.y,
				( penetration.z + ERROR_MARGIN ) * -myMoveSign.z
			};
			return resolver;
		};

		Donya::Vector3 penetration{}; // Store absolute value.
		Donya::Vector3 resolver{};
		penetration	= CalcPenetration( movedBody, moveSign, other );
		resolver	= CalcResolver( penetration, moveSign );

		// Repulse to the more little(but greater than zero) axis side of penetration.

		enum AXIS { X = 0, Y, Z };
		auto At = []( Donya::Vector3 &refV, AXIS index )->float &
		{
			switch ( index )
			{
			case X:  return refV.x;
			case Y:  return refV.y;
			case Z:  return refV.z;
			default: break;
			}

			assert( !"Error" );
			// Fail safe.
			return refV.x;
		};
		auto CalcLowestAxis = [&At]( const Donya::Vector3 &v )->AXIS
		{
			// Fail safe.
			if ( v.IsZero() ) { return X; }
			// else

			auto Add = []( AXIS axis )
			{
				return scast<AXIS>( scast<int>( axis ) + 1 );
			};

			auto IsLowerThanOther = [&Add, &At]( Donya::Vector3 v, AXIS targetAxis )
			{
				for ( AXIS i = X; i <= Z; i = Add( i ) )
				{
					// Except the same axis.
					if ( i == targetAxis ) { continue; }
					if ( ZeroEqual( At( v, i ) ) ) { continue; }
					// else

					if ( At( v, i ) < At( v, targetAxis ) )
					{
						return false;
					}
				}

				return true;
			};
			auto AssignInitialAxis = [&Add, &At]( Donya::Vector3 v )->AXIS
			{
				for ( AXIS i = X; i <= Z; i = Add( i ) )
				{
					if ( ZeroEqual( At( v, i ) ) ) { continue; }
					// else
					return i;
				}

				// Fail safe.
				return Y;
			};

			AXIS lowestAxis = AssignInitialAxis( v );
			for ( AXIS i = X; i <= Z; i = Add( i ) )
			{
				if ( ZeroEqual( At( const_cast<Donya::Vector3 &>( v ), i ) ) ) { continue; }
				// else

				if ( IsLowerThanOther( v, i ) )
				{
					lowestAxis = i;
				}
			}

			return lowestAxis;
		};
		AXIS axis = CalcLowestAxis( penetration );

		At( movedBody.pos,	axis ) += At( resolver, axis );
		At( moveVelocity,	axis ) =  0.0f;
		At( moveSign,		axis ) =  scast<float>( Donya::SignBit( At( resolver, axis ) ) );

		if ( moveSign.IsZero()  ) { break; }
		// else

		/*
		// Repulse to the more little(but greater than zero) axis side of penetration.
		if ( ( penetration.y < penetration.x && !ZeroEqual( penetration.y ) ) || ZeroEqual( penetration.x ) )
		{
			movedBody.pos.y += resolver.y;
			moveVelocity.y  =  0.0f;
			moveSign.y = scast<float>( Donya::SignBit( resolver.y ) );
		}
		else if ( !ZeroEqual( penetration.x ) )
		{
			movedBody.pos.x += resolver.x;
			moveVelocity.x  =  0.0f;
			moveSign.x = scast<float>( Donya::SignBit( resolver.x ) );
		}
		*/
	}

	pos = movedBody.pos - hitBox.pos/* Except the offset of hitBox */;
}

Actor::CalcedRayResult Actor::CalcCorrectVelocity( const Donya::Vector3 &velocity, const std::vector<Donya::Vector3> &wsRayOffsets, const Donya::StaticMesh *pTerrain, const Donya::Vector4x4 *pTerrainMatrix, CalcedRayResult recursionResult, int recursionCount, int recursionLimit ) const
{
	// Donya::OutputDebugStr( std::string{ "Enter into CalcCorrectVelocity(), Recursive count is : " + std::to_string( recursionCount ) + ".\n" }.c_str() );

	constexpr float ERROR_ADJUST = 0.001f;

	// If we can't resolve with very small movement, we give-up the moving.
	if ( recursionLimit <= recursionCount || velocity.Length() < ERROR_ADJUST )
	{
		recursionResult.correctedVelocity = Donya::Vector3::Zero();
		return recursionResult;
	}
	// else

	const Donya::Vector4x4	&terrainMat		= *pTerrainMatrix;
	const Donya::Vector4x4	invTerrainMat	=  pTerrainMatrix->Inverse();
	const Donya::Vector3	sizeOffset		=  MakeSizeOffset( hitBox, velocity );
	const Donya::Vector3	wsRayStart		=  pos;
	const Donya::Vector3	wsRayEnd		=  wsRayStart + velocity + sizeOffset;

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

	recursionResult.wsLastIntersection	= wsIntersection;
	recursionResult.wsLastWallNormal	= wsWallNormal;
	recursionResult.wsLastWallFace[0] = Transform( terrainMat, result.lastCollidedFace[0], 1.0f );
	recursionResult.wsLastWallFace[1] = Transform( terrainMat, result.lastCollidedFace[1], 1.0f );
	recursionResult.wsLastWallFace[2] = Transform( terrainMat, result.lastCollidedFace[2], 1.0f );
	recursionResult.wasHit = true;

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
