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
	// HACK: The reason for setting the return value here is that it was necessary for the player to determine if he landed.
	// So this is not necessary for design.
	/// <summary>
	/// Returns face's normal of last collided by vertical move.
	/// </summary>
	virtual Donya::Vector3 Move( const Donya::Vector3 &wsMovement, const std::vector<Donya::Vector3> &wsRayOriginOffsets, const std::vector<Donya::AABB> &solids = {}, const Donya::StaticMesh * pTerrain = nullptr, const Donya::Vector4x4 * pTerrainWorldMatrix = nullptr );
private:
	void MoveXZImpl( const Donya::Vector3 &xzMovement, const std::vector<Donya::Vector3> &wsRayOriginOffsets, int recursionCount, const std::vector<Donya::AABB> &solids, const Donya::StaticMesh *pTerrain, const Donya::Vector4x4 *pTerrainWorldMatrix = nullptr );
	Donya::Vector3 MoveYImpl ( const Donya::Vector3 &yMovement, const std::vector<Donya::AABB> &solids, const Donya::StaticMesh *pTerrain, const Donya::Vector4x4 *pTerrainWorldMatrix = nullptr );

	Donya::AABB CalcCollidingBox( const Donya::AABB &myself, const std::vector<Donya::AABB> &solids ) const;
	void MoveInAABB( Donya::Vector3 velocity, const std::vector<Donya::AABB> &solids );

	struct CalcedRayResult
	{
		Donya::Vector3 correctedVelocity;
		Donya::Vector3 wsLastIntersection;
		Donya::Vector3 wsLastWallNormal;
		Donya::Vector3 wsLastWallFace[3];
		bool wasHit = false;
	};
	CalcedRayResult CalcCorrectVelocity( const Donya::Vector3 &velocity, const std::vector<Donya::Vector3> &wsRayOriginOffsets, const Donya::StaticMesh *pTerrain, const Donya::Vector4x4 *pTerrainWorldMatrix, CalcedRayResult recursionResult, int recursionCount, int recursionLimit ) const;
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
