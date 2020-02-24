#include "Player.h"

#include "Donya/Constant.h"		// For DEBUG_MODE macro.
#include "Donya/Serializer.h"
#include "Donya/Useful.h"		// For ZeroEqual().

#if DEBUG_MODE
#include "Donya/Keyboard.h"
#endif // DEBUG_MODE

#include "FilePath.h"
#include "Parameter.h"

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

				if ( version <= 1 )
				{
					// archive( CEREAL_NVP( x ) );
				}
			}
		};
		struct OilMember
		{
			BasicMember	basic;
			float		turnDegree		= 1.0f;		// Per frame.
			float		turnThreshold	= 0.4f;		// 0.0f ~ 1.0f, absolute.
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

				if ( version <= 1 )
				{
					// archive( CEREAL_NVP( x ) );
				}
			}
		};

		BasicMember	normal;
		OilMember	oiled;
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

			if ( version <= 1 )
			{
				archive( CEREAL_NVP( oiled ) );
			}
			if ( version <= 2 )
			{
				// archive( CEREAL_NVP( x ) );
			}
		}
	};
}
CEREAL_CLASS_VERSION( Member,				1 )
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

		if ( ImGui::TreeNode( u8"é©ã@ÇÃÉpÉâÉÅÅ[É^í≤êÆ" ) )
		{
			auto ShowBasicNode = []( const std::string &prefix, Member::BasicMember *p )
			{
				if ( !ImGui::TreeNode( prefix.c_str() ) ) { return; }
				// else
				
				ImGui::DragFloat( ( prefix + u8"ÅFâ¡ë¨ó "	).c_str(),		&p->accel,			0.01f, 0.0f );
				ImGui::DragFloat( ( prefix + u8"ÅFå∏ë¨ó "	).c_str(),		&p->decel,			0.01f, 0.0f );
				ImGui::DragFloat( ( prefix + u8"ÅFç≈çÇë¨ìx"	).c_str(),		&p->maxSpeed,		0.01f, 0.0f );
				ImGui::DragFloat( ( prefix + u8"ÅFíµñÙóÕ"	).c_str(),		&p->jumpStrength,	0.01f, 0.0f );
				ImGui::DragFloat( ( prefix + u8"ÅFèdóÕ"		).c_str(),		&p->gravity,		0.01f, 0.0f );
				ParameterHelper::ShowAABBNode( prefix + u8"ÅFìñÇΩÇËîªíËÅEÇuÇrínå`", &p->hitBoxStage );

				ImGui::TreePop();
			};

			ShowBasicNode( u8"í èÌéû", &m.normal );

			if ( ImGui::TreeNode( u8"ÉIÉCÉãéû" ) )
			{
				ShowBasicNode( u8"ï®óùãììÆ", &m.oiled.basic );

				ImGui::DragFloat( u8"âÒì]Ç≥ÇπÇÈÇµÇ´Ç¢íl",		&m.oiled.turnThreshold,	0.01f, 0.0f, 0.99f	);
				ImGui::DragFloat( u8"ÇPÇeÇ…ã»Ç™ÇÈäpìx",		&m.oiled.turnDegree,	0.1f, 0.0f, 180.0f	);
				ImGui::Text( "" );
				ImGui::DragFloat( u8"ÇPÇeÇ…åXÇØÇÈäpìx",		&m.oiled.tiltDegree,	0.1f, 0.0f, 180.0f	);
				ImGui::DragFloat( u8"ÇPÇeÇ…åXÇ´ÇñﬂÇ∑äpìx",	&m.oiled.untiltDegree,	0.1f, 0.0f, 180.0f	);
				ImGui::DragFloat( u8"åXÇ≠äpìxÇÃç≈ëÂ",			&m.oiled.maxTiltDegree, 0.1f, 0.0f, 180.0f	);

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
	const auto data = FetchMember();

	// Doing untilt only.
	if ( fabsf( input.moveVectorXZ.x ) < data.oiled.turnThreshold )
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

	const int sideSign = Donya::SignBit( input.moveVectorXZ.x );

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
Donya::Quaternion Player::OilMover::GetExtraRotation() const
{
	return Donya::Quaternion::Make( Donya::Vector3::Front(), ToRadian( -tilt ) );
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
		if ( Donya::Keyboard::Trigger( 'N' ) )
		{
			ResetMover<NormalMover>();
		}
		if ( Donya::Keyboard::Trigger( 'O' ) )
		{
			ResetMover<OilMover>();
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

void Player::PhysicUpdate( const std::vector<Solid> &collisions )
{
	Actor::Move( velocity, collisions );

	if ( pos.y <= 0.0f + hitBox.size.y )
	{
		AssignLanding();
	}
}

void Player::Draw( const Donya::Vector4x4 &matVP )
{
	const Donya::Quaternion actualOrientation = orientation.Rotated( pMover->GetExtraRotation() );
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
	pos.y		= 0.0f + hitBox.size.y;
	velocity.y	= 0.0f;
}

#if USE_IMGUI
void Player::UseImGui()
{
	if ( !ImGui::BeginIfAllowed() ) { return; }
	// else

	if ( ImGui::TreeNode( u8"é©ã@ÇÃç°ÇÃèÛãµ" ) )
	{
		ImGui::DragFloat3( u8"ç¿ïW", &pos.x,			0.01f );
		ImGui::DragFloat3( u8"ë¨ìx", &velocity.x,	0.01f );

		bool nowOiled = pMover->IsOiled(); // Immutable.
		ImGui::Checkbox( u8"ínè„Ç…Ç¢ÇÈÅH",	&onGround );
		ImGui::Checkbox( u8"Ç†Ç‘ÇÁèÛë‘Ç©ÅH",	&nowOiled );

		ImGui::TreePop();
	}

	ImGui::End();
}
#endif // USE_IMGUI
