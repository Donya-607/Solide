#pragma once

#include <string>

#include "Donya/Collision.h"
#include "Donya/Quaternion.h"
#include "Donya/UseImGui.h"
#include "Donya/Vector.h"

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
		virtual void Init( const Donya::Vector3 &wsInitialPos, float initialSpeed, const Donya::Vector3 &direction );
		virtual void Uninit() {}

		virtual void Update( float elapsedTime ) {}
		virtual void PhysicUpdate();

		virtual void Draw( const Donya::Vector4 &eyePos, float transNear, float transFar, float transLowerAlpha, const Donya::Vector4x4 &VP, const Donya::Vector4 &lightDir, const Donya::Vector4 &color ) = 0;
		virtual void DrawHitBox( const Donya::Vector4x4 &VP, const Donya::Vector4 &color ) {}
	protected:
		virtual void AttachSelfKind() = 0;
	public:
		virtual bool			ShouldRemove()	const = 0;
		virtual Kind			GetKind()		const { return kind; }
		virtual Donya::Vector3	GetPosition()	const { return pos; }
		virtual Donya::AABB		GetHitBox()		const { return Donya::AABB::Nil(); }
	public:
	#if USE_IMGUI
		/// <summary>
		/// Returns true if I wanna be removed me.
		/// </summary>
		virtual bool ShowImGuiNode( const std::string &nodeCaption );
	#endif // USE_IMGUI
	};


	class OilBullet : public BulletBase
	{
	public:
		void Init( const Donya::Vector3 &wsInitialPos, float initialSpeed, const Donya::Vector3 &direction ) override;

		void Update( float elapsedTime );
		void PhysicUpdate();

		void Draw( const Donya::Vector4 &eyePos, float transNear, float transFar, float transLowerAlpha, const Donya::Vector4x4 &VP, const Donya::Vector4 &lightDir, const Donya::Vector4 &color ) override;
	public:
		Donya::AABB GetHitBox() const override;
	};
}
