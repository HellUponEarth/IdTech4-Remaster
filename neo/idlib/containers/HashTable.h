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

#ifndef __HASHTABLE_H__
#define __HASHTABLE_H__

/*
===============================================================================

	General hash table. Slower than idHashIndex but it can also be used for
	linked lists and other data structures than just indexes or arrays.

===============================================================================
*/

template< class Type >
class idHashTable {
public:
					idHashTable( int newtablesize = 256 );
					idHashTable( const idHashTable<Type> &map );
					~idHashTable( void );

					// returns total size of allocated memory
	size_t			Allocated( void ) const;
					// returns total size of allocated memory including size of hash table type
	size_t			Size( void ) const;

	void			Set( const char *p_key, Type &value );
	bool			Get( const char *p_key, Type **p_value = NULL ) const;
	bool			Remove( const char *p_key );

	void			Clear( void );
	void			DeleteContents( void );

					// the entire contents can be itterated over, but note that the
					// exact index for a given element may change when new elements are added
	int				Num( void ) const;
	Type *			GetIndex( int index ) const;

	int				GetSpread( void ) const;

private:
	struct hashnode_s {
		idStr		key;
		Type		value;
		hashnode_s *p_next;

		hashnode_s( const idStr &k, Type v, hashnode_s *p_nextNode ) : key( k ), value( v ), p_next( p_nextNode ) {};
		hashnode_s( const char *p_k, Type v, hashnode_s *p_nextNode ) : key( p_k ), value( v ), p_next( p_nextNode ) {};
	};

	hashnode_s **	p_heads;

	int				tablesize;
	int				numentries;
	int				tablesizemask;

	int				GetHash( const char *p_key ) const;
};

/*
================
idHashTable<Type>::idHashTable
================
*/
template< class Type >
ID_INLINE idHashTable<Type>::idHashTable( int newtablesize ) {

	assert( idMath::IsPowerOfTwo( newtablesize ) );

	tablesize = newtablesize;
	assert( tablesize > 0 );

	p_heads = new hashnode_s *[ tablesize ];
	memset( p_heads, 0, sizeof( *p_heads ) * tablesize );

	numentries		= 0;

	tablesizemask = tablesize - 1;
}

/*
================
idHashTable<Type>::idHashTable
================
*/
template< class Type >
ID_INLINE idHashTable<Type>::idHashTable( const idHashTable<Type> &map ) {
	int			i;
	hashnode_s	*p_node;
	hashnode_s	**p_prev;

	assert( map.tablesize > 0 );

	tablesize		= map.tablesize;
	p_heads			= new hashnode_s *[ tablesize ];
	numentries		= map.numentries;
	tablesizemask	= map.tablesizemask;

	for( i = 0; i < tablesize; i++ ) {
		if ( !map.p_heads[ i ] ) {
			p_heads[ i ] = NULL;
			continue;
		}

		p_prev = &p_heads[ i ];
		for( p_node = map.p_heads[ i ]; p_node != NULL; p_node = p_node->p_next ) {
			*p_prev = new hashnode_s( p_node->key, p_node->value, NULL );
			p_prev = &( *p_prev )->p_next;
		}
	}
}

/*
================
idHashTable<Type>::~idHashTable<Type>
================
*/
template< class Type >
ID_INLINE idHashTable<Type>::~idHashTable( void ) {
	Clear();
	delete[] p_heads;
}

/*
================
idHashTable<Type>::Allocated
================
*/
template< class Type >
ID_INLINE size_t idHashTable<Type>::Allocated( void ) const {
	return sizeof( p_heads ) * tablesize + sizeof( *p_heads ) * numentries;
}

/*
================
idHashTable<Type>::Size
================
*/
template< class Type >
ID_INLINE size_t idHashTable<Type>::Size( void ) const {
	return sizeof( idHashTable<Type> ) + sizeof( p_heads ) * tablesize + sizeof( *p_heads ) * numentries;
}

/*
================
idHashTable<Type>::GetHash
================
*/
template< class Type >
ID_INLINE int idHashTable<Type>::GetHash( const char *p_key ) const {
	return ( idStr::Hash( p_key ) & tablesizemask );
}

/*
================
idHashTable<Type>::Set
================
*/
template< class Type >
ID_INLINE void idHashTable<Type>::Set( const char *p_key, Type &value ) {
	hashnode_s *p_node, **p_nextPtr;
	int hash, s;

	hash = GetHash( p_key );
	for( p_nextPtr = &(p_heads[hash]), p_node = *p_nextPtr; p_node != NULL; p_nextPtr = &(p_node->p_next), p_node = *p_nextPtr ) {
		s = p_node->key.Cmp( p_key );
		if ( s == 0 ) {
			p_node->value = value;
			return;
		}
		if ( s > 0 ) {
			break;
		}
	}

	numentries++;

	*p_nextPtr = new hashnode_s( p_key, value, p_heads[ hash ] );
	(*p_nextPtr)->p_next = p_node;
}

/*
================
idHashTable<Type>::Get
================
*/
template< class Type >
ID_INLINE bool idHashTable<Type>::Get( const char *p_key, Type **p_value ) const {
	hashnode_s *p_node;
	int hash, s;

	hash = GetHash( p_key );
	for( p_node = p_heads[ hash ]; p_node != NULL; p_node = p_node->p_next ) {
		s = p_node->key.Cmp( p_key );
		if ( s == 0 ) {
			if ( p_value ) {
				*p_value = &p_node->value;
			}
			return true;
		}
		if ( s > 0 ) {
			break;
		}
	}

	if ( p_value ) {
		*p_value = NULL;
	}

	return false;
}

/*
================
idHashTable<Type>::GetIndex

the entire contents can be itterated over, but note that the
exact index for a given element may change when new elements are added
================
*/
template< class Type >
ID_INLINE Type *idHashTable<Type>::GetIndex( int index ) const {
	hashnode_s	*p_node;
	int			count;
	int			i;

	if ( ( index < 0 ) || ( index > numentries ) ) {
		assert( 0 );
		return NULL;
	}

	count = 0;
	for( i = 0; i < tablesize; i++ ) {
		for( p_node = p_heads[ i ]; p_node != NULL; p_node = p_node->p_next ) {
			if ( count == index ) {
				return &p_node->value;
			}
			count++;
		}
	}

	return NULL;
}

/*
================
idHashTable<Type>::Remove
================
*/
template< class Type >
ID_INLINE bool idHashTable<Type>::Remove( const char *p_key ) {
	hashnode_s	**p_head;
	hashnode_s	*p_node;
	hashnode_s	*p_prev;
	int			hash;

	hash = GetHash( p_key );
	p_head = &p_heads[ hash ];
	if ( *p_head ) {
		for( p_prev = NULL, p_node = *p_head; p_node != NULL; p_prev = p_node, p_node = p_node->p_next ) {
			if ( p_node->key == p_key ) {
				if ( p_prev ) {
					p_prev->p_next = p_node->p_next;
				} else {
					*p_head = p_node->p_next;
				}

				delete p_node;
				numentries--;
				return true;
			}
		}
	}

	return false;
}

/*
================
idHashTable<Type>::Clear
================
*/
template< class Type >
ID_INLINE void idHashTable<Type>::Clear( void ) {
	int			i;
	hashnode_s	*p_node;
	hashnode_s	*p_next;

	for( i = 0; i < tablesize; i++ ) {
		p_next = p_heads[ i ];
		while( p_next != NULL ) {
			p_node = p_next;
			p_next = p_next->p_next;
			delete p_node;
		}

		p_heads[ i ] = NULL;
	}

	numentries = 0;
}

/*
================
idHashTable<Type>::DeleteContents
================
*/
template< class Type >
ID_INLINE void idHashTable<Type>::DeleteContents( void ) {
	int			i;
	hashnode_s	*p_node;
	hashnode_s	*p_next;

	for( i = 0; i < tablesize; i++ ) {
		p_next = p_heads[ i ];
		while( p_next != NULL ) {
			p_node = p_next;
			p_next = p_next->p_next;
			delete p_node->value;
			delete p_node;
		}

		p_heads[ i ] = NULL;
	}

	numentries = 0;
}

/*
================
idHashTable<Type>::Num
================
*/
template< class Type >
ID_INLINE int idHashTable<Type>::Num( void ) const {
	return numentries;
}

#if defined(ID_TYPEINFO)
#define __GNUC__ 99
#endif

#if !defined(__GNUC__) || __GNUC__ < 4
/*
================
idHashTable<Type>::GetSpread
================
*/
template< class Type >
int idHashTable<Type>::GetSpread( void ) const {
	int i, average, error, e;
	hashnode_s	*p_node;

	// if no items in hash
	if ( !numentries ) {
		return 100;
	}
	average = numentries / tablesize;
	error = 0;
	for ( i = 0; i < tablesize; i++ ) {
		numItems = 0;
		for( p_node = p_heads[ i ]; p_node != NULL; p_node = p_node->p_next ) {
			numItems++;
		}
		e = abs( numItems - average );
		if ( e > 1 ) {
			error += e - 1;
		}
	}
	return 100 - (error * 100 / numentries);
}
#endif

#if defined(ID_TYPEINFO)
#undef __GNUC__
#endif

#endif /* !__HASHTABLE_H__ */
