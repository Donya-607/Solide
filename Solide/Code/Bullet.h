#pragma once

#include <string>
#include <vector>

#include "Donya/Collision.h"
#include "Donya/Quaternion.h"
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

	static bool LoadBulletsResource();
	static std::string GetBulletName( Kind bulletKind );
	#if USE_IMGUI
	static void UseBulletsImGui();
	#endif // USE_IMGUI

	class BulletBase;
	class BulletAdmin : public Donya::Singleton<BulletAdmin>
	{
		//friend Donya::Singleton<BulletAdmin>;
	private:
		std::vector<std::shared_ptr<BulletBase>> bulletPtrs;
	public:
		struct FireDesc
		{
			Kind			kind	= Kind::KindCount;
			float			speed	= 0.0f;
			Donya::Vector3	direction;
			Donya::Vector3	generatePos;
		};
	public:
		/// <summary>
		/// Clear all bullets.
		/// </summary>
		void Init();
		void Uninit();

		void Update( float elapsedTime );
		void PhysicUpdate();

		void Draw( RenderingHelper *pRenderer, const Donya::Vector4 &color );
		void DrawHitBoxes( RenderingHelper *pRenderer, const Donya::Vector4x4 &VP, const Donya::Vector4 &color );
	public:
		void Append( const FireDesc &fireParameter );
	};

	class BulletBase
	{
	protected:
		Kind				kind;
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

		virtual void Update( float elapsedTime ) {}
		virtual void PhysicUpdate();

		virtual void Draw( RenderingHelper *pRenderer, const Donya::Vector4 &color );
		virtual void DrawHitBox( RenderingHelper *pRenderer, const Donya::Vector4x4 &VP, const Donya::Vector4 &color );
	protected:
		virtual void AttachSelfKind() = 0;
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
		public:
			void Update( float elapsedTime ) override;
			void PhysicUpdate() override;
		private:
			void AttachSelfKind() override;
		public:
			Donya::AABB GetHitBox() const override;
		};
	}
}
