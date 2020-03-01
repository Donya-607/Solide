#pragma once

#include <memory>
#include <string>
#include <vector>

#undef max
#undef min
#include <cereal/types/memory.hpp>
#include <cereal/types/vector.hpp>

#include "Donya/Collision.h"
#include "Donya/Serializer.h"
#include "Donya/Vector.h"

#include "Obstacles.h"

class ObstacleContainer
{
private:
	int stageNo = 0;
	std::vector<std::shared_ptr<ObstacleBase>> pObstacles;
private:
	friend class cereal::access;
	template<class Archive>
	void serialize( Archive &archive, std::uint32_t version )
	{
		archive
		(
			CEREAL_NVP( pObstacles )
		);

		if ( 1 <= version )
		{
			// archive( CEREAL_NVP( x ) );
		}
	}
	static constexpr const char *ID = "Obstacles";
public:
	void Init( int stageNo );
	void Uninit();

	void Update( float elapsedTime );

	void Draw( const Donya::Vector4 &eyePos, float transNear, float transFar, float transLowerAlpha, const Donya::Vector4x4 &VP, const Donya::Vector4 &lightDir, const Donya::Vector4 &color );
public:
	void SortByDepth();
public:
	std::vector<Donya::AABB> GetHitBoxes() const;
private:
	std::string MakeSerializePath( int stageNo, bool fromBinary ) const;

	void LoadBin ( int stageNo );
	void LoadJson( int stageNo );
#if USE_IMGUI
	void SaveBin ( int stageNo );
	void SaveJson( int stageNo );
public:
	void ShowImGuiNode( const std::string &nodeCaption );
#endif // USE_IMGUI
};
CEREAL_CLASS_VERSION( ObstacleContainer, 0 )
