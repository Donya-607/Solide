#include "SceneGame.h"

#include <vector>

#undef max
#undef min
#include <cereal/types/vector.hpp>

#include "Donya/Camera.h"
#include "Donya/Constant.h"
#include "Donya/Donya.h"		// Use GetFPS().
#include "Donya/GeometricPrimitive.h"
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

		struct
		{
			float enableNear = 1.0f; // World space.
			float enableFar	 = 2.0f; // World space.
			float lowerAlpha = 0.0f; // 0.0f ~ 1.0f.
		}
		transparency;

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
				archive
				(
					CEREAL_NVP( transparency.enableNear ),
					CEREAL_NVP( transparency.enableFar  ),
					CEREAL_NVP( transparency.lowerAlpha )
				);
			}
			if ( 3 <= version )
			{
				archive( CEREAL_NVP( waitFrameUntilFade ) );
			}
			if ( 4 <= version )
			{
				// archive( CEREAL_NVP( x ) );
			}
		}
	};
}
CEREAL_CLASS_VERSION( Member, 3 )

class ParamGame : public ParameterBase<ParamGame>
{
public:
	static constexpr const char *ID = "Game";
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

		if ( ImGui::TreeNode( u8"ゲームのパラメータ調整" ) )
		{
			ImGui::DragInt( u8"ゴールからフェードまでの秒数", &m.waitFrameUntilFade, 1.0f, 1 );
			m.waitFrameUntilFade = std::max( 1, m.waitFrameUntilFade );

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

				// vector<Section> process.
				/*
				auto &data = m.sections;
				if ( ImGui::Button( u8"追加" ) )
				{
					data.push_back( { m.addSectionPos } );
				}
				if ( 1 <= data.size() && ImGui::Button( u8"末尾を削除" ) )
				{
					data.pop_back();
				}

				const size_t count = data.size();
				size_t removeIndex = count;
				std::string strIndex{};
				for ( size_t i = 0; i < count; ++i )
				{
					strIndex = u8"[" + std::to_string( i ) + u8"]";

					bool shouldErase = false;
					data[i].ShowImGuiNode( ( u8"セクション" + strIndex ).c_str(), &shouldErase );

					if ( shouldErase ) { removeIndex = i; }
				}

				if ( removeIndex != count )
				{
					data.erase( data.begin() + removeIndex );
				}
				*/

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
		return ParamGame::Get().Data();
	}
}

SceneGame::SceneGame() :
	dirLight(),
	iCamera(),
	controller( Donya::Gamepad::PAD_1 ),
	pTerrain( nullptr ),
	pPlayer( nullptr ),
	pObstacles( nullptr ),
	pClearSentence( nullptr ),
	timer(),
	nowWaiting( false )

#if DEBUG_MODE
	, nowDebugMode( false ),
	isReverseCameraMoveX( false ),
	isReverseCameraMoveY( true  ),
	isReverseCameraRotX( false ),
	isReverseCameraRotY( false )
#endif // DEBUG_MODE

{}
SceneGame::~SceneGame() = default;

void SceneGame::Init()
{
	Donya::Sound::Play( Music::BGM_Game );

	bool result{};

	ParamGame::Get().Init();
	const auto data = FetchMember();

	pClearSentence = std::make_unique<ClearSentence>();
	pClearSentence->Init();
	result = pClearSentence->LoadSprite( L"./Data/Images/Game/Clear.png" );
	assert( result );

	pTerrain = std::make_unique<Terrain>( "./Data/Models/Terrain/Terrain.bin",  "./Data/Models/Terrain/ForCollision/Terrain.bin" );
	pTerrain->SetWorldConfig( Donya::Vector3{ 0.01f, 0.01f, 0.01f }, Donya::Vector3::Zero() );

	result = ObstacleBase::LoadModels();
	assert( result );

	ObstacleBase::ParameterInit();
	pObstacles = std::make_unique<ObstacleContainer>();
	pObstacles->Init( 1 ); // The stage-number is 1-based.(0 is title stage.)

	result = Player::LoadModels();
	assert( result );
	result = Player::LoadShadingObjects();
	assert( result );
	PlayerInit();

	CameraInit();
}
void SceneGame::Uninit()
{
	pTerrain.reset();

	if ( pObstacles ) { pObstacles->Uninit(); }

	PlayerUninit();

	ObstacleBase::ParameterUninit();
	ParamGame::Get().Uninit();

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
#endif // USE_IMGUI

	controller.Update();

	pTerrain->BuildWorldMatrix();

	pObstacles->Update( elapsedTime );

	PlayerUpdate( elapsedTime );

	PlayerPhysicUpdate( pObstacles->GetHitBoxes(), &pTerrain );

	CameraUpdate();

	if ( NowGoalMoment() )
	{
		WaitInit();
	}

	WaitUpdate( elapsedTime );

	return ReturnResult();
}

void SceneGame::Draw( float elapsedTime )
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

	pTerrain->Draw( cameraPos, trans.enableNear, trans.enableFar, trans.lowerAlpha, VP, lightDir, { 1.0f, 1.0f, 1.0f, 1.0f } );

	pObstacles->Draw( cameraPos, trans.enableNear, trans.enableFar, trans.lowerAlpha, VP, lightDir, { 1.0f, 1.0f, 1.0f, 1.0f } );

	pClearSentence->Draw( elapsedTime );

#if DEBUG_MODE
	if ( Common::IsShowCollision() )
	{
		static auto cube = Donya::Geometric::CreateCube();

		static Donya::Vector4 lightDir{ 0.0f,-1.0f, 1.0f, 0.0f };

		// Sections
		{
			constexpr float selectColorBase  = 0.6f;
			constexpr float selectColorRange = 0.2f;

			static int  selectColorTimer{}; selectColorTimer++;
			float sin = sinf( scast<float>( selectColorTimer ) );
			float selectColor = selectColorBase + sin * selectColorRange;

			// Currently selecting section.
			Section willAddition{ data.selectingPos };
			willAddition.DrawHitBox( VP, { 0.7f, 0.7f, 0.7f, 0.5f } );

			struct Bundle { const Section *pSection; Donya::Vector4 color = { 1.0f, 1.0f, 1.0f, 1.0f }; };
			std::vector<Bundle> drawList
			{
				Bundle{ &willAddition,			{ selectColor, selectColor, selectColor, 0.5f } },
				Bundle{ &data.playerInitialPos,	{ 0.5f, 1.0f, 0.8f, 0.7f } },
				Bundle{ &data.goalArea,			{ 0.8f, 0.8f, 0.1f, 0.7f } },
			};

			for ( const auto &it : drawList )
			{
				it.pSection->DrawHitBox( VP, it.color );
			}
		}

		// Ground likes.
		if ( 0 )
		{
			static Donya::Vector3 pos  { 0.0f, -1.0f, 0.0f };
			static Donya::Vector3 size { 70.0f, 1.0f, 120.0f };
			static Donya::Vector4 color{ 1.0f, 1.0f, 1.0f, 1.0f };

			Donya::Vector4x4 W
			{
				size.x, 0.0f,   0.0f,   0.0f,
				0.0f,   size.y, 0.0f,   0.0f,
				0.0f,   0.0f,   size.z, 0.0f,
				pos.x,  pos.y,  pos.z,  1.0f
			};

			cube.Render
			(
				nullptr, true, true,
				W * V * P, W,
				lightDir, color
			);
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
					ImGui::SliderFloat3( u8"板ポリのライト方向", &lightDir.x, -1.0f, 1.0f );
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

void SceneGame::PlayerInit()
{
	const auto data = FetchMember();
	pPlayer = std::make_unique<Player>();
	pPlayer->Init( data.playerInitialPos.GetPosition() );
}
void SceneGame::PlayerUpdate( float elapsedTime )
{
	if ( !pPlayer ) { return; }
	// else

	if ( pPlayer->IsDead() )
	{
		// Re-generate.
		PlayerInit();
	}

	Donya::Vector2		moveVector{};
	bool useJump		= false;
	bool useOil			= false;

	if ( controller.IsConnected() )
	{
		using Pad		= Donya::Gamepad;

		moveVector		= controller.LeftStick();
		useJump			= controller.Trigger( Pad::A );
		// useOil			= controller.Press( Pad::B ) || controller.Press( Pad::Y );
		useOil			= controller.Trigger( Pad::B ) || controller.Trigger( Pad::Y );
	}
	else
	{
		moveVector.x	+= Donya::Keyboard::Press( VK_RIGHT	) ? +1.0f : 0.0f;
		moveVector.x	+= Donya::Keyboard::Press( VK_LEFT	) ? -1.0f : 0.0f;
		moveVector.y	+= Donya::Keyboard::Press( VK_UP	) ? +1.0f : 0.0f; // Front is Plus.
		moveVector.y	+= Donya::Keyboard::Press( VK_DOWN	) ? -1.0f : 0.0f; // Front is Plus.
		moveVector.Normalize();

		useJump			=  Donya::Keyboard::Trigger( 'Z' );
		// useOil			=  Donya::Keyboard::Press( 'X' );
		useOil			=  Donya::Keyboard::Trigger( 'X' );
	}

#if DEBUG_MODE

#if USE_IMGUI
	{
		static bool disallowInput = false;
		if ( ImGui::BeginIfAllowed( "ClearConfig" ) )
		{
			ImGui::Checkbox( u8"クリア後の入力を無効にする", &disallowInput );
			ImGui::End();
		}
		if ( disallowInput && nowWaiting )
		{
			Player::Input input{};
			input.moveVectorXZ	= Donya::Vector2::Zero();
			input.useJump		= false;
			input.useOil		= false;
			pPlayer->Update( elapsedTime, input );
			return;
		}
	}
#endif // USE_IMGUI

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

	// Some Test
	if ( 0 )
	{
		static Donya::Quaternion base{};
		static bool enableAdd = false;
		Donya::Quaternion rotation = Donya::Quaternion::Make( Donya::Vector3::Up(), ToRadian( 1.0f ) );

		if ( enableAdd )
		{
			base.RotateBy( rotation );
		}

		if ( Donya::Keyboard::Trigger( 'R' ) )
		{
			Donya::Quaternion rotation = Donya::Quaternion::Make( Donya::Vector3::Front(), ToRadian( -45.0f ) );
			base.RotateBy( rotation );
		}
		if ( Donya::Keyboard::Trigger( 'T' ) )
		{
			Donya::Quaternion rotation = Donya::Quaternion::Make( Donya::Vector3::Front(), ToRadian( 45.0f ) );
			base.RotateBy( rotation );
		}
		if ( Donya::Keyboard::Trigger( 'F' ) )
		{
			Donya::Quaternion rotation = Donya::Quaternion::Make( Donya::Vector3::Right(), ToRadian( -45.0f ) );
			base.RotateBy( rotation );
		}
		if ( Donya::Keyboard::Trigger( 'G' ) )
		{
			Donya::Quaternion rotation = Donya::Quaternion::Make( Donya::Vector3::Right(), ToRadian( 45.0f ) );
			base.RotateBy( rotation );
		}

		auto nowEuler = base.GetEulerAngles();
		float x = ToDegree( nowEuler.x );
		float y = ToDegree( nowEuler.y );
		float z = ToDegree( nowEuler.z );
		auto tmp = 0;
		tmp++;

		auto CalcDifference = []( const Donya::Quaternion &lhs, const Donya::Quaternion &rhs )
		{
			// See https://qiita.com/Guvalif/items/767cf45f19c36e242fc6
			return lhs.Inverse().RotateBy( rhs );
		};
		auto diff = CalcDifference( Donya::Quaternion::Make( Donya::Vector3::Up(), ToRadian( 180.0f ) ), base );
		auto diffEuler = base.GetEulerAngles();
		float dx = ToDegree( diffEuler.x );
		float dy = ToDegree( diffEuler.y );
		float dz = ToDegree( diffEuler.z );

		ImGui::Begin( u8"テスト" );

		ImGui::Text( u8"値：[X:%5.3f][Y:%5.3f][Z:%5.3f][W:%5.3f]", base.x, base.y, base.z, base.w );
		ImGui::Text( u8"Degree角度：[X:%5.3f][Y:%5.3f][Z:%5.3f]", x, y, z );
		ImGui::Text( u8"差分：[X:%5.3f][Y:%5.3f][Z:%5.3f][W:%5.3f]", diff.x, diff.y, diff.z, diff.w );
		ImGui::Text( u8"差分角度：[X:%5.3f][Y:%5.3f][Z:%5.3f]", dx, dy, dz );
		ImGui::Checkbox( u8"インクリメントを有効にする", &enableAdd );
		ImGui::Text( u8"Ｒ：Ｚ軸でー４５度回転" );
		ImGui::Text( u8"Ｔ：Ｚ軸で４５度回転" );
		ImGui::Text( u8"Ｆ：Ｘ軸でー４５度回転" );
		ImGui::Text( u8"Ｇ：Ｘ軸で４５度回転" );

		static Donya::Vector3 front{ 0, 0, 1 };
		static Donya::Vector3 random{ 1, 0, 0 };
		if ( ImGui::Button( u8"ランダム生成" ) )
		{
			front.x = Donya::Random::GenerateFloat( -1.0f, 1.0f );
			front.y = 0.0f;
			front.z = Donya::Random::GenerateFloat( -1.0f, 1.0f );
			front.Normalize();

			random.x = Donya::Random::GenerateFloat( -1.0f, 1.0f );
			random.y = 0.0f;
			random.z = Donya::Random::GenerateFloat( -1.0f, 1.0f );
			random.Normalize();
		}

		auto ToXZ = []( const  Donya::Vector3 &v ) { return Donya::Vector2{ v.x, v.z }; };

		Donya::Vector2 xzFront = ToXZ( front );
		Donya::Vector2 xzRand  = ToXZ( random );
		ImGui::Text( u8"前方向：[X:%5.3f][Y:%5.3f]",		xzFront.x, xzFront.y );
		ImGui::Text( u8"ランダム：[X:%5.3f][Y:%5.3f]",	xzRand.x, xzRand.y   );
		ImGui::Text( u8"外積・前 x ラ：[%5.3f]", Donya::Cross( xzFront, xzRand ) );
		ImGui::Text( u8"外積・ラ x 前：[%5.3f]", Donya::Cross( xzRand, xzFront ) );
		ImGui::Text( ( Donya::Cross( xzFront, xzRand ) < 0.0f ) ? u8"ラは右です" : u8"ラは左です" );
		ImGui::Text( u8"内積：[%5.3f]", Donya::Dot( xzFront, xzRand ) );
		ImGui::Text( u8"acos：[%5.3f]", acosf( Donya::Dot( xzFront, xzRand ) ) );
		ImGui::Text( u8"Degree：[%5.3f]", ToDegree( acosf( Donya::Dot( xzFront, xzRand ) ) ) );

		ImGui::End();
	}
#endif // DEBUG_MODE

	Player::Input input{};
	input.moveVectorXZ	= moveVector;
	input.useJump		= useJump;
	input.useOil		= useOil;

	pPlayer->Update( elapsedTime, input );
}
void SceneGame::PlayerPhysicUpdate( const std::vector<Donya::AABB> &solids, const std::unique_ptr<Terrain> *ppTerrain )
{
	if ( !pPlayer ) { return; }
	if ( !ppTerrain ) { return; }
	// else

	const auto &pTerrain = *ppTerrain;
	if ( !pTerrain ) { return; }
	// else

	const Donya::Vector4x4 terrainMatrix = pTerrain->GetWorldMatrix();
	pPlayer->PhysicUpdate( solids, pTerrain->GetCollisionMesh().get(), &terrainMatrix );
}
void SceneGame::PlayerDraw( const Donya::Vector4x4 &matVP, const Donya::Vector4 &cameraPos, const Donya::Vector4 &lightDir )
{
	if ( !pPlayer ) { return; }
	// else
	pPlayer->Draw( matVP, cameraPos, lightDir );
}
void SceneGame::PlayerUninit()
{
	if ( !pPlayer ) { return; }
	// else
	pPlayer->Uninit();
	pPlayer.reset();
}

bool SceneGame::NowGoalMoment() const
{
	if ( !pPlayer ) { return false; }
	if ( Fader::Get().IsExist() ) { return false; }
	if ( nowWaiting ) { return false; }
	// else

	const Donya::AABB goalArea   = FetchMember().goalArea.GetHitBox();
	const Donya::AABB playerBody = pPlayer->GetHitBox();

	return Donya::AABB::IsHitAABB( playerBody, goalArea );
}

void SceneGame::WaitInit()
{
	timer = 0;
	nowWaiting = true;
	pClearSentence->Appear();
}
void SceneGame::WaitUpdate( float elapsedTime )
{
	if ( !nowWaiting ) { return; }
	// else

	pClearSentence->Update( elapsedTime );

	timer++;
	if ( timer == FetchMember().waitFrameUntilFade )
	{
		StartFade();
	}
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

		if ( pTerrain )
		{
			pTerrain->ShowImGuiNode( u8"地形" );
		}
		if ( pObstacles )
		{
			pObstacles->ShowImGuiNode( u8"障害物の生成・破棄" );
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
