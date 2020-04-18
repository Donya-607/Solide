#pragma once

#include <memory>
#include <string>
#include <vector>

#undef max
#undef min
#include <cereal/types/memory.hpp>
#include <cereal/types/vector.hpp>

#include "Donya/Collision.h"
#include "Donya/ModelPolygon.h"
#include "Donya/Serializer.h"
#include "Donya/Vector.h"

#include "Enemy.h"
#include "Renderer.h"

namespace Enemy
{
	/// <summary>
	/// Store and manage an enemies per stage.
	/// </summary>
	class Container
	{
	private:
		int stageNo = 0;
		std::vector<std::shared_ptr<Enemy::Base>> enemyPtrs;
	private:
		friend class cereal::access;
		template<class Archive>
		void serialize( Archive &archive, std::uint32_t version )
		{
			archive
			(
				CEREAL_NVP( enemyPtrs )
			);

			if ( 1 <= version )
			{
				// archive( CEREAL_NVP( x ) );
			}
		}
		static constexpr const char *ID = "Enemies";
	public:
		void Init( int stageNo );
		void Uninit();

		void Update( float elapsedTime, const Donya::Vector3 &targetPosition );
		void PhysicUpdate( const std::vector<Donya::AABB> &solids = {}, const Donya::Model::PolygonGroup *pTerrain = nullptr, const Donya::Vector4x4 *pTerrainWorldMatrix = nullptr );

		void Draw( RenderingHelper *pRenderer );
		void DrawHitBoxes( RenderingHelper *pRenderer, const Donya::Vector4x4 &VP );
	public:
		void AcquireHitBoxes( std::vector<Donya::AABB> *pAppendDest ) const;
		void AcquireHurtBoxes( std::vector<Donya::AABB> *pAppendDest ) const;
	private:
		void LoadBin ( int stageNo );
		void LoadJson( int stageNo );
	#if USE_IMGUI
		void SaveBin ( int stageNo );
		void SaveJson( int stageNo );
	public:
		void ShowImGuiNode( const std::string &nodeCaption );
	#endif // USE_IMGUI
	};
}
CEREAL_CLASS_VERSION( Enemy::Container, 0 )
