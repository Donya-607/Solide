#pragma once

#include <windows.h> // Use for enable bitwise operation of enum class.

#include "Donya/Serializer.h"

class Element
{
public:
	/// <summary>
	/// You can use bitwise operation.
	/// </summary>
	enum class Type : int
	{
		Nil		= 0,
		Oil		= 1 << 0,
		Flame	= 1 << 1,
		Ice		= 1 << 2,
	};
private:
	Type type; // The type stored by bitwise operation.
public:
	constexpr Element() : type( Type::Nil ) {}
	constexpr Element( const Type &type ) : type( type ) {}
private:
	friend class cereal::access;
	template<class Archive>
	void serialize( Archive &archive, std::uint32_t version )
	{
		archive
		(
			CEREAL_NVP( type )
		);

		if ( 1 <= version )
		{
			// archive( CEREAL_NVP( x ) );
		}
	}
public:
	constexpr Type Get() const { return type; }
	Element Add( Type addition );
	Element Assign( Type newType );
	Element Subtract( Type subtraction );
};

DEFINE_ENUM_FLAG_OPERATORS( Element::Type );
