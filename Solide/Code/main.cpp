#include <locale.h>
#include <time.h>
#include <windows.h>

#include "Donya/Constant.h"
#include "Donya/Donya.h"
#include "Donya/Sound.h"

#include "Common.h"
#include "EffectAdmin.h"
#include "Framework.h"
#include "Icon.h"

void ClearBackGround();

INT WINAPI wWinMain( _In_ HINSTANCE instance, _In_opt_ HINSTANCE prevInstance, _In_ LPWSTR cmdLine, _In_ INT cmdShow )
{
#if defined( DEBUG ) | defined( _DEBUG )
	// reference:https://docs.microsoft.com/ja-jp/visualstudio/debugger/crt-debug-heap-details?view=vs-2015
	_CrtSetDbgFlag
	(
		_CRTDBG_ALLOC_MEM_DF
		| _CRTDBG_LEAK_CHECK_DF
		// | _CRTDBG_CHECK_ALWAYS_DF
	);
	// When memory leak detected, if you assign the output number to "_crtBreakAlloc",
	// program will be stop in that memory allocate place. ex : _crtBreakAlloc = 123;
	// _crtBreakAlloc = ;
#endif

	setlocale( LC_ALL, "JPN" );

	srand( scast<unsigned int>( time( NULL ) ) );

	Donya::LibraryInitializer desc{};
	desc.screenWidth			= Common::ScreenWidth();
	desc.screenHeight			= Common::ScreenHeight();
	desc.windowCaption			= "オイリー！";
	desc.enableCaptionBar		= true;
	desc.enableMultiThreaded	= true;
	desc.fullScreenMode			= false;
	Donya::Init( cmdShow, desc );

	Donya::SetWindowIcon( instance, IDI_ICON );
	ClearBackGround();

	const bool effectResult = EffectAdmin::Get().Init( Donya::GetDevice(), Donya::GetImmediateContext() );
	if ( !effectResult ) { return Donya::Uninit(); }
	// else

	Framework framework{};
	framework.Init();

	while ( Donya::MessageLoop() )
	{
		ClearBackGround();

		Donya::SystemUpdate();
		framework.Update( Donya::GetElapsedTime() );

		framework.Draw( Donya::GetElapsedTime() );
		Donya::Present( 1 );
	}

	framework.Uninit();

	EffectAdmin::Get().Uninit();

	auto   returnValue = Donya::Uninit();
	return returnValue;
}

void ClearBackGround()
{
	// 1.0f is too dazzling.
	constexpr FLOAT color[4]{ 0.7f, 0.7f, 0.7f, 1.0f };
	Donya::ClearViews( color );
}
