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

#ifndef __STRPOOL_H__
#define __STRPOOL_H__

/*
===============================================================================

	idStrPool

===============================================================================
*/

class idStrPool;

class idPoolStr : public idStr {
	friend class idStrPool;

public:
						idPoolStr() { numUsers = 0; }
						~idPoolStr() { assert( numUsers == 0 ); }

						// returns total size of allocated memory
	size_t				Allocated( void ) const { return idStr::Allocated(); }
						// returns total size of allocated memory including size of string pool type
	size_t				Size( void ) const { return sizeof( *this ) + Allocated(); }
						// returns a pointer to the pool this string was allocated from
	const idStrPool *	GetPool( void ) const { return p_pool; }

private:
	idStrPool *			p_pool;
	mutable int			numUsers;
};

class idStrPool {
public:
						idStrPool() { caseSensitive = true; }

	void				SetCaseSensitive( bool caseSensitive );

	int					Num( void ) const { return pool.Num(); }
	size_t				Allocated( void ) const;
	size_t				Size( void ) const;

	const idPoolStr *	operator[]( int index ) const { return pool[index]; }

	const idPoolStr *	AllocString( const char *p_string );
	void				FreeString( const idPoolStr *p_poolStr );
	const idPoolStr *	CopyString( const idPoolStr *p_poolStr );
	void				Clear( void );

private:
	bool				caseSensitive;
	idList<idPoolStr *>	pool;
	idHashIndex			poolHash;
};

/*
================
idStrPool::SetCaseSensitive
================
*/
ID_INLINE void idStrPool::SetCaseSensitive( bool caseSensitive ) {
	this->caseSensitive = caseSensitive;
}

/*
================
idStrPool::AllocString
================
*/
ID_INLINE const idPoolStr *idStrPool::AllocString( const char *p_string ) {
	int i, hash;
	idPoolStr *p_poolStr;

	hash = poolHash.GenerateKey( p_string, caseSensitive );
	if ( caseSensitive ) {
		for ( i = poolHash.First( hash ); i != -1; i = poolHash.Next( i ) ) {
			if ( pool[i]->Cmp( p_string ) == 0 ) {
				pool[i]->numUsers++;
				return pool[i];
			}
		}
	} else {
		for ( i = poolHash.First( hash ); i != -1; i = poolHash.Next( i ) ) {
			if ( pool[i]->Icmp( p_string ) == 0 ) {
				pool[i]->numUsers++;
				return pool[i];
			}
		}
	}

	p_poolStr = new idPoolStr;
	*static_cast<idStr *>(p_poolStr) = p_string;
	p_poolStr->p_pool = this;
	p_poolStr->numUsers = 1;
	poolHash.Add( hash, pool.Append( p_poolStr ) );
	return p_poolStr;
}

/*
================
idStrPool::FreeString
================
*/
ID_INLINE void idStrPool::FreeString( const idPoolStr *p_poolStr ) {
	int i, hash;

	assert( p_poolStr->numUsers >= 1 );
	assert( p_poolStr->p_pool == this );

	p_poolStr->numUsers--;
	if ( p_poolStr->numUsers <= 0 ) {
		hash = poolHash.GenerateKey( p_poolStr->c_str(), caseSensitive );
		if ( caseSensitive ) { 
			for ( i = poolHash.First( hash ); i != -1; i = poolHash.Next( i ) ) {
				if ( pool[i]->Cmp( p_poolStr->c_str() ) == 0 ) {
					break;
				}
			}
		} else {
			for ( i = poolHash.First( hash ); i != -1; i = poolHash.Next( i ) ) {
				if ( pool[i]->Icmp( p_poolStr->c_str() ) == 0 ) {
					break;
				}
			}
		}
		assert( i != -1 );
		assert( pool[i] == p_poolStr );
		delete pool[i];
		pool.RemoveIndex( i );
		poolHash.RemoveIndex( hash, i );
	}
}

/*
================
idStrPool::CopyString
================
*/
ID_INLINE const idPoolStr *idStrPool::CopyString( const idPoolStr *p_poolStr ) {

	assert( p_poolStr->numUsers >= 1 );

	if ( p_poolStr->p_pool == this ) {
		// the string is from this pool so just increase the user count
		p_poolStr->numUsers++;
		return p_poolStr;
	} else {
		// the string is from another pool so it needs to be re-allocated from this pool.
		return AllocString( p_poolStr->c_str() );
	}
}

/*
================
idStrPool::Clear
================
*/
ID_INLINE void idStrPool::Clear( void ) {
	int i;

	for ( i = 0; i < pool.Num(); i++ ) {
		pool[i]->numUsers = 0;
	}
	pool.DeleteContents( true );
	poolHash.Free();
}

/*
================
idStrPool::Allocated
================
*/
ID_INLINE size_t idStrPool::Allocated( void ) const {
	int i;
	size_t size;

	size = pool.Allocated() + poolHash.Allocated();
	for ( i = 0; i < pool.Num(); i++ ) {
		size += pool[i]->Allocated();
	}
	return size;
}

/*
================
idStrPool::Size
================
*/
ID_INLINE size_t idStrPool::Size( void ) const {
	int i;
	size_t size;

	size = pool.Size() + poolHash.Size();
	for ( i = 0; i < pool.Num(); i++ ) {
		size += pool[i]->Size();
	}
	return size;
}

#endif /* !__STRPOOL_H__ */
