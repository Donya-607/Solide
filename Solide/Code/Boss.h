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


enum class BossType
{
	Null	= -1, // Not exist
	First	= 0,

	BossCount
};


class BossInitializer
{
private:
	BossType			type = BossType::Null;
	Donya::Vector3		wsInitialPos;
	Donya::Quaternion	initialOrientation;
private:
	friend class cereal::access;
	template<class Archive>
	void serialize( Archive &archive, std::uint32_t version )
	{
		archive
		(
			CEREAL_NVP( type				),
			CEREAL_NVP( wsInitialPos		),
			CEREAL_NVP( initialOrientation	)
		);

		if ( 1 <= version )
		{
			// archive( CEREAL_NVP( x ) );
		}
	}
	static constexpr const char *ID = "BossInit";
public:
	bool				ShouldGenerateBoss() const;
	BossType			GetType() const;
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

class BossBase : public Actor
{
public:
	static bool LoadModels();
	static bool AssignDerivedClass( std::unique_ptr<BossBase> *pTarget, BossType assignType );
public:
	struct ModelResource
	{
		Donya::Model::SkinningModel	model;
		Donya::Model::MotionHolder	motionHolder;
	};
protected:
	int					hp = 3;
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

	virtual void Draw( RenderingHelper *pRenderer ) const;
	virtual void DrawHitBox( RenderingHelper *pRenderer, const Donya::Vector4x4 &matVP ) const;
public:
	virtual void MakeDamage( const Element &effect ) const;
public:
	virtual bool				IsDead() const;
	virtual BossType			GetType() const = 0;
	virtual Donya::Quaternion	GetOrientation() const
	{
		return orientation;
	}
protected:
	virtual void AssignSpecifyPose( int motionIndex );
	virtual void UpdateMotion( float elapsedTime, int motionIndex );
	virtual Donya::Vector4		CalcDrawColor() const;
	virtual	Donya::Vector4x4	CalcWorldMatrix( bool useForHitBox, bool useForHurtBox, bool useForDrawing ) const;
public:
#if USE_IMGUI
	virtual void ShowImGuiNode( const std::string &nodeCaption ) = 0;
#endif // USE_IMGUI
};


class BossFirst : public BossBase
{
private:
	BossType GetType() const override;
public:
#if USE_IMGUI
	void ShowImGuiNode( const std::string &nodeCaption ) override;
#endif // USE_IMGUI
};
