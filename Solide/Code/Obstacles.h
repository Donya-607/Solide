#pragma once

#include <string>

#undef max
#undef min
#include <cereal/types/polymorphic.hpp>

#include "Donya/Collision.h"
#include "Donya/UseImGui.h"
#include "Donya/Serializer.h"

#include "ObjectBase.h"

class ObstacleBase : protected Solid
{
public:
	static bool LoadModels();
	static void ParameterInit();
	static void ParameterUninit();
public:
	static int  GetModelKindCount();
	static std::string GetModelName( int modelKind );
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
	virtual void Draw( const Donya::Vector4 &eyePos, float transNear, float transFar, float transLowerAlpha, const Donya::Vector4x4 &VP, const Donya::Vector4 &lightDir, const Donya::Vector4 &color ) = 0;
public:
	virtual int GetKind() const = 0;
	virtual Donya::Vector3 GetPosition() const { return pos; }
	virtual Donya::AABB GetHitBox() const
	{
		Donya::AABB tmp = hitBox;
		tmp.pos += GetPosition();
		return tmp;
	}
public:
#if USE_IMGUI
	/// <summary>
	/// Returns true if I wanna be removed me.
	/// </summary>
	virtual bool ShowImGuiNode( const std::string &nodeCaption );
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
	void Draw( const Donya::Vector4 &eyePos, float transNear, float transFar, float transLowerAlpha, const Donya::Vector4x4 &VP, const Donya::Vector4 &lightDir, const Donya::Vector4 &color ) override;
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
	void Draw( const Donya::Vector4 &eyePos, float transNear, float transFar, float transLowerAlpha, const Donya::Vector4x4 &VP, const Donya::Vector4 &lightDir, const Donya::Vector4 &color ) override;
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
	void Draw( const Donya::Vector4 &eyePos, float transNear, float transFar, float transLowerAlpha, const Donya::Vector4x4 &VP, const Donya::Vector4 &lightDir, const Donya::Vector4 &color ) override;
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
	void Draw( const Donya::Vector4 &eyePos, float transNear, float transFar, float transLowerAlpha, const Donya::Vector4x4 &VP, const Donya::Vector4 &lightDir, const Donya::Vector4 &color ) override;
public:
	int GetKind() const override;
};
CEREAL_CLASS_VERSION( Table, 0 )
CEREAL_REGISTER_TYPE( Table )
CEREAL_REGISTER_POLYMORPHIC_RELATION( ObstacleBase, Table )

class Goal : public ObstacleBase
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
	void Draw( const Donya::Vector4 &eyePos, float transNear, float transFar, float transLowerAlpha, const Donya::Vector4x4 &VP, const Donya::Vector4 &lightDir, const Donya::Vector4 &color ) override;
public:
	int GetKind() const override;
};
CEREAL_CLASS_VERSION( Goal, 0 )
CEREAL_REGISTER_TYPE( Goal )
CEREAL_REGISTER_POLYMORPHIC_RELATION( ObstacleBase, Goal )
