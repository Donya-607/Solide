#pragma once

#include <vector>

#include "Donya/Vector.h"
#include "Donya/Collision.h"

class Actor;

/// <summary>
/// This class represents a collision. Solids don't interact with other Solids.
/// </summary>
class Solid
{
protected:
	Donya::Vector3	pos;
	Donya::AABB		hitBox;	// The "pos" acts as an offset.
public:
	/// <summary>
	/// The "relativeActors" will be pushed(or carried) if colliding.
	/// </summary>
	void Move( const Donya::Vector3 &wsMovement, const std::vector<Actor *> &relativeActors );
public:
	Donya::Vector3 GetPosition() const;
	/// <summary>
	/// Returns hit-box of world space.
	/// </summary>
	Donya::AABB GetHitBox() const;
public:
	void DrawHitBox( const Donya::Vector4x4 &matVP, const Donya::Vector4 &color = { 1.0f, 1.0f, 1.0f, 1.0f } ) const;
};

/// <summary>
/// This class is a Solids that interactable with other Solids.
/// </summary>
class Actor
{
	friend Solid; // To be able to call my "Move()" in Move() methods of the Solid when being pushed or be carried.
protected:
	Donya::Vector3	pos;
	Donya::AABB		hitBox;	// The "pos" acts as an offset.
public:
	virtual void Move( const Donya::Vector3 &wsMovement, const std::vector<Solid> &collisions );
	virtual bool IsRiding( const Solid &onto ) const;
	/// <summary>
	/// Will call when pushed by Solid.
	/// </summary>
	virtual void Squish() {}
public:
	virtual Donya::Vector3 GetPosition() const;
	virtual Donya::AABB GetHitBox() const;
public:
	virtual void DrawHitBox( const Donya::Vector4x4 &matVP, const Donya::Vector4 &color = { 1.0f, 1.0f, 1.0f, 1.0f } ) const;
};
