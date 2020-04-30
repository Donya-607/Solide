#pragma once

#include <vector>

#include "Donya/Collision.h"
#include "Donya/ModelPolygon.h"
#include "Donya/Vector.h"

#include "Renderer.h"

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
	void DrawHitBox( RenderingHelper *pRenderer, const Donya::Vector4x4 &matVP, const Donya::Vector4 &color = { 1.0f, 1.0f, 1.0f, 1.0f } ) const;
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
	// HACK: This ray-offset should separate between origin's offset and whole ray's offset.
	/// <summary>
	/// Returns face's normal of last collided by vertical move.
	/// </summary>
	virtual Donya::Vector3 Move( const Donya::Vector3 &wsMovement, const std::vector<Donya::Vector3> &wsRayOffsets, const std::vector<Donya::AABB> &solids = {}, const Donya::Model::PolygonGroup *pTerrain = nullptr, const Donya::Vector4x4 *pTerrainWorldMatrix = nullptr );
public:
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
	RecursionResult	CalcCorrectedVector( const Donya::Vector3 &wsRayStartPos, int recursionLimit, const Donya::Vector3 &targetVector, const Donya::Model::PolygonGroup *pTerrain, const Donya::Vector4x4 *pTerrainWorldMatrix ) const;
private:
	AABBResult		CalcCorrectedVectorImpl( const Donya::Vector3 &targetVector, const std::vector<Donya::AABB> &solids ) const;
	RecursionResult	CalcCorrectedVectorImpl( const Donya::Vector3 &wsRayStartPos, int recursionLimit, int recursionCount, RecursionResult prevResult, const Donya::Model::PolygonGroup &terrain, const Donya::Vector4x4 &terrainWorldMatrix ) const;
private:
	void MoveXZImpl( const Donya::Vector3 &xzMovement, const std::vector<Donya::Vector3> &wsRayOffsets, int recursionCount, const std::vector<Donya::AABB> &solids, const Donya::Model::PolygonGroup *pTerrain, const Donya::Vector4x4 *pTerrainWorldMatrix = nullptr );
	Donya::Vector3 MoveYImpl ( const Donya::Vector3 &yMovement, const std::vector<Donya::AABB> &solids, const Donya::Model::PolygonGroup *pTerrain, const Donya::Vector4x4 *pTerrainWorldMatrix = nullptr );

	/// <summary>
	/// Returns corrected velocity.
	/// </summary>
	Donya::Vector3 CalcCorrectedVectorByMyHitBox( const Donya::Vector3 &velocity, const std::vector<Donya::Vector3> &wsRayOriginOffsets, const Donya::Model::PolygonGroup *pTerrain, const Donya::Vector4x4 *pTerrainWorldMatrix );
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
	virtual void DrawHitBox( RenderingHelper *pRenderer, const Donya::Vector4x4 &matVP, const Donya::Quaternion &rotation = Donya::Quaternion::Identity(), const Donya::Vector4 &color = { 1.0f, 1.0f, 1.0f, 1.0f } ) const;
};
