#include "SceneGame.h"

#include <vector>

#undef max
#undef min
#include <cereal/types/vector.hpp>

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
#include "Donya/Mouse.h"
#include "Donya/Random.h"
#endif // DEBUG_MODE

#include "Bullet.h"
#include "Common.h"
#include "Fader.h"
#include "FilePath.h"
#include "Music.h"
#include "Obstacles.h"
#include "Parameter.h"

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
		Section playerInitialPos;
		Section goalArea;

		// The threshold member will behave as offset from the player.
		// That offset will function as: threshold = player.pos.y + offset;
		RenderingHelper::TransConstant transparency;

		int waitFrameUntilShowTutorial  = 60;
		int waitFrameUntilSlideTutorial = 60;
		int waitFrameUntilFade = 60;

		Donya::Vector4 goalColor{ 1.0f, 1.0f, 1.0f, 1.0f };

		Donya::Model::Constants::PerScene::DirectionalLight directionalLight;

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
				archive
				(
					CEREAL_NVP( playerInitialPos ),
					CEREAL_NVP( goalArea )
				);
			}
			if ( 2 <= version )
			{
				archive( CEREAL_NVP( waitFrameUntilFade ) );
			}
			if ( 3 <= version )
			{
				archive
				(
					CEREAL_NVP( waitFrameUntilShowTutorial ),
					CEREAL_NVP( waitFrameUntilSlideTutorial )
				);
			}
			if ( 4 <= version )
			{
				archive( CEREAL_NVP( goalColor ) );
			}
			if ( 5 <= version )
			{
				archive
				(
					CEREAL_NVP( directionalLight.color		),
					CEREAL_NVP( directionalLight.direction	)
				);
			}
			if ( 6 <= version )
			{
				archive
				(
					CEREAL_NVP( transparency.zNear				),
					CEREAL_NVP( transparency.zFar				),
					CEREAL_NVP( transparency.lowerAlpha			),
					CEREAL_NVP( transparency.heightThreshold	)
				);
			}
			if ( 7 <= version )
			{
				// archive( CEREAL_NVP( x ) );
			}
		}
	};
}
CEREAL_CLASS_VERSION( Member, 6 )

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
				ImGui::DragInt( u8"ゴールからフェードまでの秒数", &m.waitFrameUntilFade, 1.0f, 1 );
				m.waitFrameUntilShowTutorial	= std::max( 1, m.waitFrameUntilShowTutorial		);
				m.waitFrameUntilSlideTutorial	= std::max( 1, m.waitFrameUntilSlideTutorial	);
				m.waitFrameUntilFade			= std::max( 1, m.waitFrameUntilFade				);

				ImGui::TreePop();
			}

			if ( ImGui::TreeNode( u8"カメラ" ) )
			{
				ImGui::DragFloat ( u8"補間倍率",						&m.camera.slerpFactor,		0.01f );
				ImGui::DragFloat3( u8"自身の座標（自機からの相対）",	&m.camera.offsetPos.x,		0.01f );
				ImGui::DragFloat3( u8"注視点の座標（自機からの相対）",	&m.camera.offsetFocus.x,	0.01f );

				ImGui::TreePop();
			}
			
			if ( ImGui::TreeNode( u8"平行光" ) )
			{
				ImGui::ColorEdit4  ( u8"色",		&m.directionalLight.color.x );
				ImGui::SliderFloat4( u8"方向",	&m.directionalLight.direction.x, -1.0f, 1.0f );

				ImGui::TreePop();
			}

			if ( ImGui::TreeNode( u8"近くのオブジェクトに適用する透明度" ) )
			{
				ImGui::DragFloat( u8"範囲・手前側",	&m.transparency.zNear,		0.01f, 0.0f );
				ImGui::DragFloat( u8"範囲・奥側",	&m.transparency.zFar,		0.01f, 0.0f );
				ImGui::SliderFloat( u8"最低透明度",	&m.transparency.lowerAlpha,	0.0f,  1.0f );
				ImGui::DragFloat( u8"透明を適用する高さ（自機からの相対）",		&m.transparency.heightThreshold, 0.01f );

				ImGui::TreePop();
			}

			if ( ImGui::TreeNode( u8"セクション" ) )
			{
				ImGui::DragFloat3( u8"選択位置", &m.selectingPos.x, 0.1f );
				ImGui::Text( "" );

				auto ShowSectionGUI = [&]( const std::string &nodeCaption, Section *pSection )
				{
					if ( !ImGui::TreeNode( nodeCaption.c_str() ) ) { return; }
					// else

					if ( ImGui::Button( u8"「選択位置」に設定" ) )
					{
						pSection->SetPosition( m.selectingPos );
					}
					pSection->ShowImGuiNode( u8"直接変更" );

					ImGui::TreePop();
				};

				ShowSectionGUI( u8"自機の初期位置",	&m.playerInitialPos	);
				ShowSectionGUI( u8"ゴール位置",		&m.goalArea			);
				ImGui::ColorEdit4( u8"ゴールオブジェクトの色", &m.goalColor.x );

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
	static constexpr int FIRST_STAGE_NO = 1;

	Member FetchMember()
	{
		return ParamGame::Get().Data();
	}
}

void SceneGame::Init()
{
	Donya::Sound::Play( Music::BGM_Game );
#if DEBUG_MODE
	Donya::Sound::AppendFadePoint( Music::BGM_Game, 2.0f, 0.0f, true ); // Too noisy.
#endif // DEBUG_MODE

	bool result{};

	pRenderer = std::make_unique<RenderingHelper>();
	result = pRenderer->Init();
	assert( result );

	ParamGame::Get().Init();
	//const auto data = FetchMember();

	Bullet::BulletAdmin::Get().Init();
	Bullet::LoadBulletsResource();

	//pBG = std::make_unique<BG>();
	//result = pBG->LoadSprites( L"./Data/Images/BG/Back.png", L"./Data/Images/BG/Cloud.png" );
	//assert( result );

	//pTutorialSentence = std::make_unique<TutorialSentence>();
	//pTutorialSentence->Init();
	//result = pTutorialSentence->LoadSprite( L"./Data/Images/Game/Tutorial.png" );
	//assert( result );
	
	//pClearSentence = std::make_unique<ClearSentence>();
	//pClearSentence->Init();
	//result = pClearSentence->LoadSprite( L"./Data/Images/Game/Clear.png" );
	//assert( result );

	//pTerrain = std::make_unique<Terrain>( "./Data/Models/Terrain/Terrain.bin",  "./Data/Models/Terrain/ForCollision/Terrain.bin" );
	//pTerrain->SetWorldConfig( Donya::Vector3{ 0.01f, 0.01f, 0.01f }, Donya::Vector3::Zero() );

	result = ObstacleBase::LoadModels();
	assert( result );

	result = Goal::LoadResource();
	assert( result );

	ObstacleBase::ParameterInit();
	//pGoal = std::make_unique<Goal>();
	//pGoal->Init( data.goalArea.GetPosition() );
	//pObstacles = std::make_unique<ObstacleContainer>();
	//pObstacles->Init( 1 ); // The stage-number is 1-based.(0 is title stage.)

	result = Player::LoadModels();
	assert( result );
	//PlayerInit( stageNumber );

	//CameraInit();

	stageNumber = FIRST_STAGE_NO;
	InitStage( stageNumber );
}
void SceneGame::Uninit()
{
	UninitStage();
	//pTerrain.reset();

	//if ( pGoal		) { pGoal->Uninit();		}
	//if ( pObstacles	) { pObstacles->Uninit();	}

	//PlayerUninit();

	ObstacleBase::ParameterUninit();
	ParamGame::Get().Uninit();
	//Bullet::BulletAdmin::Get().Uninit();

	Donya::Sound::Stop( Music::BGM_Game );
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
#endif // DEBUG_MODE

#if USE_IMGUI
	ParamGame::Get().UseImGui();
	UseImGui();

	ObstacleBase::UseImGui();
	Bullet::UseBulletsImGui();
#endif // USE_IMGUI

	controller.Update();

	pBG->Update( elapsedTime );

	pTerrain->BuildWorldMatrix();

	pGoal->Update( elapsedTime );
	pObstacles->Update( elapsedTime );

	PlayerUpdate( elapsedTime );

	Bullet::BulletAdmin::Get().Update( elapsedTime );

	// Physic updates.
	{
		const auto solids  = pObstacles->GetHitBoxes();
		const auto terrain = pTerrain->GetCollisionModel();
		const Donya::Vector4x4 &terrainMatrix = pTerrain->GetWorldMatrix();

		PlayerPhysicUpdate( solids, terrain.get(), &terrainMatrix );
		Bullet::BulletAdmin::Get().PhysicUpdate( solids, terrain.get(), &terrainMatrix );
	}

	CameraUpdate();

	if ( NowGoalMoment() )
	{
		WaitInit();
		Donya::Sound::Play( Music::UI_Goal );
	}

	TutorialUpdate( elapsedTime );
	WaitUpdate( elapsedTime );

	return ReturnResult();
}

void SceneGame::Draw( float elapsedTime )
{
	elapsedTime = 1.0f; // Disable

	ClearBackGround();

	const Donya::Vector4x4 VP{ iCamera.CalcViewMatrix() * iCamera.GetProjectionMatrix() };
	const auto data = FetchMember();
	
	{
		Donya::Model::Constants::PerScene::Common constant{};
		constant.directionalLight	= data.directionalLight;
		constant.eyePosition		= Donya::Vector4{ iCamera.GetPosition(), 1.0f };
		constant.viewProjMatrix		= VP;
		pRenderer->UpdateConstant( constant );
	}

	{
		const auto &trans = data.transparency;
		RenderingHelper::TransConstant constant{};
		constant.zNear				= trans.zNear;
		constant.zFar				= trans.zFar;
		constant.lowerAlpha			= trans.lowerAlpha;
		constant.heightThreshold	= pPlayer->GetPosition().y + trans.heightThreshold;
		pRenderer->UpdateConstant( constant );
	}

	pRenderer->ActivateDepthStencilModel();
	pRenderer->ActivateRasterizerModel();
	pRenderer->ActivateSamplerModel();
	pRenderer->ActivateConstantScene();
	pRenderer->ActivateConstantTrans();
	{
		// The drawing priority is determined by the priority of the information.

		pRenderer->ActivateShaderNormalSkinning();
		PlayerDraw();
		pRenderer->DeactivateShaderNormalSkinning();

		pRenderer->ActivateShaderNormalStatic();
		Bullet::BulletAdmin::Get().Draw( pRenderer.get(), { 1.0f, 1.0f, 1.0f, 1.0f } );
		pTerrain->Draw( pRenderer.get(), { 1.0f, 1.0f, 1.0f, 1.0f } );
		pGoal->Draw( pRenderer.get(), data.goalColor );
		pObstacles->Draw( pRenderer.get(), { 1.0f, 1.0f, 1.0f, 1.0f } );
		pRenderer->DeactivateShaderNormalStatic();
	}
	pRenderer->DeactivateConstantTrans();
	pRenderer->DeactivateConstantScene();
	pRenderer->DeactivateDepthStencilModel();
	pRenderer->DeactivateRasterizerModel();
	pRenderer->DeactivateSamplerModel();

	if ( Common::IsShowCollision() )
	{
		PlayerDrawHitBox( VP );

		pGoal->DrawHitBox( pRenderer.get(), VP, { 1.0f, 1.0f, 1.0f, 0.5f } );
		Bullet::BulletAdmin::Get().DrawHitBoxes( pRenderer.get(), VP, { 1.0f, 1.0f, 1.0f, 0.5f } );
		pObstacles->DrawHitBoxes( pRenderer.get(), VP, { 1.0f, 1.0f, 1.0f, 0.5f } );
	}
	
	// Drawing to far for avoiding to trans the BG's blue.
	pBG->Draw( elapsedTime );

	// A draw check of these sentences are doing at internal of these methods.
	if ( pTutorialSentence	) { pTutorialSentence->Draw( elapsedTime );	}
	if ( pClearSentence		) { pClearSentence->Draw( elapsedTime );	}

#if DEBUG_MODE
	if ( Common::IsShowCollision() )
	{
		// Sections
		{
			constexpr float selectColorBase  = 0.6f;
			constexpr float selectColorRange = 0.2f;

			static int  selectColorTimer{}; selectColorTimer++;
			float sin = sinf( scast<float>( selectColorTimer ) );
			float selectColor = selectColorBase + sin * selectColorRange;

			// Currently selecting section.
			Section willAddition{ data.selectingPos };
			willAddition.DrawHitBox( pRenderer.get(), VP, { 0.7f, 0.7f, 0.7f, 0.5f } );

			struct Bundle { const Section *pSection; Donya::Vector4 color = { 1.0f, 1.0f, 1.0f, 1.0f }; };
			std::vector<Bundle> drawList
			{
				Bundle{ &willAddition,			{ selectColor, selectColor, selectColor, 0.5f } },
				Bundle{ &data.playerInitialPos,	{ 0.5f, 1.0f, 0.8f, 0.7f } },
				Bundle{ &data.goalArea,			{ 0.8f, 0.8f, 0.1f, 0.7f } },
			};

			for ( const auto &it : drawList )
			{
				it.pSection->DrawHitBox( pRenderer.get(), VP, it.color );
			}
		}
	}
#endif // DEBUG_MODE
}

void SceneGame::InitStage( int stageNo )
{
#if DEBUG_MODE
	// The parameters re-loading are unnecessary, but if when debugging, that is convenience.
	ParamGame::Get().Init();
	Bullet::BulletAdmin::Get().Init();
	Bullet::LoadBulletsResource();
	ObstacleBase::ParameterInit();
#endif // DEBUG_MODE

	bool result{};

	pBG = std::make_unique<BG>();
	result = pBG->LoadSprites( L"./Data/Images/BG/Back.png", L"./Data/Images/BG/Cloud.png" );
	assert( result );

	if ( stageNo == FIRST_STAGE_NO )
	{
		pTutorialSentence = std::make_unique<TutorialSentence>();
		pTutorialSentence->Init();
		result = pTutorialSentence->LoadSprite( L"./Data/Images/Game/Tutorial.png" );
		assert( result );
	}
	else
	{
		pTutorialSentence.reset();
	}

	pClearSentence = std::make_unique<ClearSentence>();
	pClearSentence->Init();
	result = pClearSentence->LoadSprite( L"./Data/Images/Game/Clear.png" );
	assert( result );

	pTerrain = std::make_unique<Terrain>( stageNo );
	pTerrain->SetWorldConfig( Donya::Vector3{ 0.01f, 0.01f, 0.01f }, Donya::Vector3::Zero() );
	// HACK : This scale(0.01f) is magic number :( ...That is adjustment of global scale, for other models.

	pGoal = std::make_unique<Goal>();
	pGoal->Init( stageNo );
	pObstacles = std::make_unique<ObstacleContainer>();
	pObstacles->Init( stageNo );

	PlayerInit( stageNo );

	CameraInit();
}
void SceneGame::UninitStage()
{
	if ( pGoal		) { pGoal->Uninit();		}
	if ( pObstacles	) { pObstacles->Uninit();	}

	pBG.reset();
	pTerrain.reset();
	PlayerUninit();
	pObstacles.reset();
	pGoal.reset();
	pTutorialSentence.reset();
	pClearSentence.reset();

	Bullet::BulletAdmin::Get().Uninit();
}

void SceneGame::CameraInit()
{
	iCamera.Init( Donya::ICamera::Mode::Look );
	iCamera.SetZRange( 0.1f, 1000.0f );
	iCamera.SetFOV( ToRadian( 30.0f ) );
	iCamera.SetScreenSize( { Common::ScreenWidthF(), Common::ScreenHeightF() } );
	AssignCameraPos();
	iCamera.SetProjectionPerspective();

	// I can setting a configuration,
	// but current data is not changed immediately.
	// So update here.
	Donya::ICamera::Controller moveInitPoint{};
	moveInitPoint.SetNoOperation();
	moveInitPoint.slerpPercent = 1.0f;
	iCamera.Update( moveInitPoint );
}
void SceneGame::AssignCameraPos()
{
	if ( !pPlayer ) { return; }
	// else

	const auto data = FetchMember();
	const Donya::Vector3   playerPos = pPlayer->GetPosition();

	iCamera.SetPosition  ( playerPos + data.camera.offsetPos   );
	iCamera.SetFocusPoint( playerPos + data.camera.offsetFocus );
}
void SceneGame::CameraUpdate()
{
	Donya::ICamera::Controller input{};
	input.SetNoOperation();
	input.slerpPercent = FetchMember().camera.slerpFactor;

#if !DEBUG_MODE
	AssignCameraPos();
	iCamera.Update( input );
#else
	if ( !nowDebugMode )
	{
		AssignCameraPos();
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
	pPlayerIniter = std::make_unique<PlayerInitializer>();
	pPlayerIniter->LoadParameter( stageNo );

	pPlayer = std::make_unique<Player>();
	pPlayer->Init( *pPlayerIniter );
}
void SceneGame::PlayerUpdate( float elapsedTime )
{
	if ( !pPlayer ) { return; }
	// else

	if ( pPlayer->IsDead() )
	{
		// Re-generate.
		PlayerInit( stageNumber );
	}

	Donya::Vector2		moveVector{};
	bool useJump		= false;
	bool useOil			= false;

	if ( controller.IsConnected() )
	{
		using Pad		= Donya::Gamepad;

		moveVector		= controller.LeftStick();
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

	if ( nowWaiting )
	{
		moveVector	= Donya::Vector2::Zero();
		useJump		= false;
		useOil		= ( pPlayer->IsOiled() ) ? true : false;
	}

#if DEBUG_MODE
	// Rotate input vector by camera.
	if ( nowDebugMode )
	{
		auto cameraRotation = iCamera.GetOrientation();

		// Disable the rotation of X-axis and Z-axis.
		cameraRotation.x = 0.0f;
		cameraRotation.z = 0.0f;
		cameraRotation.Normalize();

		Donya::Vector3 moveVector3{ moveVector.x, 0.0f, moveVector.y };
		moveVector3 = cameraRotation.RotateVector( moveVector3 );
		moveVector.x = moveVector3.x;
		moveVector.y = moveVector3.z;
	}
#endif // DEBUG_MODE

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

void SceneGame::TutorialUpdate( float elapsedTime )
{
	if ( !pTutorialSentence ) { return; }
	// else

	const int showTiming  = FetchMember().waitFrameUntilShowTutorial;
	const int slideTiming = showTiming + FetchMember().waitFrameUntilSlideTutorial;

	gameTimer++;
	gameTimer = std::min( INT_MAX - 1, gameTimer );

	if ( gameTimer == showTiming )
	{
		pTutorialSentence->Appear();
	}
	if ( gameTimer == slideTiming )
	{
		pTutorialSentence->StartSliding();
	}

	pTutorialSentence->Update( elapsedTime );
}

bool SceneGame::NowGoalMoment() const
{
	if ( !pPlayer	) { return false; }
	if ( !pGoal		) { return false; }
	if ( Fader::Get().IsExist() ) { return false; }
	if ( nowWaiting	) { return false; }
	// else

	// const Donya::AABB goalArea   = FetchMember().goalArea.GetHitBox();
	const Donya::AABB goalArea   = pGoal->GetHitBox();
	const Donya::AABB playerBody = pPlayer->GetHitBox();

	return Donya::AABB::IsHitAABB( playerBody, goalArea );
}

void SceneGame::WaitInit()
{
	clearTimer = 0;
	nowWaiting = true;
	pClearSentence->Appear();
}
void SceneGame::WaitUpdate( float elapsedTime )
{
	if ( !nowWaiting ) { return; }
	// else

	pClearSentence->Update( elapsedTime );

	clearTimer++;
	if ( clearTimer == FetchMember().waitFrameUntilFade )
	{
		StartFade();
	}
}
bool SceneGame::NowWaiting() const
{
	return nowWaiting;
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
	config.closeFrame	= Fader::GetDefaultCloseFrame();;
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

	if ( Fader::Get().IsClosed() )
	{
		Scene::Result change{};
		change.AddRequest( Scene::Request::ADD_SCENE, Scene::Request::REMOVE_ME );
		change.sceneType = Scene::Type::Title;
		return change;
	}

	bool requestPause	= controller.Trigger( Donya::Gamepad::Button::START ) || controller.Trigger( Donya::Gamepad::Button::SELECT ) || Donya::Keyboard::Trigger( 'P' );
	bool allowPause		= !Fader::Get().IsExist();
	if ( 0 && requestPause && allowPause )
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
#include <direct.h>
void SceneGame::UseImGui()
{
	if ( !ImGui::BeginIfAllowed() ) { return; }
	// else
	
	const auto data = FetchMember();

	if ( ImGui::TreeNode( u8"ゲーム・メンバーの調整" ) )
	{
		ImGui::Text( u8"「Ｆ５キー」を押すと，" );
		ImGui::Text( u8"背景の色が変わりデバッグモードとなります。" );
		ImGui::Text( "" );

		if ( ImGui::TreeNode( u8"ステージ番号の変更" ) )
		{
			static int transitionTarget = 1;
			ImGui::InputInt( u8"遷移先ステージ番号", &transitionTarget );
			transitionTarget = std::max( 0, transitionTarget );

			auto MakeTransitionTargetName = []( int stageNo )->std::string
			{
				constexpr const char *prompt = u8"に移動する";
				std::string stageName = u8"ステージ[" + std::to_string( stageNo ) + u8"]";
				if ( stageNo < FIRST_STAGE_NO )
				{
					stageName += u8"（タイトルステージ）";
				}

				return stageName + prompt;
			};
			if ( ImGui::Button( MakeTransitionTargetName( transitionTarget ).c_str() ) )
			{
				UninitStage();

				stageNumber = transitionTarget;

				InitStage( stageNumber );
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

			ImGui::Text( u8"【デバッグモード時のみ有効】" );
			ImGui::Text( u8"「ＡＬＴキー」を押している間のみ，" );
			ImGui::Text( u8"「左クリック」を押しながらマウス移動で，" );
			ImGui::Text( u8"カメラの回転ができます。" );
			ImGui::Text( u8"「マウスホイール」を押しながらマウス移動で，" );
			ImGui::Text( u8"カメラの並行移動ができます。" );
			ImGui::Text( "" );

			ImGui::TreePop();
		}

		if ( pBG )
		{
			pBG->ShowImGuiNode( u8"ＢＧ" );
		}
		if ( pTerrain )
		{
			pTerrain->ShowImGuiNode( u8"地形" );
		}
		if ( pPlayerIniter )
		{
			pPlayerIniter->ShowImGuiNode( u8"自機の初期化情報", stageNumber );
		}
		if ( pGoal )
		{
			pGoal->ShowImGuiNode( u8"ゴールオブジェクト", stageNumber );
		}
		if ( pObstacles )
		{
			pObstacles->ShowImGuiNode( u8"障害物の生成・破棄" );
		}
		if ( pTutorialSentence )
		{
			pTutorialSentence->ShowImGuiNode( u8"チュートリアル画像" );
		}
		if ( pClearSentence )
		{
			pClearSentence->ShowImGuiNode( u8"クリア画像" );
		}

		ImGui::TreePop();
	}
	
	ImGui::End();
}
#endif // USE_IMGUI
