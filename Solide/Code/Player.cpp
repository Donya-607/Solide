#include "Player.h"

#include <algorithm>			// For std::max(), min()
#include <array>

#include <cereal/types/vector.hpp>

#include "Donya/CBuffer.h"
#include "Donya/Constant.h"		// For DEBUG_MODE macro.
#include "Donya/Loader.h"
#include "Donya/Model.h"
#include "Donya/ModelMotion.h"
#include "Donya/Serializer.h"
#include "Donya/Shader.h"
#include "Donya/Sound.h"
#include "Donya/Useful.h"		// For ZeroEqual().

#if DEBUG_MODE
#include "Donya/Keyboard.h"
#endif // DEBUG_MODE

#include "Bullet.h"
#include "Common.h"
#include "FilePath.h"
#include "Music.h"
#include "Parameter.h"

#undef max
#undef min

namespace PlayerModel
{
	enum Kind
	{
		Idle,
		Run,
		Slide,
		Jump,
		Fall,

		KindCount
	};
	constexpr size_t KIND_COUNT = scast<size_t>( KindCount );
	constexpr const char *MODEL_FILE_PATH = "./Data/Models/Player/Player.bin";
	constexpr const char *KIND_NAMES[KIND_COUNT]
	{
		"Idle",
		"Run",
		"Slide",
		"Jump",
		"Fall"
	};

	struct StorageBundle
	{
		Donya::Model::SkinningModel	model;
		Donya::Model::MotionHolder	motionHolder;
	};
	static std::unique_ptr<StorageBundle> pModel{};

	bool LoadModel()
	{
		// Already has loaded.
		if ( pModel ) { return true; }
		// else

		if ( !Donya::IsExistFile( MODEL_FILE_PATH ) )
		{
			Donya::OutputDebugStr( "Error : The Player's model file does not exist." );
			return false;
		}
		// else

		Donya::Loader loader{};
		if ( !loader.Load( MODEL_FILE_PATH ) ) { return false; }
		// else

		const Donya::Model::Source source = loader.GetModelSource();

		pModel = std::make_unique<StorageBundle>();
		pModel->model = Donya::Model::SkinningModel::Create( source, loader.GetFileDirectory() );
		pModel->motionHolder.AppendSource( source );

		if ( !pModel->model.WasInitializeSucceeded() )
		{
			pModel.reset();
			return false;
		}
		// else

		return true;
	}

	bool IsOutOfRange( Kind kind )
	{
		return ( kind < 0 || KindCount <= kind ) ? true : false;
	}
	const Donya::Model::SkinningModel &GetModel()
	{
		_ASSERT_EXPR( pModel, L"Error : The Player's model does not initialized!" );
		return pModel->model;
	}
	const Donya::Model::MotionHolder  &GetMotions()
	{
		_ASSERT_EXPR( pModel, L"Error : The Player's motions does not initialized!" );
		return pModel->motionHolder;
	}
}

namespace
{
	struct Member
	{
		/// <summary>
		/// All scalar members are positive value.
		/// </summary>
		struct BasicMember
		{
			float accel			= 0.01f;
			float decel			= 0.01f;
			float maxSpeed		= 0.1f;
			float gravity		= 0.1f;
			float jumpStrength	= 0.1f;

			// The "pos" of a hitBox acts as an offset.
			// That default value is visible and basic.

			Donya::AABB hitBoxStage{ {}, { 0.5f, 0.5f, 0.5f }, true }; // Collide to a stage.

			Bullet::BulletAdmin::FireDesc shotDesc;
		private:
			friend class cereal::access;
			template<class Archive>
			void serialize( Archive &archive, std::uint32_t version )
			{
				archive
				(
					CEREAL_NVP( accel			),
					CEREAL_NVP( decel			),
					CEREAL_NVP( maxSpeed		),
					CEREAL_NVP( gravity			),
					CEREAL_NVP( jumpStrength	),
					CEREAL_NVP( hitBoxStage		)
				);

				if ( 1 <= version )
				{
					archive( CEREAL_NVP( shotDesc ) );
				}
				if ( 2 <= version )
				{
					// archive( CEREAL_NVP( x ) );
				}
			}
		};
		struct OilMember
		{
			BasicMember	basic;
			float		turnDegree		= 1.0f;		// Per frame.
			float		turnThreshold	= 0.4f;		// Degree.
			float		tiltDegree		= 1.0f;		// Per frame.
			float		untiltDegree	= 2.0f;		// Per frame.
			float		maxTiltDegree	= 45.0f;
			float		hopStrength			= 0.1f;
			float		hopRotation			= 0.1f;	// Degree.
			float		hopRotationDegree	= 0.1f;	// Degree.
		private:
			friend class cereal::access;
			template<class Archive>
			void serialize( Archive &archive, std::uint32_t version )
			{
				archive
				(
					CEREAL_NVP( basic			),
					CEREAL_NVP( turnDegree		),
					CEREAL_NVP( turnThreshold	),
					CEREAL_NVP( tiltDegree		),
					CEREAL_NVP( untiltDegree	),
					CEREAL_NVP( maxTiltDegree	)
				);

				if ( 1 <= version )
				{
					archive
					(
						CEREAL_NVP( hopStrength ),
						CEREAL_NVP( hopRotation ),
						CEREAL_NVP( hopRotationDegree )
					);
				}
				if ( 2 <= version )
				{
					// archive( CEREAL_NVP( x ) );
				}
			}
		};

		// These members are formed as appended order.

		BasicMember		normal;
		OilMember		oiled;

		std::vector<Donya::Vector3>	raypickOffsets;

		float falloutBorderPosY;

		float drawScale = 1.0f;
		std::vector<float>	motionAccelerations; // "motionAccelerations[i]" represents a value of static_cast<enumKind>( i ). This size was guaranteed to: size() == PlayerModel::KIND_COUNT
		Donya::Vector3		drawOffset;

		float canRideSlopeBorder = 0.0f;	// The standing face is ridable if that statement is true: canRideSlopeBorder <= fabsf( max( 0.0f, Dot( faceNormal, UP ) ) )

		std::vector<int> useMotionIndices;	// This size was guaranteed to: size() == PlayerModel::KIND_COUNT

		int transTriggerFrame = 1;			// The trigger of transform to oil. Will be compared to press length.
	private:
		friend class cereal::access;
		template<class Archive>
		void serialize( Archive &archive, std::uint32_t version )
		{
			archive
			(
				CEREAL_NVP( normal )
			);

			if ( 1 <= version )
			{
				archive( CEREAL_NVP( oiled ) );
			}
			if ( 2 <= version )
			{
				archive( CEREAL_NVP( raypickOffsets ) );
			}
			if ( 3 <= version )
			{
				archive( CEREAL_NVP( falloutBorderPosY ) );
			}
			if ( 4 <= version )
			{
				archive
				(
					CEREAL_NVP( drawScale ),
					CEREAL_NVP( motionAccelerations )
				);
			}
			if ( 5 <= version )
			{
				archive( CEREAL_NVP( drawOffset ) );
			}
			if ( 6 <= version )
			{
				archive( CEREAL_NVP( canRideSlopeBorder ) );
			}
			if ( 7 <= version )
			{
				archive( CEREAL_NVP( useMotionIndices ) );
			}
			if ( 8 <= version )
			{
				archive( CEREAL_NVP( transTriggerFrame ) );
			}
			if ( 9 <= version )
			{
				// archive( CEREAL_NVP( x ) );
			}
		}
	};
}
CEREAL_CLASS_VERSION( Member,				8 )
CEREAL_CLASS_VERSION( Member::BasicMember,	1 )
CEREAL_CLASS_VERSION( Member::OilMember,	1 )

class ParamPlayer : public ParameterBase<ParamPlayer>
{
public:
	static constexpr const char *ID = "Player";
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

		ResizeMotionVector();
	}
	Member Data() const { return m; }
private:
	void ResizeMotionVector()
	{
		if ( m.motionAccelerations.size() != PlayerModel::KIND_COUNT )
		{
			m.motionAccelerations.resize( PlayerModel::KIND_COUNT );
			for ( auto &it : m.motionAccelerations )
			{
				it = 1.0f;
			}
		}

		if ( m.useMotionIndices.size() != PlayerModel::KIND_COUNT )
		{
			m.useMotionIndices.resize( PlayerModel::KIND_COUNT );
			for ( size_t i = 0; i < PlayerModel::KIND_COUNT; ++i )
			{
				m.useMotionIndices[i] = i;
			}
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

		if ( ImGui::TreeNode( u8"自機のパラメータ調整" ) )
		{
			auto ShowBasicNode = []( const std::string &prefix, Member::BasicMember *p )
			{
				if ( !ImGui::TreeNode( prefix.c_str() ) ) { return; }
				// else
				
				ImGui::DragFloat( ( prefix + u8"：加速量"	).c_str(),		&p->accel,			0.01f, 0.0f );
				ImGui::DragFloat( ( prefix + u8"：減速量"	).c_str(),		&p->decel,			0.01f, 0.0f );
				ImGui::DragFloat( ( prefix + u8"：最高速度"	).c_str(),		&p->maxSpeed,		0.01f, 0.0f );
				ImGui::DragFloat( ( prefix + u8"：跳躍力"	).c_str(),		&p->jumpStrength,	0.01f, 0.0f );
				ImGui::DragFloat( ( prefix + u8"：重力"		).c_str(),		&p->gravity,		0.01f, 0.0f );
				ParameterHelper::ShowAABBNode( prefix + u8"：当たり判定・ＶＳ地形", &p->hitBoxStage );
				p->shotDesc.ShowImGuiNode( { prefix + u8"：ショット詳細" } );

				ImGui::TreePop();
			};

			ShowBasicNode( u8"通常時", &m.normal );

			if ( ImGui::TreeNode( u8"オイル時" ) )
			{
				ShowBasicNode( u8"物理挙動", &m.oiled.basic );

				ImGui::DragFloat( u8"曲げるしきい値角度",		&m.oiled.turnThreshold,	0.1f, 0.0f, 180.0f	);
				ImGui::DragFloat( u8"１Ｆに曲がる角度",		&m.oiled.turnDegree,	0.1f, 0.0f, 180.0f	);
				ImGui::Text( "" );
				ImGui::DragFloat( u8"１Ｆに傾ける角度",		&m.oiled.tiltDegree,	0.1f, 0.0f, 180.0f	);
				ImGui::DragFloat( u8"１Ｆに傾きを戻す角度",	&m.oiled.untiltDegree,	0.1f, 0.0f, 180.0f	);
				ImGui::DragFloat( u8"傾く角度の最大",			&m.oiled.maxTiltDegree, 0.1f, 0.0f, 180.0f	);
				ImGui::Text( "" );
				ImGui::DragFloat( u8"発動時・跳ねる強さ",		&m.oiled.hopStrength, 0.1f, 0.0f	);
				ImGui::DragFloat( u8"発動時・回転量",			&m.oiled.hopRotation, 0.1f, 0.0f	);
				ImGui::DragFloat( u8"発動時・１Ｆの回転角度",	&m.oiled.hopRotationDegree, 0.1f, 0.0f	);

				ImGui::TreePop();
			}

			if ( ImGui::TreeNode( u8"共通" ) )
			{
				ImGui::DragFloat( u8"描画スケール", &m.drawScale, 0.01f, 0.0f );
				ImGui::DragFloat3( u8"描画オフセット", &m.drawOffset.x, 0.01f );
				ImGui::DragFloat( u8"落下死となるＹ座標しきい値", &m.falloutBorderPosY, 0.1f );
				ImGui::SliderFloat( u8"乗ることができる坂のしきい値", &m.canRideSlopeBorder, 0.0f, 1.0f );
				ImGui::Text( "" );
				ImGui::DragInt( u8"オイル長押しの発動フレーム", &m.transTriggerFrame );
				m.transTriggerFrame = std::max( 1, m.transTriggerFrame );

				if ( ImGui::TreeNode( u8"レイピック時のレイのオフセット" ) )
				{
					auto &data = m.raypickOffsets;
					if ( ImGui::Button( u8"追加" ) )
					{
						data.push_back( {} );
					}
					if ( 1 <= data.size() && ImGui::Button( u8"末尾を削除" ) )
					{
						data.pop_back();
					}

					std::string caption{};
					const size_t count = data.size();
					for ( size_t i = 0; i < count; ++i )
					{
						caption = "[" + std::to_string( i ) + "]";
						ImGui::DragFloat3( caption.c_str(), &data[i].x, 0.1f );
					}

					ImGui::TreePop();
				}

				ImGui::TreePop();
			}

			if ( ImGui::TreeNode( u8"モーション関連" ) )
			{
				ResizeMotionVector();

				const auto &motionHolder = PlayerModel::GetMotions();
				const size_t motionCount = motionHolder.GetMotionCount();

				if ( ImGui::TreeNode( u8"プログラム側が期待するモーションたち" ) )
				{
					for ( size_t i = 0; i < motionCount; ++i )
					{
						ImGui::Text( u8"[%d]:%s", i, PlayerModel::KIND_NAMES[i] );
					}
					ImGui::TreePop();
				}
				if ( ImGui::TreeNode( u8"読みこんだモデルにあるモーションたち" ) )
				{
					for ( size_t i = 0; i < motionCount; ++i )
					{
						ImGui::Text( u8"[%d]:%s", i, motionHolder.GetMotion( i ).name.c_str() );
					}
					ImGui::TreePop();
				}

				if ( ImGui::TreeNode( u8"モーション番号の紐づけ" ) )
				{
					std::string arrayIndex{};
					std::string nowLinkMotion{};
					std::string caption{};
					for ( size_t i = 0; i < motionCount; ++i )
					{
						arrayIndex		= "[" + std::to_string( i ) + "]";
						nowLinkMotion	= motionHolder.GetMotion( m.useMotionIndices[i] ).name;
						caption			= arrayIndex + u8":" + nowLinkMotion;
						ImGui::SliderInt( caption.c_str(), &m.useMotionIndices[i], 0, motionCount - 1 );
					}

					ImGui::TreePop();
				}

				if ( ImGui::TreeNode( u8"再生速度の倍率" ) )
				{
					std::string caption{};
					for ( size_t i = 0; i < PlayerModel::KIND_COUNT; ++i )
					{
						caption = "[" + std::to_string( i ) + ":" + PlayerModel::KIND_NAMES[i] + "]";
						ImGui::DragFloat( caption.c_str(), &m.motionAccelerations[i], 0.001f, 0.0f );
					}

					ImGui::TreePop();
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


Donya::Vector3		PlayerInitializer::GetInitialPos() const { return wsInitialPos; }
Donya::Quaternion	PlayerInitializer::GetInitialOrientation() const { return initialOrientation; }
void PlayerInitializer::LoadParameter( int stageNo )
{
#if DEBUG_MODE
	LoadJson( stageNo );
#else
	LoadBin( stageNo );
#endif // DEBUG_MODE
}
void PlayerInitializer::LoadBin ( int stageNo )
{
	constexpr bool fromBinary = true;
	Donya::Serializer::Load( *this, MakeStageParamPath( ID, stageNo, fromBinary ).c_str(), ID, fromBinary );
}
void PlayerInitializer::LoadJson( int stageNo )
{
	constexpr bool fromBinary = false;
	Donya::Serializer::Load( *this, MakeStageParamPath( ID, stageNo, fromBinary ).c_str(), ID, fromBinary );
}
#if USE_IMGUI
void PlayerInitializer::SaveBin ( int stageNo )
{
	constexpr bool fromBinary = true;

	const std::string filePath = MakeStageParamPath( ID, stageNo, fromBinary );
	MakeFileIfNotExists( filePath, fromBinary );

	Donya::Serializer::Save( *this, filePath.c_str(), ID, fromBinary );
}
void PlayerInitializer::SaveJson( int stageNo )
{
	constexpr bool fromBinary = false;

	const std::string filePath = MakeStageParamPath( ID, stageNo, fromBinary );
	MakeFileIfNotExists( filePath, fromBinary );

	Donya::Serializer::Save( *this, filePath.c_str(), ID, fromBinary );
}
void PlayerInitializer::ShowImGuiNode( const std::string &nodeCaption, int stageNo )
{
	if ( !ImGui::TreeNode( nodeCaption.c_str() ) ) { return; }
	// else

	ImGui::DragFloat3( u8"初期のワールド座標", &wsInitialPos.x );
	
	Donya::Vector3 lookDir = initialOrientation.LocalFront();
	ImGui::SliderFloat3( u8"初期の前方向", &lookDir.x, -1.0f, 1.0f );

	initialOrientation = Donya::Quaternion::LookAt( Donya::Vector3::Front(), lookDir.Unit(), Donya::Quaternion::Freeze::Up );

	auto ShowIONode = [&]()
	{
		if ( !ImGui::TreeNode( u8"ファイル I/O" ) ) { return; }
		// else

		const std::string strIndex = u8"[" + std::to_string( stageNo ) + u8"]";

		static bool isBinary = true;
		if ( ImGui::RadioButton( "Binary", isBinary ) ) { isBinary = true;  }
		if ( ImGui::RadioButton( "Json",  !isBinary ) ) { isBinary = false; }

		std::string loadStr = u8"ロード" + strIndex;
		loadStr += u8"(by:";
		loadStr += ( isBinary ) ? u8"Binary" : u8"Json";
		loadStr += u8")";

		if ( ImGui::Button( ( u8"セーブ" + strIndex ).c_str() ) )
		{
			SaveBin ( stageNo );
			SaveJson( stageNo );
		}
		if ( ImGui::Button( loadStr.c_str() ) )
		{
			( isBinary ) ? LoadBin( stageNo ) : LoadJson( stageNo );
		}

		ImGui::TreePop();
	};
	ShowIONode();

	ImGui::TreePop();
}
#endif // USE_IMGUI


bool Player::LoadModels()
{
	return PlayerModel::LoadModel();
}

// Internal utility.
namespace
{
	Member FetchMember()
	{
		return ParamPlayer::Get().Data();
	}

	Donya::Vector2 ToXZVector( const Donya::Vector3 &v )
	{
		return Donya::Vector2{ v.x, v.z };
	}
	void AssignToXZ( Donya::Vector3 *pDest, const Donya::Vector2 &sourceXZ )
	{
		pDest->x = sourceXZ.x;
		pDest->z = sourceXZ.y;
	}
	void AssignToXZ( Donya::Vector3 *pDest, const Donya::Vector3 &source )
	{
		pDest->x = source.x;
		pDest->z = source.z;
	}
}

void Player::MotionManager::Init()
{
	animator.ResetTimer();
	AssignPose( 0 );
}
void Player::MotionManager::Update( Player &player, float elapsedTime )
{
	const auto  data			= FetchMember();

	const int   nowKind			= CalcNowKind( player );
	const int   nowMotion		= data.useMotionIndices[nowKind];

	const float acceleration	= data.motionAccelerations[nowMotion];

	animator.Update( elapsedTime * acceleration );
	AssignPose( nowMotion );
}
const Donya::Model::Pose &Player::MotionManager::GetPose() const
{
	return pose;
}
void Player::MotionManager::AssignPose( int motionIndex )
{
	const auto &motionHolder  = PlayerModel::GetMotions();
	_ASSERT_EXPR( !motionHolder.IsOutOfRange( motionIndex ), L"Error : Passed motion index out of range!" );

	const auto &currentMotion = motionHolder.GetMotion( motionIndex );
	pose.AssignSkeletal( animator.CalcCurrentPose( currentMotion ) );
}
int  Player::MotionManager::CalcNowKind( Player &player ) const
{
	// TODO: To separate between the Jump and the Fall.

	auto NowMoving	= [&]()
	{
		const Donya::Vector2 velocityXZ{ player.velocity.x, player.velocity.z };
		return ( velocityXZ.IsZero() ) ? false : true;
	};
	auto IsDead		= [&]()
	{
		return player.IsDead();
	};
	auto IsJump		= [&]()
	{
		return ( player.onGround ) ? false : true;
	};
	auto IsRun		= [&]()
	{
		if ( !player.onGround ) { return false; }
		// else
		return ( NowMoving() ) ? true : false;
	};
	auto IsIdle		= [&]()
	{
		if ( !player.onGround ) { return false; }
		// else
		return ( NowMoving() ) ? false : true;
	};

	// if ( IsDead() ) { return PlayerModel::Kind::Dead; }
	if ( IsJump() ) { return PlayerModel::Kind::Jump; }
	if ( IsRun()  )
	{
		return	( player.pMover->IsOiled() )
				? PlayerModel::Kind::Slide
				: PlayerModel::Kind::Run;
	}
	if ( IsIdle() ) { return PlayerModel::Kind::Idle; }
	// else

	assert( !"Error : Now is unexpected status!" );
	return PlayerModel::Kind::KindCount;
}

void Player::InputManager::Init()
{
	oilTimer				= 0;
	currInput.moveVectorXZ	= Donya::Vector2::Zero();
	currInput.useJump		= false;
	currInput.useOil		= false;
	prevInput				= currInput;
	beginPressWasOiled		= false;
	keepingPressAfterTrans	= false;
	prevIsOiled				= false;
	currIsOiled				= false;
}
void Player::InputManager::Update( const Player &player, const Input &input )
{
	prevInput	= currInput;
	currInput	= input;

	prevIsOiled	= currIsOiled;
	currIsOiled	= player.IsOiled();

	oilTimer	= IsPressOil() ? oilTimer + 1 : 0;

	if ( !prevInput.useOil && currInput.useOil )
	{
		beginPressWasOiled = currIsOiled;
	}

	if ( currIsOiled != prevIsOiled && IsPressOil() )
	{
		 keepingPressAfterTrans = true;
	}
	if ( keepingPressAfterTrans && !IsPressOil() )
	{
		 keepingPressAfterTrans = false;
	}
}
bool Player::InputManager::ShouldJump() const
{
	return currInput.useJump;
}
bool Player::InputManager::ShouldShot() const
{
	if ( currIsOiled ) { return IsTriggerOil(); }
	// else

	// You can not shot if you keep using oil even if you was transed.
	return ( beginPressWasOiled != currIsOiled ) ? false : IsReleaseOil();
}
bool Player::InputManager::ShouldTrans() const
{
	if ( currIsOiled ) { return IsTriggerOil(); }
	// else

	if ( !IsPressOil() || keepingPressAfterTrans ) { return false; }
	// else

	const auto data = FetchMember();
	return ( data.transTriggerFrame <= oilTimer );
}
bool Player::InputManager::IsTriggerOil() const
{
	return ( currInput.useOil && !prevInput.useOil );
}
bool Player::InputManager::IsReleaseOil() const
{
	return ( !currInput.useOil && prevInput.useOil );
}
bool Player::InputManager::IsPressOil() const
{
	return currInput.useOil;
}

void Player::NormalMover::Init( Player &player )
{
	const auto data = FetchMember();
	player.hitBox = data.normal.hitBoxStage;
}
void Player::NormalMover::Uninit( Player &player ) {}
void Player::NormalMover::Update( Player &player, float elapsedTime ) {}
void Player::NormalMover::Move( Player &player, float elapsedTime, Input input )
{
	const auto data = FetchMember();

	Donya::Vector2 velocityXZ = ToXZVector( player.velocity );

	if ( ZeroEqual( input.moveVectorXZ.Length() ) )
	{
		const Donya::Int2 oldSign
		{
			Donya::SignBit( velocityXZ.x ),
			Donya::SignBit( velocityXZ.y )
		};

		velocityXZ.x -= data.normal.decel * scast<float>( oldSign.x ) * elapsedTime;
		velocityXZ.y -= data.normal.decel * scast<float>( oldSign.y ) * elapsedTime;
		if ( Donya::SignBit( velocityXZ.x ) != oldSign.x ) { velocityXZ.x = 0.0f; }
		if ( Donya::SignBit( velocityXZ.y ) != oldSign.y ) { velocityXZ.y = 0.0f; }
	}
	else
	{
		velocityXZ += input.moveVectorXZ * data.normal.accel * elapsedTime;
		if ( data.normal.maxSpeed <= velocityXZ.Length() )
		{
			velocityXZ = velocityXZ.Unit() * data.normal.maxSpeed;
		}

		player.LookToInput( elapsedTime, input );
	}

	AssignToXZ( &player.velocity, velocityXZ );
}
void Player::NormalMover::Jump( Player &player, float elapsedTime )
{
	const auto data = FetchMember();
	player.velocity.y = data.normal.jumpStrength * elapsedTime;
}
void Player::NormalMover::Fall( Player &player, float elapsedTime )
{
	const auto data = FetchMember();
	player.velocity.y -= data.normal.gravity * elapsedTime;
}

void Player::OilMover::Init( Player &player )
{
	tilt  = 0.0f;
	player.StartHopping();

	const auto data = FetchMember();
	player.hitBox = data.oiled.basic.hitBoxStage;

	Donya::Vector3 initVelocity = player.orientation.LocalFront() * data.oiled.basic.maxSpeed;
	AssignToXZ( &player.velocity, initVelocity );
}
void Player::OilMover::Uninit( Player &player )
{
	player.StartHopping();

	AssignToXZ( &player.velocity, Donya::Vector2::Zero() );
}
void Player::OilMover::Update( Player &player, float elapsedTime ) {}
void Player::OilMover::Move( Player &player, float elapsedTime, Input input )
{
	input.moveVectorXZ.Normalize();

	const auto data = FetchMember();

	float betweenCross{};
	float betweenRadian{};
	{
		Donya::Vector2 nXZFront = ToXZVector( player.orientation.LocalFront().Unit() );
		
		betweenCross  = Donya::Cross( nXZFront, input.moveVectorXZ );
		betweenRadian = Donya::Dot  ( nXZFront, input.moveVectorXZ );
		betweenRadian = std::max( 0.0f, std::min( 1.0f, betweenRadian ) ); // Prevent NaN.
		betweenRadian = acosf( betweenRadian );
	}

	// Doing untilt only.
	if ( input.moveVectorXZ.IsZero() || fabsf( betweenRadian ) < ToRadian( data.oiled.turnThreshold ) )
	{
		int  sign =  Donya::SignBit( tilt );
		if ( sign == 0 ) { return; }
		// else

		float subtract = data.oiled.untiltDegree * sign;
		if ( fabsf( tilt ) <= fabsf( subtract ) )
		{
			tilt = 0.0f;
		}
		else
		{
			tilt -= subtract;
		}

		return;
	}
	// else

	const int sideSign = ( betweenCross < 0.0f ) ? 1 : -1;

	// Rotation of move-vector.
	{
		const float rotRadian = ToRadian( data.oiled.turnDegree ) * sideSign;
		const Donya::Quaternion rotation = Donya::Quaternion::Make( Donya::Vector3::Up(), rotRadian );
		player.orientation.RotateBy( rotation );
		player.orientation.Normalize();

		// Speed that the Y component is excepted.

		const float slideSpeed = data.oiled.basic.maxSpeed;
		Donya::Vector3 rotatedVelocity = player.orientation.LocalFront() * slideSpeed;
		AssignToXZ( &player.velocity, rotatedVelocity );
	}

	// Tilt the orientation.
	{
		float addition = data.oiled.tiltDegree * sideSign;
		if ( Donya::SignBit( addition ) != Donya::SignBit( tilt ) )
		{
			// Tilt to inverse side. So I want to tilt fastly.
			addition += data.oiled.untiltDegree * sideSign;
		}
		
		tilt += addition;
		if ( data.oiled.maxTiltDegree < fabsf( tilt ) )
		{
			tilt = data.oiled.maxTiltDegree * sideSign;
		}
	}
}
void Player::OilMover::Jump( Player &player, float elapsedTime )
{
	const auto data = FetchMember();
	player.velocity.y = data.oiled.basic.jumpStrength * elapsedTime;
}
void Player::OilMover::Fall( Player &player, float elapsedTime )
{
	const auto data = FetchMember();
	player.velocity.y -= data.oiled.basic.gravity * elapsedTime;
}
Donya::Quaternion Player::OilMover::GetExtraRotation( Player &player ) const
{
	return Donya::Quaternion::Make( player.orientation.LocalFront(), ToRadian( -tilt ) );
}

void Player::DeadMover::Init( Player &player )
{
	player.velocity = 0.0f;
	player.hitBox.exist = false;
}
void Player::DeadMover::Uninit( Player &player ) {}
void Player::DeadMover::Update( Player &player, float elapsedTime ) {}
void Player::DeadMover::Move( Player &player, float elapsedTime, Input input ) {}
void Player::DeadMover::Jump( Player &player, float elapsedTime ) {}
void Player::DeadMover::Fall( Player &player, float elapsedTime ) {}

void Player::Init( const PlayerInitializer &param )
{
	ParamPlayer::Get().Init();
	const auto data = FetchMember();

	pos			= param.GetInitialPos();
	velocity	= 0.0f;
	orientation	= param.GetInitialOrientation();

	ResetMover<NormalMover>();

	motionManager.Init();
}
void Player::Uninit()
{
	pMover->Uninit( *this );
	ParamPlayer::Get().Uninit();
}

void Player::Update( float elapsedTime, Input input )
{
#if USE_IMGUI
	ParamPlayer::Get().UseImGui();
	UseImGui();
#endif // USE_IMGUI

	inputManager.Update( *this, input );

	// This method depends on my status(IsOiled()), so I should call this before ShouldTrans() process.
	if ( inputManager.ShouldShot() )
	{
		Shot( elapsedTime );
	}

	if ( inputManager.ShouldTrans() && canUseOil )
	{
		( pMover->IsOiled() )
		? ResetMover<NormalMover>()
		: ResetMover<OilMover>();

		canUseOil = false;
		Donya::Sound::Play( Music::PlayerTrans );
	}

	Move( elapsedTime, input );

	if ( inputManager.ShouldJump( ) && onGround )
	{
		Jump( elapsedTime );
	}

	Fall( elapsedTime );

	pMover->Update( *this, elapsedTime );
	motionManager.Update( *this, elapsedTime );

	UpdateHopping( elapsedTime );
}

void Player::PhysicUpdate( const std::vector<Donya::AABB> &solids, const Donya::Model::PolygonGroup *pTerrain, const Donya::Vector4x4 *pTerrainMat )
{
	if ( pMover->IsDead() ) { return; }
	// else

	// For judge that to: "was landing?".
	const Donya::Vector3 oldPos = pos;

	const auto data = FetchMember();
	std::vector<Donya::Vector3> rotatedOffsets = data.raypickOffsets;
	for ( auto &it : rotatedOffsets )
	{
		it = orientation.RotateVector( it );
	}
	const Donya::Vector3 standingNormal = Actor::Move( velocity, rotatedOffsets, solids, pTerrain, pTerrainMat );

	// bool wasCorrectedV = WasCorrectedVertically( oldPos, pTerrain );

	// If now standing on some plane, that means corrected to vertically.
	bool wasCorrectedV = !standingNormal.IsZero();
	if ( wasCorrectedV )
	{
		const float	dot = Donya::Dot( standingNormal, Donya::Vector3::Up() );
		const bool	ridableFace = ( standingNormal.IsZero() )
					? false 
					: data.canRideSlopeBorder <= fabsf( std::max( 0.0f, dot ) );

		// I want erase the vertical velocity If collided to ceil or ridable floor.

		if ( 0.0f < velocity.y )
		{
			velocity.y = 0.0f;
		}
		else if ( ridableFace )
		{
			AssignLanding();
		}
	}
	else
	{
		onGround = false;
	}

	if ( IsUnderFalloutBorder() )
	{
		Die();
	}
}

void Player::Draw( RenderingHelper *pRenderer )
{
	if ( !pRenderer ) { return; }
	// else

	const auto data = FetchMember();
	const Donya::Quaternion pitchRotation		= Donya::Quaternion::Make( orientation.LocalRight(), hopPitching );
	const Donya::Quaternion actualOrientation	= // Rotation: First:My Orientatoin, Then:Pitching, Last:Extra(actually that is tilting)
		orientation.Rotated
		(
			pitchRotation.Rotated
			(
				pMover->GetExtraRotation( *this )
			)
		);
	const Donya::Vector4 bodyColor = ( pMover->IsDead() )
		? Donya::Vector4{ 1.0f, 0.5f, 0.0f, 1.0f }
		: Donya::Vector4{ 0.1f, 1.0f, 0.3f, 1.0f };
	const Donya::Vector3 drawOffset = actualOrientation.RotateVector( data.drawOffset );

	Donya::Vector4x4 W{};
	W._11 = data.drawScale;
	W._22 = data.drawScale;
	W._33 = data.drawScale;
	W *= actualOrientation.MakeRotationMatrix();
	W._41 = pos.x + drawOffset.x;
	W._42 = pos.y + drawOffset.y;
	W._43 = pos.z + drawOffset.z;

	const auto &drawModel = PlayerModel::GetModel();
	const auto &drawPose  = motionManager.GetPose();

	Donya::Model::Constants::PerModel::Common constant{};
	constant.drawColor		= bodyColor;
	constant.worldMatrix	= W;
	pRenderer->UpdateConstant( constant );
	pRenderer->ActivateConstantModel();

	pRenderer->Render( drawModel, drawPose );

	pRenderer->DeactivateConstantModel();
}
void Player::DrawHitBox( RenderingHelper *pRenderer, const Donya::Vector4x4 &matVP )
{
	if ( !Common::IsShowCollision() || !pRenderer ) { return; }
	// else
#if DEBUG_MODE
	constexpr Donya::Vector4 color{ 0.1f, 1.0f, 0.3f, 0.7f };
	Actor::DrawHitBox( pRenderer, matVP, Donya::Quaternion::Identity(), color );
#endif // DEBUG_MODE
}

void Player::LookToInput( float elapsedTime, Input input )
{
	Donya::Vector3 frontXZ = orientation.LocalFront(); frontXZ.y = 0.0f;
	Donya::Vector3 inputXZ{ input.moveVectorXZ.x, 0.0f, input.moveVectorXZ.y };

	orientation = Donya::Quaternion::LookAt( orientation, inputXZ.Unit(), Donya::Quaternion::Freeze::Up );
	orientation.Normalize();
}

void Player::Move( float elapsedTime, Input input )
{
	pMover->Move( *this, elapsedTime, input );
}

void Player::Jump( float elapsedTime )
{
	onGround = false;
	pMover->Jump( *this, elapsedTime );
	Donya::Sound::Play( Music::PlayerJump );
}
void Player::Fall( float elapsedTime )
{
	pMover->Fall( *this, elapsedTime );
}
bool Player::IsUnderFalloutBorder() const
{
	const auto data = FetchMember();
	return ( pos.y < data.falloutBorderPosY ) ? true : false;
}

bool Player::WasCorrectedVertically( const Donya::Vector3 &oldPos, const Donya::Model::PolygonGroup *pTerrain ) const
{
	const Donya::Vector3 movement = pos - oldPos;
	
	// If the actual movement is shrunk or stretched from velocity, we regard as I was corrected.
	
	constexpr float JUDGE_ERROR = 0.001f;
	const float diff = fabsf( movement.y ) - fabsf( velocity.y );
	bool wasCorrected = ( JUDGE_ERROR < fabsf( diff ) );

	// If the terrain is nothing, the criteria of landing is 0.0f.
	if ( !pTerrain )
	{
		wasCorrected = wasCorrected || ( pos.y < 0.0f + hitBox.size.y );
	}

	return wasCorrected;
}
void Player::AssignLanding()
{
	if ( !onGround )
	{
		// Prevent play the sound every frame.
		Donya::Sound::Play( Music::PlayerLanding );
	}

	onGround	= true;
	canUseOil	= true;
	velocity.y	= 0.0f;
}

void Player::Shot( float elapsedTime )
{
	const auto data	= FetchMember();
	auto useParam	= ( IsOiled() )
					? data.oiled.basic.shotDesc
					: data.normal.shotDesc;
	useParam.speed			*= elapsedTime;
	useParam.direction		=  orientation.RotateVector( useParam.direction );
	useParam.generatePos	+= GetPosition();

	Bullet::BulletAdmin::Get().Append( useParam );
}

void Player::Die()
{
	ResetMover<DeadMover>();
	onGround  = false;
	canUseOil = false;
}

void Player::StartHopping()
{
	const auto data = FetchMember();

	hopPitching = ToRadian( -data.oiled.hopRotation );
	velocity.y = data.oiled.hopStrength;
}
void Player::UpdateHopping( float elapsedTime )
{
	if ( 0.0f <= hopPitching ) { return; }
	// else

	const auto data = FetchMember();
	hopPitching += ToRadian( data.oiled.hopRotationDegree ) * elapsedTime;
	hopPitching = std::min( 0.0f, hopPitching );
}

#if USE_IMGUI
void Player::UseImGui()
{
	if ( !ImGui::BeginIfAllowed() ) { return; }
	// else

	if ( ImGui::TreeNode( u8"自機の今の状況" ) )
	{
		ImGui::DragFloat3( u8"座標", &pos.x,			0.01f );
		ImGui::DragFloat3( u8"速度", &velocity.x,	0.01f );
		if ( ImGui::Button( u8"Ｙ座標と速度をゼロにする" ) )
		{
			pos.y		= 1.0f;
			velocity.y	= 0.0f;
		}

		bool nowOiled = pMover->IsOiled(); // Immutable.
		ImGui::Checkbox( u8"地上にいる？",		&onGround	);
		ImGui::Checkbox( u8"あぶらを使えるか？",	&canUseOil	);
		ImGui::Checkbox( u8"あぶら状態か？",		&nowOiled	);

		ImGui::TreePop();
	}

	ImGui::End();
}
#endif // USE_IMGUI
