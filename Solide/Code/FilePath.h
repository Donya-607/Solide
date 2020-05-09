#pragma once

#include <string>

#include "Donya/Serializer.h"

/// <summary>
/// If set false to "useBinaryExtension", returns JSON extension.
/// </summary>
std::string GenerateSerializePath( std::string identifier, bool useBinaryExtension );
/// <summary>
/// If set false to "useBinaryExtension", returns JSON extension.
/// </summary>
std::string MakeStageParamPath( std::string objectName, int stageNumber, bool useBinaryExtension );
std::string MakeTerrainModelPath( std::string objectName, int stageNumber );
std::string MakeSaveDataPath( std::string fileName, bool useBinaryExtension );

/// <summary>
/// Returns true if the directory was created.
/// </summary>
bool MakeDirectoryIfNotExists( const std::string &relativeFilePath );
/// <summary>
/// Creates file is empty.<para></para>
/// Returns true if the file was created.
/// </summary>
bool MakeFileIfNotExists( const std::string &relativeFilePath, bool binaryMode );

enum class SpriteAttribute
{
	FMODLogoBlack,
	FMODLogoWhite,

	BackGround,
	CircleShadow,
	ClearSentence,
	Cloud,
	LoadingIcon,
	LoadingSentence,
	TutorialSentence,
	TitleLogo,
	TitlePrompt,
};
std::wstring GetSpritePath( SpriteAttribute spriteAttribute );
