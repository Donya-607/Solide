#pragma once

#include "UI.h"

class BG
{
private:
	float		horizonPos = 0.0f;
private: // Serialize members.
	UIObject	sprBG;
	UIObject	sprCloud;
	float		scrollSpeed = 1.0f;
	float		cloudWidth = 1920.0f;
private:
	friend class cereal::access;
	template<class Archive>
	void serialize( Archive &archive, std::uint32_t version )
	{
		archive
		(
			CEREAL_NVP( sprBG ),
			CEREAL_NVP( sprCloud ),
			CEREAL_NVP( scrollSpeed ),
			CEREAL_NVP( cloudWidth )
		);

		if ( 0 <= version )
		{
			// archive( CEREAL_NVP( x ) );
		}
	}
	static constexpr const char *ID = "BG";
public:
	bool LoadSprites( const std::wstring &BGFileName, const std::wstring &cloudFileName );

	void Update( float elapsedTime );

	void Draw( float elapsedTime );
private:
	void LoadBin();
	void LoadJson();
#if USE_IMGUI
	void SaveBin();
	void SaveJson();
public:
	void ShowImGuiNode( const std::string &nodeCaption );
#endif // USE_IMGUI
};
CEREAL_CLASS_VERSION( BG, 0 )
