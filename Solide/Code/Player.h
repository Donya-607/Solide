#pragma once

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
		bool useShot;
		bool useTrans;
	};
private:
	float				speed{};
	Donya::Quaternion	orientation;
	Donya::Vector3		velocity;
	bool				onGround = false;
public:
	void Init();
	void Uninit();

	void Update( float elapsedTime, Input input );
	void PhysicUpdate( const std::vector<Solid> &collisions );

	void Draw( const Donya::Vector4x4 &matVP );
private:
	void Move( float elapsedTime, Input input );

	void Jump( float elapsedTime );
	void Fall( float elapsedTime );
	void AssignLanding();
private:
#if USE_IMGUI
	void UseImGui();
#endif // USE_IMGUI
};
