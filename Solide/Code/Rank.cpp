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
	partSize.y		/= rankCount;
	sprite.texSize	=  partSize.Float();

	return succeeded;
}
void Rank::Draw( int rank, const Donya::Vector2 &ssPos, float scale, float degree, float alpha, const Donya::Vector2 &posOrigin, float drawDepth )
{
	rank = std::max( 0, std::min( rankCount - 1, rank ) );

	sprite.pos			= ssPos;
	sprite.drawScale	= scale;
	sprite.degree		= degree;
	sprite.alpha		= alpha;
	sprite.texPos.x		= 0.0f;
	sprite.texPos.y		= partSize.x * scast<float>( rank );

	sprite.DrawPart( drawDepth );
}
