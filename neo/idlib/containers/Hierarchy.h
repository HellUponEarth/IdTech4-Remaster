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

#ifndef __HIERARCHY_H__
#define __HIERARCHY_H__

/*
==============================================================================

	idHierarchy

==============================================================================
*/

template< class type >
class idHierarchy {
public:

						idHierarchy();
						~idHierarchy();
	
	void				SetOwner( type *p_object );
	type *				Owner( void ) const;
	void				ParentTo( idHierarchy &node );
	void				MakeSiblingAfter( idHierarchy &node );
	bool				ParentedBy( const idHierarchy &node ) const;
	void				RemoveFromParent( void );
	void				RemoveFromHierarchy( void );

	type *				GetParent( void ) const;		// parent of this node
	type *				GetChild( void ) const;			// first child of this node
	type *				GetSibling( void ) const;		// next node with the same parent
	type *				GetPriorSibling( void ) const;	// previous node with the same parent
	type *				GetNext( void ) const;			// goes through all nodes of the hierarchy
	type *				GetNextLeaf( void ) const;		// goes through all leaf nodes of the hierarchy

private:
	idHierarchy *		p_parent;
	idHierarchy *		p_sibling;
	idHierarchy *		p_child;
	type *				p_owner;

	idHierarchy<type>	*GetPriorSiblingNode( void ) const;	// previous node with the same parent
};

/*
================
idHierarchy<type>::idHierarchy
================
*/
template< class type >
idHierarchy<type>::idHierarchy() {
	p_owner		= NULL;
	p_parent	= NULL;
	p_sibling	= NULL;
	p_child		= NULL;
}

/*
================
idHierarchy<type>::~idHierarchy
================
*/
template< class type >
idHierarchy<type>::~idHierarchy() {
	RemoveFromHierarchy();
}

/*
================
idHierarchy<type>::Owner

Gets the object that is associated with this node.
================
*/
template< class type >
type *idHierarchy<type>::Owner( void ) const {
	return p_owner;
}

/*
================
idHierarchy<type>::SetOwner

Sets the object that this node is associated with.
================
*/
template< class type >
void idHierarchy<type>::SetOwner( type *p_object ) {
	p_owner = p_object;
}

/*
================
idHierarchy<type>::ParentedBy
================
*/
template< class type >
bool idHierarchy<type>::ParentedBy( const idHierarchy &node ) const {
	if ( p_parent == &node ) {
		return true;
	} else if ( p_parent ) {
		return p_parent->ParentedBy( node );
	}
	return false;
}

/*
================
idHierarchy<type>::ParentTo

Makes the given node the parent.
================
*/
template< class type >
void idHierarchy<type>::ParentTo( idHierarchy &node ) {
	RemoveFromParent();

	p_parent		= &node;
	p_sibling		= node.p_child;
	node.p_child	= this;
}

/*
================
idHierarchy<type>::MakeSiblingAfter

Makes the given node a sibling after the passed in node.
================
*/
template< class type >
void idHierarchy<type>::MakeSiblingAfter( idHierarchy &node ) {
	RemoveFromParent();
	p_parent		= node.p_parent;
	p_sibling		= node.p_sibling;
	node.p_sibling	= this;
}

/*
================
idHierarchy<type>::RemoveFromParent
================
*/
template< class type >
void idHierarchy<type>::RemoveFromParent( void ) {
	idHierarchy<type> *p_prev;

	if ( p_parent ) {
		p_prev = GetPriorSiblingNode();
		if ( p_prev ) {
			p_prev->p_sibling = p_sibling;
		} else {
			p_parent->p_child = p_sibling;
		}
	}

	p_parent = NULL;
	p_sibling = NULL;
}

/*
================
idHierarchy<type>::RemoveFromHierarchy

Removes the node from the hierarchy and adds it's children to the parent.
================
*/
template< class type >
void idHierarchy<type>::RemoveFromHierarchy( void ) {
	idHierarchy<type> *p_parentNode;
	idHierarchy<type> *p_node;

	p_parentNode = p_parent;
	RemoveFromParent();

	if ( p_parentNode ) {
		while( p_child ) {
			p_node = p_child;
			p_node->RemoveFromParent();
			p_node->ParentTo( *p_parentNode );
		}
	} else {
		while( p_child ) {
			p_child->RemoveFromParent();
		}
	}
}

/*
================
idHierarchy<type>::GetParent
================
*/
template< class type >
type *idHierarchy<type>::GetParent( void ) const {
	if ( p_parent ) {
		return p_parent->p_owner;
	}
	return NULL;
}

/*
================
idHierarchy<type>::GetChild
================
*/
template< class type >
type *idHierarchy<type>::GetChild( void ) const {
	if ( p_child ) {
		return p_child->p_owner;
	}
	return NULL;
}

/*
================
idHierarchy<type>::GetSibling
================
*/
template< class type >
type *idHierarchy<type>::GetSibling( void ) const {
	if ( p_sibling ) {
		return p_sibling->p_owner;
	}
	return NULL;
}

/*
================
idHierarchy<type>::GetPriorSiblingNode

Returns NULL if no parent, or if it is the first child.
================
*/
template< class type >
idHierarchy<type> *idHierarchy<type>::GetPriorSiblingNode( void ) const {
	if ( !p_parent || ( p_parent->p_child == this ) ) {
		return NULL;
	}

	idHierarchy<type> *p_prev;
	idHierarchy<type> *p_node;

	p_node = p_parent->p_child;
	p_prev = NULL;
	while( ( p_node != this ) && ( p_node != NULL ) ) {
		p_prev = p_node;
		p_node = p_node->p_sibling;
	}

	if ( p_node != this ) {
		idLib::Error( "idHierarchy::GetPriorSibling: could not find node in parent's list of children" );
	}

	return p_prev;
}

/*
================
idHierarchy<type>::GetPriorSibling

Returns NULL if no parent, or if it is the first child.
================
*/
template< class type >
type *idHierarchy<type>::GetPriorSibling( void ) const {
	idHierarchy<type> *p_prior;

	p_prior = GetPriorSiblingNode();
	if ( p_prior ) {
		return p_prior->p_owner;
	}

	return NULL;
}

/*
================
idHierarchy<type>::GetNext

Goes through all nodes of the hierarchy.
================
*/
template< class type >
type *idHierarchy<type>::GetNext( void ) const {
	const idHierarchy<type> *p_node;

	if ( p_child ) {
		return p_child->p_owner;
	} else {
		p_node = this;
		while( p_node && p_node->p_sibling == NULL ) {
			p_node = p_node->p_parent;
		}
		if ( p_node ) {
			return p_node->p_sibling->p_owner;
		} else {
			return NULL;
		}
	}
}

/*
================
idHierarchy<type>::GetNextLeaf

Goes through all leaf nodes of the hierarchy.
================
*/
template< class type >
type *idHierarchy<type>::GetNextLeaf( void ) const {
	const idHierarchy<type> *p_node;

	if ( p_child ) {
		p_node = p_child;
		while ( p_node->p_child ) {
			p_node = p_node->p_child;
		}
		return p_node->p_owner;
	} else {
		p_node = this;
		while( p_node && p_node->p_sibling == NULL ) {
			p_node = p_node->p_parent;
		}
		if ( p_node ) {
			p_node = p_node->p_sibling;
			while ( p_node->p_child ) {
				p_node = p_node->p_child;
			}
			return p_node->p_owner;
		} else {
			return NULL;
		}
	}
}

#endif /* !__HIERARCHY_H__ */
