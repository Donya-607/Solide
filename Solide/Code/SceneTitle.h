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
#include "UI.h"

class SceneTitle : public Scene
{
public:
	enum class Choice
	{
		Nil = -1,
		NewGame,
		LoadGame,

		ItemCount
	};
private:
	enum class State
	{
		Start,
		SelectItem
	};
private:
	Donya::ICamera						iCamera;
	Donya::XInput						controller{ Donya::Gamepad::PAD_1 };

	std::unique_ptr<RenderingHelper>	pRenderer;

	std::unique_ptr<BG>					pBG;
	std::unique_ptr<Terrain>			pTerrain;
	std::unique_ptr<Player>				pPlayer;
	std::unique_ptr<PlayerInitializer>	pPlayerIniter;
	std::unique_ptr<ObstacleContainer>	pObstacles;
	std::unique_ptr<TitleSentence>		pSentence;

	Choice								chooseItem	= Choice::Nil;
	State								status		= State::Start;
	UIObject							sprItem;

	int		timer		= 0;
	float	flushTimer	= 0;
	float	flushAlpha	= 1.0f;
	bool	nowWaiting	= false;

#if DEBUG_MODE
	bool nowDebugMode			= false;
	bool isReverseCameraMoveX	= false;
	bool isReverseCameraMoveY	= true;
	bool isReverseCameraRotX	= false;
	bool isReverseCameraRotY	= false;
#endif // DEBUG_MODE
public:
	SceneTitle() : Scene() {}
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

	bool	IsRequiredAdvance() const;
	bool	NowAcceptableTiming() const;

	void	UpdateByStatus( float elapsedTime );
	void	DrawByStatus( float elapsedTime );

	void	StartInit();
	void	StartUninit();
	void	StartUpdate( float elapsedTime );
	void	StartDraw( float elapsedTime );

	void	SelectInit();
	void	SelectUninit();
	void	SelectUpdate( float elapsedTime );
	void	SelectDraw( float elapsedTime );

	void	ClearBackGround() const;
	void	StartFade() const;
private:
	Result	ReturnResult();
private:
#if USE_IMGUI

	void	UseImGui();

#endif // USE_IMGUI
};
