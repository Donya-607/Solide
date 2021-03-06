#include "SceneGame.h"

#include <vector>

#undef max
#undef min
#include <cereal/types/vector.hpp>

#include "Donya/Blend.h"
#include "Donya/Camera.h"
#include "Donya/Constant.h"
#include "Donya/Donya.h"		// Use GetFPS().
#include "Donya/Keyboard.h"
#include "Donya/Serializer.h"
#include "Donya/Sound.h"
#include "Donya/Useful.h"
#include "Donya/UseImGui.h"
#include "Donya/Vector.h"
#if DEBUG_MODE
#include "Donya/GeometricPrimitive.h"
#include "Donya/Mouse.h"
#include "Donya/Random.h"
#endif // DEBUG_MODE

#include "Bullet.h"
#include "Common.h"
#include "EffectAdmin.h"
#include "Fader.h"
#include "FilePath.h"
#include "Music.h"
#include "Obstacles.h"
#include "Parameter.h"
#include "SaveData.h"
#include "StageNumberDefine.h"

namespace
{
	struct Member
	{
		struct
		{
			float slerpFactor = 0.2f;
			Donya::Vector3 offsetPos;	// The offset of position from the player position.
			Donya::Vector3 offsetFocus;	// The offset of focus from the player position.
		}
		camera;
		
		// The threshold member will behave as offset from the player.
		// That offset will function as: threshold = player.pos.y + offset;
		RenderingHelper::TransConstant transparency;

		int waitFrameUntilShowTutorial	= 60;
		int waitFrameUntilSlideTutorial	= 60;
		int waitFrameUntilPerformance	= 60;

		Donya::Model::Constants::PerScene::DirectionalLight directionalLight;

		int maxPlayerRemains = 1;

		struct BossCamera
		{
			float slerpFactor		= 1.0f;
			float tangentDegree		= 45.0f;
			float distanceInfluence	= 1.0f;
			float leaveDistance		= 1.0f;	// The leave amount of camera from the player.
			float relatedCameraPosY	= 1.0f;	// The related Y position of camera from the player.
			Donya::Vector3 offsetFocus;		// The offset of focus from the boss position.
		private:
			friend class cereal::access;
			template<class Archive>
			void serialize( Archive &archive, std::uint32_t version )
			{
				archive
				(
					CEREAL_NVP( slerpFactor			),
					CEREAL_NVP( tangentDegree		),
					CEREAL_NVP( distanceInfluence	),
					CEREAL_NVP( leaveDistance		),
					CEREAL_NVP( relatedCameraPosY	),
					CEREAL_NVP( offsetFocus			)
				);

				if ( 1 <= version )
				{
					// archive( CEREAL_NVP( x ) );
				}
			}
		};
		BossCamera cameraBoss;

		Donya::Vector2	ssCurrentTimePos;
		float			currentTimeScale = 1.0f;

		struct RemainsDraw
		{
			Donya::Vector2	ssUIPos;
			Donya::Vector2	ssNumberPos;
			float			UIScale		= 1.0f;
			float			numberScale	= 1.0f;
		private:
			friend class cereal::access;
			template<class Archive>
			void serialize( Archive &archive, std::uint32_t version )
			{
				archive
				(
					CEREAL_NVP( ssUIPos		),
					CEREAL_NVP( ssNumberPos	),
					CEREAL_NVP( UIScale		),
					CEREAL_NVP( numberScale	)
				);

				if ( 1 <= version )
				{
					// archive( CEREAL_NVP( x ) );
				}
			}
		};

		RemainsDraw remainsDraw;

		TerrainDrawStates::Constant terrainDrawState;
	public: // Does not serialize members.
		Donya::Vector3 selectingPos;
	private:
		friend class cereal::access;
		template<class Archive>
		void serialize( Archive &archive, std::uint32_t version )
		{
			archive
			(
				CEREAL_NVP( camera.slerpFactor ),
				CEREAL_NVP( camera.offsetPos ),
				CEREAL_NVP( camera.offsetFocus )
			);

			if ( 1 <= version )
			{
				archive( CEREAL_NVP( waitFrameUntilPerformance ) );
			}
			if ( 2 <= version )
			{
				archive
				(
					CEREAL_NVP( waitFrameUntilShowTutorial  ),
					CEREAL_NVP( waitFrameUntilSlideTutorial )
				);
			}
			if ( 3 <= version )
			{
				archive
				(
					CEREAL_NVP( directionalLight.color		),
					CEREAL_NVP( directionalLight.direction	)
				);
			}
			if ( 4 <= version )
			{
				archive
				(
					CEREAL_NVP( transparency.zNear				),
					CEREAL_NVP( transparency.zFar				),
					CEREAL_NVP( transparency.lowerAlpha			),
					CEREAL_NVP( transparency.heightThreshold	)
				);
			}
			if ( 5 <= version )
			{
				archive( CEREAL_NVP( maxPlayerRemains ) );
			}
			if ( 6 <= version )
			{
				archive( CEREAL_NVP( cameraBoss ) );
			}
			if ( 7 <= version )
			{
				archive
				(
					CEREAL_NVP( ssCurrentTimePos ),
					CEREAL_NVP( currentTimeScale )
				);
			}
			if ( 8 <= version )
			{
				archive( CEREAL_NVP( remainsDraw ) );
			}
			if ( 9 <= version )
			{
				archive( CEREAL_NVP( terrainDrawState ) );
			}
			if ( 10 <= version )
			{
				// archive( CEREAL_NVP( x ) );
			}
		}
	};
}
CEREAL_CLASS_VERSION( Member, 9 )

class ParamGame : public ParameterBase<ParamGame>
{
public:
	static constexpr const char *ID = "Game";
private:
	Member m;
public:
	void Init() override
	{
	#if DEBUG_MODE
		constexpr bool fromBinary = false;
	#else
		constexpr bool fromBinary = true;
	#endif // DEBUG_MODE

		Load( m, fromBinary );
	}
	Member Data() const { return m; }
private:
	std::string GetSerializeIdentifier()			override { return ID; }
	std::string GetSerializePath( bool isBinary )	override { return GenerateSerializePath( ID, isBinary ); }
public:
#if USE_IMGUI
	void UseImGui() override
	{
		if ( !ImGui::BeginIfAllowed() ) { return; }
		// else

		if ( ImGui::TreeNode( u8"ゲームのパラメータ調整" ) )
		{
			if ( ImGui::TreeNode( u8"秒数関連" ) )
			{
				ImGui::DragInt( u8"開始からチュートリアル画像表示までの秒数", &m.waitFrameUntilShowTutorial,  1.0f, 1 );
				ImGui::DragInt( u8"チュートリアル画像表示から縮小までの秒数", &m.waitFrameUntilSlideTutorial, 1.0f, 1 );
				ImGui::DragInt( u8"クリア表示から演出開始までの秒数", &m.waitFrameUntilPerformance, 1.0f, 1 );
				m.waitFrameUntilShowTutorial	= std::max( 1, m.waitFrameUntilShowTutorial		);
				m.waitFrameUntilSlideTutorial	= std::max( 1, m.waitFrameUntilSlideTutorial	);
				m.waitFrameUntilPerformance		= std::max( 1, m.waitFrameUntilPerformance		);

				ImGui::TreePop();
			}

			if ( ImGui::TreeNode( u8"カメラ" ) )
			{
				ImGui::DragFloat ( u8"補間倍率",						&m.camera.slerpFactor,		0.01f );
				ImGui::DragFloat3( u8"自身の座標（自機からの相対）",	&m.camera.offsetPos.x,		0.01f );
				ImGui::DragFloat3( u8"注視点の座標（自機からの相対）",	&m.camera.offsetFocus.x,	0.01f );

				ImGui::TreePop();
			}
			if ( ImGui::TreeNode( u8"ボス戦時のカメラ" ) )
			{
				ImGui::DragFloat ( u8"補間倍率",							&m.cameraBoss.slerpFactor,			0.01f );
				ImGui::DragFloat ( u8"tangent角（Degree）",				&m.cameraBoss.tangentDegree,		0.1f );
				ImGui::DragFloat ( u8"距離の影響度",						&m.cameraBoss.distanceInfluence,	0.01f );
				ImGui::DragFloat ( u8"カメラのＸＺ座標（自機からの相対）",	&m.cameraBoss.leaveDistance,		0.1f );
				ImGui::DragFloat ( u8"カメラＹ座標（自機からの相対）",		&m.cameraBoss.relatedCameraPosY,	0.1f );
				ImGui::DragFloat3( u8"注視点（ボスからの相対）",			&m.cameraBoss.offsetFocus.x,		0.1f );

				ImGui::TreePop();
			}
			
			if ( ImGui::TreeNode( u8"平行光" ) )
			{
				ImGui::ColorEdit4  ( u8"色",		&m.directionalLight.color.x );
				ImGui::SliderFloat4( u8"方向",	&m.directionalLight.direction.x, -1.0f, 1.0f );

				ImGui::TreePop();
			}

			ParameterHelper::ShowConstantNode( u8"近くのオブジェクトに適用する透明度", &m.transparency );

			if ( ImGui::TreeNode( u8"時間表示" ) )
			{
				ImGui::DragFloat2( u8"現在の時間描画位置",		&m.ssCurrentTimePos.x );
				ImGui::DragFloat ( u8"現在の時間描画スケール",	&m.currentTimeScale, 0.01f );

				ImGui::TreePop();
			}

			if ( ImGui::TreeNode( u8"自機の残機" ) )
			{
				ImGui::DragInt( u8"自機の最大残機数", &m.maxPlayerRemains );
				m.maxPlayerRemains = std::max( 0, m.maxPlayerRemains );

				ImGui::DragFloat2( u8"UI・描画位置",			&m.remainsDraw.ssUIPos.x );
				ImGui::DragFloat ( u8"UI・描画スケール",		&m.remainsDraw.UIScale, 0.01f );
				ImGui::DragFloat2( u8"数字・描画位置",		&m.remainsDraw.ssNumberPos.x );
				ImGui::DragFloat ( u8"数字・描画スケール",	&m.remainsDraw.numberScale, 0.01f );

				ImGui::TreePop();
			}

			if ( ImGui::TreeNode( u8"ステージ描画の模様設定" ) )
			{
				ImGui::DragFloat2 ( u8"描画間隔",		&m.terrainDrawState.interval.x,		0.01f );
				ImGui::SliderFloat( u8"暗くする度合い",	&m.terrainDrawState.darkenAlpha,	0.0f, 1.0f );

				ImGui::TreePop();
			}

			ShowIONode( m );

			ImGui::TreePop();
		}

		ImGui::End();
	}
#endif // USE_IMGUI
};

namespace
{
	constexpr bool ShouldGotoTitleScene( int nextStageNo )
	{
		return ( nextStageNo < 0 ) ? true : false;
	}

	Member FetchMember()
	{
		return ParamGame::Get().Data();
	}
}

void SceneGame::Init()
{
	SaveDataAdmin::Get().RemoveChangeStageRequest();
	SaveDataAdmin::Get().Load();

	// This save is making a save data file if that does not exist,
	// also updates the file version if the admin loads old version.
	SaveDataAdmin::Get().Save();

#if DEBUG_MODE
	gridline.Init();
	// My prefer initial settings.
	gridline.SetDrawOrigin	( { 0.0f,	0.0f,	-50.0f } );
	gridline.SetDrawLength	( { 16.0f,	64.0f } );
	gridline.SetDrawInterval( { 1.0f,	1.0f,	1.0f   } );
#endif // DEBUG_MODE

	bool result{};

	pRenderer = std::make_unique<RenderingHelper>();
	result = pRenderer->Init();
	assert( result );

	ParamGame::Get().Init();

	pTerrainDrawState = std::make_unique<TerrainDrawStates>();
	result = pTerrainDrawState->CreateStates();
	assert( result );

	pShadow = std::make_unique<Shadow>();
	pShadow->LoadTexture();
	pShadow->ClearInstances();

	result = numberDrawer.Init( GetSpritePath( SpriteAttribute::Number ) );
	assert( result );
	
	pInfoDrawer = std::make_unique<StageInfoDisplayer>();
	result = pInfoDrawer->Init();
	assert( result );

	result = sprRemains.LoadSprite( GetSpritePath( SpriteAttribute::PlayerRemains ), 4U );
	assert( result );

	const SaveData nowData = SaveDataAdmin::Get().GetNowData();
#if 0 // ENABLE_RESTART_FROM_LAST_STATUS
	stageNumber = ( nowData.isEmpty ) ? SELECT_STAGE_NO : nowData.currentStageNumber;
	InitStage( stageNumber, /* useSaveDataIfValid = */ true );
#else
	stageNumber = SELECT_STAGE_NO;
	InitStage( stageNumber, /* useSaveDataIfValid = */ false );
#endif // ENABLE_RESTART_FROM_LAST_STATUS

	WriteSaveData( stageNumber );
	SaveDataAdmin::Get().Save();

	// I must call this after initialize the warp objects.
	ExploreBossContainStageNumbers();
}
void SceneGame::Uninit()
{
	UninitStage();

	if ( pShadow ) { pShadow->ClearInstances(); }
	pShadow.reset();
	pInfoDrawer.reset();
	pTerrainDrawState.reset();

	ObstacleBase::ParameterUninit();
	ParamGame::Get().Uninit();

#if DEBUG_MODE
	gridline.Uninit();
#endif // DEBUG_MODE

	StopAllGameBGM();
}

Scene::Result SceneGame::Update( float elapsedTime )
{
	elapsedTime = 1.0f; // Disable

#if DEBUG_MODE
	if ( Donya::Keyboard::Trigger( VK_F5 ) )
	{
		nowDebugMode = !nowDebugMode;

		if ( nowDebugMode )
		{
			iCamera.ChangeMode( Donya::ICamera::Mode::Free );
		}
		else
		{
			iCamera.SetOrientation( Donya::Quaternion::Identity() );
			iCamera.Init( Donya::ICamera::Mode::Look );
		}
	}

	DebugUpdate( elapsedTime );

#endif // DEBUG_MODE

#if USE_IMGUI
	ParamGame::Get().UseImGui();
	UseImGui();
	UseDebugImGui();
	UseChosenImGui();

	Enemy::UseImGui();
	ObstacleBase::UseImGui();
	Bullet::UseBulletsImGui();
	ClearPerformance::UseImGui();
#endif // USE_IMGUI

	if ( SaveDataAdmin::Get().HasRequiredChangeStage() )
	{
		const auto pDestStageNo = SaveDataAdmin::Get().GetRequiredDestinationOrNullptr();
		if ( pDestStageNo )
		{
			stageNumber = *pDestStageNo;
			nowWaiting  = true; // Prevent some unexpected behavior.
			StartFade();
		}
		SaveDataAdmin::Get().RemoveChangeStageRequest();
	}

	// Stage transition process.
	if ( Fader::Get().IsClosed() )
	{
		if ( ShouldGotoTitleScene( stageNumber ) )
		{
			WriteSaveData( SELECT_STAGE_NO );
			SaveDataAdmin::Get().Save();
		}
		else
		{
			UninitStage();
			InitStage( stageNumber, /* useSaveDataIfValid = */ false );
	
			WriteSaveData( stageNumber );
			SaveDataAdmin::Get().Save();
		}
	}

	controller.Update();

	if ( pTutorialContainer )
	{
		// I wanna skip a frame that immediately after a user confirmed.
		// Because if allows that frame, the key of used for confirm is triggered in main update, that is not preferred(e.g. the player will jump when the confirmed frame).
		// So that flag must take before update.
		const bool wantPause = pTutorialContainer->ShouldPauseGame();

		pTutorialContainer->Update( elapsedTime, controller );

		if ( wantPause )
		{
			// If use ReturnResult(), that allows pause function. I do not want that.
			Scene::Result noop{ Scene::Request::NONE, Scene::Type::Null };
			return noop;
		}
	}

	if ( !nowWaiting )
	{
		currentTime.Update();
	}

	pBG->Update( elapsedTime );

	pTerrain->BuildWorldMatrix();

	EnemyUpdate( elapsedTime );

	// Update obstacles.
	{
		const Donya::Vector3 playerPos = ( pPlayer ) ? pPlayer->GetPosition() : Donya::Vector3{ FLT_MAX, FLT_MAX, FLT_MAX };
		pGoal->Update( elapsedTime );
		pObstacles->Update( elapsedTime, playerPos );
	}

	pWarps->Update( elapsedTime );

	PlayerUpdate( elapsedTime );
	
	BossUpdate( elapsedTime );

	Bullet::BulletAdmin::Get().Update( elapsedTime );

	PlayerVSJumpStand();

	// Physic updates.
	{
		const auto solids  = pObstacles->GetHitBoxes();
		const auto terrain = pTerrain->GetCollisionModel();
		const Donya::Vector4x4 &terrainMatrix = pTerrain->GetWorldMatrix();

		PlayerPhysicUpdate( solids, terrain.get(), &terrainMatrix );
		Bullet::BulletAdmin::Get().PhysicUpdate( solids, terrain.get(), &terrainMatrix );
		EnemyPhysicUpdate( solids, terrain.get(), &terrainMatrix );
		BossPhysicUpdate( solids, terrain.get(), &terrainMatrix );

		MakeShadows( solids, terrain.get(), &terrainMatrix );
	}

	PlayerVSTutorialGenerator();

	ProcessWarpCollision();
	ProcessCheckPointCollision();
	ProcessBulletCollision();
	ProcessEnemyCollision();
	ProcessBossCollision();
	ProcessPlayerCollision();

	if ( NowGoalMoment() )
	{
		SaveData::ClearData clearData{};
		clearData.clearRank = Rank::Calculate( currentTime, borderTimes );
		clearData.clearTime = currentTime;
		SaveDataAdmin::Get().RegisterIfFastOrNew( stageNumber, clearData );

		if ( pGoal )
		{
			const std::vector<int> unlockStageNumbers = pGoal->GetUnlockStageNumbers();
			for ( const auto &it : unlockStageNumbers )
			{
				SaveDataAdmin::Get().UnlockStage( it );
			}

			SaveDataAdmin::Get().Save();
		}

		ClearInit();
		Donya::Sound::Play( Music::UI_Goal );
	}

	ClearUpdate( elapsedTime );
	TutorialUpdate( elapsedTime );

	CameraUpdate();
	EffectAdmin::Get().SetViewMatrix( iCamera.CalcViewMatrix() );
	EffectAdmin::Get().SetProjectionMatrix( iCamera.GetProjectionMatrix() );

	return ReturnResult();
}

void SceneGame::Draw( float elapsedTime )
{
	elapsedTime = 1.0f; // Disable

	ClearBackGround();

	const Donya::Vector4x4 VP{ iCamera.CalcViewMatrix() * iCamera.GetProjectionMatrix() };
	const auto data = FetchMember();

#if DEBUG_MODE
	if ( nowDebugMode )
	{
		gridline.Draw( VP );
	}
#endif // DEBUG_MODE

	if ( pShadow )
	{
		pShadow->Draw( VP );
	}
	
	// Update scene constant.
	{
		Donya::Model::Constants::PerScene::Common constant{};
		constant.directionalLight	= data.directionalLight;
		constant.eyePosition		= Donya::Vector4{ iCamera.GetPosition(), 1.0f };
		constant.viewProjMatrix		= VP;
		pRenderer->UpdateConstant( constant );
	}

	// Update transparency constant.
	{
		const auto &trans = data.transparency;
		RenderingHelper::TransConstant constant{};
		constant.zNear				= trans.zNear;
		constant.zFar				= trans.zFar;
		constant.lowerAlpha			= trans.lowerAlpha;
		constant.heightThreshold	= pPlayer->GetPosition().y + trans.heightThreshold;
		pRenderer->UpdateConstant( constant );
	}

	auto EnableDefaultColorAdjustment = [&]()
	{
		pRenderer->UpdateConstant( RenderingHelper::AdjustColorConstant::MakeDefault() );
		pRenderer->ActivateConstantAdjustColor();
	};
	EnableDefaultColorAdjustment();

	pRenderer->ActivateDepthStencilModel();
	pRenderer->ActivateRasterizerModel();
	pRenderer->ActivateSamplerModel();
	pRenderer->ActivateConstantScene();
	pRenderer->ActivateConstantTrans();
	{
		// The drawing priority is determined by the priority of the information.

		pRenderer->ActivateShaderNormalSkinning();
		{
			PlayerDraw();
			EnableDefaultColorAdjustment();

			pEnemies->Draw( pRenderer.get() );
			EnableDefaultColorAdjustment();
			
			BossDraw();
			EnableDefaultColorAdjustment();
		}
		pRenderer->DeactivateShaderNormalSkinning();
		
		pRenderer->DeactivateConstantAdjustColor();
		{
			pTerrainDrawState->ActivateShader();
			pTerrainDrawState->Update( data.terrainDrawState );
			pTerrainDrawState->ActivateConstant();
			pTerrain->Draw( pRenderer.get(), { 1.0f, 1.0f, 1.0f, 1.0f } );
			pTerrainDrawState->DeactivateConstant();
			pTerrainDrawState->DeactivateShader();
		}
		pRenderer->ActivateConstantAdjustColor();

		pRenderer->ActivateShaderNormalStatic();
		{
			Bullet::BulletAdmin::Get().Draw( pRenderer.get(), { 1.0f, 1.0f, 1.0f, 1.0f } );
			pTerrain->Draw( pRenderer.get(), { 1.0f, 1.0f, 1.0f, 1.0f } );
			pGoal->Draw( pRenderer.get(), { 1.0f, 1.0f, 1.0f, 1.0f } );
			pObstacles->Draw( pRenderer.get(), { 1.0f, 1.0f, 1.0f, 1.0f } );
			pWarps->Draw( pRenderer.get(), { 1.0f, 1.0f, 1.0f, 1.0f } );
		}
		pRenderer->DeactivateShaderNormalStatic();
	}
	pRenderer->DeactivateConstantTrans();
	pRenderer->DeactivateConstantScene();
	pRenderer->DeactivateDepthStencilModel();
	pRenderer->DeactivateRasterizerModel();
	pRenderer->DeactivateSamplerModel();

#if DEBUG_MODE
	if ( Common::IsShowCollision() )
	{
		constexpr Donya::Vector4 blendColor{ 1.0f, 1.0f, 1.0f, 0.5f };

		pCheckPoint->DrawHitBoxes( pRenderer.get(), VP, blendColor );

		PlayerDrawHitBox( VP );

		pEnemies->DrawHitBoxes( pRenderer.get(), VP );

		pGoal->DrawHitBox( pRenderer.get(), VP, blendColor );
		Bullet::BulletAdmin::Get().DrawHitBoxes( pRenderer.get(), VP, blendColor );
		pWarps->DrawHitBoxes( pRenderer.get(), VP, blendColor );
		pObstacles->DrawHitBoxes( pRenderer.get(), VP, blendColor );

		pCameraOption->Visualize( pRenderer.get(), VP, blendColor );

		BossDrawHitBox( VP );

		if ( pTutorialContainer )
		{
			pTutorialContainer->DrawHitBoxes( pRenderer.get(), VP, blendColor.w );
		}
	}
#endif // DEBUG_MODE

	Donya::Blend::Activate( Donya::Blend::Mode::ALPHA );

	// Drawing to far for avoiding to trans the BG's blue.
	pBG->Draw( elapsedTime );

	if ( shouldDrawUI )
	{
		DrawCurrentTime();
		DrawPlayerRemains();
	}

	DrawStageInfo();

	// A draw check of these sentences are doing at internal of these methods.
	//if ( pTutorialSentence	) { pTutorialSentence->Draw( elapsedTime );	}
	if ( pClearSentence		) { pClearSentence->Draw( elapsedTime );	}
	if ( pClearPerformance	) { pClearPerformance->Draw();				}
	if ( pTutorialContainer ) { pTutorialContainer->Draw();				}

#if DEBUG_MODE
	if ( Common::IsShowCollision() )
	{
		Donya::Blend::Activate( Donya::Blend::Mode::ALPHA_NO_ATC );

		Donya::Model::Cube::Constant constant;
		constant.matViewProj	= VP;
		constant.lightDirection	= data.directionalLight.direction.XYZ();

		auto SetColor = [&]( const Donya::Vector4 &color )
		{
			constant.drawColor = color;
		};
		auto DrawCube = [&]( const Donya::Vector3 &pos, const Donya::Vector3 &scale = { 1.0f, 1.0f, 1.0f } )
		{
			constant.matWorld._11 = scale.x * 2.0f;
			constant.matWorld._22 = scale.y * 2.0f;
			constant.matWorld._33 = scale.z * 2.0f;
			constant.matWorld._41 = pos.x;
			constant.matWorld._42 = pos.y;
			constant.matWorld._43 = pos.z;
			pRenderer->ProcessDrawingCube( constant );
		};

		if ( pPlayerIniter )
		{
			SetColor( { 0.5f, 1.0f, 0.8f, 0.5f } );
			DrawCube( pPlayerIniter->GetInitialPos() );
		}

		if ( nowDebugMode )
		{
			if ( pWsIntersection )
			{
				SetColor( { 1.0f, 0.5f, 0.0f, 0.5f } );
				DrawCube( *pWsIntersection );
			}
			
			if ( pWsClickedPos )
			{
				SetColor( { 1.0f, 0.2f, 1.0f, 0.5f } );
				DrawCube( *pWsClickedPos );
			}
		}
	}
#endif // DEBUG_MODE
}

void SceneGame::StopAllGameBGM()
{
	// Stop() will returns if not playing the bgm currently.
	constexpr bool applyToAll = true;
	Donya::Sound::Stop( Music::BGM_Title,	applyToAll );
	Donya::Sound::Stop( Music::BGM_Stage1,	applyToAll );
	Donya::Sound::Stop( Music::BGM_Stage2,	applyToAll );
	Donya::Sound::Stop( Music::BGM_Stage3,	applyToAll );
	Donya::Sound::Stop( Music::BGM_Stage4,	applyToAll );
	Donya::Sound::Stop( Music::BGM_Boss,	applyToAll );
	Donya::Sound::Stop( Music::BGM_Clear,	applyToAll );
}
Music::ID SceneGame::GetBGMID( int stageNo )
{
	// Refer two digit.
	stageNo /= 10;
	stageNo =  std::min( 9, stageNo ); // I expect less-equal than two digits.

	if ( stageNo == 1 ) { return Music::BGM_Stage1; }
	if ( stageNo == 2 ) { return Music::BGM_Stage2; }
	if ( stageNo == 3 ) { return Music::BGM_Stage3; }
	if ( stageNo == 4 ) { return Music::BGM_Stage4; }
	// else

	// Also came the 0(SELECT_STAGE_NO)
	return Music::BGM_Title;
}
void SceneGame::PlayBGM( Music::ID id )
{
	Donya::Sound::Play( id );
	lastPlayMusic = id;
}

void SceneGame::InitStage( int stageNo, bool useSaveDataIfValid )
{
#if DEBUG_MODE
	// The parameters re-loading are unnecessary, but if when debugging, that is convenience.
	ParamGame::Get().Init();
	Bullet::BulletAdmin::Get().Init();
	Bullet::LoadBulletsResource();
	ObstacleBase::ParameterInit();
	ClearPerformance::LoadParameter();
#endif // DEBUG_MODE

	using Spr = SpriteAttribute;

	const SaveData nowData = SaveDataAdmin::Get().GetNowData();

	bool result{};

	// First, make a boss if exist.
	// Because I am able to judge the playing sound early.
	pBossIniter = std::make_unique<BossInitializer>();
	pBossIniter->LoadParameter( stageNo );
	BossInit( stageNo );

	if ( pBoss ) // True if exist
	{
		StopAllGameBGM();
		PlayBGM( Music::BGM_Boss );
	}
	else
	{
		const auto playID = GetBGMID( stageNo );
		if ( playID != lastPlayMusic ) // If these are same, I do not want stop the BGM.
		{
			StopAllGameBGM();
			PlayBGM( playID );
		}
	}

	pBG = std::make_unique<BG>();
	result = pBG->LoadSprites( GetSpritePath( Spr::BackGround ), GetSpritePath( Spr::Cloud ) );
	assert( result );
	
	/*
	if ( stageNo == FIRST_STAGE_NO )
	{
		pTutorialSentence = std::make_unique<TutorialSentence>();
		pTutorialSentence->Init();
		result = pTutorialSentence->LoadSprite( GetSpritePath( Spr::TutorialSentence ) );
		assert( result );
	}
	else
	{
		pTutorialSentence.reset();
	}
	*/
	pTutorialContainer = std::make_unique<TutorialContainer>();
	result = pTutorialContainer->Init( stageNo );
	assert( result );

	pClearSentence = std::make_unique<ClearSentence>();
	pClearSentence->Init();
	result = pClearSentence->LoadSprite( GetSpritePath( Spr::ClearSentence ) );
	assert( result );

	pClearPerformance = std::make_unique<ClearPerformance>();
	pClearPerformance->Init
	(
		GetSpritePath( Spr::ClearFrame			),
		GetSpritePath( Spr::ClearDescription	),
		GetSpritePath( Spr::Number				),
		GetSpritePath( Spr::ClearRank			)
	);

	pTerrain = std::make_unique<Terrain>( stageNo );

	pCameraOption = std::make_unique<CameraOption>();
	pCameraOption->Init( stageNo );

	pCheckPoint = std::make_unique<CheckPoint>();
	if ( useSaveDataIfValid && !nowData.isEmpty )
	{
		pCheckPoint->Init( nowData, stageNo );
	}
	else
	{
		pCheckPoint->Init( stageNo );
	}
	
	pEnemies = std::make_unique<Enemy::Container>();
	pEnemies->Init( stageNo );

	pGoal = std::make_unique<Goal>();
	pGoal->Init( stageNo );
	pObstacles = std::make_unique<ObstacleContainer>();
	pObstacles->Init( stageNo );

	pWarps = std::make_unique<WarpContainer>();
	pWarps->Init( stageNo );

	pPlayerIniter = std::make_unique<PlayerInitializer>();
	if ( useSaveDataIfValid && !nowData.isEmpty && nowData.pCurrentIntializer )
	{
		*pPlayerIniter = *nowData.pCurrentIntializer;
	}
	else
	{
		bool shouldUseReturningData = false;
		if ( 0 <= beforeWarpStageNumber )
		{
			if ( stageNo == beforeWarpStageNumber )
			{
				shouldUseReturningData = true;
			}
		}

		if ( shouldUseReturningData && pReturningPlayerIniter )
		{
			*pPlayerIniter = *pReturningPlayerIniter;

			pReturningPlayerIniter.reset();
			beforeWarpStageNumber = -1;
		}
		else
		{
			pPlayerIniter->LoadParameter( stageNo );
		}
	}
	PlayerInit( stageNo );
	reviveCameraOptionIndex = 0;

	if ( stageNo == SELECT_STAGE_NO )
	{
		RevivePlayerRemains();
	}

	CameraInit();

	if ( !pShadow )
	{
		pShadow = std::make_unique<Shadow>();
		pShadow->LoadTexture();
	}
	pShadow->ClearInstances();

	Bullet::BulletAdmin::Get().Init();

	currentTime.Set( 0, 0, 0 );
	if ( stageNo == SELECT_STAGE_NO || stageNo == TITLE_STAGE_NO )
	{
		shouldDrawUI = false;
	}
	else
	{
		shouldDrawUI = true;
	}

	nowWaiting = false;
}
void SceneGame::UninitStage()
{
	if ( pCameraOption		) { pCameraOption->Uninit();		}
	if ( pCheckPoint		) { pCheckPoint->Uninit();			}
	if ( pEnemies			) { pEnemies->Uninit();				}
	if ( pGoal				) { pGoal->Uninit();				}
	if ( pObstacles			) { pObstacles->Uninit();			}
	if ( pWarps				) { pWarps->Uninit();				}
	if ( pTutorialContainer	) { pTutorialContainer->Uninit();	}

	pBG.reset();
	pTerrain.reset();
	pCameraOption.reset();
	pCheckPoint.reset();
	PlayerUninit();
	BossUninit();
	pEnemies.reset();
	pObstacles.reset();
	pGoal.reset();
	pWarps.reset();
	//pTutorialSentence.reset();
	pTutorialContainer.reset();
	pClearSentence.reset();

	pShadow->ClearInstances();

	Bullet::BulletAdmin::Get().Uninit();
}

void SceneGame::WriteSaveData( int stageNo ) const
{
	SaveDataAdmin::Get().Write( stageNo );
	if ( stageNo != stageNumber )
	{
		auto pTmpCheckPoint = std::make_unique<CheckPoint>();
		pTmpCheckPoint->Init( stageNo );
		SaveDataAdmin::Get().Write( *pTmpCheckPoint );

		auto pTmpIniter = std::make_unique<PlayerInitializer>();
		pTmpIniter->LoadParameter( stageNo );
		SaveDataAdmin::Get().Write( *pTmpIniter );
	}
	else
	{
		if ( pCheckPoint	) { SaveDataAdmin::Get().Write( *pCheckPoint	); }
		if ( pPlayerIniter	) { SaveDataAdmin::Get().Write( *pPlayerIniter	); }
	}
}

Donya::Vector4x4 SceneGame::MakeScreenTransformMatrix() const
{
	const Donya::Vector4x4 V  = iCamera.CalcViewMatrix();
	const Donya::Vector4x4 P  = iCamera.GetProjectionMatrix();
	const Donya::Vector4x4 VP = Donya::Vector4x4::MakeViewport( { Common::ScreenWidthF(), Common::ScreenHeightF() } );
	return V * P * VP;
}

#if DEBUG_MODE
#include <cmath> // Use round().
void SceneGame::DebugUpdate( float elapsedTime )
{
	if ( !nowDebugMode ) { return; }
	// else

	GridControl();

	const Donya::Vector4x4	toWorld = MakeScreenTransformMatrix().Inverse();

	const Donya::Int2		mouse		{ Donya::Mouse::Coordinate().x, Donya::Mouse::Coordinate().y };
	const Donya::Vector3	ssRayStart	{ mouse.Float(), 0.0f };
	const Donya::Vector3	ssRayEnd	{ mouse.Float(), 1.0f };

	auto Transform = [&]( const Donya::Vector3 &v, float fourthParam, const Donya::Vector4x4 &m )
	{
		Donya::Vector4 tmp = m.Mul( v, fourthParam );
		tmp /= tmp.w;
		return tmp.XYZ();
	};
	const Donya::Vector3	wsRayStart	= Transform( ssRayStart,	1.0f, toWorld );
	const Donya::Vector3	wsRayEnd	= Transform( ssRayEnd,		1.0f, toWorld );

	Donya::Plane xzPlane;
	xzPlane.distance	= gridline.GetDrawOrigin().y;
	xzPlane.normal		= Donya::Vector3::Up();

	const auto planeResult = Donya::CalcIntersectionPoint( wsRayStart, wsRayEnd, xzPlane );
	if ( planeResult.isIntersect )
	{
		if ( !pWsIntersection ) { pWsIntersection = std::make_unique<Donya::Vector3>(); }
		*pWsIntersection = planeResult.intersection;
	}
	else
	{
		pWsIntersection.reset();
	}

	if ( alsoIntersectToTerrain )
	{
		const auto terrainPtr = pTerrain->GetDrawModel();
		const Donya::Vector4x4 &terrainMatrix = pTerrain->GetWorldMatrix();
		if ( terrainPtr )
		{
			const auto terrainResult = terrainPtr->RaycastWorldSpace( terrainMatrix, wsRayStart, wsRayEnd );
			if ( terrainResult.wasHit )
			{
				if ( pWsIntersection )
				{
					const float mouseDist   = ( *pWsIntersection - wsRayStart ).LengthSq();
					const float terrainDist = ( terrainResult.intersection - wsRayStart ).LengthSq();

					if ( terrainDist < mouseDist )
					{
						*pWsIntersection = terrainResult.intersection;
					}
				}
				else
				{
					pWsIntersection  = std::make_unique<Donya::Vector3>();
					*pWsIntersection = terrainResult.intersection;
				}
			}
		}
	}

	if ( alignToGrid && pWsIntersection )
	{
		const Donya::Vector3 interval = gridline.GetDrawInterval();
		Donya::Vector3 aligned = *pWsIntersection;

		Donya::Vector3 div{};
		div.x = aligned.x / interval.x;
		div.y = aligned.y / interval.y;
		div.z = aligned.z / interval.z;

		aligned.x = std::round( div.x ) * interval.x;
		aligned.y = std::round( div.y ) * interval.y;
		aligned.z = std::round( div.z ) * interval.z;

		*pWsIntersection = aligned;
	}

	if ( Donya::Keyboard::Press( VK_SHIFT ) )
	{
		if ( Donya::Keyboard::Trigger( VK_LBUTTON ) )
		{
			ChoiceObject( wsRayStart, wsRayEnd );
		}
		if ( Donya::Keyboard::Trigger( VK_RBUTTON ) )
		{
			if ( !pWsClickedPos ) { pWsClickedPos = std::make_unique<Donya::Vector3>(); }
			*pWsClickedPos = *pWsIntersection;
		}

		if ( Donya::Keyboard::Trigger( VK_SPACE ) )
		{
			int intType = scast<int>( choiceType );
			intType++;
			if ( scast<int>( ChoiceType::TypeCount ) <= intType )
			{
				intType = 0;
			}

			choiceType = scast<ChoiceType>( intType );
		}

		if ( Donya::Keyboard::Trigger( 'A' ) )
		{
			alignToGrid = !alignToGrid;
		}
	}
}

void SceneGame::ChoiceObject( const Donya::Vector3 &rayStart, const Donya::Vector3 &rayEnd )
{
	switch ( choiceType )
	{
	case ChoiceType::Enemy:		ChoiceEnemy( rayStart, rayEnd );	return;
	case ChoiceType::Obstacle:	ChoiceObstacle( rayStart, rayEnd );	return;
	default: return;
	}
}
void SceneGame::ChoiceEnemy( const Donya::Vector3 &rayStart, const Donya::Vector3 &rayEnd )
{
	pChosenEnemy.reset();
	if ( !pEnemies ) { return; }
	// else

	struct Bundle
	{
		Donya::Vector3 intersection;
		std::shared_ptr<Enemy::Base> ptr;
	public:
		Bundle() : intersection(), ptr( nullptr ) {}
	};
	std::vector<Bundle> candidates;

	std::vector<Donya::AABB> enemyBodies{};
	std::shared_ptr<Enemy::Base> pEnemy = nullptr;

	auto CalcNearestResult = [&]( const std::vector<Donya::AABB> &boxes )
	{
		Donya::RayIntersectResult tmp;
		std::vector<Donya::RayIntersectResult> results;
		for ( const auto &it : boxes )
		{
			tmp = Donya::CalcIntersectionPoint( rayStart, rayEnd, it );
			if ( !tmp.isIntersect ) { continue; }
			// else
			results.emplace_back( tmp );
		}

		Donya::RayIntersectResult nearestResult;
		float nearestDist = FLT_MAX;
		for ( const auto &it : results )
		{
			const float dist	= ( it.intersection - rayStart ).LengthSq();
			if ( dist < nearestDist )
			{
				nearestDist		= dist;
				nearestResult	= it;
			}
		}

		return nearestResult;
	};

	const size_t enemyCount = pEnemies->GetEnemyCount();
	for ( size_t i = 0; i < enemyCount; ++i )
	{
		pEnemy = pEnemies->GetEnemyPtrOrNull( i );
		if ( !pEnemy ) { continue; }
		// else

		enemyBodies.clear();
		pEnemy->AcquireHurtBoxes( &enemyBodies );

		const auto result = CalcNearestResult( enemyBodies );
		if ( result.isIntersect )
		{
			Bundle tmp;
			tmp.intersection = result.intersection;
			tmp.ptr = pEnemy;
			candidates.emplace_back( std::move( tmp ) );
		}
	}

	float nearestDist = FLT_MAX;
	for ( const auto &it : candidates )
	{
		const float dist = ( it.intersection - rayStart ).LengthSq();
		if ( dist < nearestDist )
		{
			nearestDist  = dist;
			pChosenEnemy = it.ptr;
		}
	}
}
void SceneGame::ChoiceObstacle( const Donya::Vector3 &rayStart, const Donya::Vector3 &rayEnd )
{
	pChosenObstacle.reset();

	if ( !pObstacles ) { return; }
	// else

	struct Bundle
	{
		Donya::Vector3 intersection;
		std::shared_ptr<ObstacleBase> ptr;
	public:
		Bundle() : intersection(), ptr( nullptr ) {}
	};
	std::vector<Bundle> candidates;

	Donya::AABB obstacleBody{};
	std::shared_ptr<ObstacleBase> pObstacle{};
	
	const size_t obstacleCount = pObstacles->GetObstacleCount();
	for ( size_t i = 0; i < obstacleCount; ++i )
	{
		pObstacle = pObstacles->GetObstaclePtrOrNullptr( i );
		if ( !pObstacle ) { return; }
		// else

		const auto result = Donya::CalcIntersectionPoint( rayStart, rayEnd, pObstacle->GetHitBox() );
		if ( result.isIntersect )
		{
			Bundle tmp;
			tmp.intersection = result.intersection;
			tmp.ptr = pObstacle;
			candidates.emplace_back( std::move( tmp ) );
		}
	}

	float nearestDist = FLT_MAX;
	for ( const auto &it : candidates )
	{
		const float dist = ( it.intersection - rayStart ).LengthSq();
		if ( dist < nearestDist )
		{
			nearestDist = dist;
			pChosenObstacle = it.ptr;
		}
	}
}
#endif // DEBUG_MODE

void SceneGame::CameraInit()
{
	iCamera.Init( Donya::ICamera::Mode::Look );
	iCamera.SetZRange( 1.0f, 1000.0f );
	iCamera.SetFOV( ToRadian( 30.0f ) );
	iCamera.SetScreenSize( { Common::ScreenWidthF(), Common::ScreenHeightF() } );

	const auto data = FetchMember();
	AssignCameraPos( data.camera.offsetPos, data.camera.offsetFocus );
	iCamera.SetProjectionPerspective();

	// I can setting a configuration,
	// but current data is not changed immediately.
	// So update here.
	Donya::ICamera::Controller moveInitPoint{};
	moveInitPoint.SetNoOperation();
	moveInitPoint.slerpPercent = 1.0f;
	iCamera.Update( moveInitPoint );

#if DEBUG_MODE
	if ( nowDebugMode )
	{
		iCamera.ChangeMode( Donya::ICamera::Mode::Free );
	}
#endif // DEBUG_MODE
}
void SceneGame::AssignCameraPos( const Donya::Vector3 &offsetPos, const Donya::Vector3 &offsetFocus )
{
	if ( !pPlayer ) { return; }
	// else

	const Donya::Vector3 playerPos = pPlayer->GetPosition();

	if ( pBoss )
	{
		const auto data = FetchMember().cameraBoss;

		const Donya::Vector3 targetPos = pBoss->GetPosition();
		const Donya::Vector3 targetVec
		{
			targetPos.x - playerPos.x,
			0.0f,
			targetPos.z - playerPos.z
		};

		const Donya::Vector3 posOffset = targetVec.Unit() * data.leaveDistance;
		const Donya::Vector3 cameraPos
		{
			playerPos.x + posOffset.x,
			playerPos.y + data.relatedCameraPosY,
			playerPos.z + posOffset.z
		};

		const float tan = tanf( ToRadian( data.tangentDegree ) );
		const float divedDist = targetVec.Length() / data.distanceInfluence;
		const Donya::Vector3 cameraFocus
		{
			targetPos.x			+ data.offsetFocus.x,
			( tan * divedDist )	+ data.offsetFocus.y,
			targetPos.z			+ data.offsetFocus.z
		};

		iCamera.SetPosition  ( cameraPos   );
		iCamera.SetFocusPoint( cameraFocus );

		// Setting orientation by two step, this prevent a roll-rotation(Z-axis) in local space of camera.
		{
			const Donya::Vector3 unitDirection = ( cameraFocus - cameraPos ).Unit();
			constexpr Donya::Vector3 disableY{ 1.0f, 0.0f, 1.0f };
			Donya::Quaternion orientation = Donya::Quaternion::LookAt
			(
				Donya::Vector3::Front(), disableY.Product( unitDirection ),
				Donya::Quaternion::Freeze::Up
			);

			float theta = Donya::Dot( unitDirection, orientation.LocalFront() );
			Donya::Clamp( &theta, -1.0f, 1.0f );
			const float acos = acosf( theta );
			orientation.RotateBy
			(
				Donya::Quaternion::Make( orientation.LocalRight(), acos )
			);
			iCamera.SetOrientation( orientation );
		}

		return;
	}
	// else

	iCamera.SetPosition  ( playerPos + offsetPos   );
	iCamera.SetFocusPoint( playerPos + offsetFocus );
}
void SceneGame::CameraUpdate()
{
	const auto data = FetchMember();

	Donya::ICamera::Controller input{};
	input.SetNoOperation();
	input.slerpPercent = data.camera.slerpFactor;

	Donya::Vector3 ofsPos, ofsFocus;
	if ( pCameraOption && pCameraOption->GetOptionCount() && pPlayer )
	{
		const auto option = pCameraOption->CalcCurrentOption( pPlayer->GetPosition() );
		ofsPos		= option.offsetPos;
		ofsFocus	= option.offsetFocus;
	}
	else
	{
		ofsPos		= data.camera.offsetPos;
		ofsFocus	= data.camera.offsetFocus;
	}

#if !DEBUG_MODE
	AssignCameraPos( ofsPos, ofsFocus );
	iCamera.Update( input );
#else
	if ( !nowDebugMode )
	{
		AssignCameraPos( ofsPos, ofsFocus );
		iCamera.Update( input );
		return;
	}
	// else

	static Donya::Int2 prevMouse{};
	static Donya::Int2 currMouse{};

	prevMouse = currMouse;

	auto nowMouse = Donya::Mouse::Coordinate();
	currMouse.x = scast<int>( nowMouse.x );
	currMouse.y = scast<int>( nowMouse.y );

	bool  isInputMouseButton = Donya::Mouse::Press( Donya::Mouse::Kind::LEFT ) || Donya::Mouse::Press( Donya::Mouse::Kind::MIDDLE ) || Donya::Mouse::Press( Donya::Mouse::Kind::RIGHT );
	bool  isDriveMouse = ( prevMouse != currMouse ) || Donya::Mouse::WheelRot() || isInputMouseButton;
	if ( !isDriveMouse )
	{
		input.SetNoOperation();
		iCamera.Update( input );
		return;
	}
	// else

	const Donya::Vector2 diff = ( currMouse - prevMouse ).Float();
	
	Donya::Vector3 movement{};
	Donya::Vector3 rotation{};

	if ( Donya::Keyboard::Press( VK_MENU ) )
	{
		if ( Donya::Mouse::Press( Donya::Mouse::Kind::LEFT ) )
		{
			constexpr float ROT_AMOUNT = ToRadian( 0.5f );
			rotation.x = diff.x * ROT_AMOUNT;
			rotation.y = diff.y * ROT_AMOUNT;

			if ( isReverseCameraRotX ) { rotation.x *= -1.0f; }
			if ( isReverseCameraRotY ) { rotation.y *= -1.0f; }
		}
		else
		if ( Donya::Mouse::Press( Donya::Mouse::Kind::MIDDLE ) )
		{
			constexpr float MOVE_SPEED = 0.1f;
			movement.x = diff.x * MOVE_SPEED;
			movement.y = diff.y * MOVE_SPEED;

			if ( isReverseCameraMoveX ) { movement.x *= -1.0f; }
			if ( isReverseCameraMoveY ) { movement.y *= -1.0f; }
		}

		constexpr float FRONT_SPEED = 3.5f;
		movement.z = FRONT_SPEED * scast<float>( Donya::Mouse::WheelRot() );
	}

	input.moveVelocity		= movement;
	input.yaw				= rotation.x;
	input.pitch				= rotation.y;
	input.roll				= 0.0f;
	input.moveInLocalSpace	= true;

	iCamera.Update( input );

#endif // !DEBUG_MODE
}

void SceneGame::PlayerInit( int stageNo )
{
	if ( pPlayer ) { pPlayer->Uninit(); }

	pPlayer = std::make_unique<Player>();
	pPlayer->Init( *pPlayerIniter );
}
void SceneGame::PlayerUpdate( float elapsedTime )
{
	if ( !pPlayer ) { return; }
	// else

	// Use for be able to identify to "Should I revive the remains?".
	constexpr int wantReviveRemainsSign = -2;

	if ( pPlayer->IsDead() && !nowWaiting )
	{
		if ( playerRemains <= 0 )
		{
			playerRemains = wantReviveRemainsSign;

			if ( !Fader::Get().IsExist() )
			{
				StartFade();
			}

			Player::Input empty{};
			empty.moveVectorXZ = 0.0f;
			empty.useJump = empty.useOil = false;
			pPlayer->Update( elapsedTime, empty );
			return;
		}
		// else

		playerRemains--;

		// Re-generate.
		PlayerInit( stageNumber );
		if ( pCameraOption )
		{
			pCameraOption->SetCurrentIndex( reviveCameraOptionIndex );
		}
	}

	if ( playerRemains == wantReviveRemainsSign )
	{
		RevivePlayerRemains();
	}

	Donya::Vector2		moveVector{};
	bool useJump		= false;
	bool useOil			= false;

	if ( controller.IsConnected() )
	{
		using Pad		= Donya::Gamepad;

		moveVector		=  controller.LeftStick();
		moveVector.x	+= controller.Press( Pad::Button::RIGHT	) ? +1.0f : 0.0f;
		moveVector.x	+= controller.Press( Pad::Button::LEFT	) ? -1.0f : 0.0f;
		moveVector.y	+= controller.Press( Pad::Button::UP	) ? +1.0f : 0.0f; // Front is Plus.
		moveVector.y	+= controller.Press( Pad::Button::DOWN	) ? -1.0f : 0.0f; // Front is Plus.
		moveVector.Normalize();

		useJump			= controller.Trigger( Pad::A );
		useOil			= controller.Press( Pad::B ) || controller.Press( Pad::Y );
		// useOil			= controller.Trigger( Pad::B ) || controller.Trigger( Pad::Y );
	}
	else
	{
		moveVector.x	+= Donya::Keyboard::Press( VK_RIGHT	) ? +1.0f : 0.0f;
		moveVector.x	+= Donya::Keyboard::Press( VK_LEFT	) ? -1.0f : 0.0f;
		moveVector.y	+= Donya::Keyboard::Press( VK_UP	) ? +1.0f : 0.0f; // Front is Plus.
		moveVector.y	+= Donya::Keyboard::Press( VK_DOWN	) ? -1.0f : 0.0f; // Front is Plus.
		moveVector.Normalize();

		useJump			=  Donya::Keyboard::Trigger( 'Z' );
		useOil			=  Donya::Keyboard::Press( 'X' );
		// useOil			=  Donya::Keyboard::Trigger( 'X' );
	}

	auto ShouldRestrict = [&]()
	{
		if ( nowWaiting ) { return true; }
		if ( pBoss && pBoss->NowDiePerformance() ) { return true; }
		// else

		return false;
	};
	if ( ShouldRestrict() )
	{
		moveVector	= Donya::Vector2::Zero();
		useJump		= false;
		useOil		= ( pPlayer->IsOiled() && pPlayer->OnGround() ) ? true : false;
	}
	
	// Rotate input vector by camera.
	{
		auto cameraRotation = iCamera.GetOrientation();

		// I wants rotation is Y-axis only, so disable the rotation of X-axis and Z-axis.
		cameraRotation.x = 0.0f;
		cameraRotation.z = 0.0f;
		cameraRotation.Normalize();

		Donya::Vector3 moveVector3{ moveVector.x, 0.0f, moveVector.y };
		moveVector3  = cameraRotation.RotateVector( moveVector3 );
		moveVector.x = moveVector3.x;
		moveVector.y = moveVector3.z;
	}

	Player::Input input{};
	input.moveVectorXZ	= moveVector;
	input.useJump		= useJump;
	input.useOil		= useOil;

	pPlayer->Update( elapsedTime, input );
}
void SceneGame::PlayerPhysicUpdate( const std::vector<Donya::AABB> &solids, const Donya::Model::PolygonGroup *pTerrain, const Donya::Vector4x4 *pTerrainMatrix )
{
	if ( !pPlayer ) { return; }
	
	pPlayer->PhysicUpdate( solids, pTerrain, pTerrainMatrix );
}
void SceneGame::PlayerDraw()
{
	if ( !pPlayer ) { return; }
	// else
	pPlayer->Draw( pRenderer.get() );
}
void SceneGame::PlayerDrawHitBox( const Donya::Vector4x4 &matVP )
{
	if ( !pPlayer ) { return; }
	// else
	pPlayer->DrawHitBox( pRenderer.get(), matVP );
}
void SceneGame::PlayerUninit()
{
	if ( pPlayer ) { pPlayer->Uninit(); }
	
	pPlayer.reset();
	pPlayerIniter.reset();
}
void SceneGame::RevivePlayerRemains()
{
	playerRemains = FetchMember().maxPlayerRemains;
}

void SceneGame::ExploreBossContainStageNumbers()
{
	bossContainStages.clear();

	if ( !pWarps ) { return; }
	// else

	std::vector<int> searchStageNumbers{};
	{
		const Warp *pWarp = nullptr;
		const size_t warpCount = pWarps->GetWarpCount();
		for ( size_t i = 0; i < warpCount; ++i )
		{
			pWarp = pWarps->GetWarpPtrOrNullptr( i );
			if ( !pWarp ) { continue; }
			// else

			searchStageNumbers.emplace_back
			(
				pWarp->GetDestinationStageNo()
			);
		}
	}

	BossInitializer fileLoader{};
	for ( const auto &it : searchStageNumbers )
	{
		fileLoader.LoadParameter( it );

		if ( fileLoader.ShouldGenerateBoss() )
		{
			bossContainStages.emplace_back( it );
		}
	}
}
void SceneGame::BossInit( int stageNo )
{
	if ( !pBossIniter || !pBossIniter->ShouldGenerateBoss() ) { return; }
	// else

	if ( pBoss ) { pBoss->Uninit(); }

	BossBase::AssignDerivedClass( &pBoss, pBossIniter->GetType() );
	pBoss->Init( *pBossIniter );
}
void SceneGame::BossUpdate( float elapsedTime )
{
	if ( !pBoss ) { return; }
	// else

	const Donya::Vector3 target = ( pPlayer ) ? pPlayer->GetPosition() : Donya::Vector3::Zero() /* Fail safe */;
	pBoss->Update( elapsedTime, target );
}
void SceneGame::BossPhysicUpdate( const std::vector<Donya::AABB> &solids, const Donya::Model::PolygonGroup *pTerrain, const Donya::Vector4x4 *pTerrainMatrix )
{
	if ( !pBoss ) { return; }
	// else
	pBoss->PhysicUpdate( solids, pTerrain, pTerrainMatrix );
}
void SceneGame::BossDraw()
{
	if ( !pBoss ) { return; }
	// else
	pBoss->Draw( pRenderer.get() );
}
void SceneGame::BossDrawHitBox( const Donya::Vector4x4 &matVP )
{
	if ( !pBoss ) { return; }
	// else
	pBoss->DrawHitBox( pRenderer.get(), matVP );
}
void SceneGame::BossUninit()
{
	if ( pBoss ) { pBoss->Uninit(); }

	pBoss.reset();
	pBossIniter.reset();
}

void SceneGame::EnemyUpdate( float elapsedTime )
{
	if ( !pEnemies ) { return; }
	// else
	
	const Donya::Vector3 target = ( pPlayer ) ? pPlayer->GetPosition() : Donya::Vector3::Zero() /* Fail safe */;
	pEnemies->Update( elapsedTime, target );
}
void SceneGame::EnemyPhysicUpdate( const std::vector<Donya::AABB> &solids, const Donya::Model::PolygonGroup *pTerrain, const Donya::Vector4x4 *pTerrainMatrix )
{
	if ( !pEnemies ) { return; }
	// else

	pEnemies->PhysicUpdate( solids, pTerrain, pTerrainMatrix );
}

void SceneGame::GridControl()
{
#if DEBUG_MODE
	
	if ( !nowDebugMode ) { return; }
	// else

	if ( Donya::Keyboard::Press( VK_SHIFT ) )
	{
		const Donya::Vector3 current = gridline.GetDrawOrigin();
		const float speed = gridline.GetDrawInterval().y;

		const float addition  = speed * scast<float>( Donya::Mouse::WheelRot() );
		if ( !ZeroEqual( addition ) )
		{
			const Donya::Vector3 addition3 = Donya::Vector3::Up() * addition;
			gridline.SetDrawOrigin( current + addition3 );
		}
	}

#endif // DEBUG_MODE
}

void SceneGame::DrawCurrentTime()
{
	constexpr float drawDepth = 0.1f; // < pauseDrawDepth
	const auto data = FetchMember();
	numberDrawer.DrawTime
	(
		currentTime,
		data.ssCurrentTimePos,
		data.currentTimeScale, 1.0f,
		Donya::Vector2{ 0.5f, 0.5f },
		drawDepth
	);
}
void SceneGame::DrawPlayerRemains()
{
	constexpr float drawDepth = 0.1f; // < pauseDrawDepth
	const auto data = FetchMember().remainsDraw;

	sprRemains.drawScale	= data.UIScale;
	sprRemains.pos			= data.ssUIPos;
	sprRemains.Draw( drawDepth );

	numberDrawer.DrawNumber
	(
		std::max( 0, std::min( 9, playerRemains ) ),
		data.ssNumberPos,
		data.numberScale,
		1.0f, Donya::Vector2{ 0.5f, 0.5f },
		drawDepth,
		1
	);
}
void SceneGame::DrawStageInfo()
{
	if ( !pInfoDrawer || !pWarps || !pPlayer ) { return; }
	// else

	const Donya::Vector3 plPos = pPlayer->GetPosition();

	struct Data
	{
		int				targetStageNo = -1;
		Donya::Vector3	pos{};
		bool			unlocked = false;
	};
	std::vector<Data> drawData{};

	const Warp *pWarp = nullptr;
	const size_t warpCount = pWarps->GetWarpCount();
	for ( size_t i = 0; i < warpCount; ++i )
	{
		pWarp = pWarps->GetWarpPtrOrNullptr( i );
		if ( !pWarp ) { continue; }
		// else

		Data tmp;
		tmp.pos				= pWarp->GetPosition();
		tmp.targetStageNo	= pWarp->GetDestinationStageNo();
		tmp.unlocked		= pWarp->IsUnlocked();
		drawData.emplace_back( std::move( tmp ) );
	}

	auto IsGreaterDepth = []( const Data &lhs, const Data &rhs )
	{
		return ( rhs.pos.z < lhs.pos.z );
	};
	std::sort( drawData.begin(), drawData.end(), IsGreaterDepth );

	auto IsContainBoss = [&]( int stageNo )
	{
		const auto result = std::find
		(
			bossContainStages.begin(), bossContainStages.end(),
			stageNo
		);
		return ( result != bossContainStages.end() );
	};

	const auto savedata = SaveDataAdmin::Get().GetNowData();
	const Donya::Vector4x4 matScreen = MakeScreenTransformMatrix();
	for ( const auto &it : drawData )
	{
		const bool isBossStage = IsContainBoss( it.targetStageNo );
		pInfoDrawer->DrawInfo
		(
			matScreen,
			plPos,
			it.pos,
			savedata.FetchRegisteredClearDataOrDefault( it.targetStageNo ),
			it.targetStageNo,
			it.unlocked,
			isBossStage
		);
	}
}

void SceneGame::TutorialUpdate( float elapsedTime )
{
	/*
	if ( !pTutorialSentence ) { return; }
	// else

	const int showTiming  = FetchMember().waitFrameUntilShowTutorial;
	const int slideTiming = showTiming + FetchMember().waitFrameUntilSlideTutorial;

	gameTimer++;
	gameTimer = std::min( INT_MAX - 1, gameTimer );

	if ( gameTimer == showTiming  )
	{
		pTutorialSentence->Appear();
	}
	if ( gameTimer == slideTiming )
	{
		pTutorialSentence->StartSliding();
	}

	pTutorialSentence->Update( elapsedTime );
	*/
}

void SceneGame::ClearInit()
{
	clearTimer	= 0;

	if ( !pClearPerformance ) { assert( !"Unexpected error!" ); return; }
	// else

	pClearSentence->Appear();
	pClearPerformance->ResetProcess
	(
		currentTime,
		Rank::Calculate( currentTime, borderTimes )
	);

	stageNumber	= SELECT_STAGE_NO;
	nowWaiting	= true;

	StopAllGameBGM();
	PlayBGM( Music::BGM_Clear );
}
void SceneGame::ClearUpdate( float elapsedTime )
{
	if ( !pClearSentence || pClearSentence->NowHidden() ) { return; }
	// else

	pClearSentence->Update( elapsedTime );

	clearTimer++;

	const int startPerformanceFrame = FetchMember().waitFrameUntilPerformance;
	if ( startPerformanceFrame == clearTimer )
	{
		pClearPerformance->Appear();
	}
	if ( startPerformanceFrame <= clearTimer )
	{
		pClearPerformance->Update();
	}
	if ( pClearPerformance->IsDone() && !Fader::Get().IsExist() )
	{
		StartFade();
	}
}
bool SceneGame::NowWaiting() const
{
	return nowWaiting;
}

void SceneGame::PlayerVSJumpStand()
{
	if ( !pPlayer			) { return; }
	if ( pPlayer->IsDead()	) { return; }
	if ( !pObstacles		) { return; }
	// else

	const auto  jumpStandBodies = pObstacles->GetJumpStandHitBoxes();
	Donya::AABB jumpStandBody{};

	const Donya::Vector3 prevPlayerPos = pPlayer->GetPosition();
	Donya::AABB playerMovedBody = pPlayer->GetHitBox();
	playerMovedBody.pos += pPlayer->GetVelocity();

	auto IsRidingOn = [&]( const Donya::AABB &other )
	{
		// Prevent to ride when the player is rising.
		if ( 0 < pPlayer->GetVelocity().y ) { return false; }
		// else

		// If we wanna consider to riding, The previous player position must over the other's ceil.
		const float otherCeil = other.pos.y + other.size.y;
		const float playerTop = prevPlayerPos.y; // The center position is not as suitable as a top of dedicated hitBox, still so-so good.
		if ( playerTop < otherCeil ) { return false; }
		// else

		return Donya::AABB::IsHitAABB( playerMovedBody, other );
	};

	for ( const auto &it : jumpStandBodies )
	{
		if ( IsRidingOn( it ) )
		{
			pPlayer->JumpByStand();
			break;
		}
	}
}
void SceneGame::PlayerVSTutorialGenerator()
{
	if ( nowWaiting				) { return; }	// Exempt if now is cleared.
	if ( !pPlayer				) { return; }	// Do only player related collision.
	if ( pPlayer->IsDead()		) { return; }	// Unnecessary if player already dead.
	if ( !pTutorialContainer	) { return; }
	// else

	const Donya::AABB playerBody = pPlayer->GetHitBox();

	Donya::AABB tutorialBody{};
	const size_t tutorialCount = pTutorialContainer->GetTutorialCount();
	for ( size_t i = 0;  i < tutorialCount; ++i )
	{
		auto *pTutorial = pTutorialContainer->GetTutorialPtrOrNullptr( i );
		if ( !pTutorial				) { continue; }
		if ( pTutorial->IsActive()	) { continue; }
		// else

		tutorialBody = pTutorial->GetHitBox();
		if ( Donya::AABB::IsHitAABB( playerBody, tutorialBody ) )
		{
			pTutorial->Start();
			break;
		}
	}
}

std::shared_ptr<Bullet::BulletBase> SceneGame::FindCollidedBulletOrNullptr( const Donya::AABB &other, const std::vector<Element::Type> &exceptTypes ) const
{
	auto IsExceptType = [&exceptTypes]( const Element &element )
	{
		for ( const auto &it : exceptTypes )
		{
			if ( element.Get() == it )
			{
				return true;
			}
		}

		return false;
	};

	const auto		&bullet		= Bullet::BulletAdmin::Get();
	const size_t	bulletCount	= bullet.GetBulletCount();

	Donya::AABB		bulletAABB{};
	Donya::Sphere	bulletSphere{};
	std::shared_ptr<Bullet::BulletBase> pBullet = nullptr;

	for ( size_t i = 0; i < bulletCount; ++i )
	{
		pBullet = bullet.GetBulletPtrOrNull( i );
		if ( !pBullet ) { continue; }
		if ( IsExceptType( pBullet->GetElement() ) ) { continue; }
		// else

		// The bullets hit-box is either an AABB or a Sphere.

		bulletSphere = pBullet->GetHitBoxSphere();
		if ( bulletSphere != Donya::Sphere::Nil() )
		{
			if ( Donya::Sphere::IsHitAABB( bulletSphere, other ) )
			{
				return pBullet;
			}
			// else
			continue;
		}
		else
		bulletAABB = pBullet->GetHitBoxAABB();
		if ( bulletAABB != Donya::AABB::Nil() )
		{
			if ( Donya::AABB::IsHitAABB( bulletAABB, other ) )
			{
				return pBullet;
			}
			// else
			continue;
		}
		// else
	}

	return nullptr;
}
void SceneGame::ProcessPlayerCollision()
{
	if ( nowWaiting			) { return; }	// Exempt if now is cleared.
	if ( !pPlayer			) { return; }	// Do only player related collision.
	if ( pPlayer->IsDead()	) { return; }	// Unnecessary if player already dead.
	// else

	const Donya::AABB playerBody = pPlayer->GetHitBox();
	const auto exceptTypes = pPlayer->GetUncollidableTypes();

	// VS. enemies body.
	if ( pEnemies )
	{
		std::vector<Donya::AABB>    enemyBodies{};
		pEnemies->AcquireHitBoxes( &enemyBodies );

		for ( const auto &it : enemyBodies )
		{
			if ( Donya::AABB::IsHitAABB( playerBody, it ) )
			{
				pPlayer->KillMe();
				return;
			}
		}
	}

	if ( pPlayer->IsDead() ) { return; }
	// else

	// VS. bullets.
	{
		const auto pCollidedBullet = FindCollidedBulletOrNullptr( playerBody, exceptTypes );
		if ( pCollidedBullet )
		{
			pPlayer->MakeDamage( pCollidedBullet->GetElement() );
			pCollidedBullet->HitToObject();
		}
	}

	if ( pPlayer->IsDead() ) { return; }
	// else

	// VS. Waters of obstacle.
	if ( pObstacles )
	{
		const auto waters = pObstacles->GetWaterHitBoxes();
		for ( const auto &it : waters )
		{
			if ( Donya::AABB::IsHitAABB( playerBody, it ) )
			{
				pPlayer->KillMe();
				break;
			}
		}
	}

	if ( pPlayer->IsDead() ) { return; }
	// else

	// VS. boss body.
	if ( pBoss )
	{
		const std::vector<Donya::AABB> bodies = pBoss->AcquireHitBoxes();
		for ( const auto &it : bodies )
		{
			if ( Donya::AABB::IsHitAABB( playerBody, it ) )
			{
				pPlayer->KillMe();
				return;
			}
		}
	}
}
void SceneGame::ProcessEnemyCollision()
{
	if ( nowWaiting	) { return; }	// Exempt if now is cleared.
	if ( !pEnemies	) { return; }	// Do only enemy related collision.
	// else

	std::vector<Donya::AABB> enemyBodies{};
	std::shared_ptr<Enemy::Base> pEnemy = nullptr;

	const size_t enemyCount = pEnemies->GetEnemyCount();
	for ( size_t i = 0; i < enemyCount; ++i )
	{
		pEnemy = pEnemies->GetEnemyPtrOrNull( i );
		if ( !pEnemy ) { continue; }
		// else

		enemyBodies.clear();
		pEnemy->AcquireHurtBoxes( &enemyBodies );

		for ( const auto &it : enemyBodies )
		{
			const auto pCollidedBullet = FindCollidedBulletOrNullptr( it );
			if ( pCollidedBullet )
			{
				pEnemy->MakeDamage( pCollidedBullet->GetElement() );
				pCollidedBullet->HitToObject();
				break;
			}
		}
	}
}
void SceneGame::ProcessBulletCollision()
{
	// Give each element to each colliding bullets.

	auto IsHitToBulletAABB		= []( const Donya::AABB		&hitBox, const std::shared_ptr<Bullet::BulletBase> &pOther )->bool
	{
		// The bullets hit-box is either an AABB or a Sphere.

		Donya::Sphere otherSphere = pOther->GetHitBoxSphere();
		if ( otherSphere != Donya::Sphere::Nil() )
		{
			if ( Donya::Sphere::IsHitAABB( otherSphere, hitBox ) )
			{
				return true;
			}
		}
		// else

		Donya::AABB otherAABB = pOther->GetHitBoxAABB();
		if ( otherAABB != Donya::AABB::Nil() )
		{
			if ( Donya::AABB::IsHitAABB( otherAABB, hitBox ) )
			{
				return true;
			}
		}
		// else

		return false;
	};
	auto IsHitToBulletSphere	= []( const Donya::Sphere	&hitBox, const std::shared_ptr<Bullet::BulletBase> &pOther )->bool
	{
		// The bullets hit-box is either an AABB or a Sphere.

		Donya::Sphere otherSphere = pOther->GetHitBoxSphere();
		if ( otherSphere != Donya::Sphere::Nil() )
		{
			if ( Donya::Sphere::IsHitSphere( otherSphere, hitBox ) )
			{
				return true;
			}
		}
		// else

		Donya::AABB otherAABB = pOther->GetHitBoxAABB();
		if ( otherAABB != Donya::AABB::Nil() )
		{
			if ( Donya::AABB::IsHitSphere( otherAABB, hitBox ) )
			{
				return true;
			}
		}
		// else

		return false;
	};

	const auto		&bullet		= Bullet::BulletAdmin::Get();
	const size_t	bulletCount	= bullet.GetBulletCount();

	Donya::AABB		hitBoxAABB{};
	Donya::Sphere	hitBoxSphere{};

	std::shared_ptr<Bullet::BulletBase> pLhs = nullptr;
	std::shared_ptr<Bullet::BulletBase> pRhs = nullptr;

	const std::vector<Donya::AABB> waters = ( pObstacles ) ? pObstacles->GetWaterHitBoxes() : std::vector<Donya::AABB>{};
	auto IsHitToWater = [&]( const Donya::AABB &hitBoxA, const Donya::Sphere &hitBoxB )
	{
		for ( const auto &it : waters )
		{
			if ( hitBoxB != Donya::Sphere::Nil() )
			{
				if ( Donya::AABB::IsHitSphere( it, hitBoxB ) )
				{
					return true;
				}
			}
			else
			if ( hitBoxA != Donya::AABB::Nil() )
			{
				if ( Donya::AABB::IsHitAABB( it, hitBoxA ) )
				{
					return true;
				}
			}
		}
		return false;
	};

	// Check a collision in all combination of bullets.
	// e.g.
	// 0vs1, 0vs2, 0vs3, ...
	// 1vs2, 1vs3, ...
	// 2vs3, ...
	// ...

	for ( size_t i = 0; i < bulletCount; ++i )
	{
		pLhs = bullet.GetBulletPtrOrNull( i );
		if ( !pLhs ) { continue; }
		// else

		hitBoxAABB		= pLhs->GetHitBoxAABB();
		hitBoxSphere	= pLhs->GetHitBoxSphere();

		if ( pLhs->GetElement().Get() == Element::Type::Oil && IsHitToWater( hitBoxAABB, hitBoxSphere ) )
		{
			pObstacles->GenerateHardenedBlock( pLhs->GetPosition() );
			pLhs->HitToObject();
			break;
		}
		// else

		for ( size_t j = i + 1/* Except collide to myself */; j < bulletCount; ++j )
		{
			pRhs = bullet.GetBulletPtrOrNull( j );
			if ( !pRhs ) { continue; }
			// else

			// This check is unnnecessary if each element is same.
			if ( pLhs->GetElement().Get() == pRhs->GetElement().Get() ) { continue; }
			// else
		
			// The bullets hit-box is either an AABB or a Sphere.

			if ( hitBoxSphere != Donya::Sphere::Nil() )
			{
				if ( !IsHitToBulletSphere( hitBoxSphere, pRhs ) ) { continue; }
			}
			else
			if ( hitBoxAABB != Donya::AABB::Nil() )
			{
				if ( !IsHitToBulletAABB( hitBoxAABB, pRhs ) ) { continue; }
			}
			// else

			// The bullets are collided.

			auto GenerateHardenedIfOilVSIce = [&]( std::shared_ptr<Bullet::BulletBase> *pSelf, const std::shared_ptr<Bullet::BulletBase> &other )
			{
				auto &self = *pSelf;
				if (  self->GetElement().Has( Element::Type::Oil ) && other->GetElement().Has( Element::Type::Ice ) )
				{
					pObstacles->GenerateHardenedBlock( self->GetPosition() );
					self->HitToObject();
					return true;
				}
				// else
				return false;
			};
			if ( GenerateHardenedIfOilVSIce( &pLhs, pRhs ) ) { break; }
			if ( GenerateHardenedIfOilVSIce( &pRhs, pLhs ) ) { break; }
			// else

			const Element sumElement = pLhs->GetElement().Add( pRhs->GetElement().Get() );
			pLhs->GiveElement( sumElement.Get() );
			pRhs->GiveElement( sumElement.Get() );
		}
	}
}
void SceneGame::ProcessWarpCollision()
{
	if ( Fader::Get().IsExist()	) { return; }	// except hit as continuously.
	if ( nowWaiting				) { return; }	// Exempt if now is cleared.
	if ( !pPlayer				) { return; }	// Do only player related collision.
	if ( pPlayer->IsDead()		) { return; }	// Unnecessary if player already dead.
	if ( !pWarps				) { return; }
	// else

	const Donya::AABB playerBody = pPlayer->GetHitBox();
	Donya::AABB warpBody{};

	const size_t warpCount = pWarps->GetWarpCount();
	for ( size_t i = 0;  i < warpCount; ++i )
	{
		const auto *pWarp = pWarps->GetWarpPtrOrNullptr( i );
		if ( !pWarp ) { continue; }
		if ( !pWarp->IsUnlocked() ) { continue; }
		// else

		warpBody = pWarp->GetHitBox();
		if ( Donya::AABB::IsHitAABB( playerBody, warpBody ) )
		{
			nowWaiting  = true;

			beforeWarpStageNumber	= stageNumber;
			stageNumber				= pWarp->GetDestinationStageNo();
			borderTimes				= pWarp->GetBorderTimes();
			pReturningPlayerIniter	= std::make_unique<PlayerInitializer>();
			pReturningPlayerIniter->OverwriteInitialPos( pWarp->GetReturningPosition() );
			pReturningPlayerIniter->OverwriteInitialOrientation( pWarp->GetReturningOrientation() );

			StartFade();
			break;
		}
	}
}
void SceneGame::ProcessCheckPointCollision()
{
	if ( nowWaiting			) { return; }	// Exempt if now is cleared.
	if ( !pPlayer			) { return; }	// Do only player related collision.
	if ( pPlayer->IsDead()	) { return; }	// Unnecessary if player already dead.
	if ( !pCheckPoint		) { return; }
	// else

	const Donya::AABB playerBody = pPlayer->GetHitBox();
	Donya::AABB pointBody{};

	const size_t pointCount = pCheckPoint->GetPointCount();
	for ( size_t i = 0; i < pointCount; ++i )
	{
		const auto *pPoint = pCheckPoint->GetPointPtrOrNullptr( i );
		if ( !pCheckPoint ) { continue; }
		// else

		pointBody = pPoint->GetHitBox();
		if ( Donya::AABB::IsHitAABB( playerBody, pointBody ) )
		{
			*pPlayerIniter = pPoint->GetInitializer();
			pCheckPoint->RemovePoint( i );
			
			if ( pCameraOption )
			{
				reviveCameraOptionIndex = pCameraOption->GetCurrentIndex();
			}

			break;
		}
	}
}
void SceneGame::ProcessBossCollision()
{
	if ( !pBoss ) { return; }
	// else

	const std::vector<Donya::AABB> bodies = pBoss->AcquireHurtBoxes();
	const std::vector<Element::Type> exceptTypes = pBoss->GetUncollidableTypes();

	for ( const auto &it : bodies )
	{
		const auto pCollidedBullet = FindCollidedBulletOrNullptr( it, exceptTypes );
		if ( pCollidedBullet )
		{
			pBoss->MakeDamage( pCollidedBullet->GetElement(), pCollidedBullet->GetVelocity() );
			pCollidedBullet->HitToObject();
			break;
		}
	}
}

void SceneGame::MakeShadows( const std::vector<Donya::AABB> &solids, const Donya::Model::PolygonGroup *pTerrain, const Donya::Vector4x4 *pTerrainMatrix )
{
	if ( !pShadow ) { return; }
	// else

	pShadow->ClearInstances();

	auto Add = [&]( const Donya::Vector3 &position )
	{
		pShadow->Register( position );
	};

	if ( pPlayer	) { Add( pPlayer->GetPosition() ); }
	if ( pBoss		) { Add( pBoss->GetPosition() ); }
	if ( pEnemies	)
	{
		const size_t count = pEnemies->GetEnemyCount();
		for ( size_t i = 0; i < count; ++i )
		{
			const auto pEnemy = pEnemies->GetEnemyPtrOrNull( i );
			if ( !pEnemy ) { continue; }
			// else

			Add( pEnemy->GetPosition() );
		}
	}
	if ( pWarps		)
	{
		const size_t count = pWarps->GetWarpCount();
		for ( size_t i = 0; i < count; ++i )
		{
			const auto pWarp = pWarps->GetWarpPtrOrNullptr( i );
			if ( !pWarp ) { continue; }
			// else

			Add( pWarp->GetPosition() );
		}
	}

	pShadow->CalcIntersectionPoints( solids, pTerrain, pTerrainMatrix );
}

bool SceneGame::NowGoalMoment() const
{
	if ( nowWaiting				) { return false; }	// Unnecessary if now is cleared.
	if ( !pPlayer				) { return false; }
	if ( pPlayer->IsDead()		) { return false; }	// Unnecessary if player already dead.
	if ( !pGoal					) { return false; }
	if ( Fader::Get().IsExist()	) { return false; }
	// else

	if ( pBoss )
	{
		return pBoss->IsDead();
	}
	// else

	const Donya::AABB goalArea		= pGoal->GetHitBox();
	const Donya::AABB playerBody	= pPlayer->GetHitBox();

	return Donya::AABB::IsHitAABB( playerBody, goalArea );
}

void SceneGame::ClearBackGround() const
{
	constexpr FLOAT BG_COLOR[4]{ 0.4f, 0.4f, 0.4f, 1.0f };
	Donya::ClearViews( BG_COLOR );

#if DEBUG_MODE
	if ( nowDebugMode )
	{
		constexpr FLOAT DEBUG_COLOR[4]{ 0.7f, 0.8f, 0.1f, 1.0f };
		Donya::ClearViews( DEBUG_COLOR );
	}
#endif // DEBUG_MODE
}

void SceneGame::StartFade() const
{
	Fader::Configuration config{};
	config.type			= Fader::Type::Gradually;
	config.closeFrame	= Fader::GetDefaultCloseFrame();
	config.SetColor( Donya::Color::Code::BLACK );
	Fader::Get().StartFadeOut( config );
}

Scene::Result SceneGame::ReturnResult()
{
#if DEBUG_MODE
	if ( Donya::Keyboard::Trigger( VK_F2 ) && !Fader::Get().IsExist() )
	{
		Donya::Sound::Play( Music::ItemDecision );

		StartFade();
	}
#endif // DEBUG_MODE

	if ( Fader::Get().IsClosed() && ShouldGotoTitleScene( stageNumber ) )
	{
		Scene::Result change{};
		change.AddRequest( Scene::Request::ADD_SCENE, Scene::Request::REMOVE_ME );
		change.sceneType = Scene::Type::Title;
		return change;
	}

	const bool requestPause	= Donya::Keyboard::Trigger( 'P' ) || controller.Trigger( Donya::Gamepad::Button::START ) || controller.Trigger( Donya::Gamepad::Button::SELECT );
	const bool allowPause	= !Fader::Get().IsExist() && !nowWaiting;
	if ( requestPause && allowPause )
	{
		Donya::Sound::Play( Music::ItemDecision );

		Scene::Result pause{};
		pause.AddRequest( Scene::Request::ADD_SCENE );
		pause.sceneType = Scene::Type::Pause;
		return pause;
	}
	// else

	Scene::Result noop{ Scene::Request::NONE, Scene::Type::Null };
	return noop;
}

#if USE_IMGUI

void SceneGame::UseImGui()
{
	constexpr Donya::Vector2 windowSize{ 720.0f, 508.0f };
	constexpr Donya::Vector2 windowPosLT{ 32.0f, 32.0f  };
	ImGui::SetNextWindowPos ( Donya::ToImVec( windowPosLT ), ImGuiCond_Once );
	ImGui::SetNextWindowSize( Donya::ToImVec( windowSize  ), ImGuiCond_Once );

	if ( !ImGui::BeginIfAllowed() ) { return; }
	// else
	
	const auto data = FetchMember();

	if ( ImGui::TreeNode( u8"ゲーム・メンバーの調整" ) )
	{
		if ( ImGui::TreeNode( u8"セーブデータ" ) )
		{
			if ( ImGui::Button( u8"データをファイルに保存" ) )
			{
				SaveDataAdmin::Get().Save();
			}
			if ( ImGui::Button( u8"データをファイルから読み込み" ) )
			{
				SaveDataAdmin::Get().Load();
			}

			if ( ImGui::Button( u8"今のステージ情報をデータに反映" ) )
			{
				WriteSaveData( stageNumber );
			}

			SaveDataAdmin::Get().ShowImGuiNode( u8"データを直接変更する" );

			if ( ImGui::Button( u8"今のデータを適用してステージを初期化" ) )
			{
				UninitStage();
				InitStage( stageNumber, /* useSaveDataIfValid = */ true );
			}
			
			ImGui::TreePop();
		}

		if ( ImGui::TreeNode( u8"ステージ番号の変更" ) )
		{
			static int transitionTarget = 1;
			ImGui::InputInt( u8"遷移先ステージ番号", &transitionTarget );
			transitionTarget = std::max( 0, transitionTarget );

			auto MakeTransitionTargetName = []( int stageNo )->std::string
			{
				constexpr const char *prompt = u8"に移動する";
				std::string stageName = u8"ステージ[" + std::to_string( stageNo ) + u8"]";
				if ( stageNo < SELECT_STAGE_NO )
				{
					stageName += u8"（セレクト画面）";
				}

				return stageName + prompt;
			};
			if ( ImGui::Button( MakeTransitionTargetName( transitionTarget ).c_str() ) )
			{
				UninitStage();

				stageNumber = transitionTarget;

				InitStage( stageNumber, /* useSaveDataIfValid = */ false );
			}
			// else
			ImGui::TreePop();
		}

		if ( ImGui::TreeNode( u8"カメラ情報" ) )
		{
			ImGui::Checkbox( u8"移動方向を反転する・Ｘ", &isReverseCameraMoveX );
			ImGui::Checkbox( u8"移動方向を反転する・Ｙ", &isReverseCameraMoveY );
			ImGui::Checkbox( u8"回転方向を反転する・Ｘ", &isReverseCameraRotX );
			ImGui::Checkbox( u8"回転方向を反転する・Ｙ", &isReverseCameraRotY );

			auto ShowVec3 = []( const std::string &prefix, const Donya::Vector3 &v )
			{
				ImGui::Text( ( prefix + u8"[X:%5.2f][Y:%5.2f][Z:%5.2f]" ).c_str(), v.x, v.y, v.z );
			};

			const Donya::Vector3 cameraPos = iCamera.GetPosition();
			const Donya::Vector3 playerPos = ( pPlayer ) ? pPlayer->GetPosition() : Donya::Vector3::Zero();
			ShowVec3( u8"現在位置・絶対：", cameraPos );
			ShowVec3( u8"現在位置・相対：", cameraPos - playerPos );
			ImGui::Text( "" );

			const Donya::Vector3 focusPoint = iCamera.GetFocusPoint();
			ShowVec3( u8"注視点位置・絶対：", focusPoint );
			ShowVec3( u8"注視点位置・相対：", focusPoint - playerPos );
			ImGui::Text( "" );

			ImGui::TreePop();
		}

		if ( pPlayerIniter )
		{ pPlayerIniter->ShowImGuiNode( u8"自機の初期化情報", stageNumber ); }
		if ( ImGui::TreeNode( u8"自機の残機数" ) )
		{
			ImGui::DragInt( u8"１以上で有効です", &playerRemains );
			playerRemains = std::max( 0, playerRemains );

			ImGui::TreePop();
		}
		ImGui::Text( "" );

		if ( pBossIniter )
		{
			pBossIniter->ShowImGuiNode( u8"ボスの初期化情報", stageNumber );
			if ( ImGui::Button( u8"ボスを生成しなおす" ) )
			{
				BossInit( stageNumber );
			}
		}
		if ( pBoss )
		{ pBoss->ShowImGuiNode( u8"ボス本体" ); }
		ImGui::Text( "" );

		if ( pObstacles )
		{ pObstacles->ShowImGuiNode( u8"障害物の生成・破棄" ); }
		if ( pEnemies )
		{ pEnemies->ShowImGuiNode( u8"敵の設置・削除" ); }
		ImGui::Text( "" );

		if ( pCameraOption )
		{ pCameraOption->ShowImGuiNode( u8"カメラオプション", stageNumber ); }
		if ( pCheckPoint )
		{ pCheckPoint->ShowImGuiNode( u8"中間地点", stageNumber ); }
		if ( pWarps )
		{ pWarps->ShowImGuiNode( u8"ワープオブジェクト", stageNumber ); }
		if ( pGoal )
		{ pGoal->ShowImGuiNode( u8"ゴールオブジェクト", stageNumber ); }
		if ( pTutorialContainer )
		{ pTutorialContainer->ShowImGuiNode( u8"チュートリアル生成器", stageNumber ); }
		if ( pInfoDrawer )
		{ pInfoDrawer->ShowImGuiNode( u8"ステージ情報描画" ); }
		ImGui::Text( "" );

		if ( pBG )
		{ pBG->ShowImGuiNode( u8"ＢＧ" ); }
		if ( pTerrain )
		{ pTerrain->ShowImGuiNode( u8"地形" ); }
		ImGui::Text( "" );

		// if ( pTutorialSentence )
		// { pTutorialSentence->ShowImGuiNode( u8"チュートリアル画像" ); }
		if ( pClearSentence )
		{ pClearSentence->ShowImGuiNode( u8"クリア画像" ); }
		if ( pClearPerformance )
		{ pClearPerformance->ShowImGuiNode( u8"クリア演出" ); }
		ImGui::Text( "" );

		ImGui::TreePop();
	}

	ImGui::End();
}

#if DEBUG_MODE
void SceneGame::UseDebugImGui()
{
	constexpr Donya::Vector2 windowSize{ 720.0f, 400.0f };
	constexpr Donya::Vector2 windowPosLT{ 32.0f, 572.0f };
	ImGui::SetNextWindowPos ( Donya::ToImVec( windowPosLT ), ImGuiCond_Once );
	ImGui::SetNextWindowSize( Donya::ToImVec( windowSize  ), ImGuiCond_Once );

	if ( !ImGui::BeginIfAllowed( u8"【デバッグモード】" ) ) { return; }
	// else
	
	const auto data = FetchMember();

	ImGui::Text( u8"「Ｆ５キー」を押すと，" );
	ImGui::Text( u8"背景の色が変わりデバッグモードとなります。" );
	ImGui::Checkbox( u8"今，デバッグモードか", &nowDebugMode );

	// Usage texts.
	if ( ImGui::TreeNode( u8"操作方法" ) )
	{
		ImGui::Text( u8"「ＡＬＴキー」を押している間，" );
		ImGui::Text( u8"「左クリック」を押しながらマウス移動で，" );
		ImGui::Text( u8"カメラの回転ができます。" );
		ImGui::Text( u8"「マウスホイール」を押しながらマウス移動で，" );
		ImGui::Text( u8"カメラの並行移動ができます。" );
		ImGui::Text( "" );

		ImGui::Text( u8"「SHIFTキー」を押している間，" );
		ImGui::Text( u8"「マウスホイール上下」で，" );
		ImGui::Text( u8"グリッド線の高さの調整ができます。" );
		ImGui::Text( u8"（調整間隔は「グリッド線の間隔」と同一）" );
		ImGui::Text( u8"「左クリック」で，" );
		ImGui::Text( u8"オブジェクトを選択することができます。" );
		ImGui::Text( u8"「右クリック」で，" );
		ImGui::Text( u8"マウス位置（紫）を保存することができます。" );
		ImGui::Text( "" );

		ImGui::TreePop();
	}
	if ( ImGui::TreeNode( u8"ショートカット" ) )
	{
		ImGui::Text( u8"SHIFT + A : グリッドに沿わせるかどうかの切り替え" );
		ImGui::Text( u8"SHIFT + SPACE : 選択タイプの変更" );
		ImGui::Text( "" );

		ImGui::TreePop();
	}

	ImGui::Text( "" );
	auto GetChoiceTypeName = []( ChoiceType type )
	{
		switch ( type )
		{
		case ChoiceType::Enemy:		return u8"敵";
		case ChoiceType::Obstacle:	return u8"障害物";
		default: break;
		}
		_ASSERT_EXPR( 0, L"Error: Unexpected type!" );
		return u8"ERROR_TYPE";
	};
	ImGui::Text( u8"選択タイプ：%s", GetChoiceTypeName( choiceType ) );
	{
		int intType = scast<int>( choiceType );
		ImGui::SliderInt( u8"選択物の変更", &intType, 0, scast<int>( ChoiceType::TypeCount ) - 1 );
		choiceType  = scast<ChoiceType>( intType );
	}
	ImGui::Text( "" );

	if ( ImGui::TreeNode( u8"マウス位置関連" ) )
	{
		if ( pWsIntersection )
		{ ImGui::DragFloat3( u8"現在の交点位置", &pWsIntersection->x, 0.01f ); }
		else
		{ ImGui::TextDisabled( u8"現在の交点位置" ); }

		if ( pWsClickedPos )
		{ ImGui::DragFloat3( u8"現在のマウス位置",	&pWsClickedPos->x, 0.01f ); }
		else
		{ ImGui::TextDisabled( u8"現在のマウス位置" ); }

		ImGui::Checkbox( u8"マウス位置をグリッドに沿わせるか", &alignToGrid );
		ImGui::Checkbox( u8"マウス位置の計算に地形を含めるか", &alsoIntersectToTerrain );
		ImGui::Text( u8"※地形を含める場合，グリッド平面との交点と比べ近いほうが採用されます" );

		ImGui::TreePop();
	}

	gridline.ShowImGuiNode( u8"グリッド線の調整" );

	ImGui::End();
}
void SceneGame::UseChosenImGui()
{
	if ( !pChosenEnemy && !pChosenObstacle ) { return; }
	// else

	const Donya::Vector4x4 toScreen = MakeScreenTransformMatrix();
	auto WorldToScreen = [&toScreen]( const Donya::Vector3 &world, float fourthParam )
	{
		Donya::Vector4 tmp = toScreen.Mul( world, fourthParam );
		tmp /= tmp.w;
		return tmp.XYZ();
	};

	constexpr Donya::Vector2 chosenWindowSize{ 320.0f, 180.0f };

	if ( pChosenEnemy )
	{
		const Donya::Vector3 ssPos = WorldToScreen( pChosenEnemy->GetPosition(), 1.0f );
		ImGui::SetNextWindowPos ( Donya::ToImVec( ssPos.XY() ), ImGuiCond_Always );
		ImGui::SetNextWindowSize( Donya::ToImVec( chosenWindowSize ), ImGuiCond_Always );

		const std::string caption = u8"選択した敵：" + Enemy::GetKindName( pChosenEnemy->GetKind() );
		if ( ImGui::BeginIfAllowed( caption.c_str() ) )
		{
			pChosenEnemy->ShowImGuiNode( "", /* useTreeNode = */ false );
			ImGui::End();
		}

		if ( pChosenEnemy->ShouldRemove() )
		{
			pChosenEnemy.reset();
		}
	}
	
	if ( pChosenObstacle )
	{
		const Donya::Vector3 ssPos = WorldToScreen( pChosenObstacle->GetPosition(), 1.0f );
		ImGui::SetNextWindowPos ( Donya::ToImVec( ssPos.XY() ), ImGuiCond_Always );
		ImGui::SetNextWindowSize( Donya::ToImVec( chosenWindowSize ), ImGuiCond_Always );

		const std::string caption = u8"選択した敵：" + ObstacleBase::GetModelName( pChosenObstacle->GetKind() );
		if ( ImGui::BeginIfAllowed( caption.c_str() ) )
		{
			pChosenObstacle->ShowImGuiNode( "", /* useTreeNode = */ false );
			ImGui::End();
		}

		if ( pChosenObstacle->ShouldRemove() )
		{
			pChosenObstacle.reset();
		}
	}
}
#endif // DEBUG_MODE

#endif // USE_IMGUI
