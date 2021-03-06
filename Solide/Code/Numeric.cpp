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
void NumberDrawer::DrawNumber( int number, const Donya::Vector2 &ssPos, float scale, float alpha, const Donya::Vector2 &posOrigin, float drawDepth, int drawDigit )
{
	sprite.pos		= ssPos;
	sprite.texPos.y	= 0.0f;

	auto separated = Donya::SeparateDigits( number, drawDigit );	// Store '123' to [0:3][1:2][2:1], '1' to [0:1][1:0]
	std::reverse( separated.begin(), separated.end() );				// [0:3][1:2][2:1] to [0:1][1:2][2:3], [0:1][1:0] to [0:0][1:1]

	const size_t count = separated.size();
	for ( size_t i = 0; i < count; ++i )
	{
		const int v = separated[i];
		DrawImpl( v, ssPos, scale, alpha, posOrigin, drawDepth );

		sprite.pos.x += partSize.Float().x * scale;
	}
}
void NumberDrawer::DrawNumbers( std::vector<int> numbers, Delimiter delim, const Donya::Vector2 &ssPos, float scale, float alpha, const Donya::Vector2 &posOrigin, float drawDepth, int drawDigit )
{
	if ( delim < Empty || DelimiterCount <= delim )
	{
		delim  = Empty;
	}

	Donya::Vector2 basePos = ssPos;

	const size_t count = numbers.size();
	for ( size_t i = 0; i < count; ++i )
	{
		DrawNumber( numbers[i], basePos, scale, alpha, posOrigin, drawDepth, drawDigit );
		basePos.x =  sprite.pos.x; // "sprite.pos.x" will be increased in DrawNumber().

		if ( delim != Empty && i < count - 1 )
		{
			DrawImpl( 10 + delim, basePos, scale, alpha, posOrigin, drawDepth );
		}

		basePos.x += partSize.Float().x * scale;
	}
}
void NumberDrawer::DrawTime( const Timer &time, const Donya::Vector2 &ssPos, float scale, float alpha, const Donya::Vector2 &posOrigin, float drawDepth )
{
	std::vector<int> times{}; // [0:Min][1:Sec][2:MS]
	times.emplace_back( time.Minute()  );
	times.emplace_back( time.Second()  );
	times.emplace_back( time.Current() );

	DrawNumbers( times, NumberDrawer::Colon, ssPos, scale, alpha, posOrigin, drawDepth );
}
void NumberDrawer::DrawImpl( int texOffsetX, const Donya::Vector2 &ssPos, float scale, float alpha, const Donya::Vector2 &posOrigin, float drawDepth )
{
	sprite.texPos.x		= CalcSizeOffsetX( texOffsetX, 1.0f ); // Don't scale the texture size.
	sprite.drawScale	= scale;
	sprite.alpha		= alpha;
	sprite.DrawPart( drawDepth );
}
float NumberDrawer::CalcSizeOffsetX( int index, float scale )
{
	return scast<float>( partSize.x * index ) * scale;
}
