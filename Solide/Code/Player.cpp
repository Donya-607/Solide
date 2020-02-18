#include "Player.h"

#include "Donya/Constant.h" // For DEBUG_MODE macro.
#include "Donya/UseImGui.h"

#include "Parameter.h"

class ParamPlayer : public ParameterBase
{
public:
	struct Member
	{

	};
private:
	Member m;
public:
	void Init() override
	{
	#if DEBUG_MODE
		LoadJson();
	#else
		LoadBin();
	#endif // DEBUG_MODE
	}
	void Uninit() override {}
private:
	void LoadBin() override
	{

	}
	void LoadJson() override
	{

	}
	void SaveBin() override
	{

	}
	void SaveJson() override
	{

	}
public:
#if USE_IMGUI
	void UseImGui() override
	{

	}
#endif // USE_IMGUI
};

void Player::Init()
{
	velocity = 0.0f;
	gravity.y = -0.01f;
}
void Player::Uninit()
{

}

void Player::Update( float elapsedTime, Input input )
{
	velocity.x = input.moveVectorXZ.x * elapsedTime;
	velocity.z = input.moveVectorXZ.y * elapsedTime;

	velocity += gravity * elapsedTime;
}

void Player::PhysicUpdate( const std::vector<Solid> &collisions )
{
	Move( velocity, collisions );
}

void Player::Draw( const Donya::Vector4x4 &matVP )
{
#if DEBUG_MODE
	DrawHitBox( matVP, { 0.1f, 0.3f, 1.0f, 1.0f } );
#endif // DEBUG_MODE
}
