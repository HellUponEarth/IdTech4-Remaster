/*
===========================================================================

Doom 3 GPL Source Code
Copyright (C) 1999-2011 id Software LLC, a ZeniMax Media company. 

This file is part of the Doom 3 GPL Source Code (?Doom 3 Source Code?).  

Doom 3 Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Doom 3 Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Doom 3 Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Doom 3 Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Doom 3 Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

#ifndef __STACK_H__
#define __STACK_H__

/*
===============================================================================

	Stack template

===============================================================================
*/

#define idStack( type, next )		idStackTemplate<type, offsetof( type, next )>

template< class type, size_t nextOffset >
class idStackTemplate {
public:
							idStackTemplate( void );

	void					Add( type *p_element );
	type *					Get( void );

private:
	type *					p_top;
	type *					p_bottom;
};

#define STACK_NEXT_PTR( p_element )		(*(type**)(((byte*)p_element)+nextOffset))

template< class type, size_t nextOffset >
idStackTemplate<type,nextOffset>::idStackTemplate( void ) {
	p_top = p_bottom = NULL;
}

template< class type, size_t nextOffset >
void idStackTemplate<type,nextOffset>::Add( type *p_element ) {
	STACK_NEXT_PTR(p_element) = p_top;
	p_top = p_element;
	if ( !p_bottom ) {
		p_bottom = p_element;
	}
}

template< class type, size_t nextOffset >
type *idStackTemplate<type,nextOffset>::Get( void ) {
	type *p_element;

	p_element = p_top;
	if ( p_element ) {
		p_top = STACK_NEXT_PTR(p_top);
		if ( p_bottom == p_element ) {
			p_bottom = NULL;
		}
		STACK_NEXT_PTR(p_element) = NULL;
	}
	return p_element;
}

#endif /* !__STACK_H__ */
