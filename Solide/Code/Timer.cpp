#include "Timer.h"

#include <algorithm>		// Use std::min
#include <vector>

#include "Donya/Useful.h"	// Use SeparateDigits().
#include "Donya/Template.h"	// Use Clamp().

#undef max
#undef min

void Timer::Update()
{
	current++;

	if ( 60 <= current )
	{
		current = 0;
		second++;
		if ( 60 <= second )
		{
			second = 0;
			minute++;
			minute = std::min( 99, minute );
		}
	}
}
void Timer::Set( int newMinute, int newSecond, int newCurrent )
{
	if ( 0 <= newMinute  ) { minute  = newMinute;  }
	if ( 0 <= newSecond  ) { second  = newSecond;  }
	if ( 0 <= newCurrent ) { current = newCurrent; }
}
std::string Timer::ToStr( bool isInsertColon )
{
	constexpr int		DIGIT = 2;
	constexpr size_t	SIZE  = 3;
	const std::vector<int> SEPARATED_TIMES // [0:current][1:second][2:minute]
	{
		Current(),
		Second(),
		Minute()
	};

	std::vector<int> separatedNumbers{};

	for ( size_t i = 0; i < SIZE; ++i )
	{
		auto result = Donya::SeparateDigits( SEPARATED_TIMES[i], DIGIT );
		for ( const auto &it : result )
		{
			separatedNumbers.emplace_back( it );
		}
	}

	std::string reverseNumberStr{}; // Store "123456" to "65:43:21".

	const size_t numberCount = separatedNumbers.size();
	for ( size_t i = 0; i < numberCount; ++i )
	{
		reverseNumberStr += std::to_string( separatedNumbers[i] );

		bool canInsert = ( isInsertColon && i + 1 < numberCount );
		if ( i % DIGIT == 1 && canInsert )
		{
			reverseNumberStr += ":";
		}
	}

	std::reverse( reverseNumberStr.begin(), reverseNumberStr.end() );
	return reverseNumberStr;
}
#if USE_IMGUI
void Timer::ShowImGuiNode( const std::string &nodeCaption, bool useTreeNode )
{
	if ( useTreeNode && !ImGui::TreeNode( nodeCaption.c_str() ) ) { return; }
	// else

	ImGui::DragInt( u8"Min:•ª",		&minute  );
	ImGui::DragInt( u8"Sec:•b",		&second  );
	ImGui::DragInt( u8"MSec:ƒ~ƒŠ•b",	&current );
	Donya::Clamp( &minute,  0, 59 );
	Donya::Clamp( &second,  0, 99 );
	Donya::Clamp( &current, 0, 99 );

	if ( useTreeNode ) { ImGui::TreePop(); }
}
#endif // USE_IMGUI

bool operator < ( const Timer &L, const Timer &R )
{
	if ( L.Minute()  < R.Minute()  ) { return true;  }
	if ( L.Minute()  > R.Minute()  ) { return false; }
	// else
	if ( L.Second()  < R.Second()  ) { return true;  }
	if ( L.Second()  > R.Second()  ) { return false; }
	// else
	if ( L.Current() < R.Current() ) { return true;  }
	// else
	return false;
}
