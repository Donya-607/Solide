#include "Collision.h"

#include <array>

#include "Constant.h"
#include "Useful.h"	// Use ZeroEqual().

#undef max
#undef min

namespace Donya
{
	void Box::Set			( float centerX, float centerY, float halfWidth, float halfHeight, bool isExist )
	{
		pos.x	= centerX;
		pos.y	= centerY;
		size.x	= halfWidth;
		size.y	= halfHeight;
		exist	= isExist;
	}
	bool Box::IsHitPoint	( const Box &L, const float &RX, const float &RY, bool ignoreExistFlag )
	{
		if ( !ignoreExistFlag && !L.exist ) { return false; }
		// else

		if	(
				L.pos.x - L.size.x	<= RX					&&	// X1 - W1	<= X2
				RX					<= L.pos.x + L.size.x	&&	// X2		<= X1 + W1
				L.pos.y - L.size.y	<= RY					&&	// Y1 - H1	<= Y2
				RY					<= L.pos.y + L.size.y		// Y2		<= Y1 + H2
			)
		{
			return true;
		}
		//else
		return false;
	}
	bool Box::IsHitPoint	( const Box &L, const float &LBoxScreenPosX, const float &LBoxScreenPosY, const float &RX, const float &RY, bool ignoreExistFlag )
	{
		if ( !ignoreExistFlag && !L.exist ) { return false; }
		// else

		Box tmp
		{
			L.pos.x + LBoxScreenPosX,
			L.pos.y + LBoxScreenPosY,
			L.size.x,
			L.size.y,
			true
		};
		return Box::IsHitPoint( tmp, RX, RY );
	}
	bool Box::IsHitBox		( const Box &L, const Box &R, bool ignoreExistFlag )
	{
		if ( !ignoreExistFlag && ( !L.exist || !R.exist ) ) { return false; }
		// else

		if	(
				L.pos.x - L.size.x <= R.pos.x + R.size.x	&&	// X1 - W1 < X2 + W2
				R.pos.x - R.size.x <= L.pos.x + L.size.x	&&	// X2 - W2 < X1 + W1
				L.pos.y - L.size.y <= R.pos.y + R.size.y	&&	// Y1 - H1 < Y2 + H2
				R.pos.y - R.size.y <= L.pos.y + L.size.y		// Y2 - H2 < Y1 + H1
			)
		{
			return true;
		}
		//else
		return false;
	}
	bool Box::IsHitBox		( const Box &L, const float &LBoxScreenPosX, const float &LBoxScreenPosY, const Box &R, const float &RBoxScreenPosX, const float &RBoxScreenPosY, bool ignoreExistFlag )
	{
		if ( !ignoreExistFlag && ( !L.exist || !R.exist ) ) { return false; }
		// else

		Box tmpL
		{
			L.pos.x + LBoxScreenPosX,
			L.pos.y + LBoxScreenPosY,
			L.size.x,
			L.size.y,
			true
		};
		Box tmpR
		{
			R.pos.x + RBoxScreenPosX,
			R.pos.y + RBoxScreenPosY,
			R.size.x,
			R.size.y,
			true
		};
		return Box::IsHitBox( tmpL, tmpR );
	}
	bool Box::IsHitCircle	( const Box &L, const Circle &R, bool ignoreExistFlag )
	{
		if ( !ignoreExistFlag && ( !L.exist || !R.exist ) ) { return false; }
		// else

		/*
		1.	Create a rectangle of magnification by the radius of the circle.
		2.	Judge the collision between that magnified rectangle and the center position of the circle.
		*/

		// VS The rectangle of vertically magnified.
		{
			Box tmp =
			{
				L.pos.x, 
				L.pos.y, 
				L.size.x, 
				L.size.y + R.radius, 
				true
			};
			if ( Box::IsHitPoint( tmp, R.pos.x, R.pos.y ) )
			{
				return true;
			}
		}
		// VS The rectangle of horizontally magnified.
		{
			Box tmp =
			{
				L.pos.x,
				L.pos.y,
				L.size.x + R.radius,
				L.size.y,
				true
			};
			if ( Box::IsHitPoint( tmp, R.pos.x, R.pos.y ) )
			{
				return true;
			}
		}
		// VS Center position of the circle from four-corners.
		{
			if	(
					Circle::IsHitPoint( R, L.pos.x - L.size.x,	L.pos.y - L.size.y )	||	// Left-Top
					Circle::IsHitPoint( R, L.pos.x - L.size.x,	L.pos.y + L.size.y )	||	// Left-Bottom
					Circle::IsHitPoint( R, L.pos.x + L.size.x,	L.pos.y - L.size.y )	||	// Right-Top
					Circle::IsHitPoint( R, L.pos.x + L.size.x,	L.pos.y + L.size.y )		// Right-Bottom
				)
			{
				return true;
			}
		}
		return false;
	}
	bool Box::IsHitCircle	( const Box &L, const float &LBoxScreenPosX, const float &LBoxScreenPosY, const Circle &R, const float &RCircleScreenPosX, const float &RCircleScreenPosY, bool ignoreExistFlag )
	{
		if ( !ignoreExistFlag && ( !L.exist || !R.exist ) ) { return false; }
		// else

		Box tmpL
		{
			L.pos.x + LBoxScreenPosX,
			L.pos.y + LBoxScreenPosY,
			L.size.x,
			L.size.y,
			true
		};
		Circle tmpR
		{
			R.pos.x + RCircleScreenPosX,
			R.pos.y + RCircleScreenPosY,
			R.radius,
			true
		};
		return Box::IsHitCircle( tmpL, tmpR );
	}

	void Circle::Set		( float centerX, float centerY, float rad, bool isExist )
	{
		pos.x	= centerX;
		pos.y	= centerY;
		radius	= rad;
		exist	= isExist;
	}
	bool Circle::IsHitPoint	( const Circle &L, const float &RX, const float &RY, bool ignoreExistFlag )
	{
		if ( !ignoreExistFlag && !L.exist ) { return false; }
		// else

		return ( ( ( RX - L.pos.x ) * ( RX - L.pos.x ) ) + ( ( RY - L.pos.y ) * ( RY - L.pos.y ) ) < ( L.radius * L.radius ) );
	}
	bool Circle::IsHitPoint	( const Circle &L, const float &LCircleScreenPosX, const float &LCircleScreenPosY, const float &RX, const float &RY, bool ignoreExistFlag )
	{
		if ( !ignoreExistFlag && !L.exist ) { return false; }
		// else

		Circle tmp
		{
			L.pos.x + LCircleScreenPosX,
			L.pos.y + LCircleScreenPosY,
			L.radius,
			true
		};
		return Circle::IsHitPoint( tmp, RX, RY );
	}
	bool Circle::IsHitCircle( const Circle &L, const Circle &R, bool ignoreExistFlag )
	{
		if ( !ignoreExistFlag && ( !L.exist || !R.exist ) ) { return false; }
		// else

		float dx  = R.pos.x - L.pos.x;
		float dy  = R.pos.y - L.pos.y;
		float rad = R.radius + L.radius;

		return ( ( dx * dx ) + ( dy * dy ) < rad * rad );
	}
	bool Circle::IsHitCircle( const Circle &L, const float &LCircleScreenPosX, const float &LCircleScreenPosY, const Circle &R, const float &RCircleScreenPosX, const float &RCircleScreenPosY, bool ignoreExistFlag )
	{
		if ( !ignoreExistFlag && ( !L.exist || !R.exist ) ) { return false; }
		// else

		Circle tmpL
		{
			L.pos.x + LCircleScreenPosX,
			L.pos.y + LCircleScreenPosY,
			L.radius,
			true
		};
		Circle tmpR
		{
			R.pos.x + RCircleScreenPosX,
			R.pos.y + RCircleScreenPosY,
			R.radius,
			true
		};
		return Circle::IsHitCircle( tmpL, tmpR );
	}
	bool Circle::IsHitBox	( const Circle &L, const Box &R, bool ignoreExistFlag )
	{
		if ( !ignoreExistFlag && ( !L.exist || !R.exist ) ) { return false; }
		// else

		return Box::IsHitCircle( R, L );
	}
	bool Circle::IsHitBox	( const Circle &L, const float &LCircleScreenPosX, const float &LCircleScreenPosY, const Box &R, const float &RBoxScreenPosX, const float &RBoxScreenPosY, bool ignoreExistFlag )
	{
		if ( !ignoreExistFlag && ( !L.exist || !R.exist ) ) { return false; }
		// else

		return Box::IsHitCircle( R, RBoxScreenPosX, RBoxScreenPosY, L, LCircleScreenPosX, LCircleScreenPosY );
	}

	bool AABB::IsHitPoint	( const AABB &L, const Donya::Vector3 &R, bool ignoreExistFlag )
	{
		if ( !ignoreExistFlag && !L.exist ) { return false; }
		// else

		constexpr size_t AXIS_COUNT = 3;

		const std::array<float, AXIS_COUNT> point{ R.x,			R.y,		R.z			};	// [0:X][1:Y][2:Z]
		const std::array<float, AXIS_COUNT> AABB { L.pos.x,		L.pos.y,	L.pos.z		};	// [0:X][1:Y][2:Z]
		const std::array<float, AXIS_COUNT> size { L.size.x,	L.size.y,	L.size.z	};	// [0:X][1:Y][2:Z]

		for ( size_t i = 0; i < AXIS_COUNT; ++i )
		{
			// If isn't [Min <= Point <= Max], judge to false.
			if ( point[i] < AABB[i] - size[i] ) { return false; }
			if ( point[i] > AABB[i] + size[i] ) { return false; }
		}

		return true;
	}
	bool AABB::IsHitAABB	( const AABB &L, const AABB &R, bool ignoreExistFlag )
	{
		if ( !ignoreExistFlag && ( !L.exist || !R.exist ) ) { return false; }
		// else

		// Judge by "AABB of extended by size of other" vs "position of other".
	
		AABB extAABB = L;
		extAABB.size += R.size;	// If "R.size" is negative, maybe this method does not work.

		const Donya::Vector3 &point = R.pos;

		return IsHitPoint( extAABB, point );
	
	}
	bool AABB::IsHitSphere	( const AABB &L, const Sphere &R, bool ignoreExistFlag )
	{
		if ( !ignoreExistFlag && ( !L.exist || !R.exist ) ) { return false; }
		// else

		auto CalcShortestDistanceSq = []( const AABB &L, const Donya::Vector3 &R )->float
		{
			// from http://marupeke296.com/COL_3D_No11_AABBvsPoint.html

			constexpr size_t AXIS_COUNT = 3;

			const std::array<float, AXIS_COUNT> point{ R.x,			R.y,		R.z			};	// [0:X][1:Y][2:Z]
			const std::array<float, AXIS_COUNT> AABB { L.pos.x,		L.pos.y,	L.pos.z		};	// [0:X][1:Y][2:Z]
			const std::array<float, AXIS_COUNT> size { L.size.x,	L.size.y,	L.size.z	};	// [0:X][1:Y][2:Z]

			std::array<float, AXIS_COUNT> distance{};

			float max{}, min{};
			for ( size_t i = 0; i < AXIS_COUNT; ++i )
			{
				max = AABB[i] + size[i];
				min = AABB[i] - size[i];

				if ( point[i] > max ) { distance[i] = point[i] - max; }
				if ( point[i] < min ) { distance[i] = point[i] - min; }
			}

			Donya::Vector3 v{ distance[0], distance[1], distance[2] };

			return v.LengthSq();
		};

		float distanceSq = CalcShortestDistanceSq( L, R.pos );
		return ( distanceSq < ( R.radius * R.radius ) );
	}
	
	bool Sphere::IsHitPoint	( const Sphere &L, const Donya::Vector3 &R, bool ignoreExistFlag )
	{
		if ( !ignoreExistFlag && !L.exist ) { return false; }
		// else

		Donya::Vector3 d = R - L.pos;
		return ( d.LengthSq() < L.radius * L.radius );
	}
	bool Sphere::IsHitSphere( const Sphere &L, const Sphere &R, bool ignoreExistFlag )
	{
		if ( !ignoreExistFlag && ( !L.exist || !R.exist ) ) { return false; }
		// else

		float r = L.radius + R.radius;
		Donya::Vector3 d = R.pos - L.pos;

		return ( d.LengthSq() < ( r * r ) );
	}
	bool Sphere::IsHitAABB	( const Sphere &L, const AABB &R, bool ignoreExistFlag )
	{
		if ( !ignoreExistFlag && ( !L.exist || !R.exist ) ) { return false; }
		// else

		return AABB::IsHitSphere( R, L );
	}

	bool OBB::IsHitOBB( const OBB &L, const OBB &R, bool ignoreExistFlag )
	{
		if ( !ignoreExistFlag && ( !L.exist || !R.exist ) ) { return false; }
		// else

		// see http://marupeke296.com/COL_3D_No13_OBBvsOBB.html

		/// <summary>
		/// Returns projecected line-segment length is half.
		/// </summary>
		auto CalcLengthOfProjectionLineSeg = []( const Donya::Vector3 &nSeparateAxis, const Donya::Vector3 &nDirX, const Donya::Vector3 &nDirY, const Donya::Vector3 *pDirZ/* If needed */ = nullptr )
		{
			float  projLengthX = fabsf( Dot( nSeparateAxis, nDirX ) );
			float  projLengthY = fabsf( Dot( nSeparateAxis, nDirY ) );
			float  projLengthZ = ( !pDirZ ) ? 0.0f : fabsf( Dot( nSeparateAxis, *pDirZ ) );
			return projLengthX + projLengthY + projLengthZ;
		};

		const Donya::Vector3 nDirLX = L.orientation.LocalRight(),	dirLX = nDirLX * L.size.x;
		const Donya::Vector3 nDirLY = L.orientation.LocalUp(),		dirLY = nDirLY * L.size.y;
		const Donya::Vector3 nDirLZ = L.orientation.LocalFront(),	dirLZ = nDirLZ * L.size.z;
		const Donya::Vector3 nDirRX = R.orientation.LocalRight(),	dirRX = nDirRX * R.size.x;
		const Donya::Vector3 nDirRY = R.orientation.LocalUp(),		dirRY = nDirRY * R.size.y;
		const Donya::Vector3 nDirRZ = R.orientation.LocalFront(),	dirRZ = nDirRZ * R.size.z;
		const Donya::Vector3 between = L.pos - R.pos;

		float projLenL{}, projLenR{}, distance{};
		auto IsOverlapping = [&projLenL, &projLenR, &distance]()->bool
		{
			return ( distance <= projLenL + projLenR ) ? true : false;
		};

		projLenL = dirLX.Length();
		projLenR = CalcLengthOfProjectionLineSeg( nDirLX, dirRX, dirRY, &dirRZ );
		distance = fabsf( Dot( between, nDirLX ) );
		if ( !IsOverlapping() ) { return false; }
		// else
		projLenL = dirLY.Length();
		projLenR = CalcLengthOfProjectionLineSeg( nDirLY, dirRX, dirRY, &dirRZ );
		distance = fabsf( Dot( between, nDirLY ) );
		if ( !IsOverlapping() ) { return false; }
		// else
		projLenL = dirLZ.Length();
		projLenR = CalcLengthOfProjectionLineSeg( nDirLZ, dirRX, dirRY, &dirRZ );
		distance = fabsf( Dot( between, nDirLZ ) );
		if ( !IsOverlapping() ) { return false; }
		// else

		projLenL = CalcLengthOfProjectionLineSeg( nDirRX, dirLX, dirLY, &dirLZ );
		projLenR = dirRX.Length();
		distance = fabsf( Dot( between, nDirRX ) );
		if ( !IsOverlapping() ) { return false; }
		// else
		projLenL = CalcLengthOfProjectionLineSeg( nDirRY, dirLX, dirLY, &dirLZ );
		projLenR = dirRY.Length();
		distance = fabsf( Dot( between, nDirRY ) );
		if ( !IsOverlapping() ) { return false; }
		// else
		projLenL = CalcLengthOfProjectionLineSeg( nDirRZ, dirLX, dirLY, &dirLZ );
		projLenR = dirRZ.Length();
		distance = fabsf( Dot( between, nDirRZ ) );
		if ( !IsOverlapping() ) { return false; }
		// else

		Donya::Vector3 cross{}; // Should I normalize this at every result?

		cross = Cross( nDirLX, nDirRX );
		projLenL = CalcLengthOfProjectionLineSeg( cross, dirLY, dirLZ );
		projLenL = CalcLengthOfProjectionLineSeg( cross, dirRY, dirRZ );
		distance = Dot( between, cross );
		if ( !IsOverlapping() ) { return false; }
		// elee
		cross = Cross( nDirLX, nDirRY );
		projLenL = CalcLengthOfProjectionLineSeg( cross, dirLY, dirLZ );
		projLenL = CalcLengthOfProjectionLineSeg( cross, dirRX, dirRZ );
		distance = Dot( between, cross );
		if ( !IsOverlapping() ) { return false; }
		// elee
		cross = Cross( nDirLX, nDirRZ );
		projLenL = CalcLengthOfProjectionLineSeg( cross, dirLY, dirLZ );
		projLenL = CalcLengthOfProjectionLineSeg( cross, dirRX, dirRY );
		distance = Dot( between, cross );
		if ( !IsOverlapping() ) { return false; }
		// elee

		cross = Cross( nDirLY, nDirRX );
		projLenL = CalcLengthOfProjectionLineSeg( cross, dirLX, dirLZ );
		projLenL = CalcLengthOfProjectionLineSeg( cross, dirRY, dirRZ );
		distance = Dot( between, cross );
		if ( !IsOverlapping() ) { return false; }
		// elee
		cross = Cross( nDirLY, nDirRY );
		projLenL = CalcLengthOfProjectionLineSeg( cross, dirLX, dirLZ );
		projLenL = CalcLengthOfProjectionLineSeg( cross, dirRX, dirRZ );
		distance = Dot( between, cross );
		if ( !IsOverlapping() ) { return false; }
		// elee
		cross = Cross( nDirLY, nDirRZ );
		projLenL = CalcLengthOfProjectionLineSeg( cross, dirLX, dirLZ );
		projLenL = CalcLengthOfProjectionLineSeg( cross, dirRX, dirRY );
		distance = Dot( between, cross );
		if ( !IsOverlapping() ) { return false; }
		// elee

		cross = Cross( nDirLZ, nDirRX );
		projLenL = CalcLengthOfProjectionLineSeg( cross, dirLX, dirLY );
		projLenL = CalcLengthOfProjectionLineSeg( cross, dirRY, dirRZ );
		distance = Dot( between, cross );
		if ( !IsOverlapping() ) { return false; }
		// elee
		cross = Cross( nDirLZ, nDirRY );
		projLenL = CalcLengthOfProjectionLineSeg( cross, dirLX, dirLY );
		projLenL = CalcLengthOfProjectionLineSeg( cross, dirRX, dirRZ );
		distance = Dot( between, cross );
		if ( !IsOverlapping() ) { return false; }
		// elee
		cross = Cross( nDirLZ, nDirRZ );
		projLenL = CalcLengthOfProjectionLineSeg( cross, dirLX, dirLY );
		projLenL = CalcLengthOfProjectionLineSeg( cross, dirRX, dirRY );
		distance = Dot( between, cross );
		if ( !IsOverlapping() ) { return false; }
		// elee

		return true;
	}

	float CalcShortestDistance( const OBB &L, const Donya::Vector3 &point )
	{
		// see http://marupeke296.com/COL_3D_No12_OBBvsPoint.html

		enum IntAxis { X = 0, Y, Z, END };
		auto ExtractHalfLength  = []( const OBB &OBB, IntAxis axisIndex )->float
		{
			_ASSERT_EXPR( 0 <= axisIndex && axisIndex < END, L"Error : Passed index out of range !" );

			switch ( axisIndex )
			{
			case X: return OBB.size.x; // break;
			case Y: return OBB.size.y; // break;
			case Z: return OBB.size.z; // break;
			default: break;
			}

			return NULL;
		};
		auto ExtractRotatedAxis = []( const OBB &OBB, IntAxis axisIndex )->Donya::Vector3
		{
			_ASSERT_EXPR( 0 <= axisIndex && axisIndex < END, L"Error : Passed index out of range !" );

			constexpr std::array<Donya::Vector3, END> AXES
			{
				Donya::Vector3::Right(),
				Donya::Vector3::Up(),
				Donya::Vector3::Front()
			};

			Donya::Vector3 axis = AXES[axisIndex];
			return OBB.orientation.RotateVector( axis );
		};

		Donya::Vector3 protrudedSum{};

		for ( int i = 0; i < END; ++i )
		{
			float halfLength = ExtractHalfLength( L, static_cast<IntAxis>( i ) );
			if (  halfLength <= 0 ) { continue; } // This case can not calculate.
			// else

			Donya::Vector3 axis = ExtractRotatedAxis( L, static_cast<IntAxis>( i ) );

			float magni = Donya::Vector3::Dot( ( point - L.pos ), axis ) / halfLength;
			magni = fabsf( magni );

			if ( 1.0f < magni )
			{
				protrudedSum += ( 1.0f - magni ) * halfLength * axis;
			}
		}

		return protrudedSum.Length();
	}
	bool OBB::IsHitSphere( const OBB &L, const Sphere &R, bool ignoreExistFlag )
	{
		if ( !ignoreExistFlag && ( !L.exist || !R.exist ) ) { return false; }
		// else

		float distance = CalcShortestDistance( L, R.pos );
		return ( distance <= R.radius ) ? true : false;
	}

	bool operator == ( const Box	&L, const Box		&R )
	{
		if ( !ZeroEqual( L.pos.x  - R.pos.x  ) )	{ return false; }
		if ( !ZeroEqual( L.pos.y  - R.pos.y  ) )	{ return false; }
		if ( !ZeroEqual( L.size.x - R.size.x ) )	{ return false; }
		if ( !ZeroEqual( L.size.y - R.size.y ) )	{ return false; }
		if ( L.exist != R.exist )					{ return false; }
		// else
		return true;
	}
	bool operator == ( const Circle	&L, const Circle	&R )
	{
		if ( !ZeroEqual( L.pos.x  - R.pos.x  ) )	{ return false; }
		if ( !ZeroEqual( L.pos.y  - R.pos.y  ) )	{ return false; }
		if ( !ZeroEqual( L.radius - R.radius ) )	{ return false; }
		if ( L.exist != R.exist )					{ return false; }
		// else
		return true;
	}
	bool operator == ( const AABB	&L, const AABB		&R )
	{
		if ( ! ( L.pos  - R.pos  ).IsZero() )		{ return false; }
		if ( ! ( L.size - R.size ).IsZero() )		{ return false; }
		if ( L.exist != R.exist )					{ return false; }
		// else
		return true;
	}
	bool operator == ( const Sphere	&L, const Sphere	&R )
	{
		if ( ! ( L.pos  - R.pos  ).IsZero() )		{ return false; }
		if ( !ZeroEqual( L.radius - R.radius ) )	{ return false; }
		if ( L.exist != R.exist )					{ return false; }
		// else
		return true;
	}
	bool operator == ( const OBB	&L, const OBB		&R )
	{
		if ( !( L.pos  - R.pos  ).IsZero() ) { return false; }
		if ( !( L.size - R.size ).IsZero() ) { return false; }
		if ( !L.orientation.IsSameRotation( R.orientation ) ) { return false; }
		if ( L.exist != R.exist ) { return false; }
		// else
		return true;
	}


	RayIntersectResult CalcIntersectionPoint( const Donya::Vector3 &rayStart, const Donya::Vector3 &rayEnd, const AABB &box )
	{
		const auto &a  = rayStart;
		const auto &b  = rayEnd;
		// const auto dir = ( rayEnd - rayStart ).Unit();
		const auto dir = ( rayEnd - rayStart );
		const Donya::Vector3 invDir
		{
			1.0f / dir.x,
			1.0f / dir.y,
			1.0f / dir.z,
		};

		const auto boxMin = box.pos - box.size;
		const auto boxMax = box.pos + box.size;
		
		Donya::Vector3 tMin;
		Donya::Vector3 tMax;
		for ( int i = 0; i < 3; ++i )
		{
			tMin[i] = ( boxMin[i] - a[i] ) * invDir[i];
			tMax[i] = ( boxMax[i] - a[i] ) * invDir[i];

			if ( tMax[i] < tMin[i] )
			{
				auto tmp = tMax[i];
				tMax[i] = tMin[i];
				tMin[i] = tmp;
			}
		}

		float greatestMin = -FLT_MAX;
		float smallestMax = +FLT_MAX;
		for ( int i = 0; i < 3; ++i )
		{
			greatestMin = std::max( tMin[i], greatestMin );
			smallestMax = std::min( tMax[i], smallestMax );
		}

		bool isIntersect = true;
		for ( int i = 0; i < 3; ++i )
		{
			if ( tMax[i] < tMin[i] ) { isIntersect = false; break; }
			if ( tMax[i] < greatestMin ) { isIntersect = false; break; }
			if ( smallestMax < tMin[i] ) { isIntersect = false; break; }
		}

		RayIntersectResult result;
		result.isIntersect = isIntersect;
		result.intersection = a + dir * greatestMin;

		result.normal = 0.0f;
		const Donya::Vector3 diff = result.intersection - box.pos;
		if ( diff.y < diff.x && diff.z < diff.x ) { result.normal.x = scast<float>( Donya::SignBit( diff.x ) ); }
		if ( diff.x < diff.y && diff.z < diff.y ) { result.normal.y = scast<float>( Donya::SignBit( diff.y ) ); }
		if ( diff.x < diff.z && diff.y < diff.z ) { result.normal.z = scast<float>( Donya::SignBit( diff.z ) ); }

		return result;
	}
}
