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

#ifndef __BTREE_H__
#define __BTREE_H__

/*
===============================================================================

	Balanced Search Tree

===============================================================================
*/

//#define BTREE_CHECK

template< class objType, class keyType >
class idBTreeNode {
public:
	keyType							key;			// key used for sorting
	objType *						p_object;			// if != NULL pointer to object stored in leaf node
	idBTreeNode *					p_parent;			// parent node
	idBTreeNode *					p_next;				// next sibling
	idBTreeNode *					p_prev;				// previous sibling
	int								numChildren;	// number of children
	idBTreeNode *					p_firstChild;		// first child
	idBTreeNode *					p_lastChild;		// last child
};


template< class objType, class keyType, int maxChildrenPerNode >
class idBTree {
public:
									idBTree( void );
									~idBTree( void );

	void							Init( void );
	void							Shutdown( void );

	idBTreeNode<objType,keyType> *	Add( objType *p_object, keyType key );						// add an object to the tree
	void							Remove( idBTreeNode<objType,keyType> *p_node );				// remove an object node from the tree

	objType *						Find( keyType key ) const;									// find an object using the given key
	objType *						FindSmallestLargerEqual( keyType key ) const;				// find an object with the smallest key larger equal the given key
	objType *						FindLargestSmallerEqual( keyType key ) const;				// find an object with the largest key smaller equal the given key

	idBTreeNode<objType,keyType> *	GetRoot( void ) const;										// returns the root node of the tree
	int								GetNodeCount( void ) const;									// returns the total number of nodes in the tree
	idBTreeNode<objType,keyType> *	GetNext( idBTreeNode<objType,keyType> *p_node ) const;		// goes through all nodes of the tree
	idBTreeNode<objType,keyType> *	GetNextLeaf( idBTreeNode<objType,keyType> *p_node ) const;	// goes through all leaf nodes of the tree

private:
	idBTreeNode<objType,keyType> *	p_root;
	idBlockAlloc<idBTreeNode<objType,keyType>,128>	nodeAllocator;

	idBTreeNode<objType,keyType> *	AllocNode( void );
	void							FreeNode( idBTreeNode<objType,keyType> *p_node );
	void							SplitNode( idBTreeNode<objType,keyType> *p_node );
	idBTreeNode<objType,keyType> *	MergeNodes( idBTreeNode<objType,keyType> *p_node1, idBTreeNode<objType,keyType> *p_node2 );

	void							CheckTree_r( idBTreeNode<objType,keyType> *p_node, int &numNodes ) const;
	void							CheckTree( void ) const;
};


template< class objType, class keyType, int maxChildrenPerNode >
ID_INLINE idBTree<objType,keyType,maxChildrenPerNode>::idBTree( void ) {
	assert( maxChildrenPerNode >= 4 );
	p_root = NULL;
}

template< class objType, class keyType, int maxChildrenPerNode >
ID_INLINE idBTree<objType,keyType,maxChildrenPerNode>::~idBTree( void ) {
	Shutdown();
}

template< class objType, class keyType, int maxChildrenPerNode >
ID_INLINE void idBTree<objType,keyType,maxChildrenPerNode>::Init( void ) {
	p_root = AllocNode();
}

template< class objType, class keyType, int maxChildrenPerNode >
ID_INLINE void idBTree<objType,keyType,maxChildrenPerNode>::Shutdown( void ) {
	nodeAllocator.Shutdown();
	p_root = NULL;
}

template< class objType, class keyType, int maxChildrenPerNode >
ID_INLINE idBTreeNode<objType,keyType> *idBTree<objType,keyType,maxChildrenPerNode>::Add( objType *p_object, keyType key ) {
	idBTreeNode<objType,keyType> *p_node, *p_child, *p_newNode;

	if ( p_root->numChildren >= maxChildrenPerNode ) {
		p_newNode = AllocNode();
		p_newNode->key = p_root->key;
		p_newNode->p_firstChild = p_root;
		p_newNode->p_lastChild = p_root;
		p_newNode->numChildren = 1;
		p_root->p_parent = p_newNode;
		SplitNode( p_root );
		p_root = p_newNode;
	}

	p_newNode = AllocNode();
	p_newNode->key = key;
	p_newNode->p_object = p_object;

	for ( p_node = p_root; p_node->p_firstChild != NULL; p_node = p_child ) {

		if ( key > p_node->key ) {
			p_node->key = key;
		}

		// find the first child with a key larger equal to the key of the new node
		for( p_child = p_node->p_firstChild; p_child->p_next; p_child = p_child->p_next ) {
			if ( key <= p_child->key ) {
				break;
			}
		}

		if ( p_child->p_object ) {

			if ( key <= p_child->key ) {
				// insert new node before child
				if ( p_child->p_prev ) {
					p_child->p_prev->p_next = p_newNode;
				} else {
					p_node->p_firstChild = p_newNode;
				}
				p_newNode->p_prev = p_child->p_prev;
				p_newNode->p_next = p_child;
				p_child->p_prev = p_newNode;
			} else {
				// insert new node after child
				if ( p_child->p_next ) {
					p_child->p_next->p_prev = p_newNode;
				} else {
					p_node->p_lastChild = p_newNode;
				}
				p_newNode->p_prev = p_child;
				p_newNode->p_next = p_child->p_next;
				p_child->p_next = p_newNode;
			}

			p_newNode->p_parent = p_node;
			p_node->numChildren++;

#ifdef BTREE_CHECK
			CheckTree();
#endif

			return p_newNode;
		}

		// make sure the child has room to store another node
		if ( p_child->numChildren >= maxChildrenPerNode ) {
			SplitNode( p_child );
			if ( key <= p_child->p_prev->key ) {
				p_child = p_child->p_prev;
			}
		}
	}

	// we only end up here if the root node is empty
	p_newNode->p_parent = p_root;
	p_root->key = key;
	p_root->p_firstChild = p_newNode;
	p_root->p_lastChild = p_newNode;
	p_root->numChildren++;

#ifdef BTREE_CHECK
	CheckTree();
#endif

	return p_newNode;
}

template< class objType, class keyType, int maxChildrenPerNode >
ID_INLINE void idBTree<objType,keyType,maxChildrenPerNode>::Remove( idBTreeNode<objType,keyType> *p_node ) {
	idBTreeNode<objType,keyType> *p_parent;

	assert( p_node->p_object != NULL );

	// unlink the node from its parent
	if ( p_node->p_prev ) {
		p_node->p_prev->p_next = p_node->p_next;
	} else {
		p_node->p_parent->p_firstChild = p_node->p_next;
	}
	if ( p_node->p_next ) {
		p_node->p_next->p_prev = p_node->p_prev;
	} else {
		p_node->p_parent->p_lastChild = p_node->p_prev;
	}
	p_node->p_parent->numChildren--;

	// make sure there are no parent nodes with a single child
	for ( p_parent = p_node->p_parent; p_parent != p_root && p_parent->numChildren <= 1; p_parent = p_parent->p_parent ) {

		if ( p_parent->p_next ) {
			p_parent = MergeNodes( p_parent, p_parent->p_next );
		} else if ( p_parent->p_prev ) {
			p_parent = MergeNodes( p_parent->p_prev, p_parent );
		}

		// a parent may not use a key higher than the key of its last child
		if ( p_parent->key > p_parent->p_lastChild->key ) {
			p_parent->key = p_parent->p_lastChild->key;
		}

		if ( p_parent->numChildren > maxChildrenPerNode ) {
			SplitNode( p_parent );
			break;
		}
	}
	for ( ; p_parent != NULL && p_parent->p_lastChild != NULL; p_parent = p_parent->p_parent ) {
		// a parent may not use a key higher than the key of its last child
		if ( p_parent->key > p_parent->p_lastChild->key ) {
			p_parent->key = p_parent->p_lastChild->key;
		}
	}

	// free the node
	FreeNode( p_node );

	// remove the root node if it has a single internal node as child
	if ( p_root->numChildren == 1 && p_root->p_firstChild->p_object == NULL ) {
		idBTreeNode<objType,keyType> *p_oldRoot = p_root;
		p_root->p_firstChild->p_parent = NULL;
		p_root = p_root->p_firstChild;
		FreeNode( p_oldRoot );
	}

#ifdef BTREE_CHECK
	CheckTree();
#endif
}

template< class objType, class keyType, int maxChildrenPerNode >
ID_INLINE objType *idBTree<objType,keyType,maxChildrenPerNode>::Find( keyType key ) const {
	idBTreeNode<objType,keyType> *p_node;

	for ( p_node = p_root->p_firstChild; p_node != NULL; p_node = p_node->p_firstChild ) {
		while( p_node->p_next ) {
			if ( p_node->key >= key ) {
				break;
			}
			p_node = p_node->p_next;
		}
		if ( p_node->p_object ) {
			if ( p_node->key == key ) {
				return p_node->p_object;
			} else {
				return NULL;
			}
		}
	}
	return NULL;
}

template< class objType, class keyType, int maxChildrenPerNode >
ID_INLINE objType *idBTree<objType,keyType,maxChildrenPerNode>::FindSmallestLargerEqual( keyType key ) const {
	idBTreeNode<objType,keyType> *p_node;

	for ( p_node = p_root->p_firstChild; p_node != NULL; p_node = p_node->p_firstChild ) {
		while( p_node->p_next ) {
			if ( p_node->key >= key ) {
				break;
			}
			p_node = p_node->p_next;
		}
		if ( p_node->p_object ) {
			if ( p_node->key >= key ) {
				return p_node->p_object;
			} else {
				return NULL;
			}
		}
	}
	return NULL;
}

template< class objType, class keyType, int maxChildrenPerNode >
ID_INLINE objType *idBTree<objType,keyType,maxChildrenPerNode>::FindLargestSmallerEqual( keyType key ) const {
	idBTreeNode<objType,keyType> *p_node;

	for ( p_node = p_root->p_lastChild; p_node != NULL; p_node = p_node->p_lastChild ) {
		while( p_node->p_prev ) {
			if ( p_node->key <= key ) {
				break;
			}
			p_node = p_node->p_prev;
		}
		if ( p_node->p_object ) {
			if ( p_node->key <= key ) {
				return p_node->p_object;
			} else {
				return NULL;
			}
		}
	}
	return NULL;
}

template< class objType, class keyType, int maxChildrenPerNode >
ID_INLINE idBTreeNode<objType,keyType> *idBTree<objType,keyType,maxChildrenPerNode>::GetRoot( void ) const {
	return p_root;
}

template< class objType, class keyType, int maxChildrenPerNode >
ID_INLINE int idBTree<objType,keyType,maxChildrenPerNode>::GetNodeCount( void ) const {
	return nodeAllocator.GetAllocCount();
}

template< class objType, class keyType, int maxChildrenPerNode >
ID_INLINE idBTreeNode<objType,keyType> *idBTree<objType,keyType,maxChildrenPerNode>::GetNext( idBTreeNode<objType,keyType> *p_node ) const {
	if ( p_node->p_firstChild ) {
		return p_node->p_firstChild;
	} else {
		while( p_node && p_node->p_next == NULL ) {
			p_node = p_node->p_parent;
		}
		return p_node;
	}
}

template< class objType, class keyType, int maxChildrenPerNode >
ID_INLINE idBTreeNode<objType,keyType> *idBTree<objType,keyType,maxChildrenPerNode>::GetNextLeaf( idBTreeNode<objType,keyType> *p_node ) const {
	if ( p_node->p_firstChild ) {
		while ( p_node->p_firstChild ) {
			p_node = p_node->p_firstChild;
		}
		return p_node;
	} else {
		while( p_node && p_node->p_next == NULL ) {
			p_node = p_node->p_parent;
		}
		if ( p_node ) {
			p_node = p_node->p_next;
			while ( p_node->p_firstChild ) {
				p_node = p_node->p_firstChild;
			}
			return p_node;
		} else {
			return NULL;
		}
	}
}

template< class objType, class keyType, int maxChildrenPerNode >
ID_INLINE idBTreeNode<objType,keyType> *idBTree<objType,keyType,maxChildrenPerNode>::AllocNode( void ) {
	idBTreeNode<objType,keyType> *p_node = nodeAllocator.Alloc();
	p_node->key = 0;
	p_node->p_parent = NULL;
	p_node->p_next = NULL;
	p_node->p_prev = NULL;
	p_node->numChildren = 0;
	p_node->p_firstChild = NULL;
	p_node->p_lastChild = NULL;
	p_node->p_object = NULL;
	return p_node;
}

template< class objType, class keyType, int maxChildrenPerNode >
ID_INLINE void idBTree<objType,keyType,maxChildrenPerNode>::FreeNode( idBTreeNode<objType,keyType> *p_node ) {
	nodeAllocator.Free( p_node );
}

template< class objType, class keyType, int maxChildrenPerNode >
ID_INLINE void idBTree<objType,keyType,maxChildrenPerNode>::SplitNode( idBTreeNode<objType,keyType> *p_node ) {
	int i;
	idBTreeNode<objType,keyType> *p_child, *p_newNode;

	// allocate a new node
	p_newNode = AllocNode();
	p_newNode->p_parent = p_node->p_parent;

	// divide the children over the two nodes
	p_child = p_node->p_firstChild;
	p_child->p_parent = p_newNode;
	for ( i = 3; i < p_node->numChildren; i += 2 ) {
		p_child = p_child->p_next;
		p_child->p_parent = p_newNode;
	}

	p_newNode->key = p_child->key;
	p_newNode->numChildren = p_node->numChildren / 2;
	p_newNode->p_firstChild = p_node->p_firstChild;
	p_newNode->p_lastChild = p_child;

	p_node->numChildren -= p_newNode->numChildren;
	p_node->p_firstChild = p_child->p_next;

	p_child->p_next->p_prev = NULL;
	p_child->p_next = NULL;

	// add the new child to the parent before the split node
	assert( p_node->p_parent->numChildren < maxChildrenPerNode );

	if ( p_node->p_prev ) {
		p_node->p_prev->p_next = p_newNode;
	} else {
		p_node->p_parent->p_firstChild = p_newNode;
	}
	p_newNode->p_prev = p_node->p_prev;
	p_newNode->p_next = p_node;
	p_node->p_prev = p_newNode;

	p_node->p_parent->numChildren++;
}

template< class objType, class keyType, int maxChildrenPerNode >
ID_INLINE idBTreeNode<objType,keyType> *idBTree<objType,keyType,maxChildrenPerNode>::MergeNodes( idBTreeNode<objType,keyType> *p_node1, idBTreeNode<objType,keyType> *p_node2 ) {
	idBTreeNode<objType,keyType> *p_child;

	assert( p_node1->p_parent == p_node2->p_parent );
	assert( p_node1->p_next == p_node2 && p_node2->p_prev == p_node1 );
	assert( p_node1->p_object == NULL && p_node2->p_object == NULL );
	assert( p_node1->numChildren >= 1 && p_node2->numChildren >= 1 );

	for ( p_child = p_node1->p_firstChild; p_child->p_next; p_child = p_child->p_next ) {
		p_child->p_parent = p_node2;
	}
	p_child->p_parent = p_node2;
	p_child->p_next = p_node2->p_firstChild;
	p_node2->p_firstChild->p_prev = p_child;
	p_node2->p_firstChild = p_node1->p_firstChild;
	p_node2->numChildren += p_node1->numChildren;

	// unlink the first node from the parent
	if ( p_node1->p_prev ) {
		p_node1->p_prev->p_next = p_node2;
	} else {
		p_node1->p_parent->p_firstChild = p_node2;
	}
	p_node2->p_prev = p_node1->p_prev;
	p_node2->p_parent->numChildren--;

	FreeNode( p_node1 );

	return p_node2;
}

template< class objType, class keyType, int maxChildrenPerNode >
ID_INLINE void idBTree<objType,keyType,maxChildrenPerNode>::CheckTree_r( idBTreeNode<objType,keyType> *p_node, int &numNodes ) const {
	int numChildren;
	idBTreeNode<objType,keyType> *p_child;

	numNodes++;

	// the root node may have zero children and leaf nodes always have zero children, all other nodes should have at least 2 and at most maxChildrenPerNode children
	assert( ( p_node == p_root ) || ( p_node->p_object != NULL && p_node->numChildren == 0 ) || ( p_node->numChildren >= 2 && p_node->numChildren <= maxChildrenPerNode ) );
	// the key of a node may never be larger than the key of its last child
	assert( ( p_node->p_lastChild == NULL ) || ( p_node->key <= p_node->p_lastChild->key ) );

	numChildren = 0;
	for ( p_child = p_node->p_firstChild; p_child; p_child = p_child->p_next ) {
		numChildren++;
		// make sure the children are properly linked
		if ( p_child->p_prev == NULL ) {
			assert( p_node->p_firstChild == p_child );
		} else {
			assert( p_child->p_prev->p_next == p_child );
		}
		if ( p_child->p_next == NULL ) {
			assert( p_node->p_lastChild == p_child );
		} else {
			assert( p_child->p_next->p_prev == p_child );
		}
		// recurse down the tree
		CheckTree_r( p_child, numNodes );
	}
	// the number of children should equal the number of linked children
	assert( numChildren == p_node->numChildren );
}

template< class objType, class keyType, int maxChildrenPerNode >
ID_INLINE void idBTree<objType,keyType,maxChildrenPerNode>::CheckTree( void ) const {
	int numNodes = 0;
	idBTreeNode<objType,keyType> *p_node, *p_lastNode;

	CheckTree_r( p_root, numNodes );

	// the number of nodes in the tree should equal the number of allocated nodes
	assert( numNodes == nodeAllocator.GetAllocCount() );

	// all the leaf nodes should be ordered
	p_lastNode = GetNextLeaf( GetRoot() );
	if ( p_lastNode ) {
		for ( p_node = GetNextLeaf( p_lastNode ); p_node; p_lastNode = p_node, p_node = GetNextLeaf( p_node ) ) {
			assert( p_lastNode->key <= p_node->key );
		}
	}
}

#endif /* !__BTREE_H__ */
