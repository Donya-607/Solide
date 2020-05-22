#pragma once

#pragma once

#include <memory>
#include <string>
#include <vector>

#include "Donya/Model.h"
#include "Donya/ModelMotion.h"
#include "Donya/ModelPolygon.h"
#include "Donya/ModelPose.h"
#include "Donya/Quaternion.h"
#include "Donya/Serializer.h"
#include "Donya/UseImGui.h"
#include "Donya/Vector.h"

#include "ObjectBase.h"
#include "Element.h"
#include "Renderer.h"


class BossInitializer
{
private:
	Donya::Vector3		wsInitialPos;
	Donya::Quaternion	initialOrientation;
private:
	friend class cereal::access;
	template<class Archive>
	void serialize( Archive &archive, std::uint32_t version )
	{
		archive
		(
			CEREAL_NVP( wsInitialPos ),
			CEREAL_NVP( initialOrientation )
		);

		if ( 1 <= version )
		{
			// archive( CEREAL_NVP( x ) );
		}
	}
	static constexpr const char *ID = "BossInit";
public:
	Donya::Vector3		GetInitialPos() const;
	Donya::Quaternion	GetInitialOrientation() const;
public:
	void LoadParameter( int stageNo );
private:
	void LoadBin ( int stageNo );
	void LoadJson( int stageNo );
#if USE_IMGUI
	void SaveBin ( int stageNo );
	void SaveJson( int stageNo );
public:
	void ShowImGuiNode( const std::string &nodeCaption, int stageNo, bool allowShowIONode = true );
#endif // USE_IMGUI
};
CEREAL_CLASS_VERSION( BossInitializer, 0 )

class EffectHandle;
class BossBase : public Actor
{
public:
	static bool LoadModels();
public:
	struct ModelResource
	{
		Donya::Model::SkinningModel	model;
		Donya::Model::MotionHolder	motionHolder;
	};
protected:
	Donya::Vector3		velocity;
	Donya::Quaternion	orientation;
	mutable Element		element;		// Will change in const method.
	struct
	{
		std::shared_ptr<ModelResource>	pResource;
		Donya::Model::Pose				pose;
		Donya::Model::Animator			animator;
	} model;
public:
	virtual void Init( const BossInitializer &parameter );
	virtual void Uninit() {}

	virtual void Update( float elapsedTime, const Donya::Vector3 &targetPos );
	virtual void PhysicUpdate( const std::vector<Donya::AABB> &solids = {}, const Donya::Model::PolygonGroup *pTerrain = nullptr, const Donya::Vector4x4 *pTerrainWorldMatrix = nullptr );

	virtual void Draw( RenderingHelper *pRenderer ) = 0;
	virtual void DrawHitBox( RenderingHelper *pRenderer, const Donya::Vector4x4 &matVP );
public:
	virtual void MakeDamage( const Element &effect ) const;
public:
	virtual bool				IsDead() const = 0;
	virtual Donya::Quaternion	GetOrientation() const
	{
		return orientation;
	}
private:
#if USE_IMGUI
	virtual void ShowImGuiNode( const std::string &nodeCaption ) = 0;
#endif // USE_IMGUI
};