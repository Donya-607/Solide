#pragma once

#include <functional>
#include <memory>
#include <string>

#undef max
#undef min
#include <cereal/types/polymorphic.hpp>

#include "Donya/Model.h"
#include "Donya/ModelMotion.h"
#include "Donya/ModelPose.h"
#include "Donya/Quaternion.h"
#include "Donya/Serializer.h"
#include "Donya/UseImGui.h"
#include "Donya/Vector.h"

#include "Bullet.h"
#include "Element.h"
#include "Renderer.h"

namespace Enemy
{
	enum class Kind
	{
		Straight,
		Archer,
		GateKeeper,
		Chaser,

		KindCount
	};

	bool LoadResources();
#if USE_IMGUI
	std::string GetKindName( Kind kind );
	void UseImGui();
#endif // USE_IMGUI

	
	struct InitializeParam
	{
		float				searchRadius = 1.0f;
		Donya::Vector3		wsPos;
		Donya::Quaternion	orientation;
	private:
		friend class cereal::access;
		template<class Archive>
		void serialize( Archive &archive, std::uint32_t version )
		{
			archive
			(
				CEREAL_NVP( wsPos		),
				CEREAL_NVP( orientation	)
			);

			if ( 1 <= version )
			{
				archive( CEREAL_NVP( searchRadius ) );
			}
			if ( 2 <= version )
			{
				// archive( CEREAL_NVP( x ) );
			}
		}
	public:
	#if USE_IMGUI
		void ShowImGuiNode( const std::string &nodeCaption );
	#endif // USE_IMGUI
	};
	
	struct MoveParam
	{
		enum class LookDirection
		{
			MoveDirection,
			Target
		};
		LookDirection	lookDirection	= LookDirection::MoveDirection;
		float			speed			= 0.0f;	// The maximum speed per frame.
		float			rangeLimit		= 0.0f;	// 0.0f means limitless.
		Donya::Vector3	direction;				// Unit vector.
		bool			alignToTarget	= false;
	private:
		friend class cereal::access;
		template<class Archive>
		void serialize( Archive &archive, std::uint32_t version )
		{
			archive
			(
				CEREAL_NVP( lookDirection	),
				CEREAL_NVP( speed			),
				CEREAL_NVP( rangeLimit		),
				CEREAL_NVP( direction		),
				CEREAL_NVP( alignToTarget	)
			);

			if ( 1 <= version )
			{
				// archive( CEREAL_NVP( x ) );
			}
		}
	public:
		Donya::Vector3 CalcMoveDirection( const Donya::Vector3 moverPos, const Donya::Vector3 &targetPos ) const;

	#if USE_IMGUI
		void ShowImGuiNode( const std::string &nodeCaption );
	#endif // USE_IMGUI
	};

	struct ModelParam
	{
		Donya::Model::SkinningModel	model;
		Donya::Model::MotionHolder	motionHolder;
	};

	struct HurtDesc
	{
		Element hurtElement;
	};


	class Base
	{
	protected:
		InitializeParam				initializer; // Usually do not change this.
	protected:
		Donya::Vector3				pos;
		Donya::Quaternion			orientation;
		std::shared_ptr<ModelParam>	pModelParam;
		Donya::Model::Pose			pose;
		Donya::Model::Animator		animator;

		// Will changes by const method.
		mutable Element				element;
	public:
		Base() = default;
		virtual ~Base() = default;
	private:
		friend class cereal::access;
		template<class Archive>
		void serialize( Archive &archive, std::uint32_t version )
		{
			archive
			(
				CEREAL_NVP( initializer )
			);

			if ( 1 <= version )
			{
				// archive( CEREAL_NVP( x ) );
			}
		}
	public:
		virtual void Init( const InitializeParam &initializer );
		virtual void Uninit() {}

		virtual void Update( float elapsedTime, const Donya::Vector3 &targetPosition ) = 0;
		virtual void PhysicUpdate( const std::vector<Donya::AABB> &solids = {}, const Donya::Model::PolygonGroup *pTerrain = nullptr, const Donya::Vector4x4 *pTerrainWorldMatrix = nullptr ) = 0;

		virtual void Draw( RenderingHelper *pRenderer );
		virtual void DrawHitBox( RenderingHelper *pRenderer, const Donya::Vector4x4 &matVP );
	public:
		virtual bool			ShouldRemove()		const = 0;
		virtual Kind			GetKind()			const = 0;
		virtual Element			GetElement()		const { return element;		}
		const	InitializeParam	&GetInitializer()	const { return initializer;	}
		const	Donya::Vector3	&GetPosition()		const { return pos;			}
		virtual void MakeDamage( const Element &effect ) const;
		virtual void AcquireHitBoxes ( std::vector<Donya::AABB> *pAppendDest ) const;
		virtual void AcquireHurtBoxes( std::vector<Donya::AABB> *pAppendDest ) const;
	protected: // Hit/Hurt box acquisition method of open to outside is only whole hit/hurt boxes..
		virtual Donya::AABB AcquireHitBox( bool wantWorldSpace ) const;
		virtual Donya::AABB AcquireHurtBox( bool wantWorldSpace ) const;
	protected:
		virtual void UpdateMotion( float elapsedTime, int useMotionIndex );
		virtual Donya::Vector4		CalcDrawColor() const;
		virtual	Donya::Vector4x4	CalcWorldMatrix( bool useForHitBox, bool useForHurtBox, bool useForDrawing ) const;
	protected:
		struct AABBResult
		{
			Donya::Vector3 correctedVector;
			bool wasHit = false;
		};
		struct RecursionResult
		{
			Donya::Vector3				correctedVector;
			Donya::Model::RaycastResult	raycastResult;
		};
		AABBResult		CalcCorrectedVector( const Donya::Vector3 &targetVector, const std::vector<Donya::AABB> &solids ) const;
		RecursionResult	CalcCorrectedVector( int recursionLimit, const Donya::Vector3 &targetVector, const Donya::Model::PolygonGroup *pTerrain, const Donya::Vector4x4 *pTerrainWorldMatrix ) const;
	private:
		AABBResult		CalcCorrectedVectorImpl( const Donya::Vector3 &targetVector, const std::vector<Donya::AABB> &solids ) const;
		RecursionResult	CalcCorrectedVectorImpl( int recursionLimit, int recursionCount, RecursionResult prevResult, const Donya::Model::PolygonGroup &terrain, const Donya::Vector4x4 &terrainWorldMatrix ) const;
	public:
	#if USE_IMGUI
		/// <summary>
		/// You can set nullptr to "outputWantRemove".
		/// </summary>
		virtual void ShowImGuiNode( const std::string &nodeCaption, bool *outputWantRemove ) = 0;
	#endif // USE_IMGUI
	};
#if USE_IMGUI
	void AssignDerivedInstance( Kind kind, std::shared_ptr<Base> *pBasePtr );
#endif // USE_IMGUI


	/// <summary>
	/// This class moves by following specified direction.
	/// </summary>
	class Straight : public Base
	{
	private:
		MoveParam moveParam; // Usually do not change this.
	private:
		Donya::Vector3 velocity;
		float	moveDistanceSum		= 0.0f;
		bool	nowMoveToPositive	= true;
	public:
		Straight() : Base() {}
	private:
		friend class cereal::access;
		template<class Archive>
		void serialize( Archive &archive, std::uint32_t version )
		{
			archive
			(
				cereal::base_class<Base>( this ),
				CEREAL_NVP( moveParam )
			);
			if ( 1 <= version )
			{
				// archive( CEREAL_NVP( x ) );
			}
		}
	public:
		void Init( const InitializeParam &initializer ) override;

		void Update( float elapsedTime, const Donya::Vector3 &targetPosition ) override;
		void PhysicUpdate( const std::vector<Donya::AABB> &solids = {}, const Donya::Model::PolygonGroup *pTerrain = nullptr, const Donya::Vector4x4 *pTerrainWorldMatrix = nullptr ) override;
	public:
		bool ShouldRemove()	const override;
		Kind GetKind()		const override;
	private:
		/// <summary>
		/// This may return not a unit vector.
		/// </summary>
		Donya::Vector3 CalcNowMoveDirection( const Donya::Vector3 &targetPosition ) const;
		void AssignVelocity( const Donya::Vector3 &targetPosition );
		void AssignOrientation( const Donya::Vector3 &targetPosition );
	public:
	#if USE_IMGUI
		/// <summary>
		/// You can set nullptr to "outputWantRemove".
		/// </summary>
		void ShowImGuiNode( const std::string &nodeCaption, bool *outputWantRemove ) override;
	#endif // USE_IMGUI
	};


	/// <summary>
	/// This class shots some bullet.
	/// </summary>
	class Archer : public Base
	{
	private:
		class MoverBase
		{
		public:
			virtual void Init( Archer &target );
			virtual void Update( Archer &target, float elapsedTime, const Donya::Vector3 &targetPos ) = 0;
			virtual int  AcquireMotionIndex() const = 0;
			virtual bool ShouldChangeState( Archer &target ) const = 0;
			virtual std::function<void()> GetChangeStateMethod( Archer &target ) const = 0;
		protected:
			void LookToTarget( Archer &target, const Donya::Vector3 &targetPos );
		public:
		#if USE_IMGUI
			virtual std::string GetStateName() const = 0;
		#endif // USE_IMGUI
		};
		class Wait : public MoverBase
		{
		private:
			bool wasSearched = false;
		public:
			void Init( Archer &target ) override;
			void Update( Archer &target, float elapsedTime, const Donya::Vector3 &targetPos ) override;
			int  AcquireMotionIndex() const override;
			bool ShouldChangeState( Archer &target ) const override;
			std::function<void()> GetChangeStateMethod( Archer &target ) const override;
		public:
		#if USE_IMGUI
			std::string GetStateName() const override;
		#endif // USE_IMGUI
		};
		class Aim : public MoverBase
		{
		public:
			void Init( Archer &target ) override;
			void Update( Archer &target, float elapsedTime, const Donya::Vector3 &targetPos ) override;
			int  AcquireMotionIndex() const override;
			bool ShouldChangeState( Archer &target ) const override;
			std::function<void()> GetChangeStateMethod( Archer &target ) const override;
		public:
		#if USE_IMGUI
			std::string GetStateName() const override;
		#endif // USE_IMGUI
		};
		class Fire : public MoverBase
		{
		public:
			void Update( Archer &target, float elapsedTime, const Donya::Vector3 &targetPos ) override;
			int  AcquireMotionIndex() const override;
			bool ShouldChangeState( Archer &target ) const override;
			std::function<void()> GetChangeStateMethod( Archer &target ) const override;
		public:
		#if USE_IMGUI
			std::string GetStateName() const override;
		#endif // USE_IMGUI
		};
	private: // Usually do not change these member.
		Bullet::BulletAdmin::FireDesc shotDesc{};
		int		waitFrame		= 1;
		int		aimingFrame		= 1;
		int		intervalFrame	= 1;
		bool	aimToTarget		= false;
	private:
		int		timer			= 0;
		std::unique_ptr<MoverBase> pMover = nullptr;
	public:
		Archer() : Base() {}
	private:
		friend class cereal::access;
		template<class Archive>
		void serialize( Archive &archive, std::uint32_t version )
		{
			archive
			(
				cereal::base_class<Base>( this ),
				CEREAL_NVP( shotDesc		),
				CEREAL_NVP( waitFrame		),
				CEREAL_NVP( aimingFrame		),
				CEREAL_NVP( intervalFrame	),
				CEREAL_NVP( aimToTarget		)
			);
			if ( 1 <= version )
			{
				// archive( CEREAL_NVP( x ) );
			}
		}
	public:
		void Init( const InitializeParam &initializer ) override;

		void Update( float elapsedTime, const Donya::Vector3 &targetPosition ) override;
		void PhysicUpdate( const std::vector<Donya::AABB> &solids = {}, const Donya::Model::PolygonGroup *pTerrain = nullptr, const Donya::Vector4x4 *pTerrainWorldMatrix = nullptr ) override;
	public:
		bool ShouldRemove()	const override;
		Kind GetKind()		const override;
	private:
		template<class Mover>
		void AssignMover()
		{
			pMover = std::make_unique<Mover>();
			pMover->Init( *this );
		}

		void GenerateShot();
	public:
	#if USE_IMGUI
		/// <summary>
		/// You can set nullptr to "outputWantRemove".
		/// </summary>
		void ShowImGuiNode( const std::string &nodeCaption, bool *outputWantRemove ) override;
	#endif // USE_IMGUI
	};


	/// <summary>
	/// This class blocks on the spot.
	/// </summary>
	class GateKeeper : public Base
	{
	private:
		class MoverBase
		{
		public:
			virtual void Init( GateKeeper &target );
			virtual void Update( GateKeeper &target, float elapsedTime, const Donya::Vector3 &targetPos ) = 0;
			virtual int  AcquireMotionIndex() const = 0;
			virtual bool ShouldChangeState( GateKeeper &target ) const = 0;
			virtual std::function<void()> GetChangeStateMethod( GateKeeper &target ) const = 0;
		public:
			virtual Donya::Quaternion GetRotation() const { return Donya::Quaternion::Identity(); }
		#if USE_IMGUI
			virtual std::string GetStateName() const = 0;
		#endif // USE_IMGUI
		};
		class Wait : public MoverBase
		{
		public:
			void Update( GateKeeper &target, float elapsedTime, const Donya::Vector3 &targetPos ) override;
			int  AcquireMotionIndex() const override;
			bool ShouldChangeState( GateKeeper &target ) const override;
			std::function<void()> GetChangeStateMethod( GateKeeper &target ) const override;
		public:
		#if USE_IMGUI
			std::string GetStateName() const override;
		#endif // USE_IMGUI
		};
		class Attack : public MoverBase
		{
		private:
			float yaw = 0.0f; // Degree.
		public:
			void Init( GateKeeper &target ) override;
			void Update( GateKeeper &target, float elapsedTime, const Donya::Vector3 &targetPos ) override;
			int  AcquireMotionIndex() const override;
			bool ShouldChangeState( GateKeeper &target ) const override;
			std::function<void()> GetChangeStateMethod( GateKeeper &target ) const override;
		public:
			virtual Donya::Quaternion GetRotation() const override;
		#if USE_IMGUI
			std::string GetStateName() const override;
		#endif // USE_IMGUI
		};
		class Rest : public MoverBase
		{
		public:
			void Update( GateKeeper &target, float elapsedTime, const Donya::Vector3 &targetPos ) override;
			int  AcquireMotionIndex() const override;
			bool ShouldChangeState( GateKeeper &target ) const override;
			std::function<void()> GetChangeStateMethod( GateKeeper &target ) const override;
		public:
		#if USE_IMGUI
			std::string GetStateName() const override;
		#endif // USE_IMGUI
		};
	private: // Usually do not change these member.
		int		waitFrame	= 1;
		int		attackFrame	= 1;
		float	rotateSpeed = 10.0f; // Rotate degree per frame.
	private:
		int		timer		= 0;
		std::unique_ptr<MoverBase> pMover = nullptr;
	public:
		GateKeeper() : Base() {}
	private:
		friend class cereal::access;
		template<class Archive>
		void serialize( Archive &archive, std::uint32_t version )
		{
			archive
			(
				cereal::base_class<Base>( this ),
				CEREAL_NVP( waitFrame	),
				CEREAL_NVP( attackFrame	),
				CEREAL_NVP( rotateSpeed	)
			);
			if ( 1 <= version )
			{
				// archive( CEREAL_NVP( x ) );
			}
		}
	public:
		void Init( const InitializeParam &initializer ) override;

		void Update( float elapsedTime, const Donya::Vector3 &targetPosition ) override;
		void PhysicUpdate( const std::vector<Donya::AABB> &solids = {}, const Donya::Model::PolygonGroup *pTerrain = nullptr, const Donya::Vector4x4 *pTerrainWorldMatrix = nullptr ) override;

		void Draw( RenderingHelper *pRenderer ) override;
	public:
		bool ShouldRemove()	const override;
		Kind GetKind()		const override;
	private:
		template<class Mover>
		void AssignMover()
		{
			pMover = std::make_unique<Mover>();
			pMover->Init( *this );
		}
	public:
	#if USE_IMGUI
		/// <summary>
		/// You can set nullptr to "outputWantRemove".
		/// </summary>
		void ShowImGuiNode( const std::string &nodeCaption, bool *outputWantRemove ) override;
	#endif // USE_IMGUI
	};


	/// <summary>
	/// This class blocks on the spot.
	/// </summary>
	class Chaser : public Base
	{
	private:
		class MoverBase
		{
		public:
			virtual void Init( Chaser &target );
			virtual void Update( Chaser &target, float elapsedTime, const Donya::Vector3 &targetPos ) = 0;
			virtual int  AcquireMotionIndex() const = 0;
			virtual bool ShouldChangeState( Chaser &target, const Donya::Vector3 &targetPos ) const = 0;
			virtual std::function<void()> GetChangeStateMethod( Chaser &target ) const = 0;
		protected:
			bool IsTargetClose( Chaser &target, const Donya::Vector3 &targetPos ) const;
			void LookToVelocity( Chaser &target );
		public:
		#if USE_IMGUI
			virtual std::string GetStateName() const = 0;
		#endif // USE_IMGUI
		};
		class Return : public MoverBase
		{
		public:
			void Update( Chaser &target, float elapsedTime, const Donya::Vector3 &targetPos ) override;
			int  AcquireMotionIndex() const override;
			bool ShouldChangeState( Chaser &target, const Donya::Vector3 &targetPos ) const override;
			std::function<void()> GetChangeStateMethod( Chaser &target ) const override;
		public:
		#if USE_IMGUI
			std::string GetStateName() const override;
		#endif // USE_IMGUI
		};
		class Chase : public MoverBase
		{
		public:
			void Update( Chaser &target, float elapsedTime, const Donya::Vector3 &targetPos ) override;
			int  AcquireMotionIndex() const override;
			bool ShouldChangeState( Chaser &target, const Donya::Vector3 &targetPos ) const override;
			std::function<void()> GetChangeStateMethod( Chaser &target ) const override;
		public:
		#if USE_IMGUI
			std::string GetStateName() const override;
		#endif // USE_IMGUI
		};
	private: // Usually do not change these member.
		float	chaseSpeed	= 1.0f;
		float	returnSpeed	= 1.0f;
	private:
		Donya::Vector3 destPos;
		Donya::Vector3 velocity;
		std::unique_ptr<MoverBase> pMover = nullptr;
	public:
		Chaser() : Base() {}
	private:
		friend class cereal::access;
		template<class Archive>
		void serialize( Archive &archive, std::uint32_t version )
		{
			archive
			(
				cereal::base_class<Base>( this ),
				CEREAL_NVP( chaseSpeed	),
				CEREAL_NVP( returnSpeed	)
			);
			if ( 1 <= version )
			{
				// archive( CEREAL_NVP( x ) );
			}
		}
	public:
		void Init( const InitializeParam &initializer ) override;

		void Update( float elapsedTime, const Donya::Vector3 &targetPosition ) override;
		void PhysicUpdate( const std::vector<Donya::AABB> &solids = {}, const Donya::Model::PolygonGroup *pTerrain = nullptr, const Donya::Vector4x4 *pTerrainWorldMatrix = nullptr ) override;
	public:
		bool ShouldRemove()	const override;
		Kind GetKind()		const override;
	private:
		template<class Mover>
		void AssignMover()
		{
			pMover = std::make_unique<Mover>();
			pMover->Init( *this );
		}
	public:
	#if USE_IMGUI
		/// <summary>
		/// You can set nullptr to "outputWantRemove".
		/// </summary>
		void ShowImGuiNode( const std::string &nodeCaption, bool *outputWantRemove ) override;
	#endif // USE_IMGUI
	};


}
CEREAL_CLASS_VERSION( Enemy::InitializeParam,	1 )
CEREAL_CLASS_VERSION( Enemy::MoveParam,			0 )

CEREAL_CLASS_VERSION( Enemy::Straight,			0 )
CEREAL_REGISTER_TYPE( Enemy::Straight )
CEREAL_REGISTER_POLYMORPHIC_RELATION( Enemy::Base, Enemy::Straight )

CEREAL_CLASS_VERSION( Enemy::Archer,			0 )
CEREAL_REGISTER_TYPE( Enemy::Archer )
CEREAL_REGISTER_POLYMORPHIC_RELATION( Enemy::Base, Enemy::Archer )

CEREAL_CLASS_VERSION( Enemy::GateKeeper,		0 )
CEREAL_REGISTER_TYPE( Enemy::GateKeeper )
CEREAL_REGISTER_POLYMORPHIC_RELATION( Enemy::Base, Enemy::GateKeeper )

CEREAL_CLASS_VERSION( Enemy::Chaser,			0 )
CEREAL_REGISTER_TYPE( Enemy::Chaser )
CEREAL_REGISTER_POLYMORPHIC_RELATION( Enemy::Base, Enemy::Chaser )
