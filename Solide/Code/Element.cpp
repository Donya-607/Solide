#include "Element.h"

bool	Element::Has		( Type val	) const { return ( ( type & val ) != Type::Nil ); }
Element	Element::Add		( Type add	) { type |= add;	return *this; }
Element	Element::Assign		( Type val	) { type =  val;	return *this; }
Element	Element::Subtract	( Type sub	) { type &= ~sub;	return *this; }
