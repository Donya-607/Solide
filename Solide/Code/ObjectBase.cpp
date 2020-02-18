#include "ObjectBase.h"

#include <memory>

#include "Donya/GeometricPrimitive.h"

namespace
{
	Donya::Vector4x4 MakeWorldMatrix( const Donya::AABB &hitBox )
	{
		// The size of hitBox is half-size.
		// But that will using as scale, so we should multiply to double.
		
		Donya::Vector4x4 m{};
		m._11 = hitBox.size.x * 2.0f;
		m._22 = hitBox.size.y * 2.0f;
		m._33 = hitBox.size.z * 2.0f;
		m._41 = hitBox.pos.x;
		m._42 = hitBox.pos.y;
		m._43 = hitBox.pos.z;
		return m;
	}
	void DrawCube( const Donya::Vector4x4 &W, const Donya::Vector4x4 &VP, const Donya::Vector4 &color )
	{
		static auto cube = Donya::Geometric::CreateCube();
		constexpr Donya::Vector4 lightDir{ 0.0f, -1.0f, 0.0f, 0.0f };

		cube.Render
		(
			nullptr,
			/* useDefaultShading	= */ true,
			/* isEnableFill			= */ true,
			W * VP, W,
			lightDir, color
		);
	}
}

void Solid::Move( const Donya::Vector3 &movement, const std::vector<Actor *> &actors )
{
	// TODO : Implement collision resolving process.
	pos += movement;
}

Donya::AABB Solid::GetHitBox() const
{
	Donya::AABB tmp = hitBox;
	tmp.pos += pos;
	return tmp;
}

void Solid::DrawHitBox( const Donya::Vector4x4 &VP, const Donya::Vector4 &color ) const
{
	DrawCube( MakeWorldMatrix( hitBox ), VP, color );
}



void Actor::Move( const Donya::Vector3 &movement, const std::vector<Solid> &collisions )
{
	// TODO : Implement collision resolving process.
	pos += movement;
}

bool Actor::IsRiding( const Solid &onto ) const
{
	constexpr Donya::Vector3 checkOffset{ 0.0f, 0.01f, 0.0f };

	Donya::AABB myself = hitBox;
	myself.pos += checkOffset;

	return Donya::AABB::IsHitAABB( myself, onto.GetHitBox() );
}

Donya::AABB Actor::GetHitBox() const
{
	Donya::AABB tmp = hitBox;
	tmp.pos += pos;
	return tmp;
}

void Actor::DrawHitBox( const Donya::Vector4x4 &VP, const Donya::Vector4 &color ) const
{
	DrawCube( MakeWorldMatrix( hitBox ), VP, color );
}
