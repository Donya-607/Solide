#pragma once

#include <vector>

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
	Donya::Vector3 velocity;
	Donya::Vector3 gravity;
public:
	void Init();
	void Uninit();

	void Update( float elapsedTime, Input input );
	void PhysicUpdate( const std::vector<Solid> &collisions );

	void Draw( const Donya::Vector4x4 &matVP );
private:

};
