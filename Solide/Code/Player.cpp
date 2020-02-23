#include "Player.h"

#include "Donya/Constant.h"		// For DEBUG_MODE macro.
#include "Donya/Serializer.h"

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

		BasicMember normal;
	public:
		bool  isValid		= true; // Use for validation of dynamic_cast. Do not serialize.
	private:
		friend class cereal::access;
		template<class Archive>
		void serialize( Archive &archive, std::uint32_t version )
		{
			archive
			(
				CEREAL_NVP( normal ),
			);

			if ( version <= 1 )
			{
				// archive( CEREAL_NVP( x ) );
			}
		}
	};
}
CEREAL_CLASS_VERSION( Member,				0 )
CEREAL_CLASS_VERSION( Member::BasicMember,	0 )

class ParamPlayer : public ParameterBase
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
			auto ShowAABB = []( const std::string &prefix, Donya::AABB *p )
			{
				ImGui::DragFloat3( ( prefix + u8"：中心のオフセット" ).c_str(),		&p->pos.x,  0.01f );
				ImGui::DragFloat3( ( prefix + u8"：サイズ（半分を指定）" ).c_str(),	&p->size.x, 0.01f );
				ImGui::Checkbox  ( ( prefix + u8"：判定を有効にする" ).c_str(),		&p->exist );
			};
			auto ShowBasicNode = [&ShowAABB]( const std::string &prefix, Member::BasicMember *p )
			{
				if ( !ImGui::TreeNode( u8"通常時" ) ) { return; }
				// else
				
				ImGui::DragFloat( ( prefix + u8"：加速量"	).c_str(),		&p->accel,			0.1f, 0.0f );
				ImGui::DragFloat( ( prefix + u8"：減速量"	).c_str(),		&p->decel,			0.1f, 0.0f );
				ImGui::DragFloat( ( prefix + u8"：最高速度"	).c_str(),		&p->maxSpeed,		0.1f, 0.0f );
				ImGui::DragFloat( ( prefix + u8"：跳躍力"	).c_str(),		&p->jumpStrength,	0.1f, 0.0f );
				ImGui::DragFloat( ( prefix + u8"：重力"		).c_str(),		&p->gravity,		0.1f, 0.0f );
				ShowAABB( prefix + u8"：当たり判定・ＶＳ地形", &p->hitBoxStage );

				ImGui::TreePop();
			};

			ShowBasicNode( u8"通常時", &m.normal );

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
		return ParameterStorage::Get().Find( ParamPlayer::ID );
	}

	Member FetchMember()
	{
		Member nil{}; nil.isValid = false;

		auto  pBase = FindHelper();
		if ( !pBase ) { return nil; }
		// else

		ParamPlayer *pDerived = dynamic_cast<ParamPlayer *>( pBase->get() );
		return ( pDerived ) ? pDerived->Data() : nil;
	}
}

void Player::Init()
{
	if ( !FindHelper() )
	{
		ParameterStorage::Get().Register<ParamPlayer>( ParamPlayer::ID );
	}

	if ( FindHelper() )
	{
		( *FindHelper() )->Init();
	}

	const auto data = FetchMember();
	velocity	= 0.0f;
	hitBox		= data.normal.hitBoxStage;
}
void Player::Uninit()
{
	if ( FindHelper() )
	{
		( *FindHelper() )->Uninit();
	}
}

void Player::Update( float elapsedTime, Input input )
{
#if USE_IMGUI
	if ( FindHelper() )
	{
		( *FindHelper() )->UseImGui();
	}
	UseImGui();
#endif // USE_IMGUI

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
#if DEBUG_MODE
	DrawHitBox( matVP, { 0.1f, 1.0f, 0.3f, 1.0f } );
#endif // DEBUG_MODE
}

void Player::Move( float elapsedTime, Input input )
{
	const auto data = FetchMember();

	velocity.x = input.moveVectorXZ.x * data.moveSpeed * elapsedTime;
	velocity.z = input.moveVectorXZ.y * data.moveSpeed * elapsedTime;
}

void Player::Jump( float elapsedTime )
{
	const auto data = FetchMember();

	onGround = false;
	velocity.y = data.jumpStrength * elapsedTime;
}
void Player::Fall( float elapsedTime )
{
	const auto data = FetchMember();

	velocity.y -= data.gravity * elapsedTime;
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

	if ( ImGui::TreeNode( u8"自機の今の状況" ) )
	{
		ImGui::DragFloat3( u8"座標", &pos.x,			0.01f );
		ImGui::DragFloat3( u8"速度", &velocity.x,	0.01f );
		ImGui::Checkbox( u8"地上にいる？", &onGround );

		ImGui::TreePop();
	}

	ImGui::End();
}
#endif // USE_IMGUI
