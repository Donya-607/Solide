#include "Player.h"

#include <algorithm>			// For std::max(), min()

#include <cereal/types/vector.hpp>

#include "Donya/Constant.h"		// For DEBUG_MODE macro.
#include "Donya/Serializer.h"
#include "Donya/Useful.h"		// For ZeroEqual().

#if DEBUG_MODE
#include "Donya/Keyboard.h"
#endif // DEBUG_MODE

#include "FilePath.h"
#include "Parameter.h"

#undef max
#undef min

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

		BasicMember		normal;
		OilMember		oiled;
		std::vector<Donya::Vector3>	raypickOffsets;
	public:
		bool isValid = true; // Use for validation of dynamic_cast. Do not serialize.
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
				// archive( CEREAL_NVP( x ) );
			}
		}
	};
}
CEREAL_CLASS_VERSION( Member,				2 )
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

			ShowIONode();

			ImGui::TreePop();
		}

		ImGui::End();
	}
#endif // USE_IMGUI
};

// INternal utility.
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

void Player::Init()
{
	ParamPlayer::Get().Init();
	const auto data = FetchMember();

	velocity	= 0.0f;
	orientation	= Donya::Quaternion::Identity();

	ResetMover<NormalMover>();
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

#if DEBUG_MODE
	{
		if ( input.useOil )
		{
			( pMover->IsOiled() )
			? ResetMover<NormalMover>()
			: ResetMover<OilMover>();
		}
	}
#endif // DEBUG_MODE


	Move( elapsedTime, input );

	if ( input.useJump && onGround )
	{
		Jump( elapsedTime );
	}

	Fall( elapsedTime );
}

void Player::PhysicUpdate( const Donya::StaticMesh *pTerrain, const Donya::Vector4x4 *pTerrainMat )
{
	const Donya::Vector3 oldPos = pos;

	const auto data = FetchMember();
	Actor::Move( velocity, data.raypickOffsets, pTerrain, pTerrainMat );

	float diffY = pos.y - oldPos.y;
	bool  wasLanding = ( fabsf( diffY ) < fabsf( velocity.y ) - 0.001f ); // If the actual movement is lower than velocity, that represents to was landing.
	if ( !pTerrain || !pTerrainMat ) // If the terrain is nothing, the criteria of landing is 0.0f.
	{
		wasLanding = wasLanding || ( pos.y < 0.0f + hitBox.size.y );
	}
	if (  wasLanding )
	{
		AssignLanding();
	}
}

void Player::Draw( const Donya::Vector4x4 &matVP )
{
	const Donya::Quaternion actualOrientation = orientation.Rotated( pMover->GetExtraRotation( *this ) );
#if DEBUG_MODE
	DrawHitBox( matVP, actualOrientation, { 0.1f, 1.0f, 0.3f, 1.0f } );
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
}
void Player::Fall( float elapsedTime )
{
	pMover->Fall( *this, elapsedTime );
}
void Player::AssignLanding()
{
	onGround	= true;
	velocity.y	= 0.0f;
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
		if ( ImGui::Button( u8"Ｙ座標と速度をリセット" ) )
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
