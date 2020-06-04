#pragma once

#include <array>
#include <memory>
#include <string>

#include "Donya/Constant.h"	// Use scast macro
#include "Donya/UseImGui.h"	// Use USE_IMGUI macro

#include "Numeric.h"
#include "Rank.h"
#include "Timer.h"
#include "UI.h"

class ClearPerformance
{
public:
#if USE_IMGUI
	static void LoadParameter();
	static void UseImGui();
#endif // USE_IMGUI
public:
	enum class Type : int
	{
		ShowFrame = 0,
		ShowDescription,
		ShowTime,
		ShowRank,
		Wait,

		TypeCount
	};
private:
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

	int				timer		= 0;
	int				clearRank	= 0;
	Timer			clearTime;
	bool			isFinished	= false;

	UIObject		sprFrame;
	UIObject		sprDesc;
	NumberDrawer	numberDrawer;
	Rank			rankDrawer;
public:
	bool Init( const std::wstring &frameSpritePath, const std::wstring &descriptionSpritePath, const std::wstring &numberSpritePath, const std::wstring &rankSpritePath );
	void ResetProcess( const Timer &clearTime, int clearRank );
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
public:
#if USE_IMGUI
	void ShowImGuiNode( const std::string &nodeCaption );
#endif // USE_IMGUI
};
