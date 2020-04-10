#include "FilePath.h"

#include "Donya/Constant.h"	// Use DEBUG_MODE.
#include "Donya/Useful.h"	// Use IsExistFile().

namespace
{
	static constexpr const char *PARAMETERS_DIRECTORY = "./Data/Parameters/";
}

std::string GenerateSerializePath( std::string identifier, bool useBinary )
{
	const std::string EXT = ( useBinary ) ? ".bin" : ".json";
	return PARAMETERS_DIRECTORY + identifier + EXT;
}
std::string MakeStageParamPath( std::string objName, int stageNo, bool useBinary )
{
	const std::string folder = "Stage[" + std::to_string( stageNo ) + "]/";
	return GenerateSerializePath( folder + objName, useBinary );
}

std::wstring GetSpritePath( SpriteAttribute sprAttribute )
{
	switch ( sprAttribute )
	{
	case SpriteAttribute::FMODLogoBlack:
		return L"./Data/Images/Rights/FMOD Logo Black - White Background.png";	// break;
	case SpriteAttribute::FMODLogoWhite:
		return L"./Data/Images/Rights/FMOD Logo White - Black Background.png";	// break;

	default:
		assert( !"Error : Specified unexpect sprite type." ); break;
	}

	return L"ERROR_ATTRIBUTE";
}
