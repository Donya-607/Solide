#pragma once

#include <memory>
#include <vector>

#include "Donya/Quaternion.h"
#include "Donya/UseImGui.h"
#include "Donya/Vector.h"

#include "ObjectBase.h"

class Player : public Actor
{
public:
	struct Input
	{
		Donya::Vector2 moveVectorXZ;	// Y component will function as Z.
		bool useJump;
		bool useOil;
	};
private:
	class IMover
	{
	public:
		virtual void Move( Player &player, float elapsedTime, Input input ) = 0;
	};
	class NormalMover : public IMover
	{
	public:
		void Move( Player &player, float elapsedTime, Input input ) override;
	};
	class OilMover : public IMover
	{
	public:
		void Move( Player &player, float elapsedTime, Input input ) override;
	};
private:
	Donya::Vector3			velocity;
	Donya::Quaternion		orientation;
	std::unique_ptr<IMover>	pMover;
	bool					onGround = false;
public:
	void Init();
	void Uninit();

	void Update( float elapsedTime, Input input );
	void PhysicUpdate( const std::vector<Solid> &collisions );

	void Draw( const Donya::Vector4x4 &matVP );
private:
	void LookToInput( float elapsedTime, Input input );
	void Move( float elapsedTime, Input input );

	void Jump( float elapsedTime );
	void Fall( float elapsedTime );
	void AssignLanding();
private:
#if USE_IMGUI
	void UseImGui();
#endif // USE_IMGUI
};
