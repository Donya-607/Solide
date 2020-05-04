#pragma once

#include <vector>

#include "Donya/Collision.h"
#include "Donya/Quaternion.h"
#include "Donya/Serializer.h"
#include "Donya/UseImGui.h"
#include "Donya/Vector.h"

#include "Player.h"
#include "Renderer.h"

class CheckPoint
{
public:
	struct Instance
	{
		Donya::AABB			hitBox;		// The "pos" is function as offset from the initializer's position.
		PlayerInitializer	initializer;
	private:
		friend class cereal::access;
		template<class Archive>
		void serialize( Archive &archive, std::uint32_t version )
		{
			archive
			(
				CEREAL_NVP( hitBox		),
				CEREAL_NVP( initializer	) 
			);

			if ( 1 <= version )
			{
				// archive( CEREAL_NVP( x ) );
			}
		}
	public:

	public:
		Donya::AABB			GetHitBox() const;
		PlayerInitializer	GetInitializer() const;
	};
private: // Serializer member.
	int						stageNo = 1;
	std::vector<Instance>	points;
private:
	friend class cereal::access;
	template<class Archive>
	void serialize( Archive &archive, std::uint32_t version )
	{
		archive( CEREAL_NVP( points ) );

		if ( 1 <= version )
		{
			// archive( CEREAL_NVP( x ) );
		}
	}
	static constexpr const char *ID = "CheckPoint";
public:
	void Init( int stageNo );
	void Uninit();

	void Update( float elapsedTime );

	void Draw( RenderingHelper *pRenderer, const Donya::Vector4 &color );
	void DrawHitBoxes( RenderingHelper *pRenderer, const Donya::Vector4x4 &matVP, const Donya::Vector4 &color );
public:
	size_t			GetPointCount() const;
	bool			IsOutOfRange( size_t index ) const;
	const Instance	*GetPointPtrOrNullptr( size_t index ) const;
	void			RemovePoint( size_t removeIndex );
private:
	void LoadBin( int stageNo );
	void LoadJson( int stageNo );
#if USE_IMGUI
	void SaveBin( int stageNo );
	void SaveJson( int stageNo );
public:
	void ShowImGuiNode( const std::string &nodeCaption, int stageNo );
#endif // USE_IMGUI
};
CEREAL_CLASS_VERSION( CheckPoint, 0 )
