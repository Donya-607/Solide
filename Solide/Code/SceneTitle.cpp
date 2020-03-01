#include "SceneTitle.h"

#include <vector>

#include "Donya/GeometricPrimitive.h"
#include "Donya/Keyboard.h"
#include "Donya/Mouse.h"
#include "Donya/Sound.h"
#include "Donya/Sprite.h"
#include "Donya/Useful.h"

#include "Common.h"
#include "Fader.h"
#include "FilePath.h"
#include "Music.h"
#include "Parameter.h"

#undef max
#undef min

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

		struct
		{
			float enableNear = 1.0f; // World space.
			float enableFar	 = 2.0f; // World space.
			float lowerAlpha = 0.0f; // 0.0f ~ 1.0f.
		}
		transparency;

		Donya::Vector3 playerInitialPos;

		int waitFrameUntilFade = 60;

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
				CEREAL_NVP( camera.offsetFocus ),
				CEREAL_NVP( transparency.enableNear ),
				CEREAL_NVP( transparency.enableFar ),
				CEREAL_NVP( transparency.lowerAlpha ),
				CEREAL_NVP( playerInitialPos )
			);

			if ( 1 <= version )
			{
				archive( CEREAL_NVP( waitFrameUntilFade ) );
			}
			if ( 2 <= version )
			{
				// archive( CEREAL_NVP( x ) );
			}
		}
	};
}
CEREAL_CLASS_VERSION( Member, 1 )

class ParamTitle : public ParameterBase<ParamTitle>
{
public:
	static constexpr const char *ID = "Title";
private:
	Member m;
public:
	void Init()     override
	{
	#if DEBUG_MODE
		LoadJson();
	#else
		LoadBin();
	#endif // DEBUG_MODE
	}
	void Uninit()   override {}
	Member Data()   const { return m; }
private:
	void LoadBin()  override
	{
		constexpr bool fromBinary = true;
		Donya::Serializer::Load( m, GenerateSerializePath( ID, fromBinary ).c_str(), ID, fromBinary );
	}
	void LoadJson() override
	{
		constexpr bool fromBinary = false;
		Donya::Serializer::Load( m, GenerateSerializePath( ID, fromBinary ).c_str(), ID, fromBinary );
	}
	void SaveBin()  override
	{
		constexpr bool fromBinary = true;
		Donya::Serializer::Save( m, GenerateSerializePath( ID, fromBinary ).c_str(), ID, fromBinary );
	}
	void SaveJson() override
	{
		constexpr bool fromBinary = false;
		Donya::Serializer::Save( m, GenerateSerializePath( ID, fromBinary ).c_str(), ID, fromBinary );
	}
public:
#if USE_IMGUI
	void UseImGui() override
	{
		if ( !ImGui::BeginIfAllowed() ) { return; }
		// else

		if ( ImGui::TreeNode( u8"タイトルのパラメータ調整" ) )
		{
			ImGui::DragFloat3( u8"自機の初期位置", &m.playerInitialPos.x, 0.01f );
			ImGui::DragInt( u8"入力から，フェードまでの待機時間（フレーム）", &m.waitFrameUntilFade, 1.0f, 1 );
			m.waitFrameUntilFade = std::max( 1, m.waitFrameUntilFade );
			ImGui::Text( "" );

			if ( ImGui::TreeNode( u8"カメラ" ) )
			{
				ImGui::DragFloat ( u8"補間倍率",						&m.camera.slerpFactor,		0.01f );
				ImGui::DragFloat3( u8"自身の座標（自機からの相対）",	&m.camera.offsetPos.x,		0.01f );
				ImGui::DragFloat3( u8"注視点の座標（自機からの相対）",	&m.camera.offsetFocus.x,	0.01f );

				ImGui::TreePop();
			}

			if ( ImGui::TreeNode( u8"近くのオブジェクトに適用する透明度" ) )
			{
				ImGui::DragFloat( u8"範囲・手前側",	&m.transparency.enableNear,	0.01f, 0.0f );
				ImGui::DragFloat( u8"範囲・奥側",	&m.transparency.enableFar,	0.01f, 0.0f );
				ImGui::SliderFloat( u8"最低透明度", &m.transparency.lowerAlpha, 0.0f, 1.0f );

				ImGui::TreePop();
			}

			ShowIONode();

			ImGui::TreePop();
		}

		ImGui::End();
	}
#endif // USE_IMGUI
};

namespace
{
	Member FetchMember()
	{
		return ParamTitle::Get().Data();
	}
}

SceneTitle::SceneTitle() :
	iCamera(),
	controller( Donya::Gamepad::PAD_1 ),
	pBG( nullptr ),
	pTerrain( nullptr ),
	pPlayer( nullptr ),
	pObstacles( nullptr ),
	pSentence( nullptr ),
	timer(),
	nowWaiting( false )

#if DEBUG_MODE
	, nowDebugMode( false ),
	isReverseCameraMoveX( false ),
	isReverseCameraMoveY( true ),
	isReverseCameraRotX( false ),
	isReverseCameraRotY( false )
#endif // DEBUG_MODE

{}
SceneTitle::~SceneTitle() = default;

void SceneTitle::Init()
{
	Donya::Sound::Play( Music::BGM_Title );

	timer = 0;
	nowWaiting = false;

	bool result{};

	ParamTitle::Get().Init();
	const auto data = FetchMember();

	pBG = std::make_unique<BG>();
	result = pBG->LoadSprites( L"./Data/Images/BG/Back.png", L"./Data/Images/BG/Cloud.png" );
	assert( result );

	pSentence = std::make_unique<TitleSentence>();
	pSentence->Init();
	result = pSentence->LoadSprites( L"./Data/Images/Title/Logo.png", L"./Data/Images/Title/Prompt.png" );
	assert( result );

	pTerrain = std::make_unique<Terrain>( "./Data/Models/Terrain/TitleTerrain.bin", "./Data/Models/Terrain/ForCollision/TitleTerrain.bin" );
	pTerrain->SetWorldConfig( Donya::Vector3{ 1.0f, 1.0f, 1.0f }, Donya::Vector3::Zero() );

	result = ObstacleBase::LoadModels();
	assert( result );

	ObstacleBase::ParameterInit();
	pObstacles = std::make_unique<ObstacleContainer>();
	pObstacles->Init( 0 );

	result = Player::LoadModels();
	assert( result );
	result = Player::LoadShadingObjects();
	assert( result );
	PlayerInit();

	CameraInit();
}
void SceneTitle::Uninit()
{
	pTerrain.reset();

	if ( pObstacles ) { pObstacles->Uninit(); }

	PlayerUninit();

	ObstacleBase::ParameterUninit();
	ParamTitle::Get().Uninit();

	Donya::Sound::Stop( Music::BGM_Title );
}

Scene::Result SceneTitle::Update( float elapsedTime )
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
	ParamTitle::Get().UseImGui();
	UseImGui();

	ObstacleBase::UseImGui();
#endif // USE_IMGUI

	controller.Update();

	pBG->Update( elapsedTime );

	pTerrain->BuildWorldMatrix();

	pObstacles->Update( elapsedTime );

	PlayerUpdate( elapsedTime );

	PlayerPhysicUpdate( pObstacles->GetHitBoxes(), &pTerrain );

	CameraUpdate();

	if ( IsRequiredAdvance() )
	{
		nowWaiting = true;
		WaitInit();
	}

	if ( nowWaiting )
	{
		WaitUpdate( elapsedTime );
	}

	pSentence->Update( elapsedTime );

	return ReturnResult();
}

void SceneTitle::Draw( float elapsedTime )
{
	elapsedTime = 1.0f; // Disable

	ClearBackGround();

	const Donya::Vector4   cameraPos{ iCamera.GetPosition(), 1.0f };
	const Donya::Vector4   lightDir{ 0.0f, -1.0f, 0.0f, 0.0f };
	const Donya::Vector4x4 V{ iCamera.CalcViewMatrix() };
	const Donya::Vector4x4 P{ iCamera.GetProjectionMatrix() };
	const Donya::Vector4x4 VP{ V * P };
	const auto data = FetchMember();
	const auto &trans = data.transparency;

	// The drawing priority is determined by the priority of the information.

	PlayerDraw( VP, cameraPos, lightDir );

	pObstacles->Draw( cameraPos, trans.enableNear, trans.enableFar, trans.lowerAlpha, VP, lightDir, { 1.0f, 1.0f, 1.0f, 1.0f } );

	pTerrain->Draw( cameraPos, trans.enableNear, trans.enableFar, trans.lowerAlpha, VP, lightDir, { 1.0f, 1.0f, 1.0f, 1.0f } );

	// Drawing to far for avoiding to trans the BG's blue.
	pBG->Draw( elapsedTime );

	pSentence->Draw( elapsedTime );

#if DEBUG_MODE
	if ( Common::IsShowCollision() )
	{
		static auto cube = Donya::Geometric::CreateCube();
		auto DrawCube = [&]( const Donya::Vector3 &scale, const Donya::Vector3 &pos, const Donya::Vector4 &color )
		{
			Donya::Vector4x4 W{};
			W._11 = scale.x;
			W._22 = scale.y;
			W._33 = scale.z;
			W._41 = pos.x;
			W._42 = pos.y;
			W._43 = pos.z;

			cube.Render
			(
				nullptr, true, true,
				W * V * P, W,
				lightDir, color
			);
		};

		// Sections
		{
			struct Bundle { Donya::Vector3 pos; Donya::Vector4 color = { 1.0f, 1.0f, 1.0f, 1.0f }; };
			std::vector<Bundle> drawList
			{
				Bundle{ data.selectingPos,		{ 0.5f, 0.5f, 0.5f, 0.6f } },
				Bundle{ data.playerInitialPos,	{ 0.5f, 1.0f, 0.8f, 0.7f } },
			};

			for ( const auto &it : drawList )
			{
				DrawCube( { 1.0f, 1.0f, 1.0f }, it.pos, it.color );
			}
		}

		// Drawing TextureBoard Demo.
		if ( 0 )
		{
			constexpr const wchar_t *texturePath	= L"./Data/Images/Rights/FMOD Logo White - Black Background.png";
			static Donya::Geometric::TextureBoard	texBoard = Donya::Geometric::CreateTextureBoard( texturePath );
			static Donya::Vector2	texPos{};
			static Donya::Vector2	texSize{ 728.0f, 192.0f };

			static Donya::Vector3	boardScale{ 1.0f, 1.0f, 1.0f };
			static Donya::Vector3	boardPos{};
			static float			boardRadian{};

			static Donya::Vector4	boardColor{ 1.0f, 1.0f, 1.0f, 1.0f };

		#if USE_IMGUI

			if ( ImGui::BeginIfAllowed() )
			{
				if ( ImGui::TreeNode( u8"板ポリ描画テスト" ) )
				{
					ImGui::DragFloat2( u8"切り取り位置・左上", &texPos.x );
					ImGui::DragFloat2( u8"切り取りサイズ・全体", &texSize.x );
					ImGui::Text( "" );
					ImGui::DragFloat3( u8"スケール", &boardScale.x );
					ImGui::DragFloat3( u8"ワールド位置", &boardPos.x );
					ImGui::DragFloat( u8"Z回転", &boardRadian, ToRadian( 10.0f ) );
					ImGui::Text( "" );
					ImGui::ColorEdit4( u8"ブレンド色", &boardColor.x );
					ImGui::Text( "" );

					ImGui::TreePop();
				}

				ImGui::End();
			}

		#endif // USE_IMGUI

			Donya::Vector4x4 TB_S = Donya::Vector4x4::MakeScaling( boardScale );
			Donya::Vector4x4 TB_R = texBoard.CalcBillboardRotation( ( iCamera.GetPosition() - boardPos ).Normalized(), boardRadian );
			Donya::Vector4x4 TB_T = Donya::Vector4x4::MakeTranslation( boardPos );
			Donya::Vector4x4 TB_W = TB_S * TB_R * TB_T;

			texBoard.RenderPart
			(
				texPos, texSize,
				nullptr, // Specify use library's device-context.
				/* useDefaultShading = */ true,
				/* isEnableFill      = */ true,
				( TB_W * VP ), TB_W,
				lightDir, boardColor
			);
		}
	}
#endif // DEBUG_MODE
}

void SceneTitle::CameraInit()
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
void SceneTitle::AssignCameraPos()
{
	if ( !pPlayer ) { return; }
	// else

	const auto data = FetchMember();
	const Donya::Vector3   playerPos = pPlayer->GetPosition();

	iCamera.SetPosition  ( playerPos + data.camera.offsetPos   );
	iCamera.SetFocusPoint( playerPos + data.camera.offsetFocus );
}
void SceneTitle::CameraUpdate()
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

void SceneTitle::PlayerInit()
{
	const auto data = FetchMember();
	pPlayer = std::make_unique<Player>();
	pPlayer->Init( data.playerInitialPos );
}
void SceneTitle::PlayerUpdate( float elapsedTime )
{
	if ( !pPlayer ) { return; }
	// else

	if ( pPlayer->IsDead() )
	{
		// Re-generate.
		PlayerInit();
	}

	const Donya::Quaternion	nowOrientation	= pPlayer->GetOrientation();
	const Donya::Vector3	nowRight		= nowOrientation.LocalRight();

	bool useJump = false;
	if ( controller.IsConnected() )
	{
		useJump = controller.Trigger( Donya::Gamepad::A );
	}
	else
	{
		useJump = Donya::Keyboard::Trigger( 'Z' );
	}

	Player::Input input{};
	input.moveVectorXZ	= Donya::Vector2{ nowRight.x, nowRight.z };
	input.useJump		= useJump;
	input.useOil		= ( pPlayer->IsOiled() ) ? false : true;

	pPlayer->Update( elapsedTime, input );
}
void SceneTitle::PlayerPhysicUpdate( const std::vector<Donya::AABB> &solids, const std::unique_ptr<Terrain> *ppTerrain )
{
	if ( !pPlayer   ) { return; }
	if ( !ppTerrain ) { return; }
	// else

	const auto &pTerrain = *ppTerrain;
	if ( !pTerrain ) { return; }
	// else

	const Donya::Vector4x4 terrainMatrix = pTerrain->GetWorldMatrix();
	pPlayer->PhysicUpdate( solids, pTerrain->GetCollisionMesh().get(), &terrainMatrix );
}
void SceneTitle::PlayerDraw( const Donya::Vector4x4 &matVP, const Donya::Vector4 &cameraPos, const Donya::Vector4 &lightDir )
{
	if ( !pPlayer ) { return; }
	// else
	pPlayer->Draw( matVP, cameraPos, lightDir );
}
void SceneTitle::PlayerUninit()
{
	if ( !pPlayer ) { return; }
	// else
	pPlayer->Uninit();
	pPlayer.reset();
}

bool SceneTitle::IsRequiredAdvance() const
{
	if ( Fader::Get().IsExist() ) { return false; }
	if ( nowWaiting ) { return false; }
	// else

	return	( controller.IsConnected() )
			? controller.Trigger( Donya::Gamepad::A )
			: Donya::Keyboard::Trigger( 'Z' );
}
void SceneTitle::WaitInit()
{
	pSentence->AdvanceState();
	Donya::Sound::Play( Music::UI_StartGame );
}
void SceneTitle::WaitUpdate( float elapsedTime )
{
	timer++;

	if ( timer == FetchMember().waitFrameUntilFade )
	{
		StartFade();
	}
}

void SceneTitle::ClearBackGround() const
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

void SceneTitle::StartFade() const
{
	Fader::Configuration config{};
	config.type			= Fader::Type::Gradually;
	config.closeFrame	= Fader::GetDefaultCloseFrame();;
	config.SetColor( Donya::Color::Code::BLACK );
	Fader::Get().StartFadeOut( config );
}

Scene::Result SceneTitle::ReturnResult()
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
		change.sceneType = Scene::Type::Game;
		return change;
	}
	// else

	Scene::Result noop{ Scene::Request::NONE, Scene::Type::Null };
	return noop;
}

#if USE_IMGUI

void SceneTitle::UseImGui()
{
	if ( ImGui::BeginIfAllowed() )
	{
		if ( ImGui::TreeNode( u8"タイトル・メンバーの調整" ) )
		{
			ImGui::Text( u8"「Ｆ５キー」を押すと，" );
			ImGui::Text( u8"背景の色が変わりデバッグモードとなります。" );
			ImGui::Text( "" );

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
			if ( pObstacles )
			{
				pObstacles->ShowImGuiNode( u8"障害物の生成・破棄" );
			}
			if ( pSentence )
			{
				pSentence->ShowImGuiNode( u8"タイトル画像" );
			}

			ImGui::TreePop();
		}

		ImGui::End();
	}
}

#endif // USE_IMGUI
