#include "Common.h"

#include "Donya/Constant.h"	// Use scast, DEBUG_MODE macros.

namespace Common
{
	constexpr long	SCREEN_WIDTH_L	= 1920L;
	constexpr int	SCREEN_WIDTH_I	= scast<int>	( SCREEN_WIDTH_L	);
	constexpr float	SCREEN_WIDTH_F	= scast<float>	( SCREEN_WIDTH_L	);

	constexpr long	SCREEN_HEIGHT_L	= 1080L;
	constexpr int	SCREEN_HEIGHT_I	= scast<int>	( SCREEN_HEIGHT_L	);
	constexpr float	SCREEN_HEIGHT_F	= scast<float>	( SCREEN_HEIGHT_L	);

	int		ScreenWidth()			{ return SCREEN_WIDTH_I;			}
	float	ScreenWidthF()			{ return SCREEN_WIDTH_F;			}
	long	ScreenWidthL()			{ return SCREEN_WIDTH_L;			}

	int		ScreenHeight()			{ return SCREEN_HEIGHT_I;			}
	float	ScreenHeightF()			{ return SCREEN_HEIGHT_F;			}
	long	ScreenHeightL()			{ return SCREEN_HEIGHT_L;			}

	int		HalfScreenWidth()		{ return SCREEN_WIDTH_I >> 1;		}
	float	HalfScreenWidthF()		{ return SCREEN_WIDTH_F * 0.5f;		}
	long	HalfScreenWidthL()		{ return SCREEN_WIDTH_L >> 1;		}

	int		HalfScreenHeight()		{ return SCREEN_HEIGHT_I >> 1;		}
	float	HalfScreenHeightF()		{ return SCREEN_HEIGHT_F * 0.5f;	}
	long	HalfScreenHeightL()		{ return SCREEN_HEIGHT_L >> 1;		}

#if DEBUG_MODE
	static bool showCollision = true;
#endif // DEBUG_MODE
	void	SetShowCollision( bool newState )
	{
	#if DEBUG_MODE
		showCollision = newState;
	#endif // DEBUG_MODE
	}
	void	ToggleShowCollision()
	{
	#if DEBUG_MODE
		showCollision = !showCollision;
	#endif // DEBUG_MODE
	}
	bool	IsShowCollision()
	{
	#if DEBUG_MODE
		return showCollision;
	#else
		return false;
	#endif // DEBUG_MODE
	}
}
