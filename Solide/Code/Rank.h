#pragma once

#include <string>
#include <vector>

#include "Donya/Vector.h"

#include "Timer.h"
#include "UI.h"

class Rank
{
public:
	static constexpr int rankCount = 4;
	/// <summary>
	/// I regard as the "borderTimes" is descending order([0] is highest rank border).
	/// </summary>
	static int Calculate( const Timer &current, const std::vector<Timer> &borderTimes );
private:
	UIObject	sprite;
	Donya::Int2	partSize;	// Whole size.
public:
	bool Init( const std::wstring &rankSpritePath );
	void Draw( int rank, const Donya::Vector2 &ssPos, float scale, const Donya::Vector2 &posOrigin = { 0.5f, 0.5f }, float drawDepth = 0.0f );
};
