#include "Shadow.h"

#include <string>

#include "Donya/GeometricPrimitive.h"
#include "Donya/Useful.h"

#include "FilePath.h"

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
	
}

void Shadow::Draw( const Donya::Vector4x4 &VP) 
{

}
