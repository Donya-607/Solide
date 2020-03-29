#pragma once

#include <memory>

#include "Donya/Camera.h"
#include "Donya/ModelCommon.h"
#include "Donya/ModelRenderer.h"
#include "Donya/GamepadXInput.h"
#include "Donya/Shader.h"
#include "Donya/UseImGui.h"
#include "Donya/Vector.h"

#include "BG.h"
#include "ObstacleContainer.h"
#include "Player.h"
#include "Renderer.h"
#include "Section.h"
#include "Sentence.h"
#include "Scene.h"
#include "Terrain.h"

class SceneGame : public Scene
{
private:
	Donya::ICamera						iCamera;
	Donya::XInput						controller;

	std::unique_ptr<RenderingHelper>	pRenderer;

	std::unique_ptr<BG>					pBG;
	std::unique_ptr<Terrain>			pTerrain;
	std::unique_ptr<Player>				pPlayer;
	std::unique_ptr<ObstacleContainer>	pObstacles;
	std::unique_ptr<Goal>				pGoal;
	std::unique_ptr<TutorialSentence>	pTutorialSentence;
	std::unique_ptr<ClearSentence>		pClearSentence;

	int  gameTimer;
	int  clearTimer;
	bool nowWaiting;

#if DEBUG_MODE
	bool nowDebugMode;
	bool isReverseCameraMoveX;
	bool isReverseCameraMoveY;
	bool isReverseCameraRotX;
	bool isReverseCameraRotY;
#endif // DEBUG_MODE
public:
	SceneGame();
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
	void	PlayerDraw();
	void	PlayerDrawHitBox( const Donya::Vector4x4 &matVP );
	void	PlayerUninit();

	void	TutorialUpdate( float elapsedTime );

	bool	NowGoalMoment() const;

	void	WaitInit();
	void	WaitUpdate( float elapsedTime );
	bool	NowWaiting() const;

	void	ClearBackGround() const;
	void	StartFade() const;
private:
	Result	ReturnResult();
private:
#if USE_IMGUI
	void	UseImGui();
#endif // USE_IMGUI
};
