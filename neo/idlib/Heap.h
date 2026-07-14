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

#ifndef __HEAP_H__
#define __HEAP_H__

#include <stddef.h>

/*
===============================================================================

	Memory Management

	This is a replacement for the compiler heap code (i.e. "C" malloc() and
	free() calls). On average 2.5-3.0 times faster than MSVC malloc()/free().
	Worst case performance is 1.65 times faster and best case > 70 times.
 
===============================================================================
*/


typedef struct {
	int		num;
	int		minSize;
	int		maxSize;
	int		totalSize;
} memoryStats_t;


void		Mem_Init( void );
void		Mem_Shutdown( void );
void		Mem_EnableLeakTest( const char *name );
void		Mem_ClearFrameStats( void );
void		Mem_GetFrameStats( memoryStats_t &allocs, memoryStats_t &frees );
void		Mem_GetStats( memoryStats_t &stats );
void		Mem_Dump_f( const class idCmdArgs &args );
void		Mem_DumpCompressed_f( const class idCmdArgs &args );
void		Mem_AllocDefragBlock( void );


#ifndef ID_DEBUG_MEMORY

void *		Mem_Alloc( const int size );
void *		Mem_ClearedAlloc( const int size );
void		Mem_Free( void *p_ptr );
char *		Mem_CopyString( const char *p_in );
void *		Mem_Alloc16( const int size );
void		Mem_Free16( void *p_ptr );

#ifdef ID_REDIRECT_NEWDELETE

__inline void *operator new( size_t s ) {
	return Mem_Alloc( s );
}
__inline void operator delete( void *p_ptr ) {
	Mem_Free( p_ptr );
}
__inline void *operator new[]( size_t s ) {
	return Mem_Alloc( s );
}
__inline void operator delete[]( void *p_ptr ) {
	Mem_Free( p_ptr );
}

#endif

#else /* ID_DEBUG_MEMORY */

void *		Mem_Alloc( const int size, const char *p_fileName, const int lineNumber );
void *		Mem_ClearedAlloc( const int size, const char *p_fileName, const int lineNumber );
void		Mem_Free( void *p_ptr, const char *p_fileName, const int lineNumber );
char *		Mem_CopyString( const char *p_in, const char *p_fileName, const int lineNumber );
void *		Mem_Alloc16( const int size, const char *p_fileName, const int lineNumber );
void		Mem_Free16( void *p_ptr, const char *p_fileName, const int lineNumber );

#ifdef ID_REDIRECT_NEWDELETE

__inline void *operator new( size_t s, int t1, int t2, char *p_fileName, int lineNumber ) {
	return Mem_Alloc( s, p_fileName, lineNumber );
}
__inline void operator delete( void *p_ptr, int t1, int t2, char *p_fileName, int lineNumber ) {
	Mem_Free( p_ptr, p_fileName, lineNumber );
}
__inline void *operator new[]( size_t s, int t1, int t2, char *p_fileName, int lineNumber ) {
	return Mem_Alloc( s, p_fileName, lineNumber );
}
__inline void operator delete[]( void *p_ptr, int t1, int t2, char *p_fileName, int lineNumber ) {
	Mem_Free( p_ptr, p_fileName, lineNumber );
}
__inline void *operator new( size_t s ) {
	return Mem_Alloc( s, "", 0 );
}
__inline void operator delete( void *p_ptr ) {
	Mem_Free( p_ptr, "", 0 );
}
__inline void *operator new[]( size_t s ) {
	return Mem_Alloc( s, "", 0 );
}
__inline void operator delete[]( void *p_ptr ) {
	Mem_Free( p_ptr, "", 0 );
}

#define ID_DEBUG_NEW						new( 0, 0, __FILE__, __LINE__ )
#undef new
#define new									ID_DEBUG_NEW

#endif

#define		Mem_Alloc( size )				Mem_Alloc( size, __FILE__, __LINE__ )
#define		Mem_ClearedAlloc( size )		Mem_ClearedAlloc( size, __FILE__, __LINE__ )
#define		Mem_Free( p_ptr )				Mem_Free( p_ptr, __FILE__, __LINE__ )
#define		Mem_CopyString( p_str )			Mem_CopyString( p_str, __FILE__, __LINE__ )
#define		Mem_Alloc16( size )				Mem_Alloc16( size, __FILE__, __LINE__ )
#define		Mem_Free16( p_ptr )				Mem_Free16( p_ptr, __FILE__, __LINE__ )

#endif /* ID_DEBUG_MEMORY */


/*
===============================================================================

	Block based allocator for fixed size objects.

	All objects of the 'type' are properly constructed.
	However, the constructor is not called for re-used objects.

===============================================================================
*/

template<class type, int blockSize>
class idBlockAlloc {
public:
							idBlockAlloc( void );
							~idBlockAlloc( void );

	void					Shutdown( void );

	type *					Alloc( void );
	void					Free( type *p_element );

	int						GetTotalCount( void ) const { return total; }
	int						GetAllocCount( void ) const { return active; }
	int						GetFreeCount( void ) const { return total - active; }

private:
	typedef struct element_s {
		struct element_s *	p_next;
		type				t;
	} element_t;
	typedef struct block_s {
		element_t			elements[blockSize];
		struct block_s *	p_next;
	} block_t;

	block_t *				p_blocks;
	element_t *				p_free;
	int						total;
	int						active;
};

template<class type, int blockSize>
idBlockAlloc<type,blockSize>::idBlockAlloc( void ) {
	p_blocks = NULL;
	p_free = NULL;
	total = active = 0;
}

template<class type, int blockSize>
idBlockAlloc<type,blockSize>::~idBlockAlloc( void ) {
	Shutdown();
}

template<class type, int blockSize>
type *idBlockAlloc<type,blockSize>::Alloc( void ) {
	if ( !p_free ) {
		block_t *p_block = new block_t;
		p_block->p_next = p_blocks;
		p_blocks = p_block;
		for ( int i = 0; i < blockSize; i++ ) {
			p_block->elements[i].p_next = p_free;
			p_free = &p_block->elements[i];
		}
		total += blockSize;
	}
	active++;
	element_t *p_element = p_free;
	p_free = p_free->p_next;
	p_element->p_next = NULL;
	return &p_element->t;
}

template<class type, int blockSize>
void idBlockAlloc<type,blockSize>::Free( type *p_element ) {
	element_t *p_elementEntry = reinterpret_cast<element_t *>( reinterpret_cast<unsigned char *>( p_element ) - offsetof( element_t, t ) );
	p_elementEntry->p_next = p_free;
	p_free = p_elementEntry;
	active--;
}

template<class type, int blockSize>
void idBlockAlloc<type,blockSize>::Shutdown( void ) {
	while( p_blocks ) {
		block_t *p_block = p_blocks;
		p_blocks = p_blocks->p_next;
		delete p_block;
	}
	p_blocks = NULL;
	p_free = NULL;
	total = active = 0;
}

/*
==============================================================================

	Dynamic allocator, simple wrapper for normal allocations which can
	be interchanged with idDynamicBlockAlloc.

	No constructor is called for the 'type'.
	Allocated blocks are always 16 byte aligned.

==============================================================================
*/

template<class type, int baseBlockSize, int minBlockSize>
class idDynamicAlloc {
public:
									idDynamicAlloc( void );
									~idDynamicAlloc( void );

	void							Init( void );
	void							Shutdown( void );
	void							SetFixedBlocks( int numBlocks ) {}
	void							SetLockMemory( bool lock ) {}
	void							FreeEmptyBaseBlocks( void ) {}

	type *							Alloc( const int num );
	type *							Resize( type *p_ptr, const int num );
	void							Free( type *p_ptr );
	const char *					CheckMemory( const type *p_ptr ) const;

	int								GetNumBaseBlocks( void ) const { return 0; }
	size_t							GetBaseBlockMemory( void ) const { return 0; }
	int								GetNumUsedBlocks( void ) const { return numUsedBlocks; }
	size_t							GetUsedBlockMemory( void ) const { return usedBlockMemory; }
	int								GetNumFreeBlocks( void ) const { return 0; }
	size_t							GetFreeBlockMemory( void ) const { return 0; }
	int								GetNumEmptyBaseBlocks( void ) const { return 0; }

private:
	int								numUsedBlocks;			// number of used blocks
	size_t							usedBlockMemory;		// total memory in used blocks

	int								numAllocs;
	int								numResizes;
	int								numFrees;

	void							Clear( void );
};

template<class type, int baseBlockSize, int minBlockSize>
idDynamicAlloc<type, baseBlockSize, minBlockSize>::idDynamicAlloc( void ) {
	Clear();
}

template<class type, int baseBlockSize, int minBlockSize>
idDynamicAlloc<type, baseBlockSize, minBlockSize>::~idDynamicAlloc( void ) {
	Shutdown();
}

template<class type, int baseBlockSize, int minBlockSize>
void idDynamicAlloc<type, baseBlockSize, minBlockSize>::Init( void ) {
}

template<class type, int baseBlockSize, int minBlockSize>
void idDynamicAlloc<type, baseBlockSize, minBlockSize>::Shutdown( void ) {
	Clear();
}

template<class type, int baseBlockSize, int minBlockSize>
type *idDynamicAlloc<type, baseBlockSize, minBlockSize>::Alloc( const int num ) {
	numAllocs++;
	if ( num <= 0 ) {
		return NULL;
	}
	numUsedBlocks++;
	usedBlockMemory += num * sizeof( type );
	return Mem_Alloc16( num * sizeof( type ) );
}

template<class type, int baseBlockSize, int minBlockSize>
type *idDynamicAlloc<type, baseBlockSize, minBlockSize>::Resize( type *p_ptr, const int num ) {

	numResizes++;

	if ( p_ptr == NULL ) {
		return Alloc( num );
	}

	if ( num <= 0 ) {
		Free( p_ptr );
		return NULL;
	}

	assert( 0 );
	return p_ptr;
}

template<class type, int baseBlockSize, int minBlockSize>
void idDynamicAlloc<type, baseBlockSize, minBlockSize>::Free( type *p_ptr ) {
	numFrees++;
	if ( p_ptr == NULL ) {
		return;
	}
	Mem_Free16( p_ptr );
}

template<class type, int baseBlockSize, int minBlockSize>
const char *idDynamicAlloc<type, baseBlockSize, minBlockSize>::CheckMemory( const type *p_ptr ) const {
	return NULL;
}

template<class type, int baseBlockSize, int minBlockSize>
void idDynamicAlloc<type, baseBlockSize, minBlockSize>::Clear( void ) {
	numUsedBlocks = 0;
	usedBlockMemory = 0;
	numAllocs = 0;
	numResizes = 0;
	numFrees = 0;
}


/*
==============================================================================

	Fast dynamic p_block p_allocator.

	No constructor is called for the 'type'.
	Allocated blocks are always 16 byte aligned.

==============================================================================
*/

#include "containers/BTree.h"

//#define DYNAMIC_BLOCK_ALLOC_CHECK

template<class type>
class idDynamicBlock {
public:
	type *							GetMemory( void ) const { return reinterpret_cast<type *>( reinterpret_cast<byte *>( const_cast<idDynamicBlock<type> *>( this ) ) + sizeof( idDynamicBlock<type> ) ); }
	size_t							GetSize( void ) const { return size; }
	void							SetSize( size_t s, bool baseBlock ) { size = s; isBaseBlock = baseBlock; }
	bool							IsBaseBlock( void ) const { return isBaseBlock; }

#ifdef DYNAMIC_BLOCK_ALLOC_CHECK
	int								id[3];
	void *							p_allocator;
#endif

	size_t							size;					// size in bytes of the block
	bool							isBaseBlock;			// true for base blocks allocated from the OS
	idDynamicBlock<type> *			p_prev;					// previous memory block
	idDynamicBlock<type> *			p_next;					// next memory block
	idBTreeNode<idDynamicBlock<type>,size_t> *p_node;		// node in the B-Tree with free blocks
};

template<class type, int baseBlockSize, int minBlockSize>
class idDynamicBlockAlloc {
public:
									idDynamicBlockAlloc( void );
									~idDynamicBlockAlloc( void );

	void							Init( void );
	void							Shutdown( void );
	void							SetFixedBlocks( int numBlocks );
	void							SetLockMemory( bool lock );
	void							FreeEmptyBaseBlocks( void );

	type *							Alloc( const int num );
	type *							Resize( type *p_ptr, const int num );
	void							Free( type *p_ptr );
	const char *					CheckMemory( const type *p_ptr ) const;

	int								GetNumBaseBlocks( void ) const { return numBaseBlocks; }
	size_t							GetBaseBlockMemory( void ) const { return baseBlockMemory; }
	int								GetNumUsedBlocks( void ) const { return numUsedBlocks; }
	size_t							GetUsedBlockMemory( void ) const { return usedBlockMemory; }
	int								GetNumFreeBlocks( void ) const { return numFreeBlocks; }
	size_t							GetFreeBlockMemory( void ) const { return freeBlockMemory; }
	int								GetNumEmptyBaseBlocks( void ) const;

private:
	idDynamicBlock<type> *			p_firstBlock;				// first block in list in order of increasing address
	idDynamicBlock<type> *			p_lastBlock;				// last block in list in order of increasing address
	idBTree<idDynamicBlock<type>,size_t,4>freeTree;		// B-Tree with free memory blocks
	bool							allowAllocs;			// allow base block allocations
	bool							lockMemory;				// lock memory so it cannot get swapped out

#ifdef DYNAMIC_BLOCK_ALLOC_CHECK
	int								blockId[3];
#endif

	int								numBaseBlocks;			// number of base blocks
	size_t							baseBlockMemory;		// total memory in base blocks
	int								numUsedBlocks;			// number of used blocks
	size_t							usedBlockMemory;		// total memory in used blocks
	int								numFreeBlocks;			// number of free blocks
	size_t							freeBlockMemory;		// total memory in free blocks

	int								numAllocs;
	int								numResizes;
	int								numFrees;

	void							Clear( void );
	idDynamicBlock<type> *			AllocInternal( const int num );
	idDynamicBlock<type> *			ResizeInternal( idDynamicBlock<type> *p_block, const int num );
	void							FreeInternal( idDynamicBlock<type> *p_block );
	void							LinkFreeInternal( idDynamicBlock<type> *p_block );
	void							UnlinkFreeInternal( idDynamicBlock<type> *p_block );
	void							CheckMemory( void ) const;
};

template<class type, int baseBlockSize, int minBlockSize>
idDynamicBlockAlloc<type, baseBlockSize, minBlockSize>::idDynamicBlockAlloc( void ) {
	Clear();
}

template<class type, int baseBlockSize, int minBlockSize>
idDynamicBlockAlloc<type, baseBlockSize, minBlockSize>::~idDynamicBlockAlloc( void ) {
	Shutdown();
}

template<class type, int baseBlockSize, int minBlockSize>
void idDynamicBlockAlloc<type, baseBlockSize, minBlockSize>::Init( void ) {
	freeTree.Init();
}

template<class type, int baseBlockSize, int minBlockSize>
void idDynamicBlockAlloc<type, baseBlockSize, minBlockSize>::Shutdown( void ) {
	idDynamicBlock<type> *p_block;

	for ( p_block = p_firstBlock; p_block != NULL; p_block = p_block->p_next ) {
		if ( p_block->p_node == NULL ) {
			FreeInternal( p_block );
		}
	}

	for ( p_block = p_firstBlock; p_block != NULL; p_block = p_firstBlock ) {
		p_firstBlock = p_block->p_next;
		assert( p_block->IsBaseBlock() );
		if ( lockMemory ) {
			idLib::sys->UnlockMemory( p_block, static_cast<int>( p_block->GetSize() + sizeof( idDynamicBlock<type> ) ) );
		}
		Mem_Free16( p_block );
	}

	freeTree.Shutdown();

	Clear();
}

template<class type, int baseBlockSize, int minBlockSize>
void idDynamicBlockAlloc<type, baseBlockSize, minBlockSize>::SetFixedBlocks( int numBlocks ) {
	idDynamicBlock<type> *p_block;

	for ( int i = numBaseBlocks; i < numBlocks; i++ ) {
		p_block = ( idDynamicBlock<type> * ) Mem_Alloc16( baseBlockSize );
		if ( lockMemory ) {
			idLib::sys->LockMemory( p_block, baseBlockSize );
		}
#ifdef DYNAMIC_BLOCK_ALLOC_CHECK
		memcpy( p_block->id, blockId, sizeof( p_block->id ) );
		p_block->p_allocator = (void*)this;
#endif
		p_block->SetSize( static_cast<size_t>( baseBlockSize ) - sizeof( idDynamicBlock<type> ), true );
		p_block->p_next = NULL;
		p_block->p_prev = p_lastBlock;
		if ( p_lastBlock ) {
			p_lastBlock->p_next = p_block;
		} else {
			p_firstBlock = p_block;
		}
		p_lastBlock = p_block;
		p_block->p_node = NULL;

		FreeInternal( p_block );

		numBaseBlocks++;
		baseBlockMemory += baseBlockSize;
	}

	allowAllocs = false;
}

template<class type, int baseBlockSize, int minBlockSize>
void idDynamicBlockAlloc<type, baseBlockSize, minBlockSize>::SetLockMemory( bool lock ) {
	lockMemory = lock;
}

template<class type, int baseBlockSize, int minBlockSize>
void idDynamicBlockAlloc<type, baseBlockSize, minBlockSize>::FreeEmptyBaseBlocks( void ) {
	idDynamicBlock<type> *p_block, *p_next;

	for ( p_block = p_firstBlock; p_block != NULL; p_block = p_next ) {
		p_next = p_block->p_next;

		if ( p_block->IsBaseBlock() && p_block->p_node != NULL && ( p_next == NULL || p_next->IsBaseBlock() ) ) {
			UnlinkFreeInternal( p_block );
			if ( p_block->p_prev ) {
				p_block->p_prev->p_next = p_block->p_next;
			} else {
				p_firstBlock = p_block->p_next;
			}
			if ( p_block->p_next ) {
				p_block->p_next->p_prev = p_block->p_prev;
			} else {
				p_lastBlock = p_block->p_prev;
			}
			if ( lockMemory ) {
				idLib::sys->UnlockMemory( p_block, static_cast<int>( p_block->GetSize() + sizeof( idDynamicBlock<type> ) ) );
			}
			numBaseBlocks--;
			baseBlockMemory -= p_block->GetSize() + sizeof( idDynamicBlock<type> );
			Mem_Free16( p_block );
		}
	}

#ifdef DYNAMIC_BLOCK_ALLOC_CHECK
	CheckMemory();
#endif
}

template<class type, int baseBlockSize, int minBlockSize>
int idDynamicBlockAlloc<type, baseBlockSize, minBlockSize>::GetNumEmptyBaseBlocks( void ) const {
	int numEmptyBaseBlocks;
	idDynamicBlock<type> *p_block;

	numEmptyBaseBlocks = 0;
	for ( p_block = p_firstBlock; p_block != NULL; p_block = p_block->p_next ) {
		if ( p_block->IsBaseBlock() && p_block->p_node != NULL && ( p_block->p_next == NULL || p_block->p_next->IsBaseBlock() ) ) {
			numEmptyBaseBlocks++;
		}
	}
	return numEmptyBaseBlocks;
}

template<class type, int baseBlockSize, int minBlockSize>
type *idDynamicBlockAlloc<type, baseBlockSize, minBlockSize>::Alloc( const int num ) {
	idDynamicBlock<type> *p_block;

	numAllocs++;

	if ( num <= 0 ) {
		return NULL;
	}

	p_block = AllocInternal( num );
	if ( p_block == NULL ) {
		return NULL;
	}
	p_block = ResizeInternal( p_block, num );
	if ( p_block == NULL ) {
		return NULL;
	}

#ifdef DYNAMIC_BLOCK_ALLOC_CHECK
	CheckMemory();
#endif

	numUsedBlocks++;
	usedBlockMemory += p_block->GetSize();

	return p_block->GetMemory();
}

template<class type, int baseBlockSize, int minBlockSize>
type *idDynamicBlockAlloc<type, baseBlockSize, minBlockSize>::Resize( type *p_ptr, const int num ) {

	numResizes++;

	if ( p_ptr == NULL ) {
		return Alloc( num );
	}

	if ( num <= 0 ) {
		Free( p_ptr );
		return NULL;
	}

	idDynamicBlock<type> *p_block = reinterpret_cast<idDynamicBlock<type> *>( reinterpret_cast<byte *>( p_ptr ) - sizeof( idDynamicBlock<type> ) );

	usedBlockMemory -= p_block->GetSize();

	p_block = ResizeInternal( p_block, num );
	if ( p_block == NULL ) {
		return NULL;
	}

#ifdef DYNAMIC_BLOCK_ALLOC_CHECK
	CheckMemory();
#endif

	usedBlockMemory += p_block->GetSize();

	return p_block->GetMemory();
}

template<class type, int baseBlockSize, int minBlockSize>
void idDynamicBlockAlloc<type, baseBlockSize, minBlockSize>::Free( type *p_ptr ) {

	numFrees++;

	if ( p_ptr == NULL ) {
		return;
	}

	idDynamicBlock<type> *p_block = reinterpret_cast<idDynamicBlock<type> *>( reinterpret_cast<byte *>( p_ptr ) - sizeof( idDynamicBlock<type> ) );

	numUsedBlocks--;
	usedBlockMemory -= p_block->GetSize();

	FreeInternal( p_block );

#ifdef DYNAMIC_BLOCK_ALLOC_CHECK
	CheckMemory();
#endif
}

template<class type, int baseBlockSize, int minBlockSize>
const char *idDynamicBlockAlloc<type, baseBlockSize, minBlockSize>::CheckMemory( const type *p_ptr ) const {
	idDynamicBlock<type> *p_block;

	if ( p_ptr == NULL ) {
		return NULL;
	}

	p_block = reinterpret_cast<idDynamicBlock<type> *>( reinterpret_cast<byte *>( const_cast<type *>( p_ptr ) ) - sizeof( idDynamicBlock<type> ) );

	if ( p_block->p_node != NULL ) {
		return "memory has been freed";
	}

#ifdef DYNAMIC_BLOCK_ALLOC_CHECK
	if ( p_block->id[0] != 0x11111111 || p_block->id[1] != 0x22222222 || p_block->id[2] != 0x33333333 ) {
		return "memory has invalid id";
	}
	if ( p_block->p_allocator != (void*)this ) {
		return "memory was allocated with different allocator";
	}
#endif

	/* base blocks can be larger than baseBlockSize which can cause this code to fail
	idDynamicBlock<type> *p_base;
	const uintptr_t blockAddress = reinterpret_cast<uintptr_t>( p_block );
	for ( p_base = p_firstBlock; p_base != NULL; p_base = p_base->p_next ) {
		if ( p_base->IsBaseBlock() ) {
			const uintptr_t baseAddress = reinterpret_cast<uintptr_t>( p_base );
			if ( blockAddress >= baseAddress && blockAddress < baseAddress + baseBlockSize ) {
				break;
			}
		}
	}
	if ( p_base == NULL ) {
		return "no base block found for memory";
	}
	*/

	return NULL;
}

template<class type, int baseBlockSize, int minBlockSize>
void idDynamicBlockAlloc<type, baseBlockSize, minBlockSize>::Clear( void ) {
	p_firstBlock = p_lastBlock = NULL;
	allowAllocs = true;
	lockMemory = false;
	numBaseBlocks = 0;
	baseBlockMemory = 0;
	numUsedBlocks = 0;
	usedBlockMemory = 0;
	numFreeBlocks = 0;
	freeBlockMemory = 0;
	numAllocs = 0;
	numResizes = 0;
	numFrees = 0;

#ifdef DYNAMIC_BLOCK_ALLOC_CHECK
	blockId[0] = 0x11111111;
	blockId[1] = 0x22222222;
	blockId[2] = 0x33333333;
#endif
}

template<class type, int baseBlockSize, int minBlockSize>
idDynamicBlock<type> *idDynamicBlockAlloc<type, baseBlockSize, minBlockSize>::AllocInternal( const int num ) {
	idDynamicBlock<type> *p_block;
	size_t alignedBytes = ( static_cast<size_t>( num ) * sizeof( type ) + 15 ) & ~static_cast<size_t>( 15 );

	p_block = freeTree.FindSmallestLargerEqual( alignedBytes );
	if ( p_block != NULL ) {
		UnlinkFreeInternal( p_block );
	} else if ( allowAllocs ) {
		size_t allocSize = Max( static_cast<size_t>( baseBlockSize ), alignedBytes + sizeof( idDynamicBlock<type> ) );
		p_block = ( idDynamicBlock<type> * ) Mem_Alloc16( static_cast<int>( allocSize ) );
		if ( lockMemory ) {
			idLib::sys->LockMemory( p_block, static_cast<int>( allocSize ) );
		}
#ifdef DYNAMIC_BLOCK_ALLOC_CHECK
		memcpy( p_block->id, blockId, sizeof( p_block->id ) );
		p_block->p_allocator = (void*)this;
#endif
		p_block->SetSize( allocSize - sizeof( idDynamicBlock<type> ), true );
		p_block->p_next = NULL;
		p_block->p_prev = p_lastBlock;
		if ( p_lastBlock ) {
			p_lastBlock->p_next = p_block;
		} else {
			p_firstBlock = p_block;
		}
		p_lastBlock = p_block;
		p_block->p_node = NULL;

		numBaseBlocks++;
		baseBlockMemory += allocSize;
	}

	return p_block;
}

template<class type, int baseBlockSize, int minBlockSize>
idDynamicBlock<type> *idDynamicBlockAlloc<type, baseBlockSize, minBlockSize>::ResizeInternal( idDynamicBlock<type> *p_block, const int num ) {
	size_t alignedBytes = ( static_cast<size_t>( num ) * sizeof( type ) + 15 ) & ~static_cast<size_t>( 15 );

#ifdef DYNAMIC_BLOCK_ALLOC_CHECK
	assert( p_block->id[0] == 0x11111111 && p_block->id[1] == 0x22222222 && p_block->id[2] == 0x33333333 && p_block->p_allocator == (void*)this );
#endif

	// if the new size is larger
	if ( alignedBytes > p_block->GetSize() ) {

		idDynamicBlock<type> *p_nextBlock = p_block->p_next;

		// try to annexate the next block if it's free
		if ( p_nextBlock && !p_nextBlock->IsBaseBlock() && p_nextBlock->p_node != NULL &&
				p_block->GetSize() + sizeof( idDynamicBlock<type> ) + p_nextBlock->GetSize() >= alignedBytes ) {

			UnlinkFreeInternal( p_nextBlock );
			p_block->SetSize( p_block->GetSize() + sizeof( idDynamicBlock<type> ) + p_nextBlock->GetSize(), p_block->IsBaseBlock() );
			p_block->p_next = p_nextBlock->p_next;
			if ( p_nextBlock->p_next ) {
				p_nextBlock->p_next->p_prev = p_block;
			} else {
				p_lastBlock = p_block;
			}
		} else {
			// allocate a new block and copy
			idDynamicBlock<type> *p_oldBlock = p_block;
			p_block = AllocInternal( num );
			if ( p_block == NULL ) {
				return NULL;
			}
			memcpy( p_block->GetMemory(), p_oldBlock->GetMemory(), p_oldBlock->GetSize() );
			FreeInternal( p_oldBlock );
		}
	}

	// if the unused space at the end of this block is large enough to hold a block with at least one element
	const size_t minSplitSize = sizeof( idDynamicBlock<type> ) + Max( static_cast<size_t>( minBlockSize ), sizeof( type ) );
	if ( p_block->GetSize() < alignedBytes + minSplitSize ) {
		return p_block;
	}

	idDynamicBlock<type> *p_newBlock;

	p_newBlock = reinterpret_cast<idDynamicBlock<type> *>( reinterpret_cast<byte *>( p_block ) + sizeof( idDynamicBlock<type> ) + alignedBytes );
#ifdef DYNAMIC_BLOCK_ALLOC_CHECK
	memcpy( p_newBlock->id, blockId, sizeof( p_newBlock->id ) );
	p_newBlock->p_allocator = (void*)this;
#endif
	p_newBlock->SetSize( p_block->GetSize() - alignedBytes - sizeof( idDynamicBlock<type> ), false );
	p_newBlock->p_next = p_block->p_next;
	p_newBlock->p_prev = p_block;
	if ( p_newBlock->p_next ) {
		p_newBlock->p_next->p_prev = p_newBlock;
	} else {
		p_lastBlock = p_newBlock;
	}
	p_newBlock->p_node = NULL;
	p_block->p_next = p_newBlock;
	p_block->SetSize( alignedBytes, p_block->IsBaseBlock() );

	FreeInternal( p_newBlock );

	return p_block;
}

template<class type, int baseBlockSize, int minBlockSize>
void idDynamicBlockAlloc<type, baseBlockSize, minBlockSize>::FreeInternal( idDynamicBlock<type> *p_block ) {

	assert( p_block->p_node == NULL );

#ifdef DYNAMIC_BLOCK_ALLOC_CHECK
	assert( p_block->id[0] == 0x11111111 && p_block->id[1] == 0x22222222 && p_block->id[2] == 0x33333333 && p_block->p_allocator == (void*)this );
#endif

	// try to merge with a next free block
	idDynamicBlock<type> *p_nextBlock = p_block->p_next;
	if ( p_nextBlock && !p_nextBlock->IsBaseBlock() && p_nextBlock->p_node != NULL ) {
		UnlinkFreeInternal( p_nextBlock );
		p_block->SetSize( p_block->GetSize() + sizeof( idDynamicBlock<type> ) + p_nextBlock->GetSize(), p_block->IsBaseBlock() );
		p_block->p_next = p_nextBlock->p_next;
		if ( p_nextBlock->p_next ) {
			p_nextBlock->p_next->p_prev = p_block;
		} else {
			p_lastBlock = p_block;
		}
	}

	// try to merge with a previous free block
	idDynamicBlock<type> *p_prevBlock = p_block->p_prev;
	if ( p_prevBlock && !p_block->IsBaseBlock() && p_prevBlock->p_node != NULL ) {
		UnlinkFreeInternal( p_prevBlock );
		p_prevBlock->SetSize( p_prevBlock->GetSize() + sizeof( idDynamicBlock<type> ) + p_block->GetSize(), p_prevBlock->IsBaseBlock() );
		p_prevBlock->p_next = p_block->p_next;
		if ( p_block->p_next ) {
			p_block->p_next->p_prev = p_prevBlock;
		} else {
			p_lastBlock = p_prevBlock;
		}
		LinkFreeInternal( p_prevBlock );
	} else {
		LinkFreeInternal( p_block );
	}
}

template<class type, int baseBlockSize, int minBlockSize>
ID_INLINE void idDynamicBlockAlloc<type, baseBlockSize, minBlockSize>::LinkFreeInternal( idDynamicBlock<type> *p_block ) {
	p_block->p_node = freeTree.Add( p_block, p_block->GetSize() );
	numFreeBlocks++;
	freeBlockMemory += p_block->GetSize();
}

template<class type, int baseBlockSize, int minBlockSize>
ID_INLINE void idDynamicBlockAlloc<type, baseBlockSize, minBlockSize>::UnlinkFreeInternal( idDynamicBlock<type> *p_block ) {
	freeTree.Remove( p_block->p_node );
	p_block->p_node = NULL;
	numFreeBlocks--;
	freeBlockMemory -= p_block->GetSize();
}

template<class type, int baseBlockSize, int minBlockSize>
void idDynamicBlockAlloc<type, baseBlockSize, minBlockSize>::CheckMemory( void ) const {
	idDynamicBlock<type> *p_block;

	for ( p_block = p_firstBlock; p_block != NULL; p_block = p_block->p_next ) {
		// make sure the block is properly linked
		if ( p_block->p_prev == NULL ) {
			assert( p_firstBlock == p_block );
		} else {
			assert( p_block->p_prev->p_next == p_block );
		}
		if ( p_block->p_next == NULL ) {
			assert( p_lastBlock == p_block );
		} else {
			assert( p_block->p_next->p_prev == p_block );
		}
	}
}

#endif /* !__HEAP_H__ */
