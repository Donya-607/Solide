#pragma once

#include <memory>
#include <vector>

#include "Donya/ModelMotion.h"
#include "Donya/ModelPolygon.h"
#include "Donya/ModelPose.h"
#include "Donya/Quaternion.h"
#include "Donya/SkinnedMesh.h"
#include "Donya/UseImGui.h"
#include "Donya/Vector.h"

#include "ObjectBase.h"
#include "Renderer.h"

class Player : public Actor
{
public:
	static bool LoadModels();
public:
	struct Input
	{
		Donya::Vector2 moveVectorXZ;	// Y component will function as Z.
		bool useJump;
		bool useOil;
	};
private:
	class MotionManager
	{
	private:
		Donya::Model::Animator	animator;
		Donya::Model::Pose		pose;
	public:
		void Init();
		void Update( Player &player, float elapsedTime );
	public:
		const Donya::Model::Pose &GetPose() const;
	private:
		void AssignPose( int motionIndex );
		int  CalcNowKind( Player &player ) const;
	};

	class MoverBase
	{
	public:
		virtual void Init( Player &player ) = 0;
		virtual void Uninit( Player &player ) = 0;
		virtual void Update( Player &player, float elapsedTime ) = 0;
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
		void Update( Player &player, float elapsedTime ) override;
	public:
		void Move( Player &player, float elapsedTime, Input input ) override;
		void Jump( Player &player, float elapsedTime ) override;
		void Fall( Player &player, float elapsedTime ) override;
	};
	class OilMover : public MoverBase
	{
	private:
		float tilt  = 0.0f; // Degree.
	public:
		void Init( Player &player ) override;
		void Uninit( Player &player ) override;
		void Update( Player &player, float elapsedTime ) override;
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
		void Update( Player &player, float elapsedTime ) override;
	public:
		void Move( Player &player, float elapsedTime, Input input ) override;
		void Jump( Player &player, float elapsedTime ) override;
		void Fall( Player &player, float elapsedTime ) override;
	public:
		bool IsDead() const override { return true; }
	};
private:
	float						hopPitching = 0.0f;	// Radian. Use when hopping that will happen when used an oil.
	Donya::Vector3				velocity;
	Donya::Quaternion			orientation;
	std::unique_ptr<MoverBase>	pMover;
	MotionManager				motionManager;
	bool						onGround  = false;
	bool						canUseOil = true;	// Will recovery when landing.
public:
	void Init( const Donya::Vector3 &wsInitialPos );
	void Uninit();

	void Update( float elapsedTime, Input input );
	void PhysicUpdate( const std::vector<Donya::AABB> &solids = {}, const Donya::Model::PolygonGroup *pTerrain = nullptr, const Donya::Vector4x4 *pTerrainWorldMatrix = nullptr );

	void Draw( RenderingHelper *pRenderer );
	void DrawHitBox( RenderingHelper *pRenderer, const Donya::Vector4x4 &matVP );
public:
	bool IsDead() const
	{
		return pMover->IsDead();
	}

	// These IsOiled() and GetOrientation() are used for controll when title scene.

	bool IsOiled() const
	{
		return pMover->IsOiled();
	}
	Donya::Quaternion GetOrientation() const
	{
		return orientation;
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

	bool WasCorrectedVertically( const Donya::Vector3 &oldPos, const Donya::Model::PolygonGroup *pTerrain ) const;
	void AssignLanding();

	void Die();
private:
	void StartHopping();
	void UpdateHopping( float elapsedTime );
private:
#if USE_IMGUI
	void UseImGui();
#endif // USE_IMGUI
};
