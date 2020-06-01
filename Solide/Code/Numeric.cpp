#include "Numeric.h"

#include "Donya/Useful.h"	// Use SeparateDigits().

bool NumberDrawer::Init( const std::wstring &sprPath )
{
	const bool succeeded = sprite.LoadSprite( sprPath, 1024U );

	const int partCount = 10 + DelimiterCount;
	_ASSERT_EXPR( 0 < partCount, L"Unexpected Error!" );

	partSize		=  sprite.GetSpriteSize();
	partSize.x		/= partCount;
	sprite.texSize	=  partSize.Float();

	return succeeded;
}
void NumberDrawer::DrawNumber( int number, const Donya::Vector2 &ssPos, const Donya::Vector2 &posOrigin, float drawDepth )
{
	sprite.pos		= ssPos;
	sprite.texPos.y	= 0.0f;

	auto separated = Donya::SeparateDigits( number );	// Store '123' to [0:3][1:2][2:1]
	std::reverse( separated.begin(), separated.end() );	// [0:3][1:2][2:1] to [0:1][1:2][2:3]

	const size_t count = separated.size();
	for ( size_t i = 0; i < count; ++i )
	{
		const int v = separated[i];
		DrawImpl( v, ssPos, posOrigin, drawDepth );

		sprite.pos.x += partSize.Float().x;
	}
}
void NumberDrawer::DrawNumbers( std::vector<int> numbers, Delimiter delim, const Donya::Vector2 &ssPos, const Donya::Vector2 &posOrigin, float drawDepth )
{
	if ( delim < Empty || DelimiterCount <= delim )
	{
		delim  = Empty;
	}

	Donya::Vector2 basePos = ssPos;

	const size_t count = numbers.size();
	for ( size_t i = 0; i < count; ++i )
	{
		DrawNumber( numbers[i], basePos, posOrigin, drawDepth );
		basePos.x =  sprite.pos.x; // "sprite.pos.x" will be increased in DrawNumber().

		if ( delim != Empty )
		{
			DrawImpl( 10 + delim, basePos, posOrigin, drawDepth );
		}

		basePos.x += partSize.Float().x;
	}
}
void NumberDrawer::DrawImpl( int texOffsetX, const Donya::Vector2 &ssPos, const Donya::Vector2 &posOrigin, float drawDepth )
{
	sprite.texPos.x	= CalcSizeOffsetX( texOffsetX );
	sprite.DrawPart( drawDepth );
}
float NumberDrawer::CalcSizeOffsetX( int index )
{
	return scast<float>( partSize.x * index );
}
