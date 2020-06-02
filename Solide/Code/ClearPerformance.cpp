#include "ClearPerformance.h"


#pragma region States

// region States
#pragma endregion

void ClearPerformance::Init()
{
	AssignProcess<ShowFrame>( &processPtrs[scast<int>( Type::ShowFrame			)] );
	AssignProcess<ShowDesc>	( &processPtrs[scast<int>( Type::ShowDescription	)] );
	AssignProcess<ShowTime>	( &processPtrs[scast<int>( Type::ShowTime			)] );
	AssignProcess<ShowRank>	( &processPtrs[scast<int>( Type::ShowRank			)] );
	AssignProcess<Wait>		( &processPtrs[scast<int>( Type::Wait				)] );

	Timer zero{}; zero.Set( 0, 0, 0 );
	ResetProcess( zero );
}
void ClearPerformance::ResetProcess( const Timer &currentTime )
{
	nowType		= scast<Type>( 0 );
	timer		= 0;
	clearTime	= currentTime;
	isFinished	= false;
}
void ClearPerformance::Uninit()
{
	for ( auto &pIt : processPtrs )
	{
		if ( pIt ) { pIt->Uninit( *this ); }
	}
}
void ClearPerformance::Update()
{
	for ( auto &pIt : processPtrs )
	{
		if ( pIt ) { pIt->Update( *this ); }
	}
}
void ClearPerformance::Draw()
{
	for ( auto &pIt : processPtrs )
	{
		if ( pIt ) { pIt->Draw( *this ); }
	}
}
