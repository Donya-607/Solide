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
	class MoverBase
	{
	public:
		virtual void Init( Player &player ) = 0;
		virtual void Uninit( Player &player ) = 0;
	public:
		virtual void Move( Player &player, float elapsedTime, Input input ) = 0;
		virtual void Jump( Player &player, float elapsedTime ) = 0;
		virtual void Fall( Player &player, float elapsedTime ) = 0;
	public:
		virtual bool IsDead() const { return false; }
		virtual bool IsOiled() const { return false; }
		virtual Donya::Quaternion GetExtraRotation( Player &player ) const
		{
			return Donya::Quaternion::Identity();
		}
	};
	class NormalMover : public MoverBase
	{
	public:
		void Init( Player &player ) override;
		void Uninit( Player &player ) override;
	public:
		void Move( Player &player, float elapsedTime, Input input ) override;
		void Jump( Player &player, float elapsedTime ) override;
		void Fall( Player &player, float elapsedTime ) override;
	};
	class OilMover : public MoverBase
	{
	private:
		float tilt  = 0.0f; // Degree.
		float pitch = 0.0f; // Radian.
	public:
		void Init( Player &player ) override;
		void Uninit( Player &player ) override;
	public:
		void Move( Player &player, float elapsedTime, Input input ) override;
		void Jump( Player &player, float elapsedTime ) override;
		void Fall( Player &player, float elapsedTime ) override;
	public:
		bool IsOiled() const override { return true; }
		Donya::Quaternion GetExtraRotation( Player &player ) const override;
	};
	class DeadMover : public MoverBase
	{
	public:
		void Init( Player &player ) override;
		void Uninit( Player &player ) override;
	public:
		void Move( Player &player, float elapsedTime, Input input ) override;
		void Jump( Player &player, float elapsedTime ) override;
		void Fall( Player &player, float elapsedTime ) override;
	public:
		bool IsDead() const override { return true; }
	};
private:
	Donya::Vector3				velocity;
	Donya::Quaternion			orientation;
	std::unique_ptr<MoverBase>	pMover;
	bool						onGround = false;
public:
	void Init( const Donya::Vector3 &wsInitialPos );
	void Uninit();

	void Update( float elapsedTime, Input input );
	// void PhysicUpdate( const std::vector<Solid> &collisions );
	void PhysicUpdate( const Donya::StaticMesh *pTerrain = nullptr, const Donya::Vector4x4 *pTerrainWorldMatrix = nullptr );

	void Draw( const Donya::Vector4x4 &matVP );
public:
	bool IsDead() const
	{
		return pMover->IsDead();
	}
private:
	template<class Mover>
	void ResetMover()
	{
		if ( pMover )
		{
			pMover->Uninit( *this );
		}

		pMover = std::make_unique<Mover>();
		pMover->Init( *this );
	}

	void LookToInput( float elapsedTime, Input input );
	void Move( float elapsedTime, Input input );

	void Jump( float elapsedTime );
	void Fall( float elapsedTime );
	bool IsUnderFalloutBorder() const;

	bool CalcWasLanding( const Donya::Vector3 &oldPos, const Donya::StaticMesh *pTerrain ) const;
	void AssignLanding();

	void Die();
private:
#if USE_IMGUI
	void UseImGui();
#endif // USE_IMGUI
};
