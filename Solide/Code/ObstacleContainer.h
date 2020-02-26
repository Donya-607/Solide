#pragma once

#include <memory>
#include <string>
#include <vector>

#include <cereal/types/memory.hpp>
#include <cereal/types/vector.hpp>

#include "Donya/Collision.h"
#include "Donya/Serializer.h"

#include "Obstacles.h"

class ObstacleContainer
{
private:
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
public:
	std::vector<Donya::AABB> GetHitBoxes() const;
public:
#if USE_IMGUI
	void ShowImGuiNode( const std::string &nodeCaption );
#endif // USE_IMGUI
};
CEREAL_CLASS_VERSION( ObstacleContainer, 0 )
