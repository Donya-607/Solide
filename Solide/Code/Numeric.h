#pragma once

#include <string>
#include <vector>

#include "Donya/Vector.h"

#include "Timer.h"
#include "UI.h"

/// <summary>
/// The numbers will be expected as align as horizontal in sprite.
/// </summary>
class NumberDrawer
{
public:
	// Use as index of texture part.
	enum Delimiter
	{
		Empty  = -1,
		Hyphen = 0,
		Colon  = 1,

		DelimiterCount
	};
private:
	UIObject	sprite;
	Donya::Int2	partSize;	// Whole size.
public:
	bool Init( const std::wstring &numberSpritePath );
	void DrawNumber( int number, const Donya::Vector2 &ssPos, float scale, float alpha = 1.0f, const Donya::Vector2 &posOrigin = { 0.5f, 0.5f }, float drawDepth = 0.0f );
	void DrawNumbers( std::vector<int> numbers, Delimiter delimiterIndex, const Donya::Vector2 &ssPos, float scale, float alpha = 1.0f, const Donya::Vector2 &posOrigin = { 0.5f, 0.5f }, float drawDepth = 0.0f );
	void DrawTime( const Timer &time, const Donya::Vector2 &ssPos, float scale, float alpha = 1.0f, const Donya::Vector2 &posOrigin = { 0.5f, 0.5f }, float drawDepth = 0.0f );
private:
	void DrawImpl( int texOffsetX, const Donya::Vector2 &ssPos, float scale, float alpha = 1.0f, const Donya::Vector2 &posOrigin = { 0.5f, 0.5f }, float drawDepth = 0.0f );
	float CalcSizeOffsetX( int index, float scale );
};
