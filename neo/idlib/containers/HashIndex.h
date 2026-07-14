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

#ifndef __HASHINDEX_H__
#define __HASHINDEX_H__

/*
===============================================================================

	Fast hash table for indexes and arrays.
	Does not allocate memory until the first key/index pair is added.

===============================================================================
*/

#define DEFAULT_HASH_SIZE			1024
#define DEFAULT_HASH_GRANULARITY	1024

class idHashIndex {
public:
					idHashIndex( void );
					idHashIndex( const int initialHashSize, const int initialIndexSize );
					~idHashIndex( void );

					// returns total size of allocated memory
	size_t			Allocated( void ) const;
					// returns total size of allocated memory including size of hash index type
	size_t			Size( void ) const;

	idHashIndex &	operator=( const idHashIndex &other );
					// add an index to the hash, assumes the index has not yet been added to the hash
	void			Add( const int key, const int index );
					// remove an index from the hash
	void			Remove( const int key, const int index );
					// get the first index from the hash, returns -1 if empty hash entry
	int				First( const int key ) const;
					// get the next index from the hash, returns -1 if at the end of the hash chain
	int				Next( const int index ) const;
					// insert an entry into the index and add it to the hash, increasing all indexes >= index
	void			InsertIndex( const int key, const int index );
					// remove an entry from the index and remove it from the hash, decreasing all indexes >= index
	void			RemoveIndex( const int key, const int index );
					// clear the hash
	void			Clear( void );
					// clear and resize
	void			Clear( const int newHashSize, const int newIndexSize );
					// free allocated memory
	void			Free( void );
					// get size of hash table
	int				GetHashSize( void ) const;
					// get size of the index
	int				GetIndexSize( void ) const;
					// set granularity
	void			SetGranularity( const int newGranularity );
					// force resizing the index, current hash table stays intact
	void			ResizeIndex( const int newIndexSize );
					// returns number in the range [0-100] representing the spread over the hash table
	int				GetSpread( void ) const;
					// returns a key for a string
	int				GenerateKey( const char *p_string, bool caseSensitive = true ) const;
					// returns a key for a vector
	int				GenerateKey( const idVec3 &v ) const;
					// returns a key for two integers
	int				GenerateKey( const int n1, const int n2 ) const;

private:
	int				hashSize;
	int *			p_hash;
	int				indexSize;
	int *			p_indexChain;
	int				granularity;
	int				hashMask;
	int				lookupMask;

	static int		INVALID_INDEX[1];

	void			Init( const int initialHashSize, const int initialIndexSize );
	void			Allocate( const int newHashSize, const int newIndexSize );
};

/*
================
idHashIndex::idHashIndex
================
*/
ID_INLINE idHashIndex::idHashIndex( void ) {
	Init( DEFAULT_HASH_SIZE, DEFAULT_HASH_SIZE );
}

/*
================
idHashIndex::idHashIndex
================
*/
ID_INLINE idHashIndex::idHashIndex( const int initialHashSize, const int initialIndexSize ) {
	Init( initialHashSize, initialIndexSize );
}

/*
================
idHashIndex::~idHashIndex
================
*/
ID_INLINE idHashIndex::~idHashIndex( void ) {
	Free();
}

/*
================
idHashIndex::Allocated
================
*/
ID_INLINE size_t idHashIndex::Allocated( void ) const {
	return hashSize * sizeof( int ) + indexSize * sizeof( int );
}

/*
================
idHashIndex::Size
================
*/
ID_INLINE size_t idHashIndex::Size( void ) const {
	return sizeof( *this ) + Allocated();
}

/*
================
idHashIndex::operator=
================
*/
ID_INLINE idHashIndex &idHashIndex::operator=( const idHashIndex &other ) {
	granularity = other.granularity;
	hashMask = other.hashMask;
	lookupMask = other.lookupMask;

	if ( other.lookupMask == 0 ) {
		hashSize = other.hashSize;
		indexSize = other.indexSize;
		Free();
	}
	else {
		if ( other.hashSize != hashSize || p_hash == INVALID_INDEX ) {
			if ( p_hash != INVALID_INDEX ) {
				delete[] p_hash;
			}
			hashSize = other.hashSize;
			p_hash = new int[hashSize];
		}
		if ( other.indexSize != indexSize || p_indexChain == INVALID_INDEX ) {
			if ( p_indexChain != INVALID_INDEX ) {
				delete[] p_indexChain;
			}
			indexSize = other.indexSize;
			p_indexChain = new int[indexSize];
		}
		memcpy( p_hash, other.p_hash, hashSize * sizeof( p_hash[0] ) );
		memcpy( p_indexChain, other.p_indexChain, indexSize * sizeof( p_indexChain[0] ) );
	}

	return *this;
}

/*
================
idHashIndex::Add
================
*/
ID_INLINE void idHashIndex::Add( const int key, const int index ) {
	int h;

	assert( index >= 0 );
	if ( p_hash == INVALID_INDEX ) {
		Allocate( hashSize, index >= indexSize ? index + 1 : indexSize );
	}
	else if ( index >= indexSize ) {
		ResizeIndex( index + 1 );
	}
	h = key & hashMask;
	p_indexChain[index] = p_hash[h];
	p_hash[h] = index;
}

/*
================
idHashIndex::Remove
================
*/
ID_INLINE void idHashIndex::Remove( const int key, const int index ) {
	int k = key & hashMask;

	if ( p_hash == INVALID_INDEX ) {
		return;
	}
	if ( p_hash[k] == index ) {
		p_hash[k] = p_indexChain[index];
	}
	else {
		for ( int i = p_hash[k]; i != -1; i = p_indexChain[i] ) {
			if ( p_indexChain[i] == index ) {
				p_indexChain[i] = p_indexChain[index];
				break;
			}
		}
	}
	p_indexChain[index] = -1;
}

/*
================
idHashIndex::First
================
*/
ID_INLINE int idHashIndex::First( const int key ) const {
	return p_hash[key & hashMask & lookupMask];
}

/*
================
idHashIndex::Next
================
*/
ID_INLINE int idHashIndex::Next( const int index ) const {
	assert( index >= 0 && index < indexSize );
	return p_indexChain[index & lookupMask];
}

/*
================
idHashIndex::InsertIndex
================
*/
ID_INLINE void idHashIndex::InsertIndex( const int key, const int index ) {
	int i, max;

	if ( p_hash != INVALID_INDEX ) {
		max = index;
		for ( i = 0; i < hashSize; i++ ) {
			if ( p_hash[i] >= index ) {
				p_hash[i]++;
				if ( p_hash[i] > max ) {
					max = p_hash[i];
				}
			}
		}
		for ( i = 0; i < indexSize; i++ ) {
			if ( p_indexChain[i] >= index ) {
				p_indexChain[i]++;
				if ( p_indexChain[i] > max ) {
					max = p_indexChain[i];
				}
			}
		}
		if ( max >= indexSize ) {
			ResizeIndex( max + 1 );
		}
		for ( i = max; i > index; i-- ) {
			p_indexChain[i] = p_indexChain[i-1];
		}
		p_indexChain[index] = -1;
	}
	Add( key, index );
}

/*
================
idHashIndex::RemoveIndex
================
*/
ID_INLINE void idHashIndex::RemoveIndex( const int key, const int index ) {
	int i, max;

	Remove( key, index );
	if ( p_hash != INVALID_INDEX ) {
		max = index;
		for ( i = 0; i < hashSize; i++ ) {
			if ( p_hash[i] >= index ) {
				if ( p_hash[i] > max ) {
					max = p_hash[i];
				}
				p_hash[i]--;
			}
		}
		for ( i = 0; i < indexSize; i++ ) {
			if ( p_indexChain[i] >= index ) {
				if ( p_indexChain[i] > max ) {
					max = p_indexChain[i];
				}
				p_indexChain[i]--;
			}
		}
		for ( i = index; i < max; i++ ) {
			p_indexChain[i] = p_indexChain[i+1];
		}
		p_indexChain[max] = -1;
	}
}

/*
================
idHashIndex::Clear
================
*/
ID_INLINE void idHashIndex::Clear( void ) {
	// only clear the hash table because clearing the indexChain is not really needed
	if ( p_hash != INVALID_INDEX ) {
		memset( p_hash, 0xff, hashSize * sizeof( p_hash[0] ) );
	}
}

/*
================
idHashIndex::Clear
================
*/
ID_INLINE void idHashIndex::Clear( const int newHashSize, const int newIndexSize ) {
	Free();
	hashSize = newHashSize;
	indexSize = newIndexSize;
}

/*
================
idHashIndex::GetHashSize
================
*/
ID_INLINE int idHashIndex::GetHashSize( void ) const {
	return hashSize;
}

/*
================
idHashIndex::GetIndexSize
================
*/
ID_INLINE int idHashIndex::GetIndexSize( void ) const {
	return indexSize;
}

/*
================
idHashIndex::SetGranularity
================
*/
ID_INLINE void idHashIndex::SetGranularity( const int newGranularity ) {
	assert( newGranularity > 0 );
	granularity = newGranularity;
}

/*
================
idHashIndex::GenerateKey
================
*/
ID_INLINE int idHashIndex::GenerateKey( const char *p_string, bool caseSensitive ) const {
	if ( caseSensitive ) {
		return ( idStr::Hash( p_string ) & hashMask );
	} else {
		return ( idStr::IHash( p_string ) & hashMask );
	}
}

/*
================
idHashIndex::GenerateKey
================
*/
ID_INLINE int idHashIndex::GenerateKey( const idVec3 &v ) const {
	return ( (((int) v[0]) + ((int) v[1]) + ((int) v[2])) & hashMask );
}

/*
================
idHashIndex::GenerateKey
================
*/
ID_INLINE int idHashIndex::GenerateKey( const int n1, const int n2 ) const {
	return ( ( n1 + n2 ) & hashMask );
}

#endif /* !__HASHINDEX_H__ */
