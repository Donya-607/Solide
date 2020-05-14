#pragma once

#include <string>

#undef max
#undef min
#include <cereal/types/polymorphic.hpp>

#include "Donya/Collision.h"
#include "Donya/UseImGui.h"
#include "Donya/Serializer.h"

#include "Bullet.h"		// For FireDesc.
#include "ObjectBase.h"

class RenderingHelper;

class ObstacleBase : protected Solid
{
#if USE_IMGUI
protected:
	bool wantRemoveByGui = false;
#endif // USE_IMGUI
public:
	static bool LoadModels();
	static void ParameterInit();
	static void ParameterUninit();
public:
	static int  GetModelKindCount();
	static std::string GetModelName( int modelKind );
	static bool IsWaterKind( int obstacleKind );
	/// <summary>
	/// If set nullptr by this to "pOutputPtr", that means the "modelKind" is invalid.
	/// </summary>
	static void AssignDerivedModel( int modelKind, std::shared_ptr<ObstacleBase> *pOutputPtr );
public:
#if USE_IMGUI
	static void UseImGui();
#endif // USE_IMGUI
private:
	friend class cereal::access;
	template<class Archive>
	void serialize( Archive &archive, std::uint32_t version )
	{
		archive
		(
			CEREAL_NVP( pos ),
			CEREAL_NVP( hitBox )
		);

		if ( 1 <= version )
		{
			// archive( CEREAL_NVP( x ) );
		}
	}
public:
	virtual void Init( const Donya::Vector3 &wsInitialPos )
	{
		pos = wsInitialPos;
	}
	virtual void Uninit() {}
	virtual void Update( float elapsedTime ) = 0;
	virtual void Draw( RenderingHelper *pRenderer, const Donya::Vector4 &color ) = 0;
	virtual void DrawHitBox( RenderingHelper *pRenderer, const Donya::Vector4x4 &matVP, const Donya::Vector4 &color );
public:
	virtual bool ShouldRemove() const;
	virtual int  GetKind() const = 0;
	virtual Donya::Vector3 GetPosition() const { return pos; }
	virtual Donya::AABB GetHitBox() const
	{
		Donya::AABB tmp = hitBox;
		tmp.pos += GetPosition();
		return tmp;
	}
public:
#if USE_IMGUI
	virtual void ShowImGuiNode( const std::string &nodeCaption, bool useTreeNode = true );
#endif // USE_IMGUI
};
CEREAL_CLASS_VERSION( ObstacleBase, 0 )

class Stone : public ObstacleBase
{
private:
	friend class cereal::access;
	template<class Archive>
	void serialize( Archive &archive, std::uint32_t version )
	{
		archive
		(
			cereal::base_class<ObstacleBase>( this )
		);
		if ( 1 <= version )
		{
			// archive( CEREAL_NVP( x ) );
		}
	}
public:
	void Update( float elapsedTime ) override;
	void Draw( RenderingHelper *pRenderer, const Donya::Vector4 &color ) override;
	void DrawHitBox( RenderingHelper *pRenderer, const Donya::Vector4x4 &matVP, const Donya::Vector4 &color ) override;
public:
	int GetKind() const override;
};
CEREAL_CLASS_VERSION( Stone, 0 )
CEREAL_REGISTER_TYPE( Stone )
CEREAL_REGISTER_POLYMORPHIC_RELATION( ObstacleBase, Stone )

class Log : public ObstacleBase
{
private:
	friend class cereal::access;
	template<class Archive>
	void serialize( Archive &archive, std::uint32_t version )
	{
		archive
		(
			cereal::base_class<ObstacleBase>( this )
		);
		if ( 1 <= version )
		{
			// archive( CEREAL_NVP( x ) );
		}
	}
public:
	void Update( float elapsedTime ) override;
	void Draw( RenderingHelper *pRenderer,const Donya::Vector4 &color ) override;
	void DrawHitBox( RenderingHelper *pRenderer, const Donya::Vector4x4 &matVP, const Donya::Vector4 &color ) override;
public:
	int GetKind() const override;
};
CEREAL_CLASS_VERSION( Log, 0 )
CEREAL_REGISTER_TYPE( Log )
CEREAL_REGISTER_POLYMORPHIC_RELATION( ObstacleBase, Log )

class Tree : public ObstacleBase
{
private:
	friend class cereal::access;
	template<class Archive>
	void serialize( Archive &archive, std::uint32_t version )
	{
		archive
		(
			cereal::base_class<ObstacleBase>( this )
		);
		if ( 1 <= version )
		{
			// archive( CEREAL_NVP( x ) );
		}
	}
public:
	void Update( float elapsedTime ) override;
	void Draw( RenderingHelper *pRenderer, const Donya::Vector4 &color ) override;
	void DrawHitBox( RenderingHelper *pRenderer, const Donya::Vector4x4 &matVP, const Donya::Vector4 &color ) override;
public:
	int GetKind() const override;
};
CEREAL_CLASS_VERSION( Tree, 0 )
CEREAL_REGISTER_TYPE( Tree )
CEREAL_REGISTER_POLYMORPHIC_RELATION( ObstacleBase, Tree )

class Table : public ObstacleBase
{
private:
	friend class cereal::access;
	template<class Archive>
	void serialize( Archive &archive, std::uint32_t version )
	{
		archive
		(
			cereal::base_class<ObstacleBase>( this )
		);
		if ( 1 <= version )
		{
			// archive( CEREAL_NVP( x ) );
		}
	}
public:
	void Update( float elapsedTime ) override;
	void Draw( RenderingHelper *pRenderer, const Donya::Vector4 &color ) override;
	void DrawHitBox( RenderingHelper *pRenderer, const Donya::Vector4x4 &matVP, const Donya::Vector4 &color ) override;
public:
	int GetKind() const override;
};
CEREAL_CLASS_VERSION( Table, 0 )
CEREAL_REGISTER_TYPE( Table )
CEREAL_REGISTER_POLYMORPHIC_RELATION( ObstacleBase, Table )


class Spray : public ObstacleBase
{
private:
	int  shotTimer			= 0;
	int  startupTimer		= 0;
	bool nowSpraying		= false;
private: // Serialize targets. usually do not change.
	int  startupFrame		= 0;
	int  sprayingFrame		= 1;
	int  cooldownFrame		= 1;
	int  shotGenInterval	= 2;
	Bullet::BulletAdmin::FireDesc	shotDesc;
	Donya::Quaternion				orientation;
private:
	friend class cereal::access;
	template<class Archive>
	void serialize( Archive &archive, std::uint32_t version )
	{
		archive
		(
			cereal::base_class<ObstacleBase>( this ),
			CEREAL_NVP( startupFrame	),
			CEREAL_NVP( sprayingFrame	),
			CEREAL_NVP( cooldownFrame	),
			CEREAL_NVP( shotGenInterval	),
			CEREAL_NVP( shotDesc		),
			CEREAL_NVP( orientation		)
		);
		if ( 1 <= version )
		{
			// archive( CEREAL_NVP( x ) );
		}
	}
public:
	void Update( float elapsedTime ) override;
	void Draw( RenderingHelper *pRenderer, const Donya::Vector4 &color ) override;
	void DrawHitBox( RenderingHelper *pRenderer, const Donya::Vector4x4 &matVP, const Donya::Vector4 &color ) override;
public:
	int GetKind() const override;
private:
	void UpdateHitBox();
	void UpdateShot( float elapsedTime );
private:
	void UpdateSpray( float elapsedTime );
	void UpdateCooldown( float elapsedTime );
	void GenerateShot();
	bool ShouldChangeMode() const;
public:
#if USE_IMGUI
	void ShowImGuiNode( const std::string &nodeCaption, bool useTreeNode = true ) override;
#endif // USE_IMGUI
};
CEREAL_CLASS_VERSION( Spray, 0 )
CEREAL_REGISTER_TYPE( Spray )
CEREAL_REGISTER_POLYMORPHIC_RELATION( ObstacleBase, Spray )


class Water : public ObstacleBase
{
private:
	friend class cereal::access;
	template<class Archive>
	void serialize( Archive &archive, std::uint32_t version )
	{
		archive
		(
			cereal::base_class<ObstacleBase>( this )
		);
		if ( 1 <= version )
		{
			// archive( CEREAL_NVP( x ) );
		}
	}
public:
	void Update( float elapsedTime ) override;
	void Draw( RenderingHelper *pRenderer, const Donya::Vector4 &color ) override;
	void DrawHitBox( RenderingHelper *pRenderer, const Donya::Vector4x4 &matVP, const Donya::Vector4 &color ) override;
public:
	Donya::Vector4x4 GetWorldMatrix() const;
	int GetKind() const override;
};
CEREAL_CLASS_VERSION( Water, 0 )
CEREAL_REGISTER_TYPE( Water )
CEREAL_REGISTER_POLYMORPHIC_RELATION( ObstacleBase, Water )
