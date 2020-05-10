#pragma once

#include <memory>
#include <vector>

#include "Donya/Collision.h"
#include "Donya/ModelPolygon.h"
#include "Donya/Vector.h"

namespace Donya
{
	namespace Geometric
	{
		class TextureBoard;
	}
}

/// <summary>
/// First, Register start points of ray.
/// Second, Calculate intersection points by ray.
/// Finally, Draw a circle shadows on intersection points.
/// </summary>
class Shadow
{
private:
	struct Instance
	{
		Donya::Vector3	origin;
		float			rayLength = 10.0f;
		Donya::Vector3	intersection;
		Donya::Vector3	normal;
		bool			exist = true;
	};
private:
	std::vector<Instance> shadows;
	std::unique_ptr<Donya::Geometric::TextureBoard> pTexture = nullptr;
public:
	Shadow();
	Shadow( const Shadow &copy ) = default;
	Shadow(	      Shadow &&ref ) = default;
	Shadow &operator = ( const Shadow &copy ) = default;
	Shadow &operator = (       Shadow &&ref ) = default;
	~Shadow();
public:
	bool LoadTexture();
	void ClearInstances();
public:
	void Register( const Donya::Vector3 &wsRayStartPos, float rayLength = 10.0f );
	/// <summary>
	/// You can set empty or nullptr to argument(except "rayDirection"). That argument will be ignored.
	/// </summary>
	void CalcIntersectionPoints( const std::vector<Donya::AABB> &solids, const Donya::Model::PolygonGroup *pTerrain, const Donya::Vector4x4 *pTerrainWorldMatrix, const Donya::Vector3 &rayDirection = { 0.0f, -1.0f, 0.0f } );
	void Draw( const Donya::Vector4x4 &matVP );
};
