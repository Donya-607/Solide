#pragma once

#include "Donya/UseImGui.h"

#include "Scene.h"
#include "UI.h"

class SceneLoad : public Scene
{
private:
	bool finishResources	= false;
	bool finishSounds		= false;
	bool succeeded			= true;

	UIObject	sprIcon;
	UIObject	sprNowLoading;
	float		flushingTimer = 0.0f;
public:
	SceneLoad() : Scene() {}
public:
	void	Init() override;
	void	Uninit() override;

	Result	Update( float elapsedTime ) override;

	void	Draw( float elapsedTime ) override;
private:
	void	SpritesUpdate( float elapsedTime );
private:
	void	ClearBackGround() const;
	void	StartFade() const;
private:
	Result	ReturnResult();
private:
#if USE_IMGUI
	void	UseImGui();
#endif // USE_IMGUI
};
