#pragma once

#include "Donya/Camera.h"
#include "Donya/GamepadXInput.h"
#include "Donya/Vector.h"
#include "Donya/UseImGui.h"

#include "Scene.h"
#include "Player.h"

class SceneGame : public Scene
{
public:
	struct DirectionalLight
	{
		Donya::Vector4 color{ 1.0f, 1.0f, 1.0f, 1.0f };
		Donya::Vector4 dir	{ 0.0f,-1.0f, 1.0f, 0.0f };
	};
private:
	DirectionalLight	dirLight;

	Donya::ICamera		iCamera;
	Donya::XInput		controller;

	Player				player;
public:
	SceneGame();
	~SceneGame();
public:
	void	Init() override;
	void	Uninit() override;

	Result	Update( float elapsedTime ) override;

	void	Draw( float elapsedTime ) override;
private:
	void	CameraInit();
	void	AssignCameraPos();
	void	CameraUpdate();

	void	PlayerUpdate( float elapsedTime );

	void	StartFade() const;
private:
	Result	ReturnResult();
private:
#if USE_IMGUI

	void	UseImGui();

#endif // USE_IMGUI
};
