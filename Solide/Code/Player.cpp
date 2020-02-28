#include "Player.h"

#include <algorithm>			// For std::max(), min()
#include <array>

#include <cereal/types/vector.hpp>

#include "Donya/CBuffer.h"
#include "Donya/Constant.h"		// For DEBUG_MODE macro.
#include "Donya/Loader.h"
#include "Donya/Serializer.h"
#include "Donya/Shader.h"
#include "Donya/Sound.h"
#include "Donya/Useful.h"		// For ZeroEqual().

#if DEBUG_MODE
#include "Donya/Keyboard.h"
#endif // DEBUG_MODE

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
	constexpr const char *MODEL_DIRECTORY = "./Data/Models/Player/";
	constexpr const char *EXTENSION = ".bin";
	constexpr std::array<const char *, KIND_COUNT> MODEL_NAMES
	{
		"Idle",
		"Run",
		"Slide",
		"Jump",
		"Fall",
	};

	struct StorageBundle
	{
		std::shared_ptr<Donya::SkinnedMesh>	pModel;
		Donya::MotionChunk					motionsPerMesh;
	};
	struct Shader
	{
		Donya::VertexShader	VS;
		Donya::PixelShader	PS;
	};
	struct CBuffer
	{
		struct PerScene
		{
			Donya::Vector4 eyePos;
			Donya::Vector4 lightColor;
			Donya::Vector4 lightDirection;
		};
		struct PerModel
		{
			Donya::Vector4 materialColor;
		};

		Donya::CBuffer<PerScene> scene;
		Donya::CBuffer<PerModel> model;
	};
	struct SettingOption
	{
		unsigned int setSlot = 0;
		bool setVS = true;
		bool setPS = true;
	};
	static std::array<std::unique_ptr<StorageBundle>, KIND_COUNT> models{};
	static std::unique_ptr<Shader>  shader{};
	static std::unique_ptr<CBuffer> cbuffer{};

	bool LoadModels()
	{
		bool result		= true;
		bool succeeded	= true;

		auto Load = []( const std::string &filePath, std::unique_ptr<StorageBundle> *pDest )->bool
		{
			// Already loaded.
			if ( *pDest ) { return true; }
			// else

			bool result = true;
			Donya::Loader loader{};

			result = loader.Load( filePath, nullptr );
			if ( !result ) { return false; }
			// else

			std::unique_ptr<StorageBundle> &refDest = *pDest;
			refDest = std::make_unique<StorageBundle>();

			refDest->pModel = std::make_shared<Donya::SkinnedMesh>();
			result = Donya::SkinnedMesh::Create( loader, refDest->pModel.get() );
			if ( !result ) { return false; }
			// else

			result = Donya::MotionChunk::Create( loader, &refDest->motionsPerMesh );
			if ( !result ) { return false; }
			// else

			return result;
		};

		std::string filePath{};
		const std::string prefix = MODEL_DIRECTORY;
		for ( size_t i = 0; i < KIND_COUNT; ++i )
		{
			filePath = prefix + MODEL_NAMES[i] + EXTENSION;
			if ( !Donya::IsExistFile( filePath ) )
			{
				const std::string outputMsgBase{ "Error : The model file does not exist. That is : " };
				Donya::OutputDebugStr( ( outputMsgBase + "[" + filePath + "]" + "\n" ).c_str() );
				continue;
			}
			// else

			result = Load( filePath, &models[i] );
			if ( !result )
			{
				const std::wstring errMsgBase{ L"Failed : Loading a model. That is : " };
				const std::wstring errMsg = errMsgBase + Donya::MultiToWide( filePath );
				_ASSERT_EXPR( 0, errMsg.c_str() );

				succeeded = false;
			}
		}

		return succeeded;
	}
	bool LoadShaders()
	{
		// Already loaded.
		if ( shader ) { return true; }
		// else
		shader = std::make_unique<Shader>();

		bool succeeded = true;
		bool result{};

		constexpr const char *VSFilePath = "./Data/Shaders/SkinnedMeshVS.cso";
		constexpr const char *PSFilePath = "./Data/Shaders/SkinnedMeshPS.cso";
		using IE_DESC = D3D11_INPUT_ELEMENT_DESC;
		constexpr std::array<IE_DESC, 5> inputElementDesc
		{
			IE_DESC{ "POSITION"		, 0, DXGI_FORMAT_R32G32B32_FLOAT,		0, D3D11_APPEND_ALIGNED_ELEMENT,	D3D11_INPUT_PER_VERTEX_DATA, 0 },
			IE_DESC{ "NORMAL"		, 0, DXGI_FORMAT_R32G32B32_FLOAT,		0, D3D11_APPEND_ALIGNED_ELEMENT,	D3D11_INPUT_PER_VERTEX_DATA, 0 },
			IE_DESC{ "TEXCOORD"		, 0, DXGI_FORMAT_R32G32_FLOAT,			0, D3D11_APPEND_ALIGNED_ELEMENT,	D3D11_INPUT_PER_VERTEX_DATA, 0 },
			IE_DESC{ "BONES"		, 0, DXGI_FORMAT_R32G32B32A32_UINT,		0, D3D11_APPEND_ALIGNED_ELEMENT,	D3D11_INPUT_PER_VERTEX_DATA, 0 },
			IE_DESC{ "WEIGHTS"		, 0, DXGI_FORMAT_R32G32B32A32_FLOAT,	0, D3D11_APPEND_ALIGNED_ELEMENT,	D3D11_INPUT_PER_VERTEX_DATA, 0 },
		};

		result = shader->VS.CreateByCSO( VSFilePath, std::vector<IE_DESC>{ inputElementDesc.begin(), inputElementDesc.end() } );
		if ( !result ) { succeeded = false; }

		result = shader->PS.CreateByCSO( PSFilePath );
		if ( !result ) { succeeded = false; }

		return succeeded;
	}
	bool CreateCBuffers()
	{
		// Already loaded.
		if ( cbuffer ) { return true; }
		// else
		cbuffer = std::make_unique<CBuffer>();

		bool succeeded = true;
		bool result{};

		result = cbuffer->scene.Create();
		if ( !result ) { succeeded = false; }
		result = cbuffer->model.Create();
		if ( !result ) { succeeded = false; }

		return succeeded;
	}

	bool IsOutOfRange( Kind kind )
	{
		return ( kind < 0 || KindCount <= kind ) ? true : false;
	}
	std::string GetModelName( Kind kind )
	{
		if ( IsOutOfRange( kind ) )
		{
			_ASSERT_EXPR( 0, L"Error : Passed argument outs of range!" );
			return "";
		}
		// else

		return std::string{ MODEL_NAMES[kind] };
	}
	std::unique_ptr<StorageBundle> *GetModelBundleOrNullptr( Kind kind )
	{
		if ( IsOutOfRange( kind ) )
		{
			_ASSERT_EXPR( 0, L"Error : Passed argument outs of range!" );
			return nullptr;
		}
		// else

		return &models[kind];
	}

	void UpdateCBuffer( const CBuffer::PerScene &scene, const CBuffer::PerModel &model )
	{
		cbuffer->scene.data = scene;
		cbuffer->model.data = model;
	}
	void ActivateCBuffer( const SettingOption &scene, const SettingOption &model, ID3D11DeviceContext *pImmediateContext = nullptr )
	{
		cbuffer->scene.Activate( scene.setSlot, scene.setVS, scene.setPS, pImmediateContext );
		cbuffer->model.Activate( model.setSlot, model.setVS, model.setPS, pImmediateContext );
	}
	void ActivateShader( ID3D11DeviceContext *pImmediateContext = nullptr )
	{
		shader->VS.Activate( pImmediateContext );
		shader->PS.Activate( pImmediateContext );
	}
	void DeactivateCBuffer( ID3D11DeviceContext *pImmediateContext = nullptr )
	{
		cbuffer->scene.Deactivate( pImmediateContext );
		cbuffer->model.Deactivate( pImmediateContext );
	}
	void DeactivateShader( ID3D11DeviceContext *pImmediateContext = nullptr )
	{
		shader->VS.Deactivate( pImmediateContext );
		shader->PS.Deactivate( pImmediateContext );
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
		std::vector<float> motionAccelerations; // Usually 1.0f.
		Donya::Vector3 drawOffset;
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
				// archive( CEREAL_NVP( x ) );
			}
		}
	};
}
CEREAL_CLASS_VERSION( Member,				5 )
CEREAL_CLASS_VERSION( Member::BasicMember,	0 )
CEREAL_CLASS_VERSION( Member::OilMember,	0 )

class ParamPlayer : public ParameterBase<ParamPlayer>
{
public:
	static constexpr const char *ID = "Player";
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

		ResizeMotionVector();
	}
	void Uninit()   override {}
	Member Data()   const { return m; }
private:
	void ResizeMotionVector()
	{
		if ( m.motionAccelerations.size() == PlayerModel::KIND_COUNT ) { return; }
		// else

		m.motionAccelerations.resize( PlayerModel::KIND_COUNT );
		for ( auto &it : m.motionAccelerations )
		{
			it = 1.0f;
		}
	}
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

				ImGui::TreePop();
			}

			if ( ImGui::TreeNode( u8"共通" ) )
			{
				ImGui::DragFloat( u8"描画スケール", &m.drawScale, 0.01f, 0.0f );
				ImGui::DragFloat3( u8"描画オフセット", &m.drawOffset.x, 0.01f );
				ImGui::DragFloat( u8"落下死となるＹ座標しきい値", &m.falloutBorderPosY, 0.1f );

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

				if ( ImGui::TreeNode( u8"再生速度の倍率" ) )
				{
					std::string caption{};
					for ( size_t i = 0; i < PlayerModel::KIND_COUNT; ++i )
					{
						caption = "[" + std::to_string( i ) + ":" + PlayerModel::MODEL_NAMES[i] + "]";
						ImGui::DragFloat( caption.c_str(), &m.motionAccelerations[i], 0.01f, 0.0f );
					}

					ImGui::TreePop();
				}


				ImGui::TreePop();
			}

			ShowIONode();

			ImGui::TreePop();
		}

		ImGui::End();
	}
#endif // USE_IMGUI
};

bool Player::LoadModels()
{
	return PlayerModel::LoadModels();
}
bool Player::LoadShadingObjects()
{
	bool result{};
	bool succeeded = true;

	result = PlayerModel::LoadShaders();
	if ( !result ) { succeeded = false; }
	
	result = PlayerModel::CreateCBuffers();
	if ( !result ) { succeeded = false; }

	return succeeded;
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
	animator.Init();
	animator.SetInterpolateFlag( true );
}
void Player::MotionManager::Update( Player &player, float elapsedTime )
{
	animator.Update( elapsedTime );
}
void Player::MotionManager::SetIdle()
{
	ResetAnimation();
}
void Player::MotionManager::SetRun()
{
	ResetAnimation();
}
void Player::MotionManager::SetJump()
{
	ResetAnimation();
}
void Player::MotionManager::SetFall()
{
	ResetAnimation();
}
Player::MotionManager::Bundle Player::MotionManager::CalcNowModel( Player &player ) const
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

	auto Convert	= []( const std::unique_ptr<PlayerModel::StorageBundle> *pStorage )->Player::MotionManager::Bundle
	{
		const std::unique_ptr<PlayerModel::StorageBundle> &refStorage = *pStorage;
		return Player::MotionManager::Bundle
		{
			refStorage->pModel,
			&refStorage->motionsPerMesh
		};
	};
	auto Getter		= []( PlayerModel::Kind kind )
	{
		// For typing easily.
		return PlayerModel::GetModelBundleOrNullptr( kind );
	};

	// if ( IsDead() ) { return PlayerModel::GetModelPtr( PlayerModel::Kind::Dead ); }
	if ( IsJump() ) { return Convert( Getter( PlayerModel::Kind::Jump ) ); }
	if ( IsRun()  ) { return Convert( Getter( PlayerModel::Kind::Run  ) ); }
	if ( IsIdle() ) { return Convert( Getter( PlayerModel::Kind::Idle ) ); }
	// else

	return Player::MotionManager::Bundle{ nullptr, nullptr };
}
void Player::MotionManager::ResetAnimation()
{
	animator.SetCurrentElapsedTime( 0.0f );
}

void Player::NormalMover::Init( Player &player )
{
	const auto data = FetchMember();
	player.hitBox = data.normal.hitBoxStage;
}
void Player::NormalMover::Uninit( Player &player ) {}
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
			velocityXZ = velocityXZ.Normalized() * data.normal.maxSpeed;
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
	tilt = 0.0f;

	const auto data = FetchMember();

	player.hitBox = data.oiled.basic.hitBoxStage;

	Donya::Vector3 initVelocity = player.orientation.LocalFront() * data.oiled.basic.maxSpeed;
	AssignToXZ( &player.velocity, initVelocity );
}
void Player::OilMover::Uninit( Player &player )
{
	AssignToXZ( &player.velocity, Donya::Vector2::Zero() );
}
void Player::OilMover::Move( Player &player, float elapsedTime, Input input )
{
#if DEBUG_MODE
	pitch += ToRadian( 6.0f );
#endif // DEBUG_MODE

	input.moveVectorXZ.Normalize();

	const auto data = FetchMember();

	float betweenCross{};
	float betweenRadian{};
	{
		Donya::Vector2 nXZFront = ToXZVector( player.orientation.LocalFront().Normalized() );
		
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

		// Speed that the Y component is excepted.
		const float currentSpeed = ToXZVector( player.velocity ).Length();

		Donya::Vector3 rotatedVelocity = player.orientation.LocalFront() * currentSpeed;
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
	Donya::Quaternion pitching = Donya::Quaternion::Make( player.orientation.LocalRight(), pitch );
	Donya::Quaternion tilting  = Donya::Quaternion::Make( player.orientation.LocalFront(), ToRadian( -tilt ) );
	return pitching.Rotated( tilting ); // Rotation: First:Pitch, Then:Tilt.
}

void Player::DeadMover::Init( Player &player )
{
	player.velocity = 0.0f;
	player.hitBox.exist = false;
}
void Player::DeadMover::Uninit( Player &player ) {}
void Player::DeadMover::Move( Player &player, float elapsedTime, Input input ) {}
void Player::DeadMover::Jump( Player &player, float elapsedTime ) {}
void Player::DeadMover::Fall( Player &player, float elapsedTime ) {}

void Player::Init( const Donya::Vector3 &wsInitialPos )
{
	ParamPlayer::Get().Init();
	const auto data = FetchMember();

	pos			= wsInitialPos;
	velocity	= 0.0f;
	orientation	= Donya::Quaternion::Identity();

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

	{
		if ( input.useOil )
		{
			( pMover->IsOiled() )
			? ResetMover<NormalMover>()
			: ResetMover<OilMover>();

			Donya::Sound::Play( Music::PlayerTrans );
		}
	}

	Move( elapsedTime, input );

	if ( input.useJump && onGround )
	{
		Jump( elapsedTime );
	}

	Fall( elapsedTime );

	motionManager.Update( *this, elapsedTime );
}

void Player::PhysicUpdate( const std::vector<Donya::AABB> &solids, const Donya::StaticMesh *pTerrain, const Donya::Vector4x4 *pTerrainMat )
{
	if ( pMover->IsDead() ) { return; }
	// else

	// For judge that to: "was landing?".
	const Donya::Vector3 oldPos = pos;

	const auto data = FetchMember();
	Actor::Move( velocity, data.raypickOffsets, solids, pTerrain, pTerrainMat );

	bool wasCorrectedV = WasCorrectedVertically( oldPos, pTerrain );
	if ( wasCorrectedV )
	{
		if ( velocity.y <= 0.0f )
		{
			AssignLanding();
		}

		velocity.y = 0.0f;
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

void Player::Draw( const Donya::Vector4x4 &matVP, const Donya::Vector4 &cameraPos, const Donya::Vector4 &lightDir )
{
	const auto data = FetchMember();
	const Donya::Quaternion actualOrientation = orientation.Rotated( pMover->GetExtraRotation( *this ) );
	const Donya::Vector4 bodyColor = ( pMover->IsDead() )
		? Donya::Vector4{ 1.0f, 0.5f, 0.0f, 1.0f }
		: Donya::Vector4{ 0.1f, 1.0f, 0.3f, 1.0f };
	const Donya::Vector3 drawOffset = actualOrientation.RotateVector( data.drawOffset );

	Donya::Vector4x4 W{};
	W._11 = data.drawScale;
	W._22 = data.drawScale;
	W._33 = data.drawScale;
	W *= actualOrientation.RequireRotationMatrix();
	W._41 = pos.x + drawOffset.x;
	W._42 = pos.y + drawOffset.y;
	W._43 = pos.z + drawOffset.z;

	auto modelBundle	= motionManager.CalcNowModel( *this );
	auto animator		= motionManager.GetAnimator();

	if ( modelBundle.pNowModel )
	{
		auto MakeConstantsPerScene = [&]()
		{
			PlayerModel::CBuffer::PerScene constants{};
			constants.eyePos			= cameraPos;
			constants.lightColor		= Donya::Vector4{ 1.0f, 1.0f, 1.0f, 1.0f };
			constants.lightDirection	= lightDir;
			// Donya::Vector3 lightDir3{ lightDir.x, lightDir.y, lightDir.z };
			// constants.lightDirection	= Donya::Vector4{ actualOrientation.RotateVector( lightDir3 ), 0.0f };
			return constants;
		};
		auto MakeConstantsPerModel = []( const Donya::Vector4 &color )
		{
			PlayerModel::CBuffer::PerModel constants{};
			constants.materialColor = color;
			return constants;
		};
		PlayerModel::UpdateCBuffer
		(
			MakeConstantsPerScene(),
			MakeConstantsPerModel( bodyColor )
		);

		PlayerModel::SettingOption optScene{};
		optScene.setSlot = 0;
		optScene.setVS = true;
		optScene.setPS = true;
		PlayerModel::SettingOption optModel{};
		optModel.setSlot = 1;
		optModel.setVS = true;
		optModel.setPS = true;
		PlayerModel::ActivateCBuffer( optScene, optModel );

		Donya::SkinnedMesh::CBSetOption optMesh{};
		optMesh.setSlot = 2;
		optMesh.setVS = true;
		optMesh.setPS = true;
		Donya::SkinnedMesh::CBSetOption optSubset{};
		optSubset.setSlot = 3;
		optSubset.setVS = true;
		optSubset.setPS = true;
		constexpr int samplerSlot		= 0;
		constexpr int diffuseMapSlot	= 0;

		PlayerModel::ActivateShader();

		modelBundle.pNowModel->Render
		(
			*modelBundle.pNowMotions, animator,
			W * matVP, W,
			optMesh,
			optSubset,
			samplerSlot,
			diffuseMapSlot
		);

		PlayerModel::DeactivateShader();
		PlayerModel::DeactivateCBuffer();
	}

#if DEBUG_MODE
	if ( Common::IsShowCollision() )
	{
		const Donya::Vector4 subForHitBox{ 0.0f, 0.0f, 0.0f, 0.7f };
		DrawHitBox( matVP, actualOrientation, bodyColor - subForHitBox );
	}
#endif // DEBUG_MODE
}

void Player::LookToInput( float elapsedTime, Input input )
{
	Donya::Vector3 frontXZ = orientation.LocalFront(); frontXZ.y = 0.0f;
	Donya::Vector3 inputXZ{ input.moveVectorXZ.x, 0.0f, input.moveVectorXZ.y };

	orientation = Donya::Quaternion::LookAt( orientation, inputXZ.Normalized(), Donya::Quaternion::Freeze::Up );
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

bool Player::WasCorrectedVertically( const Donya::Vector3 &oldPos, const Donya::StaticMesh *pTerrain ) const
{
	const float diffY = pos.y - oldPos.y;
	
	// If the actual movement is lower than velocity, that represents to was corrected.
	bool wasCorrected = ( fabsf( diffY ) < fabsf( velocity.y ) - 0.001f );

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
		Donya::Sound::Play( Music::PlayerLanding );
	}

	onGround	= true;
	velocity.y	= 0.0f;
}

void Player::Die()
{
	ResetMover<DeadMover>();
	onGround = false;
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
		ImGui::Checkbox( u8"地上にいる？",	&onGround );
		ImGui::Checkbox( u8"あぶら状態か？",	&nowOiled );

		ImGui::TreePop();
	}

	ImGui::End();
}
#endif // USE_IMGUI
