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

#include "../precompiled.h"
#pragma hdrstop

int idHashIndex::INVALID_INDEX[1] = { -1 };

/*
================
idHashIndex::Init
================
*/
void idHashIndex::Init( const int initialHashSize, const int initialIndexSize ) {
	assert( idMath::IsPowerOfTwo( initialHashSize ) );

	hashSize = initialHashSize;
	p_hash = INVALID_INDEX;
	indexSize = initialIndexSize;
	p_indexChain = INVALID_INDEX;
	granularity = DEFAULT_HASH_GRANULARITY;
	hashMask = hashSize - 1;
	lookupMask = 0;
}

/*
================
idHashIndex::Allocate
================
*/
void idHashIndex::Allocate( const int newHashSize, const int newIndexSize ) {
	assert( idMath::IsPowerOfTwo( newHashSize ) );

	Free();
	hashSize = newHashSize;
	p_hash = new int[hashSize];
	memset( p_hash, 0xff, hashSize * sizeof( p_hash[0] ) );
	indexSize = newIndexSize;
	p_indexChain = new int[indexSize];
	memset( p_indexChain, 0xff, indexSize * sizeof( p_indexChain[0] ) );
	hashMask = hashSize - 1;
	lookupMask = -1;
}

/*
================
idHashIndex::Free
================
*/
void idHashIndex::Free( void ) {
	if ( p_hash != INVALID_INDEX ) {
		delete[] p_hash;
		p_hash = INVALID_INDEX;
	}
	if ( p_indexChain != INVALID_INDEX ) {
		delete[] p_indexChain;
		p_indexChain = INVALID_INDEX;
	}
	lookupMask = 0;
}

/*
================
idHashIndex::ResizeIndex
================
*/
void idHashIndex::ResizeIndex( const int newIndexSize ) {
	int *p_oldIndexChain, mod, newSize;

	if ( newIndexSize <= indexSize ) {
		return;
	}

	mod = newIndexSize % granularity;
	if ( !mod ) {
		newSize = newIndexSize;
	} else {
		newSize = newIndexSize + granularity - mod;
	}

	if ( p_indexChain == INVALID_INDEX ) {
		indexSize = newSize;
		return;
	}

	p_oldIndexChain = p_indexChain;
	p_indexChain = new int[newSize];
	memcpy( p_indexChain, p_oldIndexChain, indexSize * sizeof(int) );
	memset( p_indexChain + indexSize, 0xff, (newSize - indexSize) * sizeof(int) );
	delete[] p_oldIndexChain;
	indexSize = newSize;
}

/*
================
idHashIndex::GetSpread
================
*/
int idHashIndex::GetSpread( void ) const {
	int i, index, totalItems, *p_numHashItems, average, error, e;

	if ( p_hash == INVALID_INDEX ) {
		return 100;
	}

	totalItems = 0;
	p_numHashItems = new int[hashSize];
	for ( i = 0; i < hashSize; i++ ) {
		p_numHashItems[i] = 0;
		for ( index = p_hash[i]; index >= 0; index = p_indexChain[index] ) {
			p_numHashItems[i]++;
		}
		totalItems += p_numHashItems[i];
	}
	// if no items in hash
	if ( totalItems <= 1 ) {
		delete[] p_numHashItems;
		return 100;
	}
	average = totalItems / hashSize;
	error = 0;
	for ( i = 0; i < hashSize; i++ ) {
		e = abs( p_numHashItems[i] - average );
		if ( e > 1 ) {
			error += e - 1;
		}
	}
	delete[] p_numHashItems;
	return 100 - (error * 100 / totalItems);
}
