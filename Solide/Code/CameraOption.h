#pragma once

#include <vector>

#include "Donya/Collision.h"
#include "Donya/Quaternion.h"
#include "Donya/Serializer.h"
#include "Donya/UseImGui.h"
#include "Donya/Vector.h"

#include "Renderer.h"

class CameraOption
{
public:
	struct Instance
	{
		Donya::Vector3 offsetPos;	// The offset of position from the player position.
		Donya::Vector3 offsetFocus;	// The offset of focus from the player position.
		Donya::Vector3 wsPos;
	private:
		friend class cereal::access;
		template<class Archive>
		void serialize( Archive &archive, std::uint32_t version )
		{
			archive
			(
				CEREAL_NVP( offsetPos	),
				CEREAL_NVP( offsetFocus	),
				CEREAL_NVP( wsPos		)
			);

			if ( 1 <= version )
			{
				// archive( CEREAL_NVP( x ) );
			}
		}
	};
private: // Serializer member.
	int						stageNo		= 1;
	int						targetIndex	= 0; // The index of last specified target belongs.
	std::vector<Instance>	options;
private:
	friend class cereal::access;
	template<class Archive>
	void serialize( Archive &archive, std::uint32_t version )
	{
		archive( CEREAL_NVP( options ) );

		if ( 1 <= version )
		{
			// archive( CEREAL_NVP( x ) );
		}
	}
	static constexpr const char *ID = "CameraOption";
public:
	void Init( int stageNo );
	void Uninit();

	void Visualize( RenderingHelper *pRenderer, const Donya::Vector4x4 &matVP, const Donya::Vector4 &color );
public:
	size_t GetOptionCount() const { return options.size(); }
	Instance CalcCurrentOption( const Donya::Vector3 &targetPos );
	void ResetToInitialState();
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
CEREAL_CLASS_VERSION( CameraOption, 0 )
