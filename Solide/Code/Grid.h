#pragma once

#include <string>
#include <memory>

#include "Donya/Vector.h"
#include "Donya/UseImGui.h"

namespace Donya
{
	namespace Geometric
	{
		class Line;
	}
}

class GridLine
{
private:
	static constexpr unsigned int MAX_LINE_COUNT = 512U;
private:
	float					height = 0.0f;
	Donya::Vector2			lineLength;		// Half length. 'Y' is used to 'Z'.
	Donya::Vector2			drawInterval;	// 'Y' is used to 'Z'.
	std::unique_ptr<Donya::Geometric::Line>	pLine;
public:
	~GridLine();
public:
	bool Init();
	void Uninit();

	void Draw( const Donya::Vector4x4 &VP ) const;
public:
	void SetDrawHeight( float coordY );
	void SetDrawLength( const Donya::Vector2 halfDrawLength );
	void SetDrawInterval( const Donya::Vector2 drawInterval );

	float GetDrawHeight() const;
	Donya::Vector2 GetDrawLength() const;
	Donya::Vector2 GetDrawInterval() const;
private:
	Donya::Int2 CalcDrawCount() const;
#if USE_IMGUI
public:
	void ShowImGuiNode( const std::string &nodeCaption );
#endif // USE_IMGUI
};
