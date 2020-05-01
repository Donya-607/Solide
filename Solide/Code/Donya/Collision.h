#ifndef INCLUDED_DONYA_COLLISION_H_
#define INCLUDED_DONYA_COLLISION_H_

#include <cstdint> // use for std::uint32_t

#include <cereal/cereal.hpp>

#include "Vector.h"
#include "Quaternion.h"

namespace Donya
{
	class Circle;

	/// <summary>
	/// Hit-Box of rectangle.<para></para>
	/// Vector2 pos : Center-position. The calculate method regards the belong space is the same.<para></para>
	/// Vector2 size : Half-size(like radius). the left is pos.x - size.x. please set to only positive value.<para></para>
	/// bool exist : Is enable collision ?
	/// </summary>
	class Box
	{
	public:
		Donya::Vector2 pos;		// Position of center.
		Donya::Vector2 size;	// Half size.
		bool	exist;			// Is enable collision ?
	public:
		Box() : pos(), size(), exist( true ) {}
		Box
		(
			float centerX, float centerY,
			float halfWidth, float halfHeight,
			bool  isExist = true
		) :
			pos( centerX, centerY ),
			size( halfWidth, halfHeight ),
			exist( isExist )
		{}
	private:
		friend class cereal::access;
		template<class Archive>
		void serialize( Archive &archive, std::uint32_t version )
		{
			archive
			(
				CEREAL_NVP( pos ),
				CEREAL_NVP( size ),
				CEREAL_NVP( exist )
			);
			if ( 1 <= version )
			{
				// archive();
			}
		}
	public:
		void Set				( float centerX, float centerY, float halfWidth, float halfHeight, bool isExist = true );
	public:
		static bool IsHitPoint	( const Box &L, const float &RX, const float &RY, bool ignoreExistFlag = false );
		/// <summary>
		/// The "ScreenPos"s are add to position.
		/// </summary>
		static bool IsHitPoint	( const Box &L, const float &LBoxScreenPosX, const float &LBoxScreenPosY, const float &RX, const float &RY, bool ignoreExistFlag = false );
		static bool IsHitBox	( const Box &L, const Box &R, bool ignoreExistFlag = false );
		/// <summary>
		/// The "ScreenPos"s are add to position.
		/// </summary>
		static bool IsHitBox	( const Box &L, const float &LBoxScreenPosX, const float &LBoxScreenPosY, const Box &R, const float &RBoxScreenPosX, const float &RBoxScreenPosY, bool ignoreExistFlag = false );
		static bool IsHitCircle	( const Box &L, const Circle &R, bool ignoreExistFlag = false );
		/// <summary>
		/// The "ScreenPos"s are add to position.
		/// </summary>
		static bool IsHitCircle	( const Box &L, const float &LBoxScreenPosX, const float &LBoxScreenPosY, const Circle &R, const float &RCircleScreenPosX, const float &RCircleScreenPosY, bool ignoreExistFlag = false );
	public:
		static inline Box Nil()
		{
			return{ 0.0f, 0.0f, 0.0f, 0.0f, false };
		}
	};

	/// <summary>
	/// Hit-Box of circle.<para></para>
	/// Vector2 pos : Center-position. The calculate method regards the belong space is the same.<para></para>
	/// float radius : Specify radius.<para></para>
	/// bool exist :  Is enable collision ?
	/// </summary>
	class Circle
	{
	public:
		Donya::Vector2	pos;	// Position of center.
		float			radius;
		bool			exist;	// Is enable collision ?
	public:
		Circle() : pos(), radius(), exist( true ) {}
		Circle
		(
			float centerX, float centerY,
			float rad,
			bool  isExist = true
		) :
			pos( centerX, centerY ),
			radius( rad ),
			exist( isExist )
		{}
	private:
		friend class cereal::access;
		template<class Archive>
		void serialize( Archive &archive, std::uint32_t version )
		{
			archive
			(
				CEREAL_NVP( pos ),
				CEREAL_NVP( radius ),
				CEREAL_NVP( exist )
			);
			if ( 1 <= version )
			{
				// archive();
			}
		}
	public:
		void Set				( float centerX, float centerY, float rad, bool isExist = true );
	public:
		static bool IsHitPoint	( const Circle &L, const float &RX, const float &RY, bool ignoreExistFlag = false );
		/// <summary>
		/// The "ScreenPos"s are add to position.
		/// </summary>
		static bool IsHitPoint	( const Circle &L, const float &LCircleScreenPosX, const float &LCircleScreenPosY, const float &RX, const float &RY, bool ignoreExistFlag = false );
		static bool IsHitCircle	( const Circle &L, const Circle &R, bool ignoreExistFlag = false );
		/// <summary>
		/// The "ScreenPos"s are add to position.
		/// </summary>
		static bool IsHitCircle	( const Circle &L, const float &LCircleScreenPosX, const float &LCircleScreenPosY, const Circle &R, const float &RCircleScreenPosX, const float &RCircleScreenPosY, bool ignoreExistFlag = false );
		static bool IsHitBox	( const Circle &L, const Box &R, bool ignoreExistFlag = false );
		/// <summary>
		/// The "ScreenPos"s are add to position.
		/// </summary>
		static bool IsHitBox	( const Circle &L, const float &LCircleScreenPosX, const float &LCircleScreenPosY, const Box &R, const float &RBoxScreenPosX, const float &RBoxScreenPosY, bool ignoreExistFlag = false );
	public:
		static inline Circle Nil()
		{
			return Circle{ 0, 0, 0, false };
		}
	};

	class Sphere;

	/// <summary>
	/// Hit-Box of AABB.<para></para>
	/// Vector3 pos : Center-position. The calculate method regards the belong space is the same.<para></para>
	/// Vector3 size : Half-size(like radius). the left is pos.x - size.x. please set to only positive value.<para></para>
	/// bool exist : Is enable collision ?
	/// </summary>
	class AABB
	{
	public:
		Donya::Vector3 pos{};	// Center-position. The calculate method regards the belong space is the same.
		Donya::Vector3 size{};	// Half-size(like radius). the left is pos.x - size.x. please set to only positive value.
		bool exist{ true };		// Is enable collision ?
	private:
		friend class cereal::access;
		template<class Archive>
		void serialize( Archive &archive, std::uint32_t version )
		{
			archive
			(
				CEREAL_NVP( pos ),
				CEREAL_NVP( size ),
				CEREAL_NVP( exist )
			);
			if ( 1 <= version )
			{
				// archive();
			}
		}
	public:
		/// <summary>
		/// AABB vs Point, assumes that these belong in world-space when collision checking.<para></para>
		/// The "ignoreExistFlag" is specify disable of exist flag.
		/// </summary>
		static bool IsHitPoint( const AABB &worldSpaceBox, const Donya::Vector3 &worldSpacePoint, bool ignoreExistFlag = false );
		/// <summary>
		/// AABB vs AABB, assumes that these belong in world-space when collision checking.<para></para>
		/// The "ignoreExistFlag" is specify disable of exist flag.
		/// </summary>
		static bool IsHitAABB( const AABB &worldSpaceBoxL, const AABB &worldSpaceBoxR, bool ignoreExistFlag = false );
		/// <summary>
		/// AABB vs Sphere, assumes that these belong in world-space when collision checking.<para></para>
		/// The "ignoreExistFlag" is specify disable of exist flag.
		/// </summary>
		static bool IsHitSphere( const AABB &worldSpaceBox, const Sphere &worldSpaceSphere, bool ignoreExistFlag = false );
	public:
		static AABB Nil()
		{
			return AABB
			{
				Donya::Vector3{ 0.0f, 0.0f, 0.0f },
				Donya::Vector3{ 0.0f, 0.0f, 0.0f },
				false
			};
		}
	};

	/// <summary>
	/// Hit-Box of Sphere.<para></para>
	/// Vector3 pos : Center-position. The calculate method regards the belong space is the same..<para></para>
	/// float radius : Radius of sphere in world-space. please set to only positive value.<para></para>
	/// bool exist : Is enable collision ?
	/// </summary>
	class Sphere
	{
	public:
		Donya::Vector3	pos{};			// Center-position. The calculate method regards the belong space is the same.
		float			radius{};		// Radius of sphere in world-space. please set to only positive value.
		bool			exist{ true };	// Is enable collision ?
	private:
		friend class cereal::access;
		template<class Archive>
		void serialize( Archive &archive, std::uint32_t version )
		{
			archive
			(
				CEREAL_NVP( pos ),
				CEREAL_NVP( radius ),
				CEREAL_NVP( exist )
			);
			if ( 1 <= version )
			{
				// archive();
			}
		}
	public:
		/// <summary>
		/// Sphere vs Point, assumes that these belong in world-space when collision checking.<para></para>
		/// The "ignoreExistFlag" is specify disable of exist flag.
		/// </summary>
		static bool IsHitPoint( const Sphere &worldSpaceSphere, const Donya::Vector3 &worldSpacePoint, bool ignoreExistFlag = false );
		/// <summary>
		/// Sphere vs Sphere, assumes that these belong in world-space when collision checking.<para></para>
		/// The "ignoreExistFlag" is specify disable of exist flag.
		/// </summary>
		static bool IsHitSphere( const Sphere &worldSpaceSphereL, const Sphere &worldSpaceSphereR, bool ignoreExistFlag = false );
		/// <summary>
		/// Sphere vs AABB, assumes that these belong in world-space when collision checking.<para></para>
		/// The "ignoreExistFlag" is specify disable of exist flag.
		/// </summary>
		static bool IsHitAABB( const Sphere &worldSpaceSphere, const AABB &worldSpaceBox, bool ignoreExistFlag = false );
	public:
		static Sphere Nil()
		{
			return Sphere
			{
				Donya::Vector3{ 0.0f, 0.0f, 0.0f },
				0.0f,
				false
			};
		}
	};

	/// <summary>
	/// Hit-Box of AABB.<para></para>
	/// Vector3 pos : Center-position. The calculate method regards the belong space is the same.<para></para>
	/// Vector3 size : Half-size(like radius). the left is pos.x - size.x. please set to only positive value.<para></para>
	/// Quaternion orientation : Represent a rotation. The "size" vector will rotate by "orientation" at the calculate method.<para></para>
	/// bool exist : Is enable collision ?
	/// </summary>
	class OBB
	{
	public:
		Donya::Vector3		pos{};			// Center-position. The calculate method regards the belong space is the same.
		Donya::Vector3		size{};			// Half-size(like radius). the left is pos.x - size.x. please set to only positive value.
		Donya::Quaternion	orientation{};	// Represent a rotation. The "size" vector will rotate by "orientation" at the calculate method.
		bool exist{ true };					// Is enable collision ?
	private:
		friend class cereal::access;
		template<class Archive>
		void serialize( Archive &archive, std::uint32_t version )
		{
			archive
			(
				CEREAL_NVP( pos ),
				CEREAL_NVP( size ),
				CEREAL_NVP( orientation ),
				CEREAL_NVP( exist )
			);
			if ( 1 <= version )
			{
				// archive();
			}
		}
	public:
		static bool IsHitOBB( const OBB &L, const OBB &R, bool ignoreExistFlag = false );
		static bool IsHitSphere( const OBB &L, const Sphere &R, bool ignoreExistFlag = false );
	public:
		static OBB Nil()
		{
			return OBB{ {}, {}, {}, false };
		}
	};

	bool		operator == ( const Box &L,		const Box &R );
	static bool	operator != ( const Box &L,		const Box &R ) { return !( L == R ); }

	bool		operator == ( const Circle &L,	const Circle &R );
	static bool	operator != ( const Circle &L,	const Circle &R ) { return !( L == R ); }

	bool		operator == ( const AABB &L,	const AABB &R );
	static bool	operator != ( const AABB &L,	const AABB &R ) { return !( L == R ); }

	bool		operator == ( const Sphere &L,	const Sphere &R );
	static bool	operator != ( const Sphere &L,	const Sphere &R ) { return !( L == R ); }
	
	bool		operator == ( const OBB &L,		const OBB &R );
	static bool	operator != ( const OBB &L,		const OBB &R ) { return !( L == R ); }


	struct RayIntersectResult
	{
		Donya::Vector3 intersection;
		bool isIntersect = false;
	};
	RayIntersectResult CalcIntersectionPoint( const Donya::Vector3 &rayStart, const Donya::Vector3 &rayEnd, const AABB &box );
}

CEREAL_CLASS_VERSION( Donya::Box,		1 );
CEREAL_CLASS_VERSION( Donya::Circle,	1 );
CEREAL_CLASS_VERSION( Donya::AABB,		0 );
CEREAL_CLASS_VERSION( Donya::Sphere,	0 );
CEREAL_CLASS_VERSION( Donya::OBB,		0 );

#endif // INCLUDED_DONYA_COLLISION_H_
