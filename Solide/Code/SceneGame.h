#pragma once

#include <memory>

#include "Donya/Camera.h"
#include "Donya/GamepadXInput.h"
#include "Donya/UseImGui.h"
#include "Donya/Vector.h"

#include "Player.h"
#include "Section.h"
#include "Scene.h"
#include "Terrain.h"
#include "ObstacleContainer.h"

class SceneGame : public Scene
{
public:
	struct DirectionalLight
	{
		Donya::Vector4 color{ 1.0f, 1.0f, 1.0f, 1.0f };
		Donya::Vector4 dir	{ 0.0f,-1.0f, 1.0f, 0.0f };
	};
private:
	DirectionalLight					dirLight;
	Donya::ICamera						iCamera;
	Donya::XInput						controller;

	std::unique_ptr<Terrain>			pTerrain;
	std::unique_ptr<Player>				pPlayer;
	std::unique_ptr<ObstacleContainer>	pObstacles;

#if DEBUG_MODE
	bool nowDebugMode;
	bool isReverseCameraMoveX;
	bool isReverseCameraMoveY;
	bool isReverseCameraRotX;
	bool isReverseCameraRotY;
#endif // DEBUG_MODE
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

	void	PlayerInit();
	void	PlayerUpdate( float elapsedTime );
	void	PlayerPhysicUpdate( const std::vector<Donya::AABB> &solids, const std::unique_ptr<Terrain> *ppTerrain );
	void	PlayerDraw( const Donya::Vector4x4 &matViewProj, const Donya::Vector4 &cameraPosition, const Donya::Vector4 &lightDirection );
	void	PlayerUninit();

	bool	NowGoalMoment() const;

	void	StartFade() const;
private:
	Result	ReturnResult();
private:
#if USE_IMGUI
	void	UseImGui();
#endif // USE_IMGUI
};
