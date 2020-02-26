#pragma once

#include <vector>

#include "Donya/Vector.h"
#include "Donya/Collision.h"

// For ray-pick.
namespace Donya { class StaticMesh; }

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
	Donya::Vector4x4 GetWorldMatrix() const;
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
	// virtual void Move( const Donya::Vector3 &wsMovement, const std::vector<Solid> &collisions = std::vector<Solid>{}, const Donya::StaticMesh *pTerrain = nullptr );
	virtual void Move( const Donya::Vector3 &wsMovement, const std::vector<Donya::Vector3> &wsRayOriginOffsets, const Donya::StaticMesh *pTerrain = nullptr, const Donya::Vector4x4 *pTerrainWorldMatrix = nullptr );
private:
	void MoveXZImpl( const Donya::Vector3 &xzMovement, const std::vector<Donya::Vector3> &wsRayOriginOffsets, int recursionCount, const Donya::StaticMesh *pTerrain, const Donya::Vector4x4 *pTerrainWorldMatrix = nullptr );
	void MoveYImpl ( const Donya::Vector3 &yMovement,  const Donya::StaticMesh *pTerrain, const Donya::Vector4x4 *pTerrainWorldMatrix = nullptr );
public:
	virtual bool IsRiding( const Solid &onto ) const;
	/// <summary>
	/// Will call when pushed by Solid.
	/// </summary>
	virtual void Squish() {}
public:
	virtual Donya::Vector3 GetPosition() const;
	virtual Donya::AABB GetHitBox() const;
	virtual Donya::Vector4x4 GetWorldMatrix() const;
public:
	virtual void DrawHitBox( const Donya::Vector4x4 &matVP, const Donya::Quaternion &rotation = Donya::Quaternion::Identity(), const Donya::Vector4 &color = { 1.0f, 1.0f, 1.0f, 1.0f } ) const;
};
