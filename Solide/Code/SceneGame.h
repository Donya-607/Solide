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
#include "Boss.h"
#include "CameraOption.h"
#include "CheckPoint.h"
#include "ClearPerformance.h"
#include "EnemyContainer.h"
#include "Goal.h"
#include "Numeric.h"
#include "ObstacleContainer.h"
#include "Player.h"
#include "Renderer.h"
#include "Section.h"
#include "Sentence.h"
#include "Scene.h"
#include "Shadow.h"
#include "Terrain.h"
#include "Timer.h"
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
	std::unique_ptr<CameraOption>		pCameraOption;
	std::unique_ptr<CheckPoint>			pCheckPoint;
	std::unique_ptr<Player>				pPlayer;
	std::unique_ptr<PlayerInitializer>	pPlayerIniter;
	std::unique_ptr<BossBase>			pBoss;
	std::unique_ptr<BossInitializer>	pBossIniter;
	std::unique_ptr<Enemy::Container>	pEnemies;
	std::unique_ptr<ObstacleContainer>	pObstacles;
	std::unique_ptr<Goal>				pGoal;
	std::unique_ptr<WarpContainer>		pWarps;
	std::unique_ptr<Shadow>				pShadow;
	std::unique_ptr<TutorialSentence>	pTutorialSentence;
	std::unique_ptr<ClearSentence>		pClearSentence;
	std::unique_ptr<ClearPerformance>	pClearPerformance;

	Timer								currentTime;
	std::vector<Timer>					borderTimes;
	NumberDrawer						numberDrawer;
	bool								shouldDrawCurrentTimer = false;

	int  stageNumber	= 1;
	int  playerRemains	= 1;
	int  gameTimer		= 0;
	int  clearTimer		= 0;
	bool nowWaiting		= false;

#if DEBUG_MODE
	bool nowDebugMode			= false;
	bool isReverseCameraMoveX	= true;
	bool isReverseCameraMoveY	= false;
	bool isReverseCameraRotX	= false;
	bool isReverseCameraRotY	= false;

	GridLine gridline;
	std::unique_ptr<Donya::Vector3> pWsIntersection	= nullptr; // When did not intersected, this will be nullptr.
	std::unique_ptr<Donya::Vector3> pWsClickedPos	= nullptr; // When did not intersected, this will be nullptr.
	bool alsoIntersectToTerrain	= true;
	bool alignToGrid			= true;
	enum class ChoiceType
	{
		Enemy,
		Obstacle,

		TypeCount
	};
	ChoiceType choiceType = ChoiceType::Enemy;
	std::shared_ptr<Enemy::Base>	pChosenEnemy	= nullptr;
	std::shared_ptr<ObstacleBase>	pChosenObstacle	= nullptr;
#endif // DEBUG_MODE
public:
	SceneGame() : Scene() {}
public:
	void	Init() override;
	void	Uninit() override;

	Result	Update( float elapsedTime ) override;

	void	Draw( float elapsedTime ) override;
private:
	void	InitStage( int stageNo, bool useSaveDataIfValid );
	void	UninitStage();

	void	WriteSaveData( int stageNo ) const;

#if DEBUG_MODE
	Donya::Vector4x4 MakeScreenTransformMatrix() const;
	void	DebugUpdate( float elapsedTime );
	void	ChoiceObject( const Donya::Vector3 &rayStart, const Donya::Vector3 &rayEnd);
	void	ChoiceEnemy( const Donya::Vector3 &rayStart, const Donya::Vector3 &rayEnd );
	void	ChoiceObstacle( const Donya::Vector3 &rayStart, const Donya::Vector3 &rayEnd );
#endif // DEBUG_MODE

	void	CameraInit();
	void	AssignCameraPos( const Donya::Vector3 &offsetPos, const Donya::Vector3 &offsetFocus );
	void	CameraUpdate();

	void	PlayerInit( int stageNo );
	void	PlayerUpdate( float elapsedTime );
	void	PlayerPhysicUpdate( const std::vector<Donya::AABB> &solids, const Donya::Model::PolygonGroup *pTerrain, const Donya::Vector4x4 *pTerrainMatrix );
	void	PlayerDraw();
	void	PlayerDrawHitBox( const Donya::Vector4x4 &matVP );
	void	PlayerUninit();
	void	RevivePlayerRemains();

	void	BossInit( int stageNo );
	void	BossUpdate( float elapsedTime );
	void	BossPhysicUpdate( const std::vector<Donya::AABB> &solids, const Donya::Model::PolygonGroup *pTerrain, const Donya::Vector4x4 *pTerrainMatrix );
	void	BossDraw();
	void	BossDrawHitBox( const Donya::Vector4x4 &matVP );
	void	BossUninit();

	void	EnemyUpdate( float elapsedTime );
	void	EnemyPhysicUpdate( const std::vector<Donya::AABB> &solids, const Donya::Model::PolygonGroup *pTerrain, const Donya::Vector4x4 *pTerrainMatrix );

	void	GridControl();

	void	DrawCurrentTime();

	void	TutorialUpdate( float elapsedTime );

	void	ClearInit();
	void	ClearUpdate( float elapsedTime );
	bool	NowWaiting() const;

	void	PlayerVSJumpStand();

	std::shared_ptr<Bullet::BulletBase> FindCollidedBulletOrNullptr( const Donya::AABB &other, const std::vector<Element::Type> &exceptTypes = {} ) const;
	void	ProcessPlayerCollision();
	void	ProcessEnemyCollision();
	void	ProcessBulletCollision();
	void	ProcessWarpCollision();
	void	ProcessCheckPointCollision();
	void	ProcessBossCollision();

	void	MakeShadows( const std::vector<Donya::AABB> &solids, const Donya::Model::PolygonGroup *pTerrain, const Donya::Vector4x4 *pTerrainMatrix );

	bool	NowGoalMoment() const;

	void	ClearBackGround() const;
	void	StartFade() const;
private:
	Result	ReturnResult();
private:
#if USE_IMGUI
	void	UseImGui();
#if DEBUG_MODE
	void	UseDebugImGui();
	void	UseChosenImGui();
#endif // DEBUG_MODE
#endif // USE_IMGUI
};
