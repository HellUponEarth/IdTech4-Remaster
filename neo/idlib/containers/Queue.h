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

#ifndef __QUEUE_H__
#define __QUEUE_H__

/*
===============================================================================

	Queue template

===============================================================================
*/

#define idQueue( type, next )		idQueueTemplate<type, offsetof( type, next )>

template< class type, size_t nextOffset >
class idQueueTemplate {
public:
							idQueueTemplate( void );

	void					Add( type *p_element );
	type *					Get( void );

private:
	type *					p_first;
	type *					p_last;
};

#define QUEUE_NEXT_PTR( p_element )		(*((type**)(((byte*)p_element)+nextOffset)))

template< class type, size_t nextOffset >
idQueueTemplate<type,nextOffset>::idQueueTemplate( void ) {
	p_first = p_last = NULL;
}

template< class type, size_t nextOffset >
void idQueueTemplate<type,nextOffset>::Add( type *p_element ) {
	QUEUE_NEXT_PTR(p_element) = NULL;
	if ( p_last ) {
		QUEUE_NEXT_PTR(p_last) = p_element;
	} else {
		p_first = p_element;
	}
	p_last = p_element;
}

template< class type, size_t nextOffset >
type *idQueueTemplate<type,nextOffset>::Get( void ) {
	type *p_element;

	p_element = p_first;
	if ( p_element ) {
		p_first = QUEUE_NEXT_PTR(p_first);
		if ( p_last == p_element ) {
			p_last = NULL;
		}
		QUEUE_NEXT_PTR(p_element) = NULL;
	}
	return p_element;
}

#endif /* !__QUEUE_H__ */
