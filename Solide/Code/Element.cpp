#include "Element.h"

bool	Element::Has		( Type val	) const { return ( ( type & val ) != Type::Nil ); }
Element	Element::Add		( Type add	) { type |= add;	return *this; }
Element	Element::Assign		( Type val	) { type =  val;	return *this; }
Element	Element::Subtract	( Type sub	) { type &= ~sub;	return *this; }

#if USE_IMGUI

namespace
{
	Element::Type AdvanceType( Element::Type from )
	{
		const int intType = scast<int>( from );
		return	( intType <= 0 )
				? scast<Element::Type>( 1 )
				: scast<Element::Type>( intType << 1 );
	}

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
		
		std::string wholeTypeName;

		Element::Type type = AdvanceType( Element::Type::Nil );
		while ( scast<int>( type ) < scast<int>( Element::Type::_TypeCount ) )
		{
			if ( Has( x, type ) )
			{
				constexpr const char *delimiter = "|";
				Append( &wholeTypeName, GetTypeName( type ), delimiter );
			}

			type = AdvanceType( type );
		}

		return wholeTypeName;
	}
}

void Element::ShowImGuiNode( bool useTreeNode, const std::string &nodeCaption )
{
	if ( useTreeNode && !ImGui::TreeNode( nodeCaption.c_str() ) ) { return; }
	// else

	const std::string typeName = MakeWholeTypeName( type );
	const std::string caption  = u8"現在の属性：" + typeName;
	ImGui::Text( caption.c_str() );

	if ( ImGui::TreeNode( u8"属性の変更" ) )
	{
		enum Work { Add, Assign, Sub };
		static Work workType = Add;
		if ( ImGui::RadioButton( u8"操作：追加", workType == Add		) ) { workType = Add;		}
		if ( ImGui::RadioButton( u8"操作：代入", workType == Assign	) ) { workType = Assign;	}
		if ( ImGui::RadioButton( u8"操作：削除", workType == Sub		) ) { workType = Sub;		}
		ImGui::Text( "" );

		auto Affect = []( Element::Type *pTarget, const Element::Type &action )
		{
			switch ( workType )
			{
			case Add:		*pTarget |=  action; return;
			case Assign:	*pTarget =   action; return;
			case Sub:		*pTarget &= ~action; return;
			default:		return;
			}
		};

		Element::Type index = Element::Type::Nil;
		while ( scast<int>( index ) < scast<int>( Element::Type::_TypeCount ) )
		{
			const std::string caption = u8"[" + GetTypeName( index ) + u8"]を適用する";
			if ( ImGui::Button( caption.c_str() ) )
			{
				Affect( &type, index );
			}

			index = AdvanceType( index );
		}
		ImGui::TreePop();
	}

	if ( useTreeNode ) { ImGui::TreePop(); }
}
#endif // USE_IMGUI
