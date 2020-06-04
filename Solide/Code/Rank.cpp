#include "Rank.h"

#include <algorithm>		// Use std::min

#include "Donya/Constant.h" // Use scast macro

int Rank::Calculate( const Timer &current, const std::vector<Timer> &borderTimes )
{
	const int limit = scast<int>( borderTimes.size() );

	int rank = 0;
	for ( ; rank < limit; ++rank )
	{
		if ( current < borderTimes[rank] )
		{
			break;
		}
	}
	return rank;
}

bool Rank::Init( const std::wstring &sprPath )
{
	const bool succeeded = sprite.LoadSprite( sprPath, 128U );

	partSize		=  sprite.GetSpriteSize();
	partSize.x		/= rankCount;
	sprite.texSize	=  partSize.Float();

	return succeeded;
}
void Rank::Draw( int rank, const Donya::Vector2 &ssPos, float scale, const Donya::Vector2 &posOrigin, float drawDepth )
{
	rank = std::max( 0, std::min( rankCount - 1, rank ) );

	sprite.pos			= ssPos;
	sprite.drawScale	= scale;
	sprite.texPos.x		= partSize.x * scast<float>( rank );
	sprite.texPos.y		= 0.0f;

	sprite.DrawPart( drawDepth );
}
