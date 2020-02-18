#include "FilePath.h"

#include "Donya/Constant.h"	// Use DEBUG_MODE.
#include "Donya/Useful.h"	// Use IsExistFile().

std::string GenerateSerializePath( std::string identifier, bool useBinary )
{
	const std::string EXT = ( useBinary ) ? ".bin" : ".json";

	return "./Data/Parameters/" + identifier + EXT;
}

std::wstring GetSpritePath( SpriteAttribute sprAttribute )
{
	switch ( sprAttribute )
	{
	case SpriteAttribute::FMODLogoBlack:
		return L"./Data/Images/Rights/FMOD Logo Black - White Background.png";	// break;
	case SpriteAttribute::FMODLogoWhite:
		return L"./Data/Images/Rights/FMOD Logo White - Black Background.png";	// break;
	case SpriteAttribute::Player:
		return L"./Data/Images/Player/Player.png";	// break;
	case SpriteAttribute::PlayerEffects:
		return L"./Data/Images/Player/Effect.png";	// break;
	case SpriteAttribute::MapChip:
		return L"./Data/Images/BG/MapChip.png";		// break;
	case SpriteAttribute::Enemy:
		return L"./Data/Images/Enemy/Enemy.png";	// break;
	case SpriteAttribute::Object:
		return L"./Data/Images/Object/Object.png";	// break;
	case SpriteAttribute::TitleLogo:
		return L"./Data/Images/Title/Logo.png";		// break;
	case SpriteAttribute::SelectableStage:
		return L"./Data/Images/Title/Stages.png";	// break;
	case SpriteAttribute::Usage:
		return L"./Data/Images/UI/usage.png";		// break;
	case SpriteAttribute::BG_Title:
		return L"./Data/Images/BG/Title.png";		// break;
	case SpriteAttribute::BG_1:
		return L"./Data/Images/BG/BG_1.png";		// break;
	case SpriteAttribute::BG_2:
		return L"./Data/Images/BG/BG_2.png";		// break;
	case SpriteAttribute::BG_3:
		return L"./Data/Images/BG/BG_3.png";		// break;
	case SpriteAttribute::UI:
		return L"./Data/Images/UI/Statements.png";	// break;
	case SpriteAttribute::ScoreBoard:
		return L"./Data/Images/UI/ScoreBoard.png";	// break;
	
	case SpriteAttribute::TestFont:
		return L"./Data/Images/Font/TestFont.png";	// break;

	case SpriteAttribute::SP_BG:
		return L"./Data/ForShaderProgram/Images/BG.png";	// break;
	case SpriteAttribute::SP_FireA:
		return L"./Data/ForShaderProgram/Images/FireA.png";	// break;
	case SpriteAttribute::SP_FireB:
		return L"./Data/ForShaderProgram/Images/FireB.png";	// break;

	default:
		assert( !"Error : Specified unexpect sprite type." ); break;
	}

	return L"ERROR_ATTRIBUTE";
}

std::string GetModelPath( ModelAttribute modelAttribute )
{
	switch ( modelAttribute )
	{
	case ModelAttribute::ChiPlayer:
		return "./Data/Models/Player/Player.bin"; // break;
		break;
	default:
		assert( !"Error : Specified unexpect model type." ); break;
		break;
	}

	return "ERROR_ATTRIBUTE";
}
