#include "FilePath.h"

#include "Effekseer.h"		// Use EFK_CHAR.

#include "Donya/Constant.h"	// Use DEBUG_MODE.
#include "Donya/Useful.h"	// Use IsExistFile().

namespace
{
	static constexpr const char		*PARAMETERS_DIRECTORY	= "./Data/Parameters/";	// Relative path.
	static constexpr const wchar_t	*SPRITE_DIRECTORY		= L"./Data/Images/";	// Relative path.
	static constexpr const EFK_CHAR	*EFFECT_DIRECTORY		= u"./Data/Effects/";	// Relative path.
	static constexpr const char		*MODELS_DIRECTORY		= "./Data/Models/";		// Relative path.
	static constexpr const char		*MODEL_EXTENSION		= ".bin";
	static constexpr const char		*SAVE_DATA_DIRECTORY	= "./Data/Save/";		// Relative path.
}

std::string GenerateSerializePath( std::string identifier, bool useBinary )
{
	const std::string EXT = ( useBinary ) ? ".bin" : ".json";
	return PARAMETERS_DIRECTORY + identifier + EXT;
}
std::string MakeStageParamPath( std::string objName, int stageNo, bool useBinary )
{
	const std::string folder = "Stage" + Donya::MakeArraySuffix( stageNo ) + "/";
	return GenerateSerializePath( folder + objName, useBinary );
}
std::string MakeTerrainModelPath( std::string objName, int stageNo )
{
	const std::string folder = "Terrain/Stage" + Donya::MakeArraySuffix( stageNo ) + "/";
	return MODELS_DIRECTORY + folder + objName + MODEL_EXTENSION;
}
std::string MakeSaveDataPath( std::string fileName, bool useBinary )
{
	const std::string EXT = ( useBinary ) ? ".bin" : ".json";
	return SAVE_DATA_DIRECTORY + fileName + EXT;
}

bool MakeDirectoryIfNotExists( const std::string &filePath )
{
	const std::string fullPath		= Donya::ToFullPath( filePath );
	const std::string fileDirectory	= Donya::ExtractFileDirectoryFromFullPath( fullPath );
	const int result = Donya::MakeDirectory( fileDirectory );
	return  ( result == 0 ) ? true : false;
}
bool MakeFileIfNotExists( const std::string &filePath, bool binaryMode )
{
	if ( Donya::IsExistFile( filePath ) ) { return false; }
	// else

	bool wasCreated = MakeDirectoryIfNotExists( filePath );
	if ( wasCreated || !Donya::IsExistFile( filePath ) )
	{
		const auto openMode = ( binaryMode ) ? std::ios::out | std::ios::binary : std::ios::out;
		std::ofstream ofs{ filePath, openMode };
		// The file is created in here.
		ofs.close();

		wasCreated = true;
	}
	return wasCreated;
}

std::wstring GetSpritePath( SpriteAttribute attr )
{
	auto ToRelPath = []( const std::wstring &fileName )
	{
		return SPRITE_DIRECTORY + fileName;
	};
	
	switch ( attr )
	{
	case SpriteAttribute::FMODLogoBlack:
		return ToRelPath( L"Rights/FMOD Logo Black - White Background.png" );
	case SpriteAttribute::FMODLogoWhite:
		return ToRelPath( L"Rights/FMOD Logo White - Black Background.png" );
	case SpriteAttribute::BackGround:
		return ToRelPath( L"BG/Back.png" );
	case SpriteAttribute::CircleShadow:
		return ToRelPath( L"Shadow/Circle.png" );
	case SpriteAttribute::ClearDescription:
		return ToRelPath( L"Clear/Description.png" );
	case SpriteAttribute::ClearFrame:
		return ToRelPath( L"Clear/Frame.png" );
	case SpriteAttribute::ClearRank:
		return ToRelPath( L"Clear/Rank.png" );
	case SpriteAttribute::ClearSentence:
		return ToRelPath( L"Clear/Clear.png" );
	case SpriteAttribute::Cloud:
		return ToRelPath( L"BG/Cloud.png" );
	case SpriteAttribute::LoadingIcon:
		return ToRelPath( L"Loading/Icon.png" );
	case SpriteAttribute::LoadingSentence:
		return ToRelPath( L"Loading/NowLoading.png" );
	case SpriteAttribute::Number:
		return ToRelPath( L"UI/Number.png" );
	case SpriteAttribute::Pause:
		return ToRelPath( L"Pause/Items.png" );
	case SpriteAttribute::TutorialFrame:
		return ToRelPath( L"Tutorial/Frame.png" );
	case SpriteAttribute::TutorialSentence:
		return ToRelPath( L"Tutorial/Sentences.png" );
	case SpriteAttribute::TitleItems:
		return ToRelPath( L"Title/Items.png" );
	case SpriteAttribute::TitleLogo:
		return ToRelPath( L"Title/Logo.png" );
	case SpriteAttribute::TitlePrompt:
		return ToRelPath( L"Title/Prompt.png" );
	default: break;
	}

	_ASSERT_EXPR( 0, L"Error: Specified unexpect sprite type." );
	return L"ERROR_ATTRIBUTE";
}


#include "EffectUtil.h"
std::basic_string<EFK_CHAR> GetEffectPath( EffectAttribute attr )
{
	auto ToRelPath = []( const std::basic_string<EFK_CHAR> &fileName )
	{
		return EFFECT_DIRECTORY + fileName;
	};

	switch ( attr )
	{
	case EffectAttribute::Fire:
		return ToRelPath( u"Fire/Fire.efkefc" );
	case EffectAttribute::FlameCannon:
		return ToRelPath( u"Flame/Flame.efkefc" );
	case EffectAttribute::IceCannon:
		return ToRelPath( u"Ice/Ice.efkefc" );
	case EffectAttribute::ColdSmoke:
		return ToRelPath( u"Smoke/Cold.efkefc" );
	default: break;
	}

	_ASSERT_EXPR( 0, L"Error : Unexpect effect type." );
	return u"ERROR_ATTRIBUTE";
}
