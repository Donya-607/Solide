#include "Element.h"

bool	Element::Has		( Type val	) const { return ( ( type & val ) != Type::Nil ); }
Element	Element::Add		( Type add	) { type |= add;	return *this; }
Element	Element::Assign		( Type val	) { type =  val;	return *this; }
Element	Element::Subtract	( Type sub	) { type &= ~sub;	return *this; }

#if USE_IMGUI

namespace
{
	/// <summary>
	/// A bitwise operated type is not support.
	/// </summary>
	std::string GetTypeName( Element::Type x )
	{
		switch ( x )
		{
		case Element::Type::Nil:	return "Nil";
		case Element::Type::Oil:	return "Oil";
		case Element::Type::Flame:	return "Flame";
		case Element::Type::Ice:	return "Ice";
		default: break;
		}
		return "ERROR_TYPE";
	}

	/// <summary>
	/// Make combined string of whole types that the x has.
	/// </summary>
	std::string MakeWholeTypeName( Element::Type x )
	{
		if ( x == Element::Type::Nil ) { return "Nil"; }
		// else

		auto Has		= []( Element::Type lhs, Element::Type rhs )
		{
			return ( lhs & rhs ) != Element::Type::Nil;
		};
		auto Append		= []( std::string *pStr, const std::string &addition, const std::string &delimiter )
		{
			if ( !pStr->empty() ) { *pStr += delimiter; }
			*pStr += addition;
		};
		auto Advance	= []( Element::Type *p )
		{
			const int intType = scast<int>( *p );
			*p =	( intType <= 0 )
					? scast<Element::Type>( 1 )
					: scast<Element::Type>( intType << 1 );
		};

		std::string wholeTypeName;

		Element::Type type = Element::Type::Nil;
		Advance( &type );
		while ( scast<int>( type ) < scast<int>( Element::Type::_TypeCount ) )
		{
			if ( Has( x, type ) )
			{
				constexpr const char *delimiter = "|";
				Append( &wholeTypeName, GetTypeName( type ), delimiter );
			}

			Advance( &type );
		}

		return wholeTypeName;
	}
}

void Element::ShowImGuiNode( bool useTreeNode, const std::string &nodeCaption )
{
	if ( useTreeNode && !ImGui::TreeNode( nodeCaption.c_str() ) ) { return; }
	// else

	const std::string typeName = MakeWholeTypeName( type );
	const std::string caption  = u8"åªç›ÇÃèÛë‘ÅF" + typeName;
	ImGui::Text( caption.c_str() );

	if ( useTreeNode ) { ImGui::TreePop(); }
}
#endif // USE_IMGUI
