#include "SceneTitle.h"

#include <vector>

#include "Donya/Blend.h"
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
#include "SaveData.h"
#include "StageNumberDefine.h"

#undef max
#undef min

namespace
{
	constexpr int ItemCount = scast<int>( SceneTitle::Choice::ItemCount );
	std::string ToString( SceneTitle::Choice c )
	{
		switch ( c )
		{
		case SceneTitle::Choice::Nil:		return u8"Nil";
		case SceneTitle::Choice::NewGame:	return u8"はじめから";
		case SceneTitle::Choice::LoadGame:	return u8"つづきから";
		default: break;
		}
		_ASSERT_EXPR( 0, L"Error : Unexpected type!" );
		return "EROR";
	}

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

		Donya::Vector3 playerInitialPos;

		int waitFrameUntilFade = 60;

		Donya::Model::Constants::PerScene::DirectionalLight directionalLight;

		struct Item
		{
			float			drawScale = 1.0f;
			Donya::Vector2	ssDrawPos{};	// Center
			Donya::Vector2	texPartPos{};	// Left-Top
			Donya::Vector2	texPartSize{};	// Whole size
		private:
			friend class cereal::access;
			template<class Archive>
			void serialize( Archive &archive, std::uint32_t version )
			{
				archive
				(
					CEREAL_NVP( drawScale ),
					CEREAL_NVP( ssDrawPos ),
					CEREAL_NVP( texPartPos ),
					CEREAL_NVP( texPartSize )
				);

				if ( 1 <= version )
				{
					// archive( CEREAL_NVP( x ) );
				}
			}
		};
		std::vector<Item> items; // size() == SceneTitle::Choice::ItemCount
		float	choiceMagni					= 1.2f;
		int		waitFrameUntilChoiceItem	= 60;
		float	flushInterval				= 0.5f; // Seconds.
		float	flushRange					= 1.0f;
		float	flushLowestAlpha			= 0.0f;
	public: // Does not serialize members.
		Donya::Vector3 selectingPos;
	private:
		friend class cereal::access;
		template<class Archive>
		void serialize( Archive &archive, std::uint32_t version )
		{
			archive
			(
				CEREAL_NVP( camera.slerpFactor	),
				CEREAL_NVP( camera.offsetPos	),
				CEREAL_NVP( camera.offsetFocus	),
				CEREAL_NVP( playerInitialPos	)
			);

			if ( 1 <= version )
			{
				archive( CEREAL_NVP( waitFrameUntilFade ) );
			}
			if ( 2 <= version )
			{
				archive
				(
					CEREAL_NVP( transparency.zNear				),
					CEREAL_NVP( transparency.zFar				),
					CEREAL_NVP( transparency.lowerAlpha			),
					CEREAL_NVP( transparency.heightThreshold	),
					CEREAL_NVP( directionalLight.color			),
					CEREAL_NVP( directionalLight.direction		)
				);
			}
			if ( 3 <= version )
			{
				archive
				(
					CEREAL_NVP( items						),
					CEREAL_NVP( choiceMagni					),
					CEREAL_NVP( waitFrameUntilChoiceItem	),
					CEREAL_NVP( flushInterval				),
					CEREAL_NVP( flushRange					),
					CEREAL_NVP( flushLowestAlpha			)
				);
			}
			if ( 4 <= version )
			{
				// archive( CEREAL_NVP( x ) );
			}
		}
	};
}
CEREAL_CLASS_VERSION( Member, 3 )

class ParamTitle : public ParameterBase<ParamTitle>
{
public:
	static constexpr const char *ID = "Title";
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

		ResizeVectorIfNeeded();
	}
	Member Data() const { return m; }
private:
	void ResizeVectorIfNeeded()
	{
		if ( scast<int>( m.items.size() ) != ItemCount )
		{
			m.items.resize( ItemCount );
		}
	}
private:
	std::string GetSerializeIdentifier()			override { return ID; }
	std::string GetSerializePath( bool isBinary )	override { return GenerateSerializePath( ID, isBinary ); }
public:
#if USE_IMGUI
	void UseImGui() override
	{
		if ( !ImGui::BeginIfAllowed() ) { return; }
		// else

		ResizeVectorIfNeeded();

		if ( ImGui::TreeNode( u8"タイトルのパラメータ調整" ) )
		{
			ImGui::DragFloat3( u8"自機の初期位置", &m.playerInitialPos.x, 0.01f );
			ImGui::DragInt( u8"スタートから，項目表示までの待機時間（フレーム）", &m.waitFrameUntilChoiceItem	);
			ImGui::DragInt( u8"項目選択から，フェードまでの待機時間（フレーム）", &m.waitFrameUntilFade			);
			m.waitFrameUntilChoiceItem	= std::max( 1, m.waitFrameUntilChoiceItem	);
			m.waitFrameUntilFade		= std::max( 1, m.waitFrameUntilFade			);
			ImGui::Text( "" );

			if ( ImGui::TreeNode( u8"カメラ" ) )
			{
				ImGui::DragFloat ( u8"補間倍率",						&m.camera.slerpFactor,		0.01f );
				ImGui::DragFloat3( u8"自身の座標（自機からの相対）",	&m.camera.offsetPos.x,		0.01f );
				ImGui::DragFloat3( u8"注視点の座標（自機からの相対）",	&m.camera.offsetFocus.x,	0.01f );

				ImGui::TreePop();
			}

			if ( ImGui::TreeNode( u8"平行光" ) )
			{
				ImGui::ColorEdit4( u8"色", &m.directionalLight.color.x );
				ImGui::SliderFloat4( u8"方向", &m.directionalLight.direction.x, -1.0f, 1.0f );

				ImGui::TreePop();
			}

			ParameterHelper::ShowConstantNode( u8"近くのオブジェクトに適用する透明度", &m.transparency );

			if ( ImGui::TreeNode( u8"項目" ) )
			{
				ImGui::DragFloat( u8"選択項目の拡大率",			&m.choiceMagni,			0.01f );
				ImGui::DragFloat( u8"項目選択時の点滅間隔（秒）",	&m.flushInterval,		0.01f );
				ImGui::DragFloat( u8"項目選択時の点滅幅（秒）",	&m.flushRange,			0.01f );
				ImGui::SliderFloat( u8"点滅時のアルファの最低値",	&m.flushLowestAlpha,	0.0f, 1.0f );
				m.flushInterval	= std::max( 0.0f, m.flushInterval	);
				m.flushRange	= std::max( 0.0f, m.flushRange		);

				auto ShowItem = []( const std::string &nodeCaption, Member::Item *p )
				{
					if ( !ImGui::TreeNode( nodeCaption.c_str() ) ) { return; }
					// else

					ImGui::DragFloat ( u8"描画スケール",				&p->drawScale		);
					ImGui::DragFloat2( u8"描画位置（中心点）",		&p->ssDrawPos.x		);
					ImGui::DragFloat2( u8"テクスチャ原点（左上）",	&p->texPartPos.x	);
					ImGui::DragFloat2( u8"テクスチャサイズ（全体）",	&p->texPartSize.x	);

					ImGui::TreePop();
				};

				std::string caption{};
				for ( int i = 0; i < ItemCount; ++i )
				{
					caption = u8"[" + std::to_string( i ) + u8":" + ToString( scast<SceneTitle::Choice>( i ) ) + u8"]";
					ShowItem( caption, &m.items[i] );
				}

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
	Member FetchMember()
	{
		return ParamTitle::Get().Data();
	}
}

void SceneTitle::Init()
{
	Donya::Sound::Play( Music::BGM_Title );
#if DEBUG_MODE
	Donya::Sound::AppendFadePoint( Music::BGM_Title, 2.0f, 0.0f, true ); // Too noisy.
#endif // DEBUG_MODE

	using Spr = SpriteAttribute;

	timer = 0;
	nowWaiting = false;

	bool result{};

	pRenderer = std::make_unique<RenderingHelper>();
	result = pRenderer->Init();
	assert( result );

	ParamTitle::Get().Init();
	const auto data = FetchMember();

	pBG = std::make_unique<BG>();
	result = pBG->LoadSprites( GetSpritePath( Spr::BackGround ), GetSpritePath( Spr::Cloud ) );
	assert( result );

	pSentence = std::make_unique<TitleSentence>();
	pSentence->Init();
	result = pSentence->LoadSprites( GetSpritePath( Spr::TitleLogo ), GetSpritePath( Spr::TitlePrompt ) );
	assert( result );

	result = sprItem.LoadSprite( GetSpritePath( Spr::TitleItems ), 16U );
	assert( result );

	pTerrain = std::make_unique<Terrain>( TITLE_STAGE_NO );
	pTerrain->SetWorldConfig( Donya::Vector3{ 1.0f, 1.0f, 1.0f }, Donya::Vector3::Zero() );

	result = ObstacleBase::LoadModels();
	assert( result );

	ObstacleBase::ParameterInit();
	pObstacles = std::make_unique<ObstacleContainer>();
	pObstacles->Init( TITLE_STAGE_NO );

	result = Player::LoadModels();
	assert( result );
	PlayerInit();

	CameraInit();

	StartInit();
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

	UpdateByStatus( elapsedTime );

	return ReturnResult();
}

void SceneTitle::Draw( float elapsedTime )
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
		pObstacles->Draw( pRenderer.get(), { 1.0f, 1.0f, 1.0f, 1.0f } );
		pTerrain->Draw( pRenderer.get(), { 1.0f, 1.0f, 1.0f, 1.0f } );
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

		pObstacles->DrawHitBoxes( pRenderer.get(), VP, { 1.0f, 1.0f, 1.0f, 1.0f } );
	}

	// Drawing to far for avoiding to trans the BG's blue.
	pBG->Draw( elapsedTime );

	DrawByStatus( elapsedTime );

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
				W * VP, W,
				data.directionalLight.direction, color
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
	pPlayerIniter = std::make_unique<PlayerInitializer>();
	pPlayerIniter->LoadParameter( TITLE_STAGE_NO );

	// const auto data = FetchMember();
	// pPlayer = std::make_unique<Player>();
	// pPlayer->Init( data.playerInitialPos.GetPosition() );
	pPlayer = std::make_unique<Player>();
	pPlayer->Init( *pPlayerIniter );
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

	const Donya::Vector4x4 &terrainMatrix = pTerrain->GetWorldMatrix();
	pPlayer->PhysicUpdate( solids, pTerrain->GetCollisionModel().get(), &terrainMatrix );
}
void SceneTitle::PlayerDraw()
{
	if ( !pPlayer ) { return; }
	// else
	pPlayer->Draw( pRenderer.get() );
}
void SceneTitle::PlayerDrawHitBox( const Donya::Vector4x4 &matVP )
{
	if ( !pPlayer ) { return; }
	// else
	pPlayer->DrawHitBox( pRenderer.get(), matVP );
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
	return	( controller.IsConnected() )
			? controller.Trigger( Donya::Gamepad::A )
			: Donya::Keyboard::Trigger( 'Z' );
}
bool SceneTitle::NowAcceptableTiming() const
{
	if ( Fader::Get().IsExist()	) { return false; }
	if ( nowWaiting				) { return false; }
	// else

	return true;
}

void SceneTitle::UpdateByStatus( float elapsedTime )
{
	switch ( status )
	{
	case State::Start:		StartUpdate( elapsedTime );		return;
	case State::SelectItem:	SelectUpdate( elapsedTime );	return;
	default: return;
	}
}
void SceneTitle::DrawByStatus( float elapsedTime )
{
	switch ( status )
	{
	case State::Start:		StartDraw( elapsedTime );		return;
	case State::SelectItem:	SelectDraw( elapsedTime );		return;
	default: return;
	}
}

void SceneTitle::StartInit()
{
	timer		= 0;
	nowWaiting	= false;
	status		= State::Start;
}
void SceneTitle::StartUninit()
{
	pSentence.reset();
}
void SceneTitle::StartUpdate( float elapsedTime )
{
	if ( IsRequiredAdvance() && NowAcceptableTiming() )
	{
		nowWaiting = true;
		
		pSentence->AdvanceState();
		Donya::Sound::Play( Music::UI_StartGame );
	}

	pSentence->Update( elapsedTime );

	if ( nowWaiting )
	{
		timer++;

		if ( timer == FetchMember().waitFrameUntilChoiceItem )
		{
			StartUninit();
			SelectInit();
		}
	}
}
void SceneTitle::StartDraw( float elapsedTime )
{
	pSentence->Draw( elapsedTime );
}

void SceneTitle::SelectInit()
{
	timer		= 0;
	flushTimer	= 0.0f;
	flushAlpha	= 0.0f;
	nowWaiting	= false;
	status		= State::SelectItem;
}
void SceneTitle::SelectUninit() {}
void SceneTitle::SelectUpdate( float elapsedTime )
{
	if ( IsRequiredAdvance() && NowAcceptableTiming() )
	{
		nowWaiting = true;
		if ( chooseItem == Choice::NewGame )
		{
			SaveDataAdmin::Get().Clear();
		}

		Donya::Sound::Play( Music::UI_StartGame );
	}

	if ( nowWaiting )
	{
		const auto data = FetchMember();

		timer++;
		if ( timer == data.waitFrameUntilFade )
		{
			StartFade();
		}

		// Calc flushing alpha that copied by TitleSentence.
		{
			const float increaseAmount = 360.0f / ( 60.0f * data.flushInterval );
			flushTimer += increaseAmount;

			const float sin_01 = ( sinf( ToRadian( flushTimer ) ) + 1.0f ) * 0.5f;
			const float shake  = sin_01 * data.flushRange;

			flushAlpha = std::max( data.flushLowestAlpha, std::min( 1.0f, shake ) );
		}
	}
}
void SceneTitle::SelectDraw( float elapsedTime )
{
	Donya::Blend::Activate( Donya::Blend::Mode::ALPHA );

	auto DrawItem = [&]( const Member::Item &source, float scaleMagni, float drawDepth )
	{
		sprItem.drawScale	= source.drawScale * scaleMagni;
		sprItem.pos			= source.ssDrawPos;
		sprItem.texPos		= source.texPartPos;
		sprItem.texSize		= source.texPartSize;

		sprItem.DrawPart( drawDepth );
	};

	constexpr float defaultDepth = 0.1f;
	constexpr float chosenDepth  = defaultDepth * 0.5f;

	const auto data = FetchMember();

	const int chosenIndex = scast<int>( chooseItem );
	for ( int i = 0; i < ItemCount; ++i )
	{
		if ( i == chosenIndex && nowWaiting )
		{
			sprItem.alpha = flushAlpha;
		}
		else
		{
			sprItem.alpha = 1.0f;
		}

		const float magni = ( i == chosenIndex ) ? data.choiceMagni	: 1.0f;
		const float depth = ( i == chosenIndex ) ? chosenDepth		: defaultDepth;
		DrawItem( data.items[i], magni, depth );
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
			if ( pPlayerIniter )
			{
				pPlayerIniter->ShowImGuiNode( u8"自機の初期化情報", TITLE_STAGE_NO );
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
