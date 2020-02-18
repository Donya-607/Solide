#pragma once

#include <vector>

#include "Donya/Vector.h"
#include "Donya/UseImGui.h"

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
	Donya::Vector3	velocity;
	bool			onGround = false;
public:
	void Init();
	void Uninit();

	void Update( float elapsedTime, Input input );
	void PhysicUpdate( const std::vector<Solid> &collisions );

	void Draw( const Donya::Vector4x4 &matVP );
private:
#if USE_IMGUI
	void UseImGui();
#endif // USE_IMGUI
};
