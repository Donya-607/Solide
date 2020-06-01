#pragma once

#include <string>

#include "Donya/Serializer.h"

/// <summary>
/// Measure elapsed time. this class can count until 99min-59sec-59ms with frame.
/// </summary>
class Timer
{
private:
	int current	= 59;	// 0 ~ 59.
	int second	= 59;	// 0 ~ 59.
	int minute	= 99;	// 0 ~ 99.
private:
	friend class cereal::access;
	template<class Archive>
	void serialize( Archive &archive, std::uint32_t version )
	{
		archive
		(
			CEREAL_NVP( current ),
			CEREAL_NVP( second  ),
			CEREAL_NVP( minute  )
		);

		if ( 1 <= version )
		{
			// archive( CEREAL_NVP() );
		}
	}
public:
	void Update();
public:
	/// <summary>
	/// If setting -1, that parameter is not change.
	/// </summary>
	void Set( int newMinute = -1, int newSecond = -1, int newCurrent = -1 );
public:
	int Current() const { return current; }
	int Second()  const { return second;  }
	int Minute()  const { return minute;  }
	/// <summary>
	/// Returns string is "XX:XX:XX", min:sec:ms.
	/// </summary>
	std::string ToStr( bool isInsertColon = true );
};
CEREAL_CLASS_VERSION( Timer, 0 )

bool operator < ( const Timer &L, const Timer &R );
static bool operator >  ( const Timer &L, const Timer &R ) { return R < L; }
static bool operator <= ( const Timer &L, const Timer &R ) { return !( R < L ); }
static bool operator >= ( const Timer &L, const Timer &R ) { return !( L < R ); }

static bool operator == ( const Timer &L, const Timer &R ) { return !( L < R ) && !( R > L ); }
static bool operator != ( const Timer &L, const Timer &R ) { return !( L == R ); }
