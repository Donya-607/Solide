#pragma once

#pragma once

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "Donya/Model.h"
#include "Donya/ModelMotion.h"
#include "Donya/ModelPolygon.h"
#include "Donya/ModelPose.h"
#include "Donya/Quaternion.h"
#include "Donya/Serializer.h"
#include "Donya/UseImGui.h"
#include "Donya/Vector.h"

#include "Bullet.h"
#include "ObjectBase.h"
#include "Element.h"
#include "Renderer.h"


enum class BossType
{
	Null	= -1, // Not exist
	First	= 0,

	BossCount
};


class BossInitializer
{
private:
	BossType			type = BossType::Null;
	Donya::Vector3		wsInitialPos;
	Donya::Quaternion	initialOrientation;
private:
	friend class cereal::access;
	template<class Archive>
	void serialize( Archive &archive, std::uint32_t version )
	{
		archive
		(
			CEREAL_NVP( type				),
			CEREAL_NVP( wsInitialPos		),
			CEREAL_NVP( initialOrientation	)
		);

		if ( 1 <= version )
		{
			// archive( CEREAL_NVP( x ) );
		}
	}
	static constexpr const char *ID = "BossInit";
public:
	bool				ShouldGenerateBoss() const;
	BossType			GetType() const;
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
CEREAL_CLASS_VERSION( BossInitializer, 0 )

class BossBase : public Actor
{
public:
	static bool LoadModels();
	static bool AssignDerivedClass( std::unique_ptr<BossBase> *pTarget, BossType assignType );
public:
	struct ModelResource
	{
		Donya::Model::SkinningModel	model;
		Donya::Model::MotionHolder	motionHolder;
	};
protected:
	int					hp = 3;			// 1-based. alive when hp is greater than 0.
	Donya::Vector3		velocity;
	Donya::Quaternion	orientation;
	mutable Element		element;		// Will change in const method.
	struct
	{
		std::shared_ptr<ModelResource>	pResource;
		Donya::Model::Pose				pose;
		Donya::Model::Animator			animator;
	} model;
public:
	virtual void Init( const BossInitializer &parameter );
	virtual void Uninit() {}

	virtual void Update( float elapsedTime, const Donya::Vector3 &targetPos );
	virtual void PhysicUpdate( const std::vector<Donya::AABB> &solids = {}, const Donya::Model::PolygonGroup *pTerrain = nullptr, const Donya::Vector4x4 *pTerrainWorldMatrix = nullptr );

	virtual void Draw( RenderingHelper *pRenderer ) const;
	virtual void DrawHitBox( RenderingHelper *pRenderer, const Donya::Vector4x4 &matVP ) const;
public:
	virtual void MakeDamage( const Element &effect, const Donya::Vector3 &othersVelocity ) const;
public:
	virtual bool						IsDead() const;
	virtual BossType					GetType() const = 0;
	virtual Donya::Quaternion			GetOrientation() const
	{
		return orientation;
	}
	virtual std::vector<Donya::AABB>	AcquireHitBoxes() const;
	virtual std::vector<Donya::AABB>	AcquireHurtBoxes() const;
	virtual std::vector<Element::Type>	GetUncollidableTypes() const { return {}; }
private:
	using Actor::GetHitBox;
protected:
	virtual void AssignSpecifyPose( int motionIndex );
	virtual void UpdateMotion( float elapsedTime, int motionIndex );
	virtual Donya::Vector4				CalcDrawColor() const;
	virtual	Donya::Vector4x4			CalcWorldMatrix( bool useForDrawing ) const;
	virtual std::vector<Donya::AABB>	FetchOwnHitBoxes( bool wantHurtBoxes ) const;
public:
#if USE_IMGUI
	virtual void ShowImGuiNode( const std::string &nodeCaption ) = 0;
#endif // USE_IMGUI
};


class BossFirst : public BossBase
{
public:
	enum class ActionType
	{
		Rush,
		Breath,
		Wait_Long,
		Wait_Short,
		Walk_Long,
		Walk_Short,

		ActionCount
	};
private:
#pragma region Mover
	class MoverBase
	{
	public:
		virtual void Init( BossFirst &instance );
		virtual void Uninit( BossFirst &instance ) = 0;
		virtual void Update( BossFirst &instance, float elapsedTime, const Donya::Vector3 &targetPos ) = 0;
		virtual void PhysicUpdate( BossFirst &instance, const std::vector<Donya::AABB> &solids = {}, const Donya::Model::PolygonGroup *pTerrain = nullptr, const Donya::Vector4x4 *pTerrainWorldMatrix = nullptr );
		virtual bool IsDead( const BossFirst &instance ) const;
		virtual bool AcceptDamage( const BossFirst &instance ) const;
		virtual bool AcceptDraw( const BossFirst &instance ) const;
		virtual bool ShouldChangeMover( BossFirst &instance ) const = 0;
		virtual std::function<void()> GetChangeStateMethod( BossFirst &instance ) const = 0;
		virtual std::string GetStateName() const = 0;
	};
	class Ready : public MoverBase
	{
	private:
		bool gotoNext = false;
	public:
		void Init( BossFirst &instance ) override;
		void Uninit( BossFirst &instance ) override;
		void Update( BossFirst &instance, float elapsedTime, const Donya::Vector3 &targetPos ) override;
		bool ShouldChangeMover( BossFirst &instance ) const override;
		std::function<void()> GetChangeStateMethod( BossFirst &instance ) const override;
		std::string GetStateName() const override;
	};
	class Rush : public MoverBase
	{
	private:
		bool shouldStop = false;
	public:
		void Init( BossFirst &instance ) override;
		void Uninit( BossFirst &instance ) override;
		void Update( BossFirst &instance, float elapsedTime, const Donya::Vector3 &targetPos ) override;
		void PhysicUpdate( BossFirst &instance, const std::vector<Donya::AABB> &solids = {}, const Donya::Model::PolygonGroup *pTerrain = nullptr, const Donya::Vector4x4 *pTerrainWorldMatrix = nullptr ) override;
		bool ShouldChangeMover( BossFirst &instance ) const override;
		std::function<void()> GetChangeStateMethod( BossFirst &instance ) const override;
		std::string GetStateName() const override;
	};
	class Brake : public MoverBase
	{
	private:
		bool isStopping	= false;
		bool gotoNext	= false;
	public:
		void Init( BossFirst &instance ) override;
		void Uninit( BossFirst &instance ) override;
		void Update( BossFirst &instance, float elapsedTime, const Donya::Vector3 &targetPos ) override;
		void PhysicUpdate( BossFirst &instance, const std::vector<Donya::AABB> &solids = {}, const Donya::Model::PolygonGroup *pTerrain = nullptr, const Donya::Vector4x4 *pTerrainWorldMatrix = nullptr ) override;
		bool ShouldChangeMover( BossFirst &instance ) const override;
		std::function<void()> GetChangeStateMethod( BossFirst &instance ) const override;
		std::string GetStateName() const override;
	};
	class Breath : public MoverBase
	{
	private:
		bool gotoNext = false;
	public:
		void Init( BossFirst &instance ) override;
		void Uninit( BossFirst &instance ) override;
		void Update( BossFirst &instance, float elapsedTime, const Donya::Vector3 &targetPos ) override;
		bool ShouldChangeMover( BossFirst &instance ) const override;
		std::function<void()> GetChangeStateMethod( BossFirst &instance ) const override;
		std::string GetStateName() const override;
	private:
		void Fire( BossFirst &instance, const Donya::Vector3 &targetPos, const Bullet::BulletAdmin::FireDesc &desc );
	};
	class Wait : public MoverBase
	{
	private:
		const int waitFrame = 0;
	public:
		Wait( int waitFrmae );
	public:
		void Init( BossFirst &instance ) override;
		void Uninit( BossFirst &instance ) override;
		void Update( BossFirst &instance, float elapsedTime, const Donya::Vector3 &targetPos ) override;
		bool ShouldChangeMover( BossFirst &instance ) const override;
		std::function<void()> GetChangeStateMethod( BossFirst &instance ) const override;
		std::string GetStateName() const override;
	};
	class Walk : public MoverBase
	{
	private:
		const int walkFrame = 0;
	public:
		Walk( int walkFrmae );
	public:
		void Init( BossFirst &instance ) override;
		void Uninit( BossFirst &instance ) override;
		void Update( BossFirst &instance, float elapsedTime, const Donya::Vector3 &targetPos ) override;
		bool ShouldChangeMover( BossFirst &instance ) const override;
		std::function<void()> GetChangeStateMethod( BossFirst &instance ) const override;
		std::string GetStateName() const override;
	};
	class Damage : public MoverBase
	{
	private:
		bool gotoNext = false;
	public:
		void Init( BossFirst &instance ) override;
		void Uninit( BossFirst &instance ) override;
		void Update( BossFirst &instance, float elapsedTime, const Donya::Vector3 &targetPos ) override;
		bool AcceptDamage( const BossFirst &instance ) const override;
		bool AcceptDraw( const BossFirst &instance ) const override;
		bool ShouldChangeMover( BossFirst &instance ) const override;
		std::function<void()> GetChangeStateMethod( BossFirst &instance ) const override;
		std::string GetStateName() const override;
	};
	class Die : public MoverBase
	{
	public:
		void Init( BossFirst &instance ) override;
		void Uninit( BossFirst &instance ) override;
		void Update( BossFirst &instance, float elapsedTime, const Donya::Vector3 &targetPos ) override;
		bool IsDead( const BossFirst &instance ) const override;
		bool ShouldChangeMover( BossFirst &instance ) const override;
		std::function<void()> GetChangeStateMethod( BossFirst &instance ) const override;
		std::string GetStateName() const override;
	};
// region Mover
#pragma endregion
private:
	int							timer				= 0;
	int							oilTimer			= 0;
	int							actionIndex			= 0;
	int							remainFeintCount	= 0; // 0 is invalid.
	Donya::Vector3				aimingPos;
	std::unique_ptr<MoverBase>	pMover = nullptr;
	mutable bool receiveDamage = false; // Will be changed at const method.
public:
	void Init( const BossInitializer &parameter ) override;
	void Uninit() override;
	void Update( float elapsedTime, const Donya::Vector3 &targetPos ) override;
	void PhysicUpdate( const std::vector<Donya::AABB> &solids = {}, const Donya::Model::PolygonGroup *pTerrain = nullptr, const Donya::Vector4x4 *pTerrainWorldMatrix = nullptr ) override;
	void Draw( RenderingHelper *pRenderer ) const override;
public:
	void MakeDamage( const Element &effect, const Donya::Vector3 &othersVelocity ) const override;
private:
	template<class Mover, typename CtorArgument>
	void AssignMover( const CtorArgument &arg )
	{
		if ( pMover ) { pMover->Uninit( *this ); }

		pMover = std::make_unique<Mover>( arg );
		pMover->Init( *this );
	}
	template<class Mover>
	void AssignMover()
	{
		if ( pMover ) { pMover->Uninit( *this ); }

		pMover = std::make_unique<Mover>();
		pMover->Init( *this );
	}
	void AssignMoverByAction( ActionType type );
	void AssignMoverByAction( int actionIndex );
	void AdvanceAction();

	void UpdateByMover( float elapsedTime, const Donya::Vector3 &targetPos );

	Donya::Vector3 CalcAimingVector( const Donya::Vector3 &targetPos, float maxRotatableDegree = 360.0f ) const;

	std::vector<ActionType> FetchActionPatterns() const;
	ActionType FetchAction( int actionIndex ) const;
private:
	bool IsDead() const override;
	BossType GetType() const override;
	std::vector<Element::Type>	GetUncollidableTypes() const override;
public:
#if USE_IMGUI
	void ShowImGuiNode( const std::string &nodeCaption ) override;
#endif // USE_IMGUI
};
