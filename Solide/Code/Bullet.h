#pragma once

#include <string>
#include <vector>

#include "Donya/Collision.h"
#include "Donya/ModelPolygon.h"
#include "Donya/Quaternion.h"
#include "Donya/Serializer.h"
#include "Donya/Template.h"
#include "Donya/UseImGui.h"
#include "Donya/Vector.h"

#include "Renderer.h"

namespace Bullet
{
	enum class Kind
	{
		Oil,

		KindCount
	};

	bool LoadBulletsResource();
	std::string GetBulletName( Kind bulletKind );
	#if USE_IMGUI
	void UseBulletsImGui();
	#endif // USE_IMGUI

	class BulletBase;
	class BulletAdmin : public Donya::Singleton<BulletAdmin>
	{
	private:
		std::vector<std::shared_ptr<BulletBase>> bulletPtrs;
	public:
		struct FireDesc
		{
			Kind			kind	= Kind::KindCount;
			float			speed	= 0.0f;
			Donya::Vector3	direction;
			Donya::Vector3	generatePos;
		private:
			friend class cereal::access;
			template<class Archive>
			void serialize( Archive &archive, std::uint32_t version )
			{
				archive
				(
					CEREAL_NVP( kind		),
					CEREAL_NVP( speed		),
					CEREAL_NVP( direction	),
					CEREAL_NVP( generatePos	)
				);

				if ( 1 <= version )
				{
					// archive( CEREAL_NVP( x ) );
				}
			}
		#if USE_IMGUI
		public:
			void ShowImGuiNode( const std::string &nodeCaption, bool generatePosIsRelative = true );
		#endif // USE_IMGUI
		};
	public:
		/// <summary>
		/// Clear all bullets.
		/// </summary>
		void Init();
		void Uninit();

		void Update( float elapsedTime );
		void PhysicUpdate( const std::vector<Donya::AABB> &solids = {}, const Donya::Model::PolygonGroup *pTerrain = nullptr, const Donya::Vector4x4 *pTerrainWorldMatrix = nullptr );

		void Draw( RenderingHelper *pRenderer, const Donya::Vector4 &color );
		void DrawHitBoxes( RenderingHelper *pRenderer, const Donya::Vector4x4 &VP, const Donya::Vector4 &color );
	public:
		void Append( const FireDesc &fireParameter );
	};

	class BulletBase
	{
	protected:
		Kind				kind = Kind::KindCount;
		Donya::Vector3		pos;
		Donya::Vector3		velocity;
		Donya::Quaternion	orientation;
	public:
		BulletBase()									= default;
		BulletBase( const BulletBase & )				= default;
		BulletBase &operator = ( const BulletBase & )	= default;
		BulletBase( BulletBase && )						= default;
		BulletBase &operator = ( BulletBase && )		= default;
		virtual ~BulletBase()							= default;
	public:
		virtual void Init( const BulletAdmin::FireDesc &initializeParameter );
		virtual void Uninit() {}

		virtual void Update( float elapsedTime );
		virtual void PhysicUpdate( const std::vector<Donya::AABB> &solids = {}, const Donya::Model::PolygonGroup *pTerrain = nullptr, const Donya::Vector4x4 *pTerrainWorldMatrix = nullptr );

		virtual void Draw( RenderingHelper *pRenderer, const Donya::Vector4 &color );
		virtual void DrawHitBox( RenderingHelper *pRenderer, const Donya::Vector4x4 &VP, const Donya::Vector4 &color );
	protected:
		virtual void AttachSelfKind() = 0;

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
		virtual bool				ShouldRemove()		const = 0;
		virtual Kind				GetKind()			const { return kind; }
		virtual Donya::Vector3		GetPosition()		const { return pos; }
		virtual Donya::AABB			GetHitBox()			const { return Donya::AABB::Nil(); }
		virtual Donya::Vector4x4	GetWorldMatrix()	const;
	public:
	#if USE_IMGUI
		/// <summary>
		/// Returns true if I wanna be removed me.
		/// </summary>
		virtual bool ShowImGuiNode( const std::string &nodeCaption );
	#endif // USE_IMGUI
	};

	namespace Impl
	{
		class OilBullet : public BulletBase
		{
		private:
			int		aliveTime  = 0;
			bool	shouldStay = false;
		public:
			void Update( float elapsedTime ) override;
			void PhysicUpdate( const std::vector<Donya::AABB> &solids = {}, const Donya::Model::PolygonGroup *pTerrain = nullptr, const Donya::Vector4x4 *pTerrainWorldMatrix = nullptr ) override;
			void Draw( RenderingHelper *pRenderer, const Donya::Vector4 &color ) override;
			void DrawHitBox( RenderingHelper *pRenderer, const Donya::Vector4x4 &VP, const Donya::Vector4 &color ) override;
		private:
			void AttachSelfKind() override;
		public:
			bool				ShouldRemove()		const override;
			Donya::AABB			GetHitBox()			const override;
			Donya::Vector4x4	GetWorldMatrix()	const override;
		};
	}
}
CEREAL_CLASS_VERSION( Bullet::BulletAdmin::FireDesc, 0 )
