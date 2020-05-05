#pragma once

#include <memory>

#include "Donya/Camera.h"
#include "Donya/Collision.h"
#include "Donya/ModelCommon.h"
#include "Donya/ModelRenderer.h"
#include "Donya/GamepadXInput.h"
#include "Donya/Shader.h"
#include "Donya/UseImGui.h"
#include "Donya/Vector.h"

#include "BG.h"
#include "CheckPoint.h"
#include "EnemyContainer.h"
#include "Goal.h"
#include "ObstacleContainer.h"
#include "Player.h"
#include "Renderer.h"
#include "Section.h"
#include "Sentence.h"
#include "Scene.h"
#include "Shadow.h"
#include "Terrain.h"
#include "Warp.h"

#if DEBUG_MODE
#include "Grid.h"
#endif // DEBUG_MODE

class SceneGame : public Scene
{
private:
	Donya::ICamera						iCamera;
	Donya::XInput						controller{ Donya::Gamepad::PAD_1 };

	std::unique_ptr<RenderingHelper>	pRenderer;

	std::unique_ptr<BG>					pBG;
	std::unique_ptr<Terrain>			pTerrain;
	std::unique_ptr<CheckPoint>			pCheckPoint;
	std::unique_ptr<Player>				pPlayer;
	std::unique_ptr<PlayerInitializer>	pPlayerIniter;
	std::unique_ptr<Enemy::Container>	pEnemies;
	std::unique_ptr<ObstacleContainer>	pObstacles;
	std::unique_ptr<Goal>				pGoal;
	std::unique_ptr<WarpContainer>		pWarps;
	std::unique_ptr<Shadow>				pShadow;
	std::unique_ptr<TutorialSentence>	pTutorialSentence;
	std::unique_ptr<ClearSentence>		pClearSentence;

	int  stageNumber	= 1;
	int  gameTimer		= 0;
	int  clearTimer		= 0;
	bool nowWaiting		= false;

#if DEBUG_MODE
	bool nowDebugMode			= false;
	bool isReverseCameraMoveX	= false;
	bool isReverseCameraMoveY	= true;
	bool isReverseCameraRotX	= false;
	bool isReverseCameraRotY	= false;
	GridLine gridline;
#endif // DEBUG_MODE
public:
	SceneGame() : Scene() {}
public:
	void	Init() override;
	void	Uninit() override;

	Result	Update( float elapsedTime ) override;

	void	Draw( float elapsedTime ) override;
private:
	void	InitStage( int stageNo );
	void	UninitStage();

	void	CameraInit();
	void	AssignCameraPos();
	void	CameraUpdate();

	void	PlayerInit( int stageNo );
	void	PlayerUpdate( float elapsedTime );
	void	PlayerPhysicUpdate( const std::vector<Donya::AABB> &solids, const Donya::Model::PolygonGroup *pTerrain, const Donya::Vector4x4 *pTerrainMatrix );
	void	PlayerDraw();
	void	PlayerDrawHitBox( const Donya::Vector4x4 &matVP );
	void	PlayerUninit();

	void	EnemyUpdate( float elapsedTime );
	void	EnemyPhysicUpdate( const std::vector<Donya::AABB> &solids, const Donya::Model::PolygonGroup *pTerrain, const Donya::Vector4x4 *pTerrainMatrix );

	void	GridControl();

	void	TutorialUpdate( float elapsedTime );

	void	ClearInit();
	void	ClearUpdate( float elapsedTime );
	bool	NowWaiting() const;

	std::shared_ptr<Bullet::BulletBase> FindCollidedBulletOrNullptr( const Donya::AABB &other, const std::vector<Element::Type> &exceptTypes = {} ) const;
	void	ProcessPlayerCollision();
	void	ProcessEnemyCollision();
	void	ProcessBulletCollision();
	void	ProcessWarpCollision();
	void	ProcessCheckPointCollision();

	void	MakeShadows( const std::vector<Donya::AABB> &solids, const Donya::Model::PolygonGroup *pTerrain, const Donya::Vector4x4 *pTerrainMatrix );

	bool	NowGoalMoment() const;

	void	ClearBackGround() const;
	void	StartFade() const;
private:
	Result	ReturnResult();
private:
#if USE_IMGUI
	void	UseImGui();
#endif // USE_IMGUI
};
