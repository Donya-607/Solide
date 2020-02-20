#include "SceneGame.h"

#include <vector>

#include "Donya/Camera.h"
#include "Donya/CBuffer.h"
#include "Donya/Constant.h"
#include "Donya/Donya.h"		// Use GetFPS().
#include "Donya/GeometricPrimitive.h"
#include "Donya/Keyboard.h"
#include "Donya/Mouse.h"
#include "Donya/Quaternion.h"
#include "Donya/Serializer.h"
#include "Donya/Sound.h"
#include "Donya/Sprite.h"
#include "Donya/Useful.h"
#include "Donya/UseImGui.h"
#include "Donya/Vector.h"

#include "Common.h"
#include "Fader.h"
#include "FilePath.h"
#include "Music.h"
#include "Parameter.h"

namespace
{
	struct Member
	{
		struct
		{
			float slerpFactor = 0.2f;
			float basePosY = 0.0f;		// The Y component of camera position is not related to player position.
			Donya::Vector3 offsetPos;	// The offset of position from the player position.
			Donya::Vector3 offsetFocus;	// The offset of focus from the player position.
		}
		camera;	// The X and Z component is: player-position + offset. The Y component is:basePosY.
	public:
		bool  isValid = true; // Use for validation of dynamic_cast. Do not serialize.
	private:
		friend class cereal::access;
		template<class Archive>
		void serialize( Archive &archive, std::uint32_t version )
		{
			if ( version <= 1 )
			{
				archive
				(
					CEREAL_NVP( camera.basePosY ),
					CEREAL_NVP( camera.offsetPos ),
					CEREAL_NVP( camera.offsetFocus )
				);
			}
			if ( version == 1 )
			{
				archive
				(
					CEREAL_NVP( camera.slerpFactor )
				);
			}
		}
	};
}
CEREAL_CLASS_VERSION( Member, 1 )

class ParamGame : public ParameterBase
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
			if ( ImGui::TreeNode( u8"カメラ" ) )
			{
				ImGui::Text( u8"カメラ位置：" );
				ImGui::Text( u8"[Ｘ：自機の座標＋自身の座標]" );
				ImGui::Text( u8"[Ｙ：自機のＹ座標]" );
				ImGui::Text( u8"[Ｚ：自機の座標＋自身の座標]" );
				ImGui::Text( "" );

				ImGui::DragFloat ( u8"補間倍率",						&m.camera.slerpFactor,		0.01f );
				ImGui::DragFloat ( u8"自身のＹ座標（絶対）",			&m.camera.basePosY,			0.01f );
				ImGui::DragFloat3( u8"自身の座標（自機からの相対）",	&m.camera.offsetPos.x,		0.01f );
				ImGui::DragFloat3( u8"注視点の座標（自機からの相対）",	&m.camera.offsetFocus.x,	0.01f );

				ImGui::TreePop();
			}

			auto ShowAABB = []( const std::string &prefix, Donya::AABB *p )
			{
				ImGui::DragFloat3( ( prefix + u8"：中心のオフセット" ).c_str(),		&p->pos.x,  0.01f );
				ImGui::DragFloat3( ( prefix + u8"：サイズ（半分を指定）" ).c_str(),	&p->size.x, 0.01f );
				ImGui::Checkbox  ( ( prefix + u8"：判定を有効にする" ).c_str(),		&p->exist );
			};

			ParameterBase::ShowIONode( this );

			ImGui::TreePop();
		}

		ImGui::End();
	}
#endif // USE_IMGUI
};

namespace
{
	std::unique_ptr<ParameterBase> *FindHelper()
	{
		return ParameterStorage::Get().Find( ParamGame::ID );
	}

	Member FetchMember()
	{
		Member nil{}; nil.isValid = false;

		auto  pBase = FindHelper();
		if ( !pBase ) { return nil; }
		// else

		ParamGame *pDerived = dynamic_cast<ParamGame *>( pBase->get() );
		return ( pDerived ) ? pDerived->Data() : nil;
	}
}

SceneGame::SceneGame() :
	dirLight(),
	iCamera(),
	controller( Donya::Gamepad::PAD_1 ),
	player()
{}
SceneGame::~SceneGame() = default;

void SceneGame::Init()
{
	Donya::Sound::Play( Music::BGM_Game );

	if ( !FindHelper() )
	{
		ParameterStorage::Get().Register<ParamGame>( ParamGame::ID );
	}
	if ( FindHelper() )
	{
		( *FindHelper() )->Init();
	}

	CameraInit();

	player.Init();
}
void SceneGame::Uninit()
{
	if ( FindHelper() )
	{
		( *FindHelper() )->Uninit();
	}

	Donya::Sound::Stop( Music::BGM_Game );
}

Scene::Result SceneGame::Update( float elapsedTime )
{
	elapsedTime = 1.0f; // Disable

#if USE_IMGUI

	if ( FindHelper() )
	{
		( *FindHelper() )->UseImGui();
	}
	UseImGui();

#endif // USE_IMGUI

	controller.Update();

	CameraUpdate();

	PlayerUpdate( elapsedTime );

	player.PhysicUpdate( {} );

	return ReturnResult();
}

void SceneGame::Draw( float elapsedTime )
{
	elapsedTime = 1.0f; // Disable

	{
		constexpr FLOAT BG_COLOR[4]{ 0.4f, 0.4f, 0.4f, 1.0f };
		Donya::ClearViews( BG_COLOR );
	}

	const Donya::Vector4x4 V{ iCamera.CalcViewMatrix() };
	const Donya::Vector4x4 P{ iCamera.GetProjectionMatrix() };
	const Donya::Vector4   cameraPos{ iCamera.GetPosition(), 1.0f };

	player.Draw( V * P );

#if DEBUG_MODE
	if ( Common::IsShowCollision() )
	{
		static auto cube = Donya::Geometric::CreateCube();

		static Donya::Vector4 lightDir{ 0.0f,-1.0f, 1.0f, 0.0f };

		// Ground likes.
		{
			static Donya::Vector3 pos  { 0.0f, -1.0f, 0.0f };
			static Donya::Vector3 size { 10.0f, 1.0f, 50.0f };
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
				( TB_W * V * P ), TB_W,
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
	const auto data = FetchMember();
	const Donya::Vector3 playerPos = player.GetPosition();

	Donya::Vector3 relativePos = playerPos + data.camera.offsetPos;
	relativePos.y = data.camera.basePosY;

	iCamera.SetPosition( relativePos );
	iCamera.SetFocusPoint( playerPos + data.camera.offsetFocus );

}
void SceneGame::CameraUpdate()
{
#if DEBUG_MODE
	if ( !Donya::Keyboard::Press( VK_MENU ) )
	{
		AssignCameraPos();
	}
#else
	AssignCameraPos();
#endif // DEBUG_MODE

	const auto data = FetchMember();

	auto MakeControlStructWithMouse = []()
	{
		constexpr float SLERP_FACTOR = 0.2f;

		auto NoOperation = [&SLERP_FACTOR]()
		{
			Donya::ICamera::Controller noop{};
			noop.SetNoOperation();
			noop.slerpPercent = SLERP_FACTOR;
			return noop;
		};

		if ( !Donya::Keyboard::Press( VK_MENU ) ) { return NoOperation(); }
		// else

		static Donya::Int2 prevMouse{};
		static Donya::Int2 currMouse{};

		prevMouse = currMouse;

		auto nowMouse = Donya::Mouse::Coordinate();
		currMouse.x = scast<int>( nowMouse.x );
		currMouse.y = scast<int>( nowMouse.y );

		bool isInputMouseButton = Donya::Mouse::Press( Donya::Mouse::Kind::LEFT ) || Donya::Mouse::Press( Donya::Mouse::Kind::MIDDLE ) || Donya::Mouse::Press( Donya::Mouse::Kind::RIGHT );
		bool isDriveMouse = ( prevMouse != currMouse ) || Donya::Mouse::WheelRot() || isInputMouseButton;
		if ( !isDriveMouse ) { return NoOperation(); }
		// else

		Donya::Vector3 diff{};
		{
			Donya::Vector2 vec2 = ( currMouse - prevMouse ).Float();

			diff.x = vec2.x;
			diff.y = vec2.y;
		}

		Donya::Vector3 movement{};
		Donya::Vector3 rotation{};

		if ( Donya::Mouse::Press( Donya::Mouse::Kind::LEFT ) )
		{
			constexpr float ROT_AMOUNT = ToRadian( 1.0f );
			rotation.x = diff.x * ROT_AMOUNT;
			rotation.y = diff.y * ROT_AMOUNT;
		}
		else
		if ( Donya::Mouse::Press( Donya::Mouse::Kind::MIDDLE ) )
		{
			constexpr float MOVE_SPEED = 0.1f;
			movement.x = diff.x * MOVE_SPEED;
			movement.y = -diff.y * MOVE_SPEED;
		}

		constexpr float FRONT_SPEED = 3.5f;
		movement.z = FRONT_SPEED * scast<float>( Donya::Mouse::WheelRot() );

		Donya::ICamera::Controller ctrl{};
		ctrl.moveVelocity		= movement;
		ctrl.yaw				= rotation.x;
		ctrl.pitch				= rotation.y;
		ctrl.roll				= 0.0f;
		ctrl.slerpPercent		= SLERP_FACTOR;
		ctrl.moveInLocalSpace	= true;

		return ctrl;
	};
	Donya::ICamera::Controller input = MakeControlStructWithMouse();
	input.slerpPercent = data.camera.slerpFactor;
	iCamera.Update( input );
}

void SceneGame::PlayerUpdate( float elapsedTime )
{
	Donya::Vector2	moveVector{};
	bool useJump	= false;
	bool useShot	= false;
	bool useTrans	= false;

	if ( controller.IsConnected() )
	{
		using Pad	= Donya::Gamepad;

		moveVector	= controller.LeftStick();
		useJump		= controller.Trigger( Pad::A );
		useShot		= controller.Trigger( Pad::B );
		useTrans	= controller.Trigger( Pad::X );
	}
	else
	{
		moveVector.x += Donya::Keyboard::Press( VK_RIGHT	) ? +1.0f : 0.0f;
		moveVector.x += Donya::Keyboard::Press( VK_LEFT		) ? -1.0f : 0.0f;
		moveVector.y += Donya::Keyboard::Press( VK_UP		) ? +1.0f : 0.0f; // Front is Plus.
		moveVector.y += Donya::Keyboard::Press( VK_DOWN		) ? -1.0f : 0.0f; // Front is Plus.

		useJump		= Donya::Keyboard::Trigger( 'Z' );
		useShot		= Donya::Keyboard::Trigger( 'X' );
		useTrans	= Donya::Keyboard::Trigger( 'C' );
	}

	Player::Input input{};
	input.moveVectorXZ	= moveVector.Normalized();
	input.useJump		= useJump;
	input.useShot		= useShot;
	input.useTrans		= useTrans;

	player.Update( elapsedTime, input );
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
		change.sceneType = Scene::Type::Clear;
		return change;
	}

	bool requestPause	= controller.Trigger( Donya::Gamepad::Button::START ) || controller.Trigger( Donya::Gamepad::Button::SELECT ) || Donya::Keyboard::Trigger( 'P' );
	bool allowPause		= !Fader::Get().IsExist();
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
	if ( !ImGui::BeginIfAllowed() ) { return; }
	// else
	
	const auto data = FetchMember();

	if ( ImGui::TreeNode( u8"ゲーム・状況" ) )
	{
		if ( ImGui::TreeNode( u8"カメラ情報" ) )
		{
			auto ShowVec3 = []( const std::string &prefix, const Donya::Vector3 &v )
			{
				ImGui::Text( ( prefix + u8"[X:%5.2f][Y:%5.2f][Z:%5.2f]" ).c_str(), v.x, v.y, v.z );
			};

			ImGui::Text( u8"「ＡＬＴキー」を押している間のみ，" );
			ImGui::Text( u8"「マウスホイール」を押しながらマウス移動で，" );
			ImGui::Text( u8"カメラの並行移動ができます。" );
			ImGui::Text( "" );

			const Donya::Vector3 cameraPos = iCamera.GetPosition();
			const Donya::Vector3 playerPos = player.GetPosition();
			ShowVec3( u8"現在位置・絶対：", cameraPos );
			ShowVec3( u8"現在位置・相対：", cameraPos - playerPos );
			ImGui::Text( "" );

			const Donya::Vector3 focusPoint = iCamera.GetFocusPoint();
			ShowVec3( u8"注視点位置・絶対：", focusPoint );
			ShowVec3( u8"注視点位置・相対：", focusPoint - playerPos );

			ImGui::TreePop();
		}

		ImGui::TreePop();
	}
	
	ImGui::End();
}

#endif // USE_IMGUI
