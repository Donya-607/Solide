#pragma once

#include "Donya/GamepadXInput.h"
#include "Donya/UseImGui.h"

#include "Scene.h"
#include "UI.h"

class ScenePause : public Scene
{
public:
	enum Choice
	{
		Resume = 0,
		Retry,
		ExitStage,
		BackToTitle,

		ItemCount
	};
private:
	Choice			choice = Choice::Resume;
	int				currentStageNo = 0;
	UIObject		sprite{};
	Donya::XInput	controller{ Donya::Gamepad::PadNumber::PAD_1 };
public:
	ScenePause() : Scene() {}
public:
	void	Init();
	void	Uninit();

	Result	Update( float elapsedTime );

	void	Draw( float elapsedTime );
private:
	void	UpdateChooseItem();
	bool	WasTriggeredDecision() const;
	void	DrawBackGround() const;
private:
	void	StartFade() const;
private:
	Result	MakeRequest( Choice choice ) const;
	Result	ReturnResult();
private:
#if USE_IMGUI
	void	UseImGui();
#endif // USE_IMGUI
};
