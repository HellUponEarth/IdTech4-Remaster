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

#ifndef __LINKLIST_H__
#define __LINKLIST_H__

/*
==============================================================================

idLinkList

Circular linked list template

==============================================================================
*/

template< class type >
class idLinkList {
public:
						idLinkList();
						~idLinkList();

	bool				IsListEmpty( void ) const;
	bool				InList( void ) const;
	int					Num( void ) const;
	void				Clear( void );

	void				InsertBefore( idLinkList &node );
	void				InsertAfter( idLinkList &node );
	void				AddToEnd( idLinkList &node );
	void				AddToFront( idLinkList &node );

	void				Remove( void );

	type *				Next( void ) const;
	type *				Prev( void ) const;

	type *				Owner( void ) const;
	void				SetOwner( type *p_object );

	idLinkList *		ListHead( void ) const;
	idLinkList *		NextNode( void ) const;
	idLinkList *		PrevNode( void ) const;

private:
	idLinkList *		p_head;
	idLinkList *		p_next;
	idLinkList *		p_prev;
	type *				p_owner;
};

/*
================
idLinkList<type>::idLinkList

Node is initialized to be the head of an empty list
================
*/
template< class type >
idLinkList<type>::idLinkList() {
	p_owner	= NULL;
	p_head	= this;
	p_next	= this;
	p_prev	= this;
}

/*
================
idLinkList<type>::~idLinkList

Removes the node from the list, or if it's the head of a list, removes
all the nodes from the list.
================
*/
template< class type >
idLinkList<type>::~idLinkList() {
	Clear();
}

/*
================
idLinkList<type>::IsListEmpty

Returns true if the list is empty.
================
*/
template< class type >
bool idLinkList<type>::IsListEmpty( void ) const {
	return p_head->p_next == p_head;
}

/*
================
idLinkList<type>::InList

Returns true if the node is in a list.  If called on the head of a list, will always return false.
================
*/
template< class type >
bool idLinkList<type>::InList( void ) const {
	return p_head != this;
}

/*
================
idLinkList<type>::Num

Returns the number of nodes in the list.
================
*/
template< class type >
int idLinkList<type>::Num( void ) const {
	idLinkList<type>	*p_node;
	int					num;

	num = 0;
	for( p_node = p_head->p_next; p_node != p_head; p_node = p_node->p_next ) {
		num++;
	}

	return num;
}

/*
================
idLinkList<type>::Clear

If node is the head of the list, clears the list.  Otherwise it just removes the node from the list.
================
*/
template< class type >
void idLinkList<type>::Clear( void ) {
	if ( p_head == this ) {
		while( p_next != this ) {
			p_next->Remove();
		}
	} else {
		Remove();
	}
}

/*
================
idLinkList<type>::Remove

Removes node from list
================
*/
template< class type >
void idLinkList<type>::Remove( void ) {
	p_prev->p_next = p_next;
	p_next->p_prev = p_prev;

	p_next = this;
	p_prev = this;
	p_head = this;
}

/*
================
idLinkList<type>::InsertBefore

Places the node before the existing node in the list.  If the existing node is the head,
then the new node is placed at the end of the list.
================
*/
template< class type >
void idLinkList<type>::InsertBefore( idLinkList &node ) {
	Remove();

	p_next			= &node;
	p_prev			= node.p_prev;
	node.p_prev		= this;
	p_prev->p_next	= this;
	p_head			= node.p_head;
}

/*
================
idLinkList<type>::InsertAfter

Places the node after the existing node in the list.  If the existing node is the head,
then the new node is placed at the beginning of the list.
================
*/
template< class type >
void idLinkList<type>::InsertAfter( idLinkList &node ) {
	Remove();

	p_prev			= &node;
	p_next			= node.p_next;
	node.p_next		= this;
	p_next->p_prev	= this;
	p_head			= node.p_head;
}

/*
================
idLinkList<type>::AddToEnd

Adds node at the end of the list
================
*/
template< class type >
void idLinkList<type>::AddToEnd( idLinkList &node ) {
	InsertBefore( *node.p_head );
}

/*
================
idLinkList<type>::AddToFront

Adds node at the beginning of the list
================
*/
template< class type >
void idLinkList<type>::AddToFront( idLinkList &node ) {
	InsertAfter( *node.p_head );
}

/*
================
idLinkList<type>::ListHead

Returns the head of the list.  If the node isn't in a list, it returns
a pointer to itself.
================
*/
template< class type >
idLinkList<type> *idLinkList<type>::ListHead( void ) const {
	return p_head;
}

/*
================
idLinkList<type>::Next

Returns the next object in the list, or NULL if at the end.
================
*/
template< class type >
type *idLinkList<type>::Next( void ) const {
	if ( !p_next || ( p_next == p_head ) ) {
		return NULL;
	}
	return p_next->p_owner;
}

/*
================
idLinkList<type>::Prev

Returns the previous object in the list, or NULL if at the beginning.
================
*/
template< class type >
type *idLinkList<type>::Prev( void ) const {
	if ( !p_prev || ( p_prev == p_head ) ) {
		return NULL;
	}
	return p_prev->p_owner;
}

/*
================
idLinkList<type>::NextNode

Returns the next node in the list, or NULL if at the end.
================
*/
template< class type >
idLinkList<type> *idLinkList<type>::NextNode( void ) const {
	if ( p_next == p_head ) {
		return NULL;
	}
	return p_next;
}

/*
================
idLinkList<type>::PrevNode

Returns the previous node in the list, or NULL if at the beginning.
================
*/
template< class type >
idLinkList<type> *idLinkList<type>::PrevNode( void ) const {
	if ( p_prev == p_head ) {
		return NULL;
	}
	return p_prev;
}

/*
================
idLinkList<type>::Owner

Gets the object that is associated with this node.
================
*/
template< class type >
type *idLinkList<type>::Owner( void ) const {
	return p_owner;
}

/*
================
idLinkList<type>::SetOwner

Sets the object that this node is associated with.
================
*/
template< class type >
void idLinkList<type>::SetOwner( type *p_object ) {
	p_owner = p_object;
}

#endif /* !__LINKLIST_H__ */
