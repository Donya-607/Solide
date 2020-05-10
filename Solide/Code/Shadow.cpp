#include "Shadow.h"

#include <string>

#include "Donya/GeometricPrimitive.h"
#include "Donya/Useful.h"

#include "FilePath.h"

Shadow::Shadow()  = default;
Shadow::~Shadow() = default;

bool Shadow::LoadTexture()
{
	// Already loaded.
	if ( pTexture ) { return true; }
	// else

	const std::wstring filePath = GetSpritePath( SpriteAttribute::CircleShadow );
	if ( !Donya::IsExistFile( filePath ) )
	{
		_ASSERT_EXPR( 0, L"Error: The shadow texture does not exist!" );
		return false;
	}
	// else

	pTexture =	std::make_unique<Donya::Geometric::TextureBoard>
				(
					Donya::Geometric::CreateTextureBoard( filePath )
				);
	return true;
}
void Shadow::ClearInstances()
{
	shadows.clear();
}

void Shadow::Register( const Donya::Vector3 &rayStart, float rayLength )
{
	Instance tmp;
	tmp.origin		= rayStart;
	tmp.rayLength	= rayLength;
	shadows.emplace_back( std::move( tmp ) );
}

void Shadow::CalcIntersectionPoints( const std::vector<Donya::AABB> &solids, const Donya::Model::PolygonGroup *pTerrain, const Donya::Vector4x4 *pTerrainMatrix, const Donya::Vector3 &rayDir )
{
	auto CalcNearestIntersectionAABB	= [&]( const Donya::Vector3 &rayStart, const Donya::Vector3 &rayEnd )
	{
		Donya::RayIntersectResult result;
		result.isIntersect = false;
		result.normal = Donya::Vector3::Up();

		if ( solids.empty() ) { return result; }
		// else

		Donya::RayIntersectResult	tmpResult;
		std::vector<Donya::Vector3>	points;
		for ( const auto &it : solids )
		{
			tmpResult = Donya::CalcIntersectionPoint( rayStart, rayEnd, it );
			if ( !tmpResult.isIntersect ) { continue; }
			// else

			points.emplace_back( tmpResult.intersection );
		}

		if ( points.empty() ) { return result; }
		// else

		float nearestDistance = FLT_MAX;
		Donya::Vector3 nearestPoint{};
		for ( const auto &it : points )
		{
			float distSq = ( it - rayStart ).LengthSq();
			if (  distSq < nearestDistance )
			{
				nearestPoint = it;
				nearestDistance = distSq;
			}
		}

		result.isIntersect = true;
		result.intersection = nearestPoint;
		return result;
	};
	auto CalcNearestIntersectionTerrain	= [&]( const Donya::Vector3 &rayStart, const Donya::Vector3 &rayEnd )
	{
		Donya::Model::RaycastResult result{};
		result.wasHit = false;

		if ( !pTerrain || !pTerrainMatrix ) { return result; }
		// else

		result = pTerrain->RaycastWorldSpace( *pTerrainMatrix, rayStart, rayEnd );
		return result;
	};
	auto CalcIntersectionPoint			= [&]( Instance &element )
	{
		const Donya::Vector3 rayStart	= element.origin;
		const Donya::Vector3 rayEnd		= rayStart + ( rayDir * element.rayLength );

		const auto vsAABB		= CalcNearestIntersectionAABB( rayStart, rayEnd );
		const auto vsTerrain	= CalcNearestIntersectionTerrain( rayStart, rayEnd );

		element.exist = false;
		if ( !vsAABB.isIntersect && !vsTerrain.wasHit ) { return; }
		// else

		element.exist = true;

		const auto &pointAABB		= vsAABB.intersection;
		const auto &pointTerrain	= vsTerrain.intersection;
		auto ApplyAABB		= [&]()
		{
			element.intersection	= pointAABB;
			element.normal			= vsAABB.normal;
		};
		auto ApplyTerrain	= [&]()
		{
			element.intersection	= pointTerrain;
			element.normal			= vsTerrain.nearestPolygon.normal;
		};

		if ( !vsAABB.isIntersect	) { ApplyTerrain();	return; }
		if ( !vsTerrain.wasHit		) { ApplyAABB();	return; }
		// else
		
		const float distAABB	= ( pointAABB - rayStart ).LengthSq();
		const float distTerrain	= ( pointTerrain - rayStart ).LengthSq();
		
		if ( distAABB < distTerrain )
		{
			ApplyAABB();
		}
		else
		{
			ApplyTerrain();
		}
	};

	constexpr Donya::Vector3 smallOffset{ 0.0f, 0.01f, 0.0f }; // Prevent a z-fighting on a terrain.
	for ( auto &it : shadows )
	{
		CalcIntersectionPoint( it );
		it.intersection += smallOffset;
	}

	auto itr = std::remove_if
	(
		shadows.begin(), shadows.end(),
		[]( Instance &element ) { return !element.exist; }
	);
	shadows.erase( itr, shadows.end() );
}

void Shadow::Draw( const Donya::Vector4x4 &VP )
{
	if ( !pTexture ) { return; }
	// else

	auto MakeWorldMatrix = []( const Instance &element )
	{
		const auto rotation = Donya::Quaternion::LookAt( Donya::Vector3::Front(), element.normal );
		Donya::Vector4x4 W  = rotation.MakeRotationMatrix();
		W._41 = element.intersection.x;
		W._42 = element.intersection.y;
		W._43 = element.intersection.z;
		return W;
	};

	Donya::Vector4x4 W;
	for ( const auto &it : shadows )
	{
		W = MakeWorldMatrix( it );

		constexpr Donya::Vector4 lightDir{ 0.0f, -1.0f, 0.0f, 0.0f };
		pTexture->Render
		(
			nullptr,
			true, true,
			W * VP, W,
			lightDir
		);
	}
}
