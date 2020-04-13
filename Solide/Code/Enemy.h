#pragma once

#include <string>
#include <memory>

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

#include "Renderer.h"

namespace Enemy
{
	enum class Kind
	{
		Straight,

		KindCount
	};

	bool LoadResources();
#if USE_IMGUI
	std::string GetKindName( Kind kind );
#endif // USE_IMGUI

	
	struct InitializeParam
	{
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
		LookDirection	lookDirection;
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
		virtual void Init( const InitializeParam &initializer ) = 0;
		virtual void Uninit() {}

		virtual void Update( float elapsedTime, const Donya::Vector3 &targetPosition ) = 0;
		virtual void PhysicUpdate() = 0;

		virtual void Draw( RenderingHelper *pRenderer );
		virtual void DrawHitBox( RenderingHelper *pRenderer, const Donya::Vector4x4 &matVP );
	public:
		virtual bool ShouldRemove()	const = 0;
		virtual Kind GetKind()		const = 0;
		const	InitializeParam	&GetInitializer()	const { return initializer; }
		const	Donya::Vector3	&GetPosition()		const { return pos; }
	protected:
		virtual	Donya::Vector4x4 CalcWorldMatrix( bool useForHitBox ) const;
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
		void PhysicUpdate() override;
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
}
CEREAL_CLASS_VERSION( Enemy::InitializeParam,	0 )
CEREAL_CLASS_VERSION( Enemy::MoveParam,			0 )

CEREAL_CLASS_VERSION( Enemy::Straight,			0 )
CEREAL_REGISTER_TYPE( Enemy::Straight			  )
CEREAL_REGISTER_POLYMORPHIC_RELATION( Enemy::Base, Enemy::Straight )
