#include "Grid.h"

#include "Donya/GeometricPrimitive.h"
#include "Donya/Useful.h"
#include "Donya/UseImGui.h"

namespace
{
	static constexpr int MAX_INSTANCE_COUNT = 4096;
}

GridLine::~GridLine() = default;

bool GridLine::Init()
{
	pLine = std::make_unique<Donya::Geometric::Line>( MAX_INSTANCE_COUNT );
	if ( !pLine->Init() )
	{
		_ASSERT_EXPR( 0, L"Failed: The line initialization was failed." );
		return false;
	}
	// else

	// Default settings.
	lineLength		= 10.0f;
	drawInterval	= 1.0f;

	return true;
}
void GridLine::Uninit()
{
	if ( pLine ) { pLine->Uninit(); }
	pLine.reset();
}

void GridLine::Draw( const Donya::Vector4x4 &matVP ) const
{
	if ( !pLine ) { return; }
	// else

	const Donya::Vector3 origin	{ -lineLength.x,		0.0f, -lineLength.y			};
	const Donya::Vector3 endX	{ lineLength.x * 2.0f,	0.0f, 0.0f					};
	const Donya::Vector3 endZ	{ 0.0f,					0.0f, lineLength.y * 2.0f	};
	const Donya::Vector3 offsetX{ drawInterval.x,		0.0f, 0.0f					};
	const Donya::Vector3 offsetZ{ 0.0f,					0.0f, drawInterval.y		};
	
	const Donya::Int2 loopCount = CalcDrawCount();

	for ( int z = 0; z < loopCount.y; ++z )
	{
		pLine->Reserve
		(
			origin +		( offsetZ * scast<float>( z ) ),
			origin + endX +	( offsetZ * scast<float>( z ) )
		);
	}
	for ( int x = 0; x < loopCount.x; ++x )
	{
		pLine->Reserve
		(
			origin +		( offsetX * scast<float>( x ) ),
			origin + endZ +	( offsetX * scast<float>( x ) )
		);
	}
	
	pLine->Flush( matVP );
}

void GridLine::SetDrawHeight( float Y )							{ height = Y; }
void GridLine::SetDrawLength( const Donya::Vector2 halfLength )	{ lineLength = halfLength; }
void GridLine::SetDrawInterval( const Donya::Vector2 interval )	{ drawInterval = interval; }

float GridLine::GetDrawHeight() const				{ return height; }
Donya::Vector2 GridLine::GetDrawLength() const		{ return lineLength; }
Donya::Vector2 GridLine::GetDrawInterval() const	{ return drawInterval; }

Donya::Int2 GridLine::CalcDrawCount() const
{
	const int xCount = ( ZeroEqual( drawInterval.x ) ) ? 0 : scast<int>( ( lineLength.x * 2.0f ) / drawInterval.x );
	const int yCount = ( ZeroEqual( drawInterval.y ) ) ? 0 : scast<int>( ( lineLength.y * 2.0f ) / drawInterval.y );
	return Donya::Int2
	{
		xCount + 1/* Edge of line*/,
		yCount + 1/* Edge of line*/
	};
}

#if USE_IMGUI
void GridLine::ShowImGuiNode( const std::string &nodeCaption )
{
	if ( !ImGui::TreeNode( nodeCaption.c_str() ) ) { return; }
	// else

	ImGui::DragFloat( u8"Y座標",		&height,			0.05f );
	ImGui::DragFloat2( u8"半径",		&lineLength.x,		0.05f );
	ImGui::DragFloat2( u8"間隔",		&drawInterval.x,	0.05f );

	const Donya::Int2 drawingCount = CalcDrawCount();
	ImGui::Text( u8"本数：[Ｘ：%d][Ｚ：%d][計：%d]", drawingCount.x - 1, drawingCount.y - 1, drawingCount.x + drawingCount.y - 2 );
	ImGui::Text( u8"最大本数：[%d]", MAX_LINE_COUNT );

	ImGui::TreePop();
}
#endif // USE_IMGUI
