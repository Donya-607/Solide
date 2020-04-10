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

enum class SpriteAttribute
{
	FMODLogoBlack,
	FMODLogoWhite,
};

std::wstring GetSpritePath( SpriteAttribute spriteAttribute );
