#pragma once

#include <array>
#include <memory>

#include "Donya/Constant.h"	// Use scast macro

#include "Timer.h"

class ClearPerformance
{
private:
	enum class Type : int
	{
		ShowFrame = 0,
		ShowDescription,
		ShowTime,
		ShowRank,
		Wait,

		TypeCount
	};
	enum class Result
	{
		Continue,
		Finish
	};
private:
#pragma region States
	class ProcessBase
	{
	protected:
		int		timer	= 0;
		float	factor	= 0.0f; // 0.0f ~ 1.0f
	public:
		virtual void	Init( ClearPerformance &instance );
		virtual void	Uninit( ClearPerformance &instance );
		virtual Result	Update( ClearPerformance &instance ) = 0;
		virtual void	Draw( ClearPerformance &instance ) = 0;
	};
	class ShowFrame : public ProcessBase
	{
	public:
		Result	Update( ClearPerformance &instance ) override;
		void	Draw( ClearPerformance &instance ) override;
	};
	class ShowDesc : public ProcessBase
	{
	public:
		Result	Update( ClearPerformance &instance ) override;
		void	Draw( ClearPerformance &instance ) override;
	};
	class ShowTime : public ProcessBase
	{
	public:
		Result	Update( ClearPerformance &instance ) override;
		void	Draw( ClearPerformance &instance ) override;
	};
	class ShowRank : public ProcessBase
	{
	public:
		Result	Update( ClearPerformance &instance ) override;
		void	Draw( ClearPerformance &instance ) override;
	};
	class Wait : public ProcessBase
	{
	public:
		Result	Update( ClearPerformance &instance ) override;
		void	Draw( ClearPerformance &instance ) override;
	};
// region States
#pragma endregion
private:
	static constexpr int TypeCount = scast<int>( Type::TypeCount );
private:
	Type nowType = scast<Type>( 0 );
	std::array<std::shared_ptr<ProcessBase>, TypeCount> processPtrs;

	int		timer = 0;
	Timer	clearTime;
	bool	isFinished = false;
public:
	void Init();
	void ResetProcess( const Timer &clearTime );
	void Uninit();
	void Update();
	void Draw();
public:
	bool IsDone() const { return isFinished; }
private:
	template<class DerivedProcess>
	void AssignProcess( std::shared_ptr<ProcessBase> *p )
	{
		if ( !p ) { return; }
		// else

		auto &ptr = *p;
		if ( ptr ) { ptr->Uninit( *this ); }

		ptr = std::make_shared<DerivedProcess>();

		ptr->Init( *this );
	}
};
