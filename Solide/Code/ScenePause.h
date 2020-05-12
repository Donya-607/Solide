#pragma once

#include "Donya/GamepadXInput.h"
#include "Donya/UseImGui.h"

#include "Scene.h"
#include "UI.h"

class ScenePause : public Scene
{
private:
	enum Choice
	{
		Resume = 0,
		Retry,
		ExitStage,
		BackToTitle,
	};
private:
	Choice			choice = Choice::Resume;
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
	void	StartFade() const;
private:
	Result	ReturnResult();
private:
#if USE_IMGUI
	void	UseImGui();
#endif // USE_IMGUI
};
