#include "Player.h"

#include "Donya/Constant.h"		// For DEBUG_MODE macro.
#include "Donya/Serializer.h"

#include "FilePath.h"
#include "Parameter.h"

namespace
{
	struct Member
	{
		float gravity;		// Positive value.
		float moveSpeed;	// Positive value.
		float jumpStrength;	// Positive value.
	public:
		bool  isValid = true; // Use for validation of dynamic_cast. Do not serialize.
	private:
		friend class cereal::access;
		template<class Archive>
		void serialize( Archive &archive, std::uint32_t version )
		{
			if ( version == 0 )
			{
				archive
				(
					CEREAL_NVP( gravity ),
					CEREAL_NVP( moveSpeed ),
					CEREAL_NVP( jumpStrength )
				);
			}
		}
	};
}
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

		if ( ImGui::TreeNode( u8"���@�̃p�����[�^����" ) )
		{
			ImGui::DragFloat( u8"�ړ����x",			&m.moveSpeed,		0.1f, 0.0f );
			ImGui::DragFloat( u8"�W�����v�E����",		&m.jumpStrength,	0.1f, 0.0f );
			ImGui::DragFloat( u8"�d��",				&m.gravity,			0.1f, 0.0f );

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

	velocity = 0.0f;
}
void Player::Uninit()
{}

void Player::Update( float elapsedTime, Input input )
{
#if USE_IMGUI
	if ( FindHelper() )
	{
		( *FindHelper() )->UseImGui();
	}
	UseImGui();
#endif // USE_IMGUI

	const auto data = FetchMember();

	velocity.x = input.moveVectorXZ.x * data.moveSpeed * elapsedTime;
	velocity.z = input.moveVectorXZ.y * data.moveSpeed * elapsedTime;

	if ( input.useJump )
	{
		onGround = false;
		velocity.y = data.jumpStrength;
	}

	velocity.y -= data.gravity * elapsedTime;

	// TODO : �A�N�^�N���X�̓s���œ����蔻�胁���o���X�V���Ȃ��Ƃ����Ȃ��B�T�C�Y�͒萔�ŗǂ��B
	// �A�N�^�N���X�ɍ��W��ǉ����Ă���̂ŁC���������Q�Ƃ���悤�ɕύX���Ă����B
	hitBox.pos = pos;
	hitBox.size = 0.5f;
}

void Player::PhysicUpdate( const std::vector<Solid> &collisions )
{
	Move( velocity, collisions );

	if ( pos.y <= 0.0f )
	{
		pos.y = 0.0f;
		velocity.y = 0.0f;
		onGround = true;
	}
}

void Player::Draw( const Donya::Vector4x4 &matVP )
{
#if DEBUG_MODE
	DrawHitBox( matVP, { 0.1f, 1.0f, 0.3f, 1.0f } );
#endif // DEBUG_MODE
}

#if USE_IMGUI
void Player::UseImGui()
{
	if ( !ImGui::BeginIfAllowed() ) { return; }
	// else

	if ( ImGui::TreeNode( u8"���@�̍��̏�" ) )
	{
		ImGui::DragFloat3( u8"���W", &pos.x,			0.01f );
		ImGui::DragFloat3( u8"���x", &velocity.x,	0.01f );
		ImGui::Checkbox( u8"�n��ɂ���H", &onGround );

		ImGui::TreePop();
	}

	ImGui::End();
}
#endif // USE_IMGUI
