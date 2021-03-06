#pragma once

#include <string>		// Use for ShowImGuiNode().
#include <windows.h>	// Use for enable bitwise operation of enum class.

#include "Donya/Serializer.h"
#include "Donya/UseImGui.h"

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
		Ice		= 1 << 2, // It will not sticking to other.

		_TypeCount
	};
private:
	Type type; // The type stored by bitwise operation.
public:
	constexpr Element() : type( Type::Nil ) {}
	constexpr Element( Type type ) : type( type ) {}
	Element &operator = ( Type newType ) { type = newType; return *this; }
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
public:
	bool	Has		( Type validation	) const;
	Element	Add		( Type addition		);
	Element	Assign	( Type newType		);
	Element	Subtract( Type subtraction	);
public:
#if USE_IMGUI
	void ShowImGuiNode( bool useTreeNode, const std::string &nodeCaption );
#endif // USE_IMGUI
};
CEREAL_CLASS_VERSION( Element, 0 )
DEFINE_ENUM_FLAG_OPERATORS( Element::Type );
