#pragma once

#include <memory>
#include <string>
#include <vector>

#include "Donya/ModelMotion.h"
#include "Donya/ModelPolygon.h"
#include "Donya/ModelPose.h"
#include "Donya/Quaternion.h"
#include "Donya/Serializer.h"
#include "Donya/SkinnedMesh.h"
#include "Donya/UseImGui.h"
#include "Donya/Vector.h"

#include "ObjectBase.h"
#include "Element.h"
#include "Renderer.h"


class PlayerInitializer
{
private:
	Donya::Vector3		wsInitialPos;
	Donya::Quaternion	initialOrientation;
private:
	friend class cereal::access;
	template<class Archive>
	void serialize( Archive &archive, std::uint32_t version )
	{
		archive
		(
			CEREAL_NVP( wsInitialPos ),
			CEREAL_NVP( initialOrientation )
		);

		if ( 1 <= version )
		{
			// archive( CEREAL_NVP( x ) );
		}
	}
	static constexpr const char *ID = "PlayerInit";
public:
	Donya::Vector3		GetInitialPos() const;
	Donya::Quaternion	GetInitialOrientation() const;
public:
	void LoadParameter( int stageNo );
private:
	void LoadBin ( int stageNo );
	void LoadJson( int stageNo );
#if USE_IMGUI
	void SaveBin ( int stageNo );
	void SaveJson( int stageNo );
public:
	void ShowImGuiNode( const std::string &nodeCaption, int stageNo, bool allowShowIONode = true );
#endif // USE_IMGUI
};
CEREAL_CLASS_VERSION( PlayerInitializer, 0 )

class EffectHandle;
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
		int prevKind = 0;
		int currKind = 0;
		Donya::Model::Animator	animator;
		Donya::Model::Pose		pose;
	public:
		void Init();
		void Update( Player &player, float elapsedTime );
	public:
		const Donya::Model::Pose &GetPose() const;
	private:
		bool ShouldEnableLoop( int kind ) const;
		void AssignPose( int motionIndex );
		int  CalcNowKind( Player &player ) const;
	};

	class InputManager
	{
	private:
		int		oilTimer = 0;
		Input	prevInput;
		Input	currInput;
		bool	beginPressWasOiled		= false;	// The status of "isOiled?" when starting the using oil.
		bool	keepingPressAfterTrans	= false;
		bool	prevIsOiled				= false;
		bool	currIsOiled				= false;
	public:
		void Init();
		void Update( const Player &player, const Input &input );
	public:
		bool ShouldJump () const;
		bool ShouldShot () const;
		bool ShouldTrans() const;
	private:
		bool IsTriggerOil() const;
		bool IsReleaseOil() const;
		bool IsPressOil() const;	// Returns "now pressing?", but I chosen consistency.
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
	int								remains		= 1;
	int								burnTimer	= 0;
	float							hopPitching	= 0.0f;		// Radian. Use when hopping that will happen when used an oil.
	mutable Element					element;				// Will change in const method.
	Donya::Vector3					velocity;
	Donya::Quaternion				orientation;
	std::unique_ptr<MoverBase>		pMover;
	MotionManager					motionManager;
	InputManager					inputManager;
	std::shared_ptr<EffectHandle>	pEffect		= nullptr;	// Will used as burning effect.
	bool							onGround	= false;
	bool							onIce		= false;
	bool							canUseOil	= true;		// Will recovery when landing.
public:
	void Init( const PlayerInitializer &parameter );
	void Uninit();

	void Update( float elapsedTime, Input input );
	void PhysicUpdate( const std::vector<Donya::AABB> &solids = {}, const Donya::Model::PolygonGroup *pTerrain = nullptr, const Donya::Vector4x4 *pTerrainWorldMatrix = nullptr );

	void Draw( RenderingHelper *pRenderer );
	void DrawHitBox( RenderingHelper *pRenderer, const Donya::Vector4x4 &matVP );
public:
	void MakeDamage( const Element &effect ) const;
	void JumpByStand();
	void ReduceRemains();
	void ReviveRemains();
	void KillMe();
public:
	std::vector<Element::Type> GetUncollidableTypes() const;
	bool IsDead() const
	{
		return pMover->IsDead();
	}
	bool IsOiled() const
	{
		return element.Has( Element::Type::Oil );
	}
	bool OnGround() const
	{
		return onGround;
	}
	bool OnIce() const
	{
		return onIce && onGround;
	}
	Donya::Vector3		GetVelocity() const
	{
		return velocity;
	}
	Donya::Quaternion	GetOrientation() const
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

	void Shot( float elapsedTime );

	bool WillDie() const;

	Donya::Vector4 CalcDrawColor() const;
private:
	void StartHopping();
	void UpdateHopping( float elapsedTime );

	void BurnUpdate( float elapsedTime );
private:
#if USE_IMGUI
	void UseImGui();
#endif // USE_IMGUI
};
