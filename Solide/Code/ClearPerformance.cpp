#include "ClearPerformance.h"


#pragma region States
void ClearPerformance::ProcessBase::Init( ClearPerformance &inst )
{
	timer	= 0;
	factor	= 0.0f;
}
void ClearPerformance::ProcessBase::Uninit( ClearPerformance &inst )
{
	timer	= 0;
	factor	= 0.0f;
}

ClearPerformance::Result ClearPerformance::ShowFrame::Update( ClearPerformance &inst )
{
	return Result::Finish;
}
void ClearPerformance::ShowFrame::Draw( ClearPerformance &inst )
{

}

ClearPerformance::Result ClearPerformance::ShowDesc::Update( ClearPerformance &inst )
{
	return Result::Finish;
}
void ClearPerformance::ShowDesc::Draw( ClearPerformance &inst )
{

}

ClearPerformance::Result ClearPerformance::ShowTime::Update( ClearPerformance &inst )
{
	return Result::Finish;
}
void ClearPerformance::ShowTime::Draw( ClearPerformance &inst )
{

}

ClearPerformance::Result ClearPerformance::ShowRank::Update( ClearPerformance &inst )
{
	return Result::Finish;
}
void ClearPerformance::ShowRank::Draw( ClearPerformance &inst )
{

}

ClearPerformance::Result ClearPerformance::Wait::Update( ClearPerformance &inst )
{
	return Result::Finish;
}
void ClearPerformance::Wait::Draw( ClearPerformance &inst )
{

}

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
	const size_t index = std::min( scast<size_t>( nowType ), processPtrs.size() );
	if ( processPtrs[index] )
	{
		Result result = processPtrs[index]->Update( *this );
		if ( result == Result::Finish )
		{
			const size_t limit = scast<size_t>( Type::TypeCount );
			nowType = scast<Type>( std::min( index + 1, limit - 1 ) );
		}
	}
}
void ClearPerformance::Draw()
{
	const size_t drawLimit = std::min( scast<size_t>( nowType ), processPtrs.size() );
	for ( size_t i = 0; i < drawLimit; ++i )
	{
		if ( processPtrs[i] )
		{
			processPtrs[i]->Draw( *this );
		}
	}
}
