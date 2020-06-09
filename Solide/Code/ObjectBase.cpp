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
		m    *= rotation.MakeRotationMatrix();
		m._41 = wsPos.x;
		m._42 = wsPos.y;
		m._43 = wsPos.z;
		return m;
	}
	void DrawCube( RenderingHelper *pRenderer, const Donya::Vector4x4 &W, const Donya::Vector4x4 &VP, const Donya::Vector4 &color )
	{
		Donya::Model::Cube::Constant constant;
		constant.matWorld		= W;
		constant.matViewProj	= VP;
		constant.drawColor		= color;
		constant.lightDirection	= -Donya::Vector3::Up();
		pRenderer->ProcessDrawingCube( constant );

		/*
		static auto cube = Donya::Geometric::CreateCube();
		constexpr Donya::Vector4 lightDir{ 0.0f, -1.0f, 0.0f, 0.0f };

		cube.Render
		(
			nullptr,
			/ useDefaultShading	= / true,
			/ isEnableFill		= / true,
			W * VP, W,
			lightDir, color
		);
		*/
	}
}
#if DEBUG_MODE
namespace
{
	static Donya::Geometric::Line debugLine{ 256U };
	void ReserveLine( const Donya::Vector3 &start, const Donya::Vector3 &end, const Donya::Vector4 &color )
	{
		debugLine.Init();

		debugLine.Reserve( start, end, color );
	}
	void FlushLine( const Donya::Vector4x4 &VP )
	{
		debugLine.Flush( VP );
	}
}
#endif // DEBUG_MODE

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

void Solid::DrawHitBox( RenderingHelper *pRenderer, const Donya::Vector4x4 &VP, const Donya::Vector4 &color ) const
{
	const auto body = GetHitBox();
	DrawCube( pRenderer, MakeWorldMatrix( body.pos, body.size ), VP, color );
}



Actor::MoveResult Actor::Move( const Donya::Vector3 &movement, const std::vector<Donya::Vector3> &wsRayOffsets, const std::vector<Donya::AABB> &solids, const Donya::Model::PolygonGroup *pTerrain, const Donya::Vector4x4 *pTerrainMatrix )
{
	Actor::MoveResult result;
	result.lastResult.wasHit = false;

	if ( !pTerrain || !pTerrainMatrix )
	{
		pos += movement;
		return result;
	}
	// else

	// First, move on XZ plane. Then correct my position by only XZ plane.
	// Second, move on Y plane.

	const Donya::Vector3 xzMovement{ movement.x,	0.0f,		movement.z	};
	const Donya::Vector3 yMovement { 0.0f,			movement.y,	0.0f		};

	if ( !xzMovement.IsZero() ) { MoveXZImpl( xzMovement, wsRayOffsets, 0, solids, pTerrain, pTerrainMatrix ); }
	if ( !yMovement.IsZero()  ) { result = MoveYImpl( yMovement, solids, pTerrain, pTerrainMatrix ); }

	// If a face is there under from my foot, I correct onto the face for considering a slope.
	// But this correction is not necessary if the vertical movement is up.
	if ( yMovement.y <= 0.0f )
	{
		// This input velocity length should be serialized.
		// Because if this is lower, the slope correction is not function,
		// if this is larger, an actor will landing too hurry.
		const Donya::Vector3 checkUnderLength{ 0.0f, -hitBox.size.y * 1.5001f, 0.0f };
		auto resultV = CalcCorrectedVector( 1, checkUnderLength, pTerrain, pTerrainMatrix );
		if ( resultV.raycastResult.wasHit )
		{
			const Donya::Vector3 verticalSize{ 0.0f, hitBox.size.y, 0.0f };
			const Donya::Vector3 footPos = pos - verticalSize;
			const Donya::Vector3 diff = resultV.raycastResult.intersection - footPos;
			pos.y += diff.y;
			result.lastNormal = resultV.raycastResult.nearestPolygon.normal;
		}
	}

	return result;
}

Actor::AABBResult		Actor::CalcCorrectedVector( const Donya::Vector3 &vector, const std::vector<Donya::AABB> &solids ) const
{
	AABBResult defaultResult{};
	defaultResult.correctedVector	= vector;
	defaultResult.wasHit			= false;

	return	( solids.empty() )
			? defaultResult
			: CalcCorrectedVectorImpl( vector, solids );
}
Actor::RecursionResult	Actor::CalcCorrectedVector( int recursionLimit, const Donya::Vector3 &vector, const Donya::Model::PolygonGroup *pTerrain, const Donya::Vector4x4 *pTerrainMatrix ) const
{
	return CalcCorrectedVector( pos, recursionLimit, vector, pTerrain, pTerrainMatrix );
}
Actor::RecursionResult	Actor::CalcCorrectedVector( const Donya::Vector3 &wsRayStart, int recursionLimit, const Donya::Vector3 &vector, const Donya::Model::PolygonGroup *pTerrain, const Donya::Vector4x4 *pTerrainMatrix ) const
{
	RecursionResult initial{};
	initial.correctedVector = vector;
	initial.raycastResult.wasHit = false;

	return	( !pTerrain || !pTerrainMatrix )
			? initial
			: CalcCorrectedVectorImpl( wsRayStart, recursionLimit, 0, initial, *pTerrain, *pTerrainMatrix );
}
Actor::AABBResult		Actor::CalcCorrectedVectorImpl( const Donya::Vector3 &vector, const std::vector<Donya::AABB> &solids ) const
{
	AABBResult result{};
	result.correctedVector = vector;
	result.wasHit = false;

	Donya::Vector3 moveSign // The moving direction of myself. Take a value of +1.0f or -1.0f.
	{
		scast<float>( Donya::SignBit( vector.x ) ),
		scast<float>( Donya::SignBit( vector.y ) ),
		scast<float>( Donya::SignBit( vector.z ) )
	};

	if ( moveSign.IsZero() ) { return result; }
	// else

	// HACK : Should I do not use hit-box? Currently, the collision processes does not use hit-box, using the point only.
	Donya::AABB movedBody = GetHitBox();
	movedBody.pos += vector;

	Donya::AABB *pOther{};

	// Will control the exist flag.
	std::vector<Donya::AABB> currSolids = solids;
	{
		// Remove a solids that is not exist.
		auto itr = std::remove_if
		(
			currSolids.begin(), currSolids.end(),
			[]( Donya::AABB &elem )
			{
				return !elem.exist;
			}
		);
		currSolids.erase( itr, currSolids.end() );
	}

	auto FindCollidingAABB	= [&]( const Donya::AABB &myself, const std::vector<Donya::AABB> &solids, bool exceptMyself = true )->Donya::AABB *
	{
		for ( auto &it : currSolids )
		{
			if ( exceptMyself && it == myself ) { continue; }
			// else

			if ( Donya::AABB::IsHitAABB( myself, it ) )
			{
				return &it;
			}
		}

		return nullptr;
	};
	
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
		constexpr float ERROR_MARGIN = 0.001f;

		Donya::Vector3 resolver
		{
			( penetration.x + ERROR_MARGIN ) * -myMoveSign.x,
			( penetration.y + ERROR_MARGIN ) * -myMoveSign.y,
			( penetration.z + ERROR_MARGIN ) * -myMoveSign.z
		};
		return resolver;
	};

	constexpr unsigned int MAX_LOOP_COUNT = 1000U;
	unsigned int loopCount{};
	while ( ++loopCount < MAX_LOOP_COUNT )
	{
		pOther = FindCollidingAABB( movedBody, solids );
		if ( !pOther ) { break; } // Does not detected a collision.
		// else

		// Store absolute value.
		const Donya::Vector3 penetration	= CalcPenetration( movedBody, moveSign, *pOther );
		const Donya::Vector3 resolver		= CalcResolver( penetration, moveSign );

		if	(
				vector[0] < penetration[0] &&
				vector[1] < penetration[1] &&
				vector[2] < penetration[2]
			)
		{
			// Bury. For prevent resolving as long distance.
			result.correctedVector = Donya::Vector3::Zero();
			result.wasHit = true;
			return result;
		}
		// else

		// Repulse to the more little(but greater than zero) axis side of penetration.

		enum AXIS { X = 0, Y, Z };
		auto Increment		= []( AXIS axis )
		{
			return scast<AXIS>( scast<int>( axis ) + 1 );
		};
		auto CalcLowestAxis	= [&Increment]( const Donya::Vector3 &v )->AXIS
		{
			// Fail safe.
			if ( v.IsZero() ) { return X; }
			// else

			auto IsLowerThanOther  = [&Increment]( Donya::Vector3 v, AXIS targetAxis )
			{
				for ( AXIS i = X; i <= Z; i = Increment( i ) )
				{
					// Except the same axis.
					if ( i == targetAxis ) { continue; }
					if ( ZeroEqual( v[i] ) ) { continue; }
					// else

					if ( v[i] < v[targetAxis] )
					{
						return false;
					}
				}

				return true;
			};
			auto AssignInitialAxis = [&Increment]( Donya::Vector3 v )->AXIS
			{
				for ( AXIS i = X; i <= Z; i = Increment( i ) )
				{
					if ( ZeroEqual( v[i] ) ) { continue; }
					// else
					return i;
				}

				// Fail safe.
				return Y;
			};

			AXIS lowestAxis = AssignInitialAxis( v );
			for ( AXIS i = X; i <= Z; i = Increment( i ) )
			{
				if ( ZeroEqual( v[i] ) ) { continue; }
				// else

				if ( IsLowerThanOther( v, i ) )
				{
					lowestAxis = i;
				}
			}

			return lowestAxis;
		};
		const AXIS min		= CalcLowestAxis( penetration );

		movedBody.pos[min] += resolver[min];
		moveSign[min]		= scast<float>( Donya::SignBit( resolver[min] ) );
		result.wasHit		= true;

		if ( moveSign.IsZero()  ) { break; }
		// else

		for ( auto &it : currSolids ) { it.exist = true; }
	}

	const Donya::Vector3 &destination = movedBody.pos - hitBox.pos/* Except the offset of hitBox */;
		
	result.correctedVector = destination - pos;
	return result;
}
Actor::RecursionResult	Actor::CalcCorrectedVectorImpl( const Donya::Vector3 &wsRayStart,int recursionLimit, int recursionCount, RecursionResult inheritedResult, const Donya::Model::PolygonGroup &terrain, const Donya::Vector4x4 &terrainMatrix ) const
{
	constexpr float ERROR_ADJUST = 0.001f;

	// If we can't resolve with very small movement, we give-up the moving.
	if ( recursionLimit <= recursionCount || inheritedResult.correctedVector.Length() < ERROR_ADJUST )
	{
		inheritedResult.correctedVector = Donya::Vector3::Zero();
		return inheritedResult;
	}
	// else

	const Donya::Vector3 wsRayEnd		=  wsRayStart + inheritedResult.correctedVector;

	const Donya::Model::RaycastResult currentResult = terrain.RaycastWorldSpace( terrainMatrix, wsRayStart, wsRayEnd );

	// The moving vector(ray) didn't collide to anything, so we can move directly.
	if ( !currentResult.wasHit ) { return inheritedResult; }
	// else

	const Donya::Vector3 internalVec	=  wsRayEnd - currentResult.intersection;
	const Donya::Vector3 wsFaceNormal	=  currentResult.nearestPolygon.normal;
	const Donya::Vector3 projVelocity	= -wsFaceNormal * Dot( internalVec, -wsFaceNormal );

	constexpr float ERROR_MAGNI = 1.0f + ERROR_ADJUST;
	inheritedResult.correctedVector		-= projVelocity * ERROR_MAGNI;
	inheritedResult.raycastResult		=  currentResult;

	// Recurse by corrected velocity.
	// This recursion will stop when the corrected velocity was not collided.
	return CalcCorrectedVectorImpl( wsRayStart, recursionLimit, recursionCount + 1, inheritedResult, terrain, terrainMatrix );
}

namespace
{
	Donya::Vector3 MakeSizeOffset( const Donya::Vector3 &size, const Donya::Vector3 &movement )
	{
		/*
		const Donya::Vector3 sizeOffset
		{
			hitBox.size.x * Donya::SignBit( movement.x ),
			hitBox.size.y * Donya::SignBit( movement.y ),
			hitBox.size.z * Donya::SignBit( movement.z ),
		};
		*/
		const Donya::Vector3 nMovement = movement.Unit();
		const Donya::Vector3 sizeOffset
		{
			size.x * std::max( -1.0f, std::min( 1.0f, nMovement.x ) ),
			size.y * std::max( -1.0f, std::min( 1.0f, nMovement.y ) ),
			size.z * std::max( -1.0f, std::min( 1.0f, nMovement.z ) ),
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
void Actor::MoveXZImpl( const Donya::Vector3 &xzMovement, const std::vector<Donya::Vector3> &wsRayOffsets, int recursionCount, const std::vector<Donya::AABB> &solids, const Donya::Model::PolygonGroup *pTerrain, const Donya::Vector4x4 *pTerrainMatrix )
{
	// The hit-box size of projected to movement component.
	const Donya::Vector3 extendSize	= MakeSizeOffset( hitBox.size, xzMovement );
	
	// For prevent the ray-start overs a polygon, start from behind myself.
	const Donya::Vector3 rayOffset	= xzMovement * 0.5f;
	const Donya::Vector3 rayStart	= pos + hitBox.pos - rayOffset;
	const Donya::Vector3 rayVector	= xzMovement + extendSize + rayOffset;
#if DEBUG_MODE
	{
		// Actual velocity.
		Donya::Vector3	start = rayStart;
		Donya::Vector3	end   = rayStart + rayVector;
		ReserveLine( start, end,	{ 1.0f, 0.0f, 0.0f, 0.8f } );

		// Extended velocity for visualize.
		start = end;
		end   = start + rayVector.Unit() * 2.0f;
		ReserveLine( start, end,	{ 1.0f, 1.0f, 0.0f, 0.8f } );

		// Raw velocity for compare.
		start = rayStart + Donya::Vector3{ 0.0f, 0.01f, 0.0f };
		end   = start + xzMovement;
		ReserveLine( start, end,	{ 1.0f, 1.0f, 1.0f, 0.8f } );
	}
#endif // DEBUG_MODE

	constexpr int RECURSIVE_LIMIT	= 4;
	auto result = CalcCorrectedVector( rayStart, RECURSIVE_LIMIT, rayVector, pTerrain, pTerrainMatrix );
	if ( result.raycastResult.wasHit )
	{
	#if DEBUG_MODE
		{
			constexpr float normLength		= 6.0f;
			const Donya::Vector3 normStart	= result.raycastResult.intersection;
			const Donya::Vector3 normEnd	= normStart + result.raycastResult.nearestPolygon.normal * normLength;
			ReserveLine( normStart, normEnd, { 0.2f, 1.0f, 0.0f, 0.8f } );
		}
	#endif // DEBUG_MODE

		// The "corrected-velocity is zero" means a wall places too nearly.
		if ( result.correctedVector.IsZero() ) { return; }
		// else

		// Prevent the sign to be inverse by subtract.

		const Donya::Int3 beforeSigns
		{
			Donya::SignBit( result.correctedVector.x ),
			Donya::SignBit( result.correctedVector.y ),
			Donya::SignBit( result.correctedVector.z ),
		};

		result.correctedVector -= extendSize;

		const Donya::Int3 afterSigns
		{
			Donya::SignBit( result.correctedVector.x ),
			Donya::SignBit( result.correctedVector.y ),
			Donya::SignBit( result.correctedVector.z ),
		};
		if ( afterSigns.x != beforeSigns.x ) { result.correctedVector.x = 0.0f; }
		if ( afterSigns.y != beforeSigns.y ) { result.correctedVector.y = 0.0f; }
		if ( afterSigns.z != beforeSigns.z ) { result.correctedVector.z = 0.0f; }
	}
	else
	{
		result.correctedVector -= extendSize;
	}

	result.correctedVector = CalcCorrectedVectorByMyHitBox( result.correctedVector, wsRayOffsets, pTerrain, pTerrainMatrix );

	auto aabbResult = CalcCorrectedVector( result.correctedVector, solids );

	aabbResult.correctedVector = CalcCorrectedVectorByMyHitBox( aabbResult.correctedVector, wsRayOffsets, pTerrain, pTerrainMatrix );

	// If still colliding to something, we regard as can not move to wanted velocity.

	const Donya::Vector3 applyVelocity = aabbResult.correctedVector;
	auto IsHitToAnything = [&]( const Donya::Vector3 &validateVelocity )
	{
		const auto vsSolids = CalcCorrectedVector( validateVelocity, solids );
		if ( vsSolids.wasHit ) { return true; }
		// else

		const auto vsTerrain = CalcCorrectedVector( rayStart, 1, validateVelocity, pTerrain, pTerrainMatrix );
		if ( vsTerrain.raycastResult.wasHit ) { return true; }
		// else

		const auto hitBoxVsTerrain = CalcCorrectedVectorByMyHitBox( validateVelocity, wsRayOffsets, pTerrain, pTerrainMatrix );
		if ( hitBoxVsTerrain != validateVelocity ) { return true; }
		// else

		return false;
	};
	if ( IsHitToAnything( applyVelocity ) ) { return; }
	// else

	pos += applyVelocity;
}
Actor::MoveResult Actor::MoveYImpl( const Donya::Vector3 &yMovement, const std::vector<Donya::AABB> &solids, const Donya::Model::PolygonGroup *pTerrain, const Donya::Vector4x4 *pTerrainMatrix )
{
	// The hit-box size of projected to movement component.
	const Donya::Vector3 extendSize = MakeSizeOffset( hitBox.size, yMovement );

	// Y moving does not need the correction recursively.
	constexpr int RECURSIVE_LIMIT = 1;
	auto rayResult = CalcCorrectedVector( RECURSIVE_LIMIT, yMovement + extendSize, pTerrain, pTerrainMatrix );
	rayResult.correctedVector -= extendSize;
	
	const float moveSign = scast<float>( Donya::SignBit( yMovement.y ) );
	const float errorMargin = 0.0001f * moveSign; // Make more distance from intersection, I want do not start the raycast from a closest point.
	const Donya::Vector3 sizeOffset	= Donya::Vector3::Up() * hitBox.size.y * moveSign;
	const Donya::Vector3 destPos	= ( rayResult.raycastResult.wasHit )
									? rayResult.raycastResult.intersection - sizeOffset - errorMargin
									: pos + rayResult.correctedVector;
	const Donya::Vector3 actualMovement = destPos - pos;
	
	const auto aabbResult = CalcCorrectedVector( actualMovement, solids );
	pos += aabbResult.correctedVector;

	MoveResult result;
	result.lastNormal = rayResult.raycastResult.nearestPolygon.normal;
	result.lastResult = rayResult.raycastResult;

	// If the position corrected by AABB, We should return the AABB's normal.
	if ( aabbResult.wasHit )
	{
		const Donya::Vector3 directlyMovedPos = pos + actualMovement;
		const Donya::Vector3 diff = pos - directlyMovedPos;
		result.lastNormal.x = 0.0f;
		result.lastNormal.y = scast<float>( Donya::SignBit( diff.y ) );
		result.lastNormal.z = 0.0f;
	}

	return result;
}

Donya::Vector3 Actor::CalcCorrectedVectorByMyHitBox( const Donya::Vector3 &velocity, const std::vector<Donya::Vector3> &wsRayOffsets, const Donya::Model::PolygonGroup *pTerrain, const Donya::Vector4x4 *pTerrainMatrix )
{
	const Donya::Vector3 horizontalSize{ hitBox.size.x, 0.0f, hitBox.size.z };

	//const Donya::Vector3 axis			= Donya::Vector3::Up();
	//const Donya::Vector3 unitVelocity	= Donya::Vector3::Front(); // velocity.Unit();
	//const Donya::Vector3 directions[]
	//{
	//	Donya::Quaternion::Make( axis, ToRadian( +45.0f  ) ).RotateVector( unitVelocity ),
	//	Donya::Quaternion::Make( axis, ToRadian( +90.0f  ) ).RotateVector( unitVelocity ),
	//	Donya::Quaternion::Make( axis, ToRadian( +135.0f ) ).RotateVector( unitVelocity ),
	//
	//	Donya::Quaternion::Make( axis, ToRadian( -45.0f  ) ).RotateVector( unitVelocity ),
	//	Donya::Quaternion::Make( axis, ToRadian( -90.0f  ) ).RotateVector( unitVelocity ),
	//	Donya::Quaternion::Make( axis, ToRadian( -135.0f ) ).RotateVector( unitVelocity ),
	//};
	constexpr Donya::Vector3 directions[]
	{
		{ +0.707f, 0.0f, +0.707f },
		{ +1.0f, 0.0f, 0.0f },
		{ +0.707f, 0.0f, -0.707f },

		{ -0.707f, 0.0f, +0.707f },
		{ -1.0f, 0.0f, 0.0f },
		{ -0.707f, 0.0f, -0.707f },

		// { 0.0f, 0.0f, +1.0f },
		// { 0.0f, 0.0f, -1.0f },

		// { +1.0f, 0.0f, +1.0f },
		// { +1.0f, 0.0f, -1.0f },
		// { -1.0f, 0.0f, -1.0f },
		// { -1.0f, 0.0f, +1.0f },
	};

	Donya::Vector3 movedPos = ( pos + hitBox.pos ) + velocity;

	for ( const auto &sign : directions )
	{
		const Donya::Vector3 velocityDirection
		{
			horizontalSize.x * sign.x,
			horizontalSize.y * sign.y,
			horizontalSize.z * sign.z,
		};

	#if DEBUG_MODE
		{
			constexpr Donya::Vector3 ofs{ 0.0f, 0.7f, 0.0f };
			ReserveLine( movedPos + ofs, movedPos + ofs + velocityDirection, { 0.7f, 0.7f, 0.7f, 0.8f } );
		}
	#endif // DEBUG_MODE

		constexpr int RECURSIVE_COUNT = 4;	// Prevent the corrected velocity to be zero that recursion method will return the zero if the recursion count arrived to limit.
		auto sideResult = CalcCorrectedVector( movedPos, RECURSIVE_COUNT, velocityDirection, pTerrain, pTerrainMatrix );
		if ( sideResult.raycastResult.wasHit )
		{
			if ( sideResult.correctedVector.IsZero() ) { continue; }
			// else

			const Donya::Vector3 diff = velocityDirection - sideResult.correctedVector;
			constexpr float ERROR_MARGIN = 1.0001f;
			movedPos -= diff * ERROR_MARGIN;
			break;
		}
	}

	const Donya::Vector3 corrected = movedPos - ( pos + hitBox.pos );
	return corrected;
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

void Actor::DrawHitBox( RenderingHelper *pRenderer, const Donya::Vector4x4 &VP, const Donya::Quaternion &rotation, const Donya::Vector4 &color ) const
{
#if DEBUG_MODE
	//FlushLine( VP );
#endif // DEBUG_MODE

	const auto body = GetHitBox();
	DrawCube( pRenderer, MakeWorldMatrix( body.pos, body.size, rotation ), VP, color );
}
