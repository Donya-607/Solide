#pragma once

#include <vector>

#include "Donya/Camera.h"
#include "Donya/Collision.h"
#include "Donya/Constant.h"
#include "Donya/GamepadXInput.h"
#include "Donya/UseImGui.h"
#include "Donya/Vector.h"

#include "BG.h"
#include "ObstacleContainer.h"
#include "Player.h"
#include "Sentence.h"
#include "Scene.h"
#include "Terrain.h"

class SceneTitle : public Scene
{
private:
	Donya::ICamera						iCamera;
	Donya::XInput						controller;

	std::unique_ptr<BG>					pBG;
	std::unique_ptr<Terrain>			pTerrain;
	std::unique_ptr<Player>				pPlayer;
	std::unique_ptr<ObstacleContainer>	pObstacles;
	std::unique_ptr<TitleSentence>		pSentence;

	int  timer;
	bool nowWaiting;

#if DEBUG_MODE
	bool nowDebugMode;
	bool isReverseCameraMoveX;
	bool isReverseCameraMoveY;
	bool isReverseCameraRotX;
	bool isReverseCameraRotY;
#endif // DEBUG_MODE
public:
	SceneTitle();
	~SceneTitle();
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

	bool	IsRequiredAdvance() const;
	void	WaitInit();
	void	WaitUpdate( float elapsedTime );

	void	ClearBackGround() const;
	void	StartFade() const;
private:
	Result	ReturnResult();
private:
#if USE_IMGUI

	void	UseImGui();

#endif // USE_IMGUI
};
