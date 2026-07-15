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

#include "../idlib/precompiled.h"
#pragma hdrstop

#include <stdint.h>

#ifndef USE_LIBC_MALLOC
	#define USE_LIBC_MALLOC		0
#endif

#ifndef CRASH_ON_STATIC_ALLOCATION
//	#define CRASH_ON_STATIC_ALLOCATION
#endif

//===============================================================
//
//	idHeap
//
//===============================================================

#define SMALL_HEADER_SIZE		( (int) ( sizeof( byte ) + sizeof( byte ) ) )
#define MEDIUM_HEADER_SIZE		( (int) ( sizeof( mediumHeapEntry_s ) + sizeof( byte ) ) )
#define LARGE_HEADER_SIZE		( (int) ( sizeof( uintptr_t ) + sizeof( byte ) ) )

#define ALIGN_SIZE( bytes )		( ( (bytes) + ALIGN - 1 ) & ~(ALIGN - 1) )
#define SMALL_ALIGN( bytes )	( ALIGN_SIZE( (bytes) + SMALL_HEADER_SIZE ) - SMALL_HEADER_SIZE )
#define MEDIUM_SMALLEST_SIZE	( ALIGN_SIZE( 256 ) + ALIGN_SIZE( MEDIUM_HEADER_SIZE ) )

static_assert( sizeof( uintptr_t ) >= sizeof( void * ), "Heap pointer headers require uintptr_t to hold native pointers" );


class idHeap {

public:
					idHeap( void );
					~idHeap( void );				// frees all associated data
	void			Init( void );					// initialize
	void *			Allocate( const dword bytes );	// allocate memory
	void			Free( void *p_ptr );			// free memory
	void *			Allocate16( const dword bytes );// allocate 16 byte aligned memory
	void			Free16( void *p_ptr );			// free 16 byte aligned memory
	dword			Msize( void *p_ptr );			// return size of data block
	void			Dump( void  );

	void 			AllocDefragBlock( void );		// hack for huge renderbumps

private:

	enum {
		ALIGN = 8									// memory alignment in bytes
	};

	enum {
		INVALID_ALLOC	= 0xdd,
		SMALL_ALLOC		= 0xaa,						// small allocation
		MEDIUM_ALLOC	= 0xbb,						// medium allocaction
		LARGE_ALLOC		= 0xcc						// large allocaction
	};

	struct page_s {									// allocation page
		void *				p_data;					// data pointer to allocated memory
		dword				dataSize;				// number of bytes of memory 'p_data' points to
		page_s *			next;					// next free page in same page manager
		page_s *			prev;					// used only when allocated
		dword				largestFree;			// this data used by the medium-size heap manager
		void *				firstFree;				// pointer to first free entry
	};

	struct mediumHeapEntry_s {
		page_s *			page;					// pointer to page
		dword				size;					// size of block
		mediumHeapEntry_s *	prev;					// previous block
		mediumHeapEntry_s *	next;					// next block
		mediumHeapEntry_s *	prevFree;				// previous free block
		mediumHeapEntry_s *	nextFree;				// next free block
		dword				freeBlock;				// non-zero if free block
	};

	// variables
	void *			smallFirstFree[256/ALIGN+1];	// small heap allocator lists (for allocs of 1-255 bytes)
	page_s *		smallCurPage;					// current page for small allocations
	dword			smallCurPageOffset;				// byte offset in current page
	page_s *		smallFirstUsedPage;				// first used page of the small heap manager

	page_s *		mediumFirstFreePage;			// first partially free page
	page_s *		mediumLastFreePage;				// last partially free page
	page_s *		mediumFirstUsedPage;			// completely used page

	page_s *		largeFirstUsedPage;				// first page used by the large heap manager

	page_s *		swapPage;

	dword			pagesAllocated;					// number of pages currently allocated
	dword			pageSize;						// size of one alloc page in bytes

	dword			pageRequests;					// page requests
	dword			OSAllocs;						// number of allocs made to the OS

	int				c_heapAllocRunningCount;

	void			*defragBlock;					// a single huge block that can be allocated
													// at startup, then freed when needed

	// methods
	page_s *		AllocatePage( dword bytes );	// allocate page from the OS
	void			FreePage( idHeap::page_s *p_page );	// free an OS allocated page

	void *			SmallAllocate( dword bytes );	// allocate memory (1-255 bytes) from small heap manager
	void			SmallFree( void *p_ptr );		// free memory allocated by small heap manager

	void *			MediumAllocateFromPage( idHeap::page_s *p_page, dword sizeNeeded );
	void *			MediumAllocate( dword bytes );	// allocate memory (256-32768 bytes) from medium heap manager
	void			MediumFree( void *p_ptr );		// free memory allocated by medium heap manager

	void *			LargeAllocate( dword bytes );	// allocate large block from OS directly
	void			LargeFree( void *p_ptr );		// free memory allocated by large heap manager

	void			ReleaseSwappedPages( void );
	void			FreePageReal( idHeap::page_s *p_page );
};


/*
================
idHeap::Init
================
*/
void idHeap::Init () {
	OSAllocs			= 0;
	pageRequests		= 0;
	pageSize			= 65536 - sizeof( idHeap::page_s );
	pagesAllocated		= 0;								// reset page allocation counter

	largeFirstUsedPage	= NULL;								// init large heap manager
	swapPage			= NULL;

	memset( smallFirstFree, 0, sizeof(smallFirstFree) );	// init small heap manager
	smallFirstUsedPage	= NULL;
	smallCurPage		= AllocatePage( pageSize );
	assert( smallCurPage );
	smallCurPageOffset	= SMALL_ALIGN( 0 );

	defragBlock = NULL;

	mediumFirstFreePage	= NULL;								// init medium heap manager
	mediumLastFreePage	= NULL;
	mediumFirstUsedPage	= NULL;

	c_heapAllocRunningCount = 0;
}

/*
================
idHeap::idHeap
================
*/
idHeap::idHeap( void ) {
	Init();
}

/*
================
idHeap::~idHeap

  returns all allocated memory back to OS
================
*/
idHeap::~idHeap( void ) {

	idHeap::page_s	*p_page;

	if ( smallCurPage ) {
		FreePage( smallCurPage );			// free small-heap current allocation page
	}
	p_page = smallFirstUsedPage;					// free small-heap allocated pages
	while( p_page ) {
		idHeap::page_s *p_next = p_page->next;
		FreePage( p_page );
		p_page = p_next;
	}

	p_page = largeFirstUsedPage;					// free large-heap allocated pages
	while( p_page ) {
		idHeap::page_s *p_next = p_page->next;
		FreePage( p_page );
		p_page = p_next;
	}

	p_page = mediumFirstFreePage;				// free medium-heap allocated pages
	while( p_page ) {
		idHeap::page_s *p_next = p_page->next;
		FreePage( p_page );
		p_page = p_next;
	}

	p_page = mediumFirstUsedPage;				// free medium-heap allocated completely used pages
	while( p_page ) {
		idHeap::page_s *p_next = p_page->next;
		FreePage( p_page );
		p_page = p_next;
	}

	ReleaseSwappedPages();			

	if ( defragBlock ) {
		free( defragBlock );
	}

	assert( pagesAllocated == 0 );
}

/*
================
idHeap::AllocDefragBlock
================
*/
void idHeap::AllocDefragBlock( void ) {
	int		size = 0x40000000;

	if ( defragBlock ) {
		return;
	}
	while( 1 ) {
		defragBlock = malloc( size );
		if ( defragBlock ) {
			break;
		}
		size >>= 1;
	}
	idLib::common->Printf( "Allocated a %i mb defrag block\n", size / (1024*1024) );
}

/*
================
idHeap::Allocate
================
*/
void *idHeap::Allocate( const dword bytes ) {
	if ( !bytes ) {
		return NULL;
	}
	c_heapAllocRunningCount++;

#if USE_LIBC_MALLOC
	return malloc( bytes );
#else
	if ( !(bytes & ~255) ) {
		return SmallAllocate( bytes );
	}
	if ( !(bytes & ~32767) ) {
		return MediumAllocate( bytes );
	}
	return LargeAllocate( bytes );
#endif
}

/*
================
idHeap::Free
================
*/
void idHeap::Free( void *p_ptr ) {
	if ( !p_ptr ) {
		return;
	}
	c_heapAllocRunningCount--;

#if USE_LIBC_MALLOC
	free( p_ptr );
#else
	switch( ((byte *)(p_ptr))[-1] ) {
		case SMALL_ALLOC: {
			SmallFree( p_ptr );
			break;
		}
		case MEDIUM_ALLOC: {
			MediumFree( p_ptr );
			break;
		}
		case LARGE_ALLOC: {
			LargeFree( p_ptr );
			break;
		}
		default: {
			idLib::common->FatalError( "idHeap::Free: invalid memory block (%s)", idLib::sys->GetCallStackCurStr( 4 ) );
			break;
		}
	}
#endif
}

/*
================
idHeap::Allocate16
================
*/
void *idHeap::Allocate16( const dword bytes ) {
	byte *p_ptr, *p_alignedPtr;

	p_ptr = (byte *) malloc( bytes + 16 + sizeof( uintptr_t ) );
	if ( !p_ptr ) {
		if ( defragBlock ) {
			idLib::common->Printf( "Freeing defragBlock on alloc of %i.\n", bytes );
			free( defragBlock );
			defragBlock = NULL;
			p_ptr = (byte *) malloc( bytes + 16 + sizeof( uintptr_t ) );
			AllocDefragBlock();
		}
		if ( !p_ptr ) {
			common->FatalError( "malloc failure for %i", bytes );
		}
	}
	p_alignedPtr = (byte *) ( ( reinterpret_cast<uintptr_t>( p_ptr ) + 15 ) & ~static_cast<uintptr_t>( 15 ) );
	if ( p_alignedPtr - p_ptr < sizeof( uintptr_t ) ) {
		p_alignedPtr += 16;
	}
	*( reinterpret_cast<uintptr_t *>( p_alignedPtr - sizeof( uintptr_t ) ) ) = reinterpret_cast<uintptr_t>( p_ptr );
	return (void *) p_alignedPtr;
}

/*
================
idHeap::Free16
================
*/
void idHeap::Free16( void *p_ptr ) {
	free( reinterpret_cast<void *>( *( reinterpret_cast<uintptr_t *>( ( (byte *) p_ptr ) - sizeof( uintptr_t ) ) ) ) );
}

/*
================
idHeap::Msize

  returns size of allocated memory block
  p_ptr	= pointer to memory block
  Notes:	size may not be the same as the size in the original
			allocation request (due to block alignment reasons).
================
*/
dword idHeap::Msize( void *p_ptr ) {

	if ( !p_ptr ) {
		return 0;
	}

#if USE_LIBC_MALLOC
	#ifdef _WIN32
		return _msize( p_ptr );
	#else
		return 0;
	#endif
#else
	switch( ((byte *)(p_ptr))[-1] ) {
		case SMALL_ALLOC: {
			return SMALL_ALIGN( ((byte *)(p_ptr))[-SMALL_HEADER_SIZE] * ALIGN );
		}
		case MEDIUM_ALLOC: {
			return ((mediumHeapEntry_s *)(((byte *)(p_ptr)) - ALIGN_SIZE( MEDIUM_HEADER_SIZE )))->size - ALIGN_SIZE( MEDIUM_HEADER_SIZE );
		}
		case LARGE_ALLOC: {
			return reinterpret_cast<idHeap::page_s *>( *( reinterpret_cast<uintptr_t *>( ((byte *)p_ptr) - ALIGN_SIZE( LARGE_HEADER_SIZE ) ) ) )->dataSize - ALIGN_SIZE( LARGE_HEADER_SIZE );
		}
		default: {
			idLib::common->FatalError( "idHeap::Msize: invalid memory block (%s)", idLib::sys->GetCallStackCurStr( 4 ) );
			return 0;
		}
	}
#endif
}

/*
================
idHeap::Dump

  dump contents of the heap
================
*/
void idHeap::Dump( void ) {
	idHeap::page_s	*p_page;

	for ( p_page = smallFirstUsedPage; p_page; p_page = p_page->next ) {
		idLib::common->Printf( "%p  bytes %-8d  (in use by small heap)\n", p_page->p_data, p_page->dataSize);
	}

	if ( smallCurPage ) {
		p_page = smallCurPage;
		idLib::common->Printf( "%p  bytes %-8d  (small heap active page)\n", p_page->p_data, p_page->dataSize );
	}

	for ( p_page = mediumFirstUsedPage; p_page; p_page = p_page->next ) {
		idLib::common->Printf( "%p  bytes %-8d  (completely used by medium heap)\n", p_page->p_data, p_page->dataSize );
	}

	for ( p_page = mediumFirstFreePage; p_page; p_page = p_page->next ) {
		idLib::common->Printf( "%p  bytes %-8d  (partially used by medium heap)\n", p_page->p_data, p_page->dataSize );
	}
	
	for ( p_page = largeFirstUsedPage; p_page; p_page = p_page->next ) {
		idLib::common->Printf( "%p  bytes %-8d  (fully used by large heap)\n", p_page->p_data, p_page->dataSize );
	}

	idLib::common->Printf( "pages allocated : %d\n", pagesAllocated );
}

/*
================
idHeap::FreePageReal

  frees page to be used by the OS
  p	= page to free
================
*/
void idHeap::FreePageReal( idHeap::page_s *p_page ) {
	assert( p_page );
	::free( p_page );
}

/*
================
idHeap::ReleaseSwappedPages

  releases the swap page to OS
================
*/
void idHeap::ReleaseSwappedPages () {
	if ( swapPage ) {
		FreePageReal( swapPage );
	}
	swapPage = NULL;
}

/*
================
idHeap::AllocatePage

  allocates memory from the OS
  bytes	= page size in bytes
  returns pointer to page
================
*/
idHeap::page_s* idHeap::AllocatePage( dword bytes ) {
	idHeap::page_s*	p_page;

	pageRequests++;

	if ( swapPage && swapPage->dataSize == bytes ) {			// if we've got a swap page somewhere
		p_page		= swapPage;
		swapPage	= NULL;
	}
	else {
		dword size;

		size = bytes + sizeof(idHeap::page_s);

		p_page = (idHeap::page_s *) ::malloc( size + ALIGN - 1 );
		if ( !p_page ) {
			if ( defragBlock ) {
				idLib::common->Printf( "Freeing defragBlock on alloc of %i.\n", size + ALIGN - 1 );
				free( defragBlock );
				defragBlock = NULL;
				p_page = (idHeap::page_s *) ::malloc( size + ALIGN - 1 );
				AllocDefragBlock();
			}
			if ( !p_page ) {
				common->FatalError( "malloc failure for %i", bytes );
			}
		}

		p_page->p_data		= (void *) ALIGN_SIZE( reinterpret_cast<uintptr_t>( (byte *)(p_page) ) + sizeof( idHeap::page_s ) );
		p_page->dataSize	= size - sizeof(idHeap::page_s);
		p_page->firstFree	= NULL;
		p_page->largestFree	= 0;
		OSAllocs++;
	}

	p_page->prev = NULL;
	p_page->next = NULL;

	pagesAllocated++;
	
	return p_page;
}

/*
================
idHeap::FreePage

  frees a page back to the operating system
  p	= pointer to page
================
*/
void idHeap::FreePage( idHeap::page_s *p_page ) {
	assert( p_page );

	if ( p_page->dataSize == pageSize && !swapPage ) {			// add to swap list?
		swapPage = p_page;
	}
	else {
		FreePageReal( p_page );
	}

	pagesAllocated--;
}

//===============================================================
//
//	small heap code
//
//===============================================================

/*
================
idHeap::SmallAllocate

  allocate memory (1-255 bytes) from the small heap manager
  bytes = number of bytes to allocate
  returns pointer to allocated memory
================
*/
void *idHeap::SmallAllocate( dword bytes ) {
	// we need at least sizeof( uintptr_t ) bytes for the free list pointer
	if ( bytes < sizeof( uintptr_t ) ) {
		bytes = sizeof( uintptr_t );
	}

	// increase the number of bytes if necessary to make sure the next small allocation is aligned
	bytes = SMALL_ALIGN( bytes );

	byte *p_smallBlock = (byte *)(smallFirstFree[bytes / ALIGN]);
	if ( p_smallBlock ) {
		uintptr_t *p_link = (uintptr_t *)(p_smallBlock + SMALL_HEADER_SIZE);
		p_smallBlock[1] = SMALL_ALLOC;					// allocation identifier
		smallFirstFree[bytes / ALIGN] = reinterpret_cast<void *>( *p_link );
		return (void *)(p_link);
	}

	dword bytesLeft = pageSize - smallCurPageOffset;
	// if we need to allocate a new page
	if ( bytes >= bytesLeft ) {

		smallCurPage->next	= smallFirstUsedPage;
		smallFirstUsedPage	= smallCurPage;
		smallCurPage		= AllocatePage( pageSize );
		if ( !smallCurPage ) {
			return NULL;
		}
		// make sure the first allocation is aligned
		smallCurPageOffset	= SMALL_ALIGN( 0 );
	}

	p_smallBlock		= ((byte *)smallCurPage->p_data) + smallCurPageOffset;
	p_smallBlock[0]		= (byte)(bytes / ALIGN);		// write # of bytes/ALIGN
	p_smallBlock[1]		= SMALL_ALLOC;					// allocation identifier
	smallCurPageOffset  += bytes + SMALL_HEADER_SIZE;	// increase the offset on the current page
	return ( p_smallBlock + SMALL_HEADER_SIZE );			// skip the first two bytes
}

/*
================
idHeap::SmallFree

  frees a block of memory allocated by SmallAllocate() call
  p_ptr = pointer to block of memory
================
*/
void idHeap::SmallFree( void *p_ptr ) {
	((byte *)(p_ptr))[-1] = INVALID_ALLOC;

	byte *p_data = ( (byte *)p_ptr ) - SMALL_HEADER_SIZE;
	uintptr_t *p_nextFree = (uintptr_t *)p_ptr;
	// index into the table with free small memory blocks
	dword ix = *p_data;

	// check if the index is correct
	if ( ix > (256 / ALIGN) ) {
		idLib::common->FatalError( "SmallFree: invalid memory block" );
	}

	*p_nextFree = reinterpret_cast<uintptr_t>( smallFirstFree[ix] );	// write next pointer
	smallFirstFree[ix] = (void *)p_data;		// link
}

//===============================================================
//
//	medium heap code
//
//	Medium-heap allocated pages not returned to OS until heap destructor
//	called (re-used instead on subsequent medium-size malloc requests).
//
//===============================================================

/*
================
idHeap::MediumAllocateFromPage

  performs allocation using the medium heap manager from a given page
  p_page		= page
  sizeNeeded	= # of bytes needed
  returns pointer to allocated memory
================
*/
void *idHeap::MediumAllocateFromPage( idHeap::page_s *p_page, dword sizeNeeded ) {

	mediumHeapEntry_s	*p_best, *p_newEntry = NULL;
	byte				*p_ret;

	p_best = (mediumHeapEntry_s *)(p_page->firstFree);			// first block is largest

	assert( p_best );
	assert( p_best->size == p_page->largestFree );
	assert( p_best->size >= sizeNeeded );

	// if we can allocate another block from this page after allocating sizeNeeded bytes
	if ( p_best->size >= (dword)( sizeNeeded + MEDIUM_SMALLEST_SIZE ) ) {
		p_newEntry = (mediumHeapEntry_s *)((byte *)p_best + p_best->size - sizeNeeded);
		p_newEntry->page		= p_page;
		p_newEntry->prev		= p_best;
		p_newEntry->next		= p_best->next;
		p_newEntry->prevFree	= NULL;
		p_newEntry->nextFree	= NULL;
		p_newEntry->size		= sizeNeeded;
		p_newEntry->freeBlock	= 0;			// used block
		if ( p_best->next ) {
			p_best->next->prev = p_newEntry;
		}
		p_best->next	= p_newEntry;
		p_best->size	-= sizeNeeded;
		
		p_page->largestFree = p_best->size;
	}
	else {
		if ( p_best->prevFree ) {
			p_best->prevFree->nextFree = p_best->nextFree;
		}
		else {
			p_page->firstFree = (void *)p_best->nextFree;
		}
		if ( p_best->nextFree ) {
			p_best->nextFree->prevFree = p_best->prevFree;
		}

		p_best->prevFree  = NULL;
		p_best->nextFree  = NULL;
		p_best->freeBlock = 0;			// used block
		p_newEntry = p_best;

		p_page->largestFree = 0;
	}

	p_ret		= (byte *)(p_newEntry) + ALIGN_SIZE( MEDIUM_HEADER_SIZE );
	p_ret[-1] = MEDIUM_ALLOC;		// allocation identifier

	return (void *)(p_ret);
}

/*
================
idHeap::MediumAllocate

  allocate memory (256-32768 bytes) from medium heap manager
  bytes	= number of bytes to allocate
  returns pointer to allocated memory
================
*/
void *idHeap::MediumAllocate( dword bytes ) {
	idHeap::page_s		*p_page;
	void				*p_data;

	dword sizeNeeded = ALIGN_SIZE( bytes ) + ALIGN_SIZE( MEDIUM_HEADER_SIZE );

	// find first page with enough space
	for ( p_page = mediumFirstFreePage; p_page; p_page = p_page->next ) {
		if ( p_page->largestFree >= sizeNeeded ) {
			break;
		}
	}

	if ( !p_page ) {								// need to allocate new page?
		p_page = AllocatePage( pageSize );
		if ( !p_page ) {
			return NULL;					// malloc failure!
		}
		p_page->prev		= NULL;
		p_page->next		= mediumFirstFreePage;
		if (p_page->next) {
			p_page->next->prev = p_page;
		}
		else {
			mediumLastFreePage	= p_page;
		}

		mediumFirstFreePage		= p_page;
		
		p_page->largestFree	= pageSize;
		p_page->firstFree	= (void *)p_page->p_data;

		mediumHeapEntry_s *p_entry;
		p_entry				= (mediumHeapEntry_s *)(p_page->firstFree);
		p_entry->page		= p_page;
		// make sure ((byte *)p_entry + p_entry->size) is aligned
		p_entry->size		= pageSize & ~(ALIGN - 1);
		p_entry->prev		= NULL;
		p_entry->next		= NULL;
		p_entry->prevFree	= NULL;
		p_entry->nextFree	= NULL;
		p_entry->freeBlock	= 1;
	}

	p_data = MediumAllocateFromPage( p_page, sizeNeeded );		// allocate data from page

    // if the page can no longer serve memory, move it away from free list
	// (so that it won't slow down the later alloc queries)
	// this modification speeds up the pageWalk from O(N) to O(sqrt(N))
	// a call to free may swap this page back to the free list

	if ( p_page->largestFree < MEDIUM_SMALLEST_SIZE ) {
		if ( p_page == mediumLastFreePage ) {
			mediumLastFreePage = p_page->prev;
		}

		if ( p_page == mediumFirstFreePage ) {
			mediumFirstFreePage = p_page->next;
		}

		if ( p_page->prev ) {
			p_page->prev->next = p_page->next;
		}
		if ( p_page->next ) {
			p_page->next->prev = p_page->prev;
		}

		// link to "completely used" list
		p_page->prev = NULL;
		p_page->next = mediumFirstUsedPage;
		if ( p_page->next ) {
			p_page->next->prev = p_page;
		}
		mediumFirstUsedPage = p_page;
		return p_data;
	} 

	// re-order linked list (so that next malloc query starts from current
	// matching block) -- this speeds up both the page walks and block walks

	if ( p_page != mediumFirstFreePage ) {
		assert( mediumLastFreePage );
		assert( mediumFirstFreePage );
		assert( p_page->prev);

		mediumLastFreePage->next	= mediumFirstFreePage;
		mediumFirstFreePage->prev	= mediumLastFreePage;
		mediumLastFreePage			= p_page->prev;
		p_page->prev->next			= NULL;
		p_page->prev				= NULL;
		mediumFirstFreePage			= p_page;
	}

	return p_data;
}

/*
================
idHeap::MediumFree

  frees a block allocated by the medium heap manager
  p_ptr	= pointer to data block
================
*/
void idHeap::MediumFree( void *p_ptr ) {
	((byte *)(p_ptr))[-1] = INVALID_ALLOC;

	mediumHeapEntry_s	*p_entry = (mediumHeapEntry_s *)((byte *)p_ptr - ALIGN_SIZE( MEDIUM_HEADER_SIZE ));
	idHeap::page_s		*p_page = p_entry->page;
	bool				isInFreeList;

	isInFreeList = p_page->largestFree >= MEDIUM_SMALLEST_SIZE;

	assert( p_entry->size );
	assert( p_entry->freeBlock == 0 );

	mediumHeapEntry_s *p_prev = p_entry->prev;

	// if the previous block is free we can merge
	if ( p_prev && p_prev->freeBlock ) {
		p_prev->size += p_entry->size;
		p_prev->next = p_entry->next;
		if ( p_entry->next ) {
			p_entry->next->prev = p_prev;
		}
		p_entry = p_prev;
	}
	else {
		p_entry->prevFree		= NULL;				// link to beginning of free list
		p_entry->nextFree		= (mediumHeapEntry_s *)p_page->firstFree;
		if ( p_entry->nextFree ) {
			assert( !(p_entry->nextFree->prevFree) );
			p_entry->nextFree->prevFree = p_entry;
		}

		p_page->firstFree	= p_entry;
		p_page->largestFree	= p_entry->size;
		p_entry->freeBlock	= 1;				// mark block as free
	}
			
	mediumHeapEntry_s *p_next = p_entry->next;

	// if the next block is free we can merge
	if ( p_next && p_next->freeBlock ) {
		p_entry->size += p_next->size;
		p_entry->next = p_next->next;
		
		if ( p_next->next ) {
			p_next->next->prev = p_entry;
		}
		
		if ( p_next->prevFree ) {
			p_next->prevFree->nextFree = p_next->nextFree;
		}
		else {
			assert( p_next == p_page->firstFree );
			p_page->firstFree = p_next->nextFree;
		}

		if ( p_next->nextFree ) {
			p_next->nextFree->prevFree = p_next->prevFree;
		}
	}

	if ( p_page->firstFree ) {
		p_page->largestFree = ((mediumHeapEntry_s *)(p_page->firstFree))->size;
	}
	else {
		p_page->largestFree = 0;
	}

	// did p_entry become the largest block of the page ?

	if ( p_entry->size > p_page->largestFree ) {
		assert( p_entry != p_page->firstFree );
		p_page->largestFree = p_entry->size;

		if ( p_entry->prevFree ) {
			p_entry->prevFree->nextFree = p_entry->nextFree;
		}
		if ( p_entry->nextFree ) {
			p_entry->nextFree->prevFree = p_entry->prevFree;
		}
		
		p_entry->nextFree = (mediumHeapEntry_s *)p_page->firstFree;
		p_entry->prevFree = NULL;
		if ( p_entry->nextFree ) {
			p_entry->nextFree->prevFree = p_entry;
		}
		p_page->firstFree = p_entry;
	}

	// if page wasn't in free list (because it was near-full), move it back there
	if ( !isInFreeList ) {

		// remove from "completely used" list
		if ( p_page->prev ) {
			p_page->prev->next = p_page->next;
		}
		if ( p_page->next ) {
			p_page->next->prev = p_page->prev;
		}
		if ( p_page == mediumFirstUsedPage ) {
			mediumFirstUsedPage = p_page->next;
		}

		p_page->next = NULL;
		p_page->prev = mediumLastFreePage;

		if ( mediumLastFreePage ) {
			mediumLastFreePage->next = p_page;
		}
		mediumLastFreePage = p_page;
		if ( !mediumFirstFreePage ) {
			mediumFirstFreePage = p_page;
		}
	} 
}

//===============================================================
//
//	large heap code
//
//===============================================================

/*
================
idHeap::LargeAllocate

  allocates a block of memory from the operating system
  bytes	= number of bytes to allocate
  returns pointer to allocated memory
================
*/
void *idHeap::LargeAllocate( dword bytes ) {
	idHeap::page_s *p_page = AllocatePage( bytes + ALIGN_SIZE( LARGE_HEADER_SIZE ) );

	assert( p_page );

	if ( !p_page ) {
		return NULL;
	}

	byte *		p_data		= (byte*)(p_page->p_data) + ALIGN_SIZE( LARGE_HEADER_SIZE );
	uintptr_t *	p_pageLink	= (uintptr_t *)(p_data - ALIGN_SIZE( LARGE_HEADER_SIZE ));
	p_pageLink[0]			= reinterpret_cast<uintptr_t>( p_page );	// write pointer back to page table
	p_data[-1]				= LARGE_ALLOC;								// allocation identifier

	// link to 'large used page list'
	p_page->prev = NULL;
	p_page->next = largeFirstUsedPage;
	if ( p_page->next ) {
		p_page->next->prev = p_page;
	}
	largeFirstUsedPage = p_page;

	return (void *)(p_data);
}

/*
================
idHeap::LargeFree

  frees a block of memory allocated by the 'large memory allocator'
  p_ptr	= pointer to allocated memory
================
*/
void idHeap::LargeFree( void *p_ptr) {
	idHeap::page_s*	p_page;

	((byte *)(p_ptr))[-1] = INVALID_ALLOC;

	// get page pointer
	p_page = reinterpret_cast<idHeap::page_s *>( *( reinterpret_cast<uintptr_t *>( ((byte *)p_ptr) - ALIGN_SIZE( LARGE_HEADER_SIZE ) ) ) );

	// unlink from doubly linked list
	if ( p_page->prev ) {
		p_page->prev->next = p_page->next;
	}
	if ( p_page->next ) {
		p_page->next->prev = p_page->prev;
	}
	if ( p_page == largeFirstUsedPage ) {
		largeFirstUsedPage = p_page->next;
	}
	p_page->next = p_page->prev = NULL;

	FreePage(p_page);
}

//===============================================================
//
//	memory allocation all in one place
//
//===============================================================

#undef new

static idHeap *			mem_heap = NULL;
static memoryStats_t	mem_total_allocs = { 0, 0x0fffffff, -1, 0 };
static memoryStats_t	mem_frame_allocs;
static memoryStats_t	mem_frame_frees;

/*
==================
Mem_ClearFrameStats
==================
*/
void Mem_ClearFrameStats( void ) {
	mem_frame_allocs.num = mem_frame_frees.num = 0;
	mem_frame_allocs.minSize = mem_frame_frees.minSize = 0x0fffffff;
	mem_frame_allocs.maxSize = mem_frame_frees.maxSize = -1;
	mem_frame_allocs.totalSize = mem_frame_frees.totalSize = 0;
}

/*
==================
Mem_GetFrameStats
==================
*/
void Mem_GetFrameStats( memoryStats_t &allocs, memoryStats_t &frees ) {
	allocs = mem_frame_allocs;
	frees = mem_frame_frees;
}

/*
==================
Mem_GetStats
==================
*/
void Mem_GetStats( memoryStats_t &stats ) {
	stats = mem_total_allocs;
}

/*
==================
Mem_UpdateStats
==================
*/
void Mem_UpdateStats( memoryStats_t &stats, int size ) {
	stats.num++;
	if ( size < stats.minSize ) {
		stats.minSize = size;
	}
	if ( size > stats.maxSize ) {
		stats.maxSize = size;
	}
	stats.totalSize += size;
}

/*
==================
Mem_UpdateAllocStats
==================
*/
void Mem_UpdateAllocStats( int size ) {
	Mem_UpdateStats( mem_frame_allocs, size );
	Mem_UpdateStats( mem_total_allocs, size );
}

/*
==================
Mem_UpdateFreeStats
==================
*/
void Mem_UpdateFreeStats( int size ) {
	Mem_UpdateStats( mem_frame_frees, size );
	mem_total_allocs.num--;
	mem_total_allocs.totalSize -= size;
}


#ifndef ID_DEBUG_MEMORY

/*
==================
Mem_Alloc
==================
*/
void *Mem_Alloc( const int size ) {
	if ( !size ) {
		return NULL;
	}
	if ( !mem_heap ) {
#ifdef CRASH_ON_STATIC_ALLOCATION
		*((int*)0x0) = 1;
#endif
		return malloc( size );
	}
	void *p_mem = mem_heap->Allocate( size );
	Mem_UpdateAllocStats( mem_heap->Msize( p_mem ) );
	return p_mem;
}

/*
==================
Mem_Free
==================
*/
void Mem_Free( void *p_ptr ) {
	if ( !p_ptr ) {
		return;
	}
	if ( !mem_heap ) {
#ifdef CRASH_ON_STATIC_ALLOCATION
		*((int*)0x0) = 1;
#endif
		free( p_ptr );
		return;
	}
	Mem_UpdateFreeStats( mem_heap->Msize( p_ptr ) );
	mem_heap->Free( p_ptr );
}

/*
==================
Mem_Alloc16
==================
*/
void *Mem_Alloc16( const int size ) {
	if ( !size ) {
		return NULL;
	}
	if ( !mem_heap ) {
#ifdef CRASH_ON_STATIC_ALLOCATION
		*((int*)0x0) = 1;
#endif
		return malloc( size );
	}
	void *p_mem = mem_heap->Allocate16( size );
	// make sure the memory is 16 byte aligned
	assert( ( reinterpret_cast<uintptr_t>( p_mem ) & 15 ) == 0 );
	return p_mem;
}

/*
==================
Mem_Free16
==================
*/
void Mem_Free16( void *p_ptr ) {
	if ( !p_ptr ) {
		return;
	}
	if ( !mem_heap ) {
#ifdef CRASH_ON_STATIC_ALLOCATION
		*((int*)0x0) = 1;
#endif
		free( p_ptr );
		return;
	}
	// make sure the memory is 16 byte aligned
	assert( ( reinterpret_cast<uintptr_t>( p_ptr ) & 15 ) == 0 );
	mem_heap->Free16( p_ptr );
}

/*
==================
Mem_ClearedAlloc
==================
*/
void *Mem_ClearedAlloc( const int size ) {
	void *p_mem = Mem_Alloc( size );
	SIMDProcessor->Memset( p_mem, 0, size );
	return p_mem;
}

/*
==================
Mem_ClearedAlloc
==================
*/
void Mem_AllocDefragBlock( void ) {
	mem_heap->AllocDefragBlock();
}

/*
==================
Mem_CopyString
==================
*/
char *Mem_CopyString( const char *in ) {
	char	*out;
	
	out = (char *)Mem_Alloc( strlen(in) + 1 );
	strcpy( out, in );
	return out;
}

/*
==================
Mem_Dump_f
==================
*/
void Mem_Dump_f( const idCmdArgs &args ) {
}

/*
==================
Mem_DumpCompressed_f
==================
*/
void Mem_DumpCompressed_f( const idCmdArgs &args ) {
}

/*
==================
Mem_Init
==================
*/
void Mem_Init( void ) {
	mem_heap = new idHeap;
	Mem_ClearFrameStats();
}

/*
==================
Mem_Shutdown
==================
*/
void Mem_Shutdown( void ) {
	idHeap *m = mem_heap;
	mem_heap = NULL;
	delete m;
}

/*
==================
Mem_EnableLeakTest
==================
*/
void Mem_EnableLeakTest( const char *name ) {
}


#else /* !ID_DEBUG_MEMORY */

#undef		Mem_Alloc
#undef		Mem_ClearedAlloc
#undef		Com_ClearedReAlloc
#undef		Mem_Free
#undef		Mem_CopyString
#undef		Mem_Alloc16
#undef		Mem_Free16

#define MAX_CALLSTACK_DEPTH		6

// size of this struct must be a multiple of 16 bytes
typedef struct debugMemory_s {
	const char *			fileName;
	int						lineNumber;
	int						frameNumber;
	int						size;
	address_t				callStack[MAX_CALLSTACK_DEPTH];
	struct debugMemory_s *	prev;
	struct debugMemory_s *	next;
} debugMemory_t;

static debugMemory_t *	mem_debugMemory = NULL;
static char				mem_leakName[256] = "";

/*
==================
Mem_CleanupFileName
==================
*/
const char *Mem_CleanupFileName( const char *fileName ) {
	int i1, i2;
	idStr newFileName;
	static char newFileNames[4][MAX_STRING_CHARS];
	static int index;

	newFileName = fileName;
	newFileName.BackSlashesToSlashes();
	i1 = newFileName.Find( "neo", false );
	if ( i1 >= 0 ) {
		i1 = newFileName.Find( "/", false, i1 );
		newFileName = newFileName.Right( newFileName.Length() - ( i1 + 1 ) );
	}
	while( 1 ) {
		i1 = newFileName.Find( "/../" );
		if ( i1 <= 0 ) {
			break;
		}
		i2 = i1 - 1;
		while( i2 > 1 && newFileName[i2-1] != '/' ) {
			i2--;
		}
		newFileName = newFileName.Left( i2 - 1 ) + newFileName.Right( newFileName.Length() - ( i1 + 4 ) );
	}
	index = ( index + 1 ) & 3;
	strncpy( newFileNames[index], newFileName.c_str(), sizeof( newFileNames[index] ) );
	return newFileNames[index];
}

/*
==================
Mem_Dump
==================
*/
void Mem_Dump( const char *fileName ) {
	int i, numBlocks, totalSize;
	char dump[32], *ptr;
	debugMemory_t *b;
	idStr module, funcName;
	FILE *f;

	f = fopen( fileName, "wb" );
	if ( !f ) {
		return;
	}

	totalSize = 0;
	for ( numBlocks = 0, b = mem_debugMemory; b; b = b->next, numBlocks++ ) {
		ptr = ((char *) b) + sizeof(debugMemory_t);
		totalSize += b->size;
		for ( i = 0; i < (sizeof(dump)-1) && i < b->size; i++) {
			if ( ptr[i] >= 32 && ptr[i] < 127 ) {
				dump[i] = ptr[i];
			} else {
				dump[i] = '_';
			}
		}
		dump[i] = '\0';
		if ( ( b->size >> 10 ) != 0 ) {
			fprintf( f, "size: %6d KB: %s, line: %d [%s], call stack: %s\r\n", ( b->size >> 10 ), Mem_CleanupFileName(b->fileName), b->lineNumber, dump, idLib::sys->GetCallStackStr( b->callStack, MAX_CALLSTACK_DEPTH ) );
		}
		else {
			fprintf( f, "size: %7d B: %s, line: %d [%s], call stack: %s\r\n", b->size, Mem_CleanupFileName(b->fileName), b->lineNumber, dump, idLib::sys->GetCallStackStr( b->callStack, MAX_CALLSTACK_DEPTH ) );
		}
	}

	idLib::sys->ShutdownSymbols();

	fprintf( f, "%8d total memory blocks allocated\r\n", numBlocks );
	fprintf( f, "%8d KB memory allocated\r\n", ( totalSize >> 10 ) );

	fclose( f );
}

/*
==================
Mem_Dump_f
==================
*/
void Mem_Dump_f( const idCmdArgs &args ) {
	const char *fileName;

	if ( args.Argc() >= 2 ) {
		fileName = args.Argv( 1 );
	}
	else {
		fileName = "memorydump.txt";
	}
	Mem_Dump( fileName );
}

/*
==================
Mem_DumpCompressed
==================
*/
typedef struct allocInfo_s {
	const char *			fileName;
	int						lineNumber;
	int						size;
	int						numAllocs;
	address_t				callStack[MAX_CALLSTACK_DEPTH];
	struct allocInfo_s *	next;
} allocInfo_t;

typedef enum {
	MEMSORT_SIZE,
	MEMSORT_LOCATION,
	MEMSORT_NUMALLOCS,
	MEMSORT_CALLSTACK
} memorySortType_t;

void Mem_DumpCompressed( const char *fileName, memorySortType_t memSort, int sortCallStack, int numFrames ) {
	int numBlocks, totalSize, r, j;
	debugMemory_t *b;
	allocInfo_t *a, *nexta, *allocInfo = NULL, *sortedAllocInfo = NULL, *prevSorted, *nextSorted;
	idStr module, funcName;
	FILE *f;

	// build list with memory allocations
	totalSize = 0;
	numBlocks = 0;
	for ( b = mem_debugMemory; b; b = b->next ) {

		if ( numFrames && b->frameNumber < idLib::frameNumber - numFrames ) {
			continue;
		}

		numBlocks++;
		totalSize += b->size;

		// search for an allocation from the same source location
		for ( a = allocInfo; a; a = a->next ) {
			if ( a->lineNumber != b->lineNumber ) {
				continue;
			}
			for ( j = 0; j < MAX_CALLSTACK_DEPTH; j++ ) {
				if ( a->callStack[j] != b->callStack[j] ) {
					break;
				}
			}
			if ( j < MAX_CALLSTACK_DEPTH ) {
				continue;
			}
			if ( idStr::Cmp( a->fileName, b->fileName ) != 0 ) {
				continue;
			}
			a->numAllocs++;
			a->size += b->size;
			break;
		}

		// if this is an allocation from a new source location
		if ( !a ) {
			a = (allocInfo_t *) ::malloc( sizeof( allocInfo_t ) );
			a->fileName = b->fileName;
			a->lineNumber = b->lineNumber;
			a->size = b->size;
			a->numAllocs = 1;
			for ( j = 0; j < MAX_CALLSTACK_DEPTH; j++ ) {
				a->callStack[j] = b->callStack[j];
			}
			a->next = allocInfo;
			allocInfo = a;
		}
	}

	// sort list
	for ( a = allocInfo; a; a = nexta ) {
		nexta = a->next;

		prevSorted = NULL;
		switch( memSort ) {
			// sort on size
			case MEMSORT_SIZE: {
				for ( nextSorted = sortedAllocInfo; nextSorted; nextSorted = nextSorted->next ) {
					if ( a->size > nextSorted->size ) {
						break;
					}
					prevSorted = nextSorted;
				}
				break;
			}
			// sort on file name and line number
			case MEMSORT_LOCATION: {
				for ( nextSorted = sortedAllocInfo; nextSorted; nextSorted = nextSorted->next ) {
					r = idStr::Cmp( Mem_CleanupFileName( a->fileName ), Mem_CleanupFileName( nextSorted->fileName ) );
					if ( r < 0 || ( r == 0 && a->lineNumber < nextSorted->lineNumber ) ) {
						break;
					}
					prevSorted = nextSorted;
				}
				break;
			}
			// sort on the number of allocations
			case MEMSORT_NUMALLOCS: {
				for ( nextSorted = sortedAllocInfo; nextSorted; nextSorted = nextSorted->next ) {
					if ( a->numAllocs > nextSorted->numAllocs ) {
						break;
					}
					prevSorted = nextSorted;
				}
				break;
			}
			// sort on call stack
			case MEMSORT_CALLSTACK: {
				for ( nextSorted = sortedAllocInfo; nextSorted; nextSorted = nextSorted->next ) {
					if ( a->callStack[sortCallStack] < nextSorted->callStack[sortCallStack] ) {
						break;
					}
					prevSorted = nextSorted;
				}
				break;
			}
		}
		if ( !prevSorted ) {
			a->next = sortedAllocInfo;
			sortedAllocInfo = a;
		}
		else {
			prevSorted->next = a;
			a->next = nextSorted;
		}
	}

	f = fopen( fileName, "wb" );
	if ( !f ) {
		return;
	}

	// write list to file
	for ( a = sortedAllocInfo; a; a = nexta ) {
		nexta = a->next;
		fprintf( f, "size: %6d KB, allocs: %5d: %s, line: %d, call stack: %s\r\n",
					(a->size >> 10), a->numAllocs, Mem_CleanupFileName(a->fileName),
							a->lineNumber, idLib::sys->GetCallStackStr( a->callStack, MAX_CALLSTACK_DEPTH ) );
		::free( a );
	}

	idLib::sys->ShutdownSymbols();

	fprintf( f, "%8d total memory blocks allocated\r\n", numBlocks );
	fprintf( f, "%8d KB memory allocated\r\n", ( totalSize >> 10 ) );

	fclose( f );
}

/*
==================
Mem_DumpCompressed_f
==================
*/
void Mem_DumpCompressed_f( const idCmdArgs &args ) {
	int argNum;
	const char *arg, *fileName;
	memorySortType_t memSort = MEMSORT_LOCATION;
	int sortCallStack = 0, numFrames = 0;

	// get cmd-line options
	argNum = 1;
	arg = args.Argv( argNum );
	while( arg[0] == '-' ) {
		arg = args.Argv( ++argNum );
		if ( idStr::Icmp( arg, "s" ) == 0 ) {
			memSort = MEMSORT_SIZE;
		} else if ( idStr::Icmp( arg, "l" ) == 0 ) {
			memSort = MEMSORT_LOCATION;
		} else if ( idStr::Icmp( arg, "a" ) == 0 ) {
			memSort = MEMSORT_NUMALLOCS;
		} else if ( idStr::Icmp( arg, "cs1" ) == 0 ) {
			memSort = MEMSORT_CALLSTACK;
			sortCallStack = 2;
		} else if ( idStr::Icmp( arg, "cs2" ) == 0 ) {
			memSort = MEMSORT_CALLSTACK;
			sortCallStack = 1;
		} else if ( idStr::Icmp( arg, "cs3" ) == 0 ) {
			memSort = MEMSORT_CALLSTACK;
			sortCallStack = 0;
		} else if ( arg[0] == 'f' ) {
			numFrames = atoi( arg + 1 );
		} else {
			idLib::common->Printf( "memoryDumpCompressed [options] [filename]\n"
						"options:\n"
						"  -s     sort on size\n"
						"  -l     sort on location\n"
						"  -a     sort on the number of allocations\n"
						"  -cs1   sort on first function on call stack\n"
						"  -cs2   sort on second function on call stack\n"
						"  -cs3   sort on third function on call stack\n"
						"  -f<X>  only report allocations the last X frames\n"
						"By default the memory allocations are sorted on location.\n"
						"By default a 'memorydump.txt' is written if no file name is specified.\n" );
			return;
		}
		arg = args.Argv( ++argNum );
	}
	if ( argNum >= args.Argc() ) {
		fileName = "memorydump.txt";
	} else {
		fileName = arg;
	}
	Mem_DumpCompressed( fileName, memSort, sortCallStack, numFrames );
}

/*
==================
Mem_AllocDebugMemory
==================
*/
void *Mem_AllocDebugMemory( const int size, const char *fileName, const int lineNumber, const bool align16 ) {
	void *p_mem;
	debugMemory_t *p_debugMemory;

	if ( !size ) {
		return NULL;
	}

	if ( !mem_heap ) {
#ifdef CRASH_ON_STATIC_ALLOCATION
		*((int*)0x0) = 1;
#endif
		// NOTE: set a breakpoint here to find memory allocations before mem_heap is initialized
		return malloc( size );
	}

	if ( align16 ) {
		p_mem = mem_heap->Allocate16( size + sizeof( debugMemory_t ) );
	}
	else {
		p_mem = mem_heap->Allocate( size + sizeof( debugMemory_t ) );
	}

	Mem_UpdateAllocStats( size );

	p_debugMemory = (debugMemory_t *) p_mem;
	p_debugMemory->fileName = fileName;
	p_debugMemory->lineNumber = lineNumber;
	p_debugMemory->frameNumber = idLib::frameNumber;
	p_debugMemory->size = size;
	p_debugMemory->next = mem_debugMemory;
	p_debugMemory->prev = NULL;
	if ( mem_debugMemory ) {
		mem_debugMemory->prev = p_debugMemory;
	}
	mem_debugMemory = p_debugMemory;
	idLib::sys->GetCallStack( p_debugMemory->callStack, MAX_CALLSTACK_DEPTH );

	return ( ( (byte *) p_mem ) + sizeof( debugMemory_t ) );
}

/*
==================
Mem_FreeDebugMemory
==================
*/
void Mem_FreeDebugMemory( void *p_mem, const char *fileName, const int lineNumber, const bool align16 ) {
	debugMemory_t *p_debugMemory;

	if ( !p_mem ) {
		return;
	}

	if ( !mem_heap ) {
#ifdef CRASH_ON_STATIC_ALLOCATION
		*((int*)0x0) = 1;
#endif
		// NOTE: set a breakpoint here to find memory being freed before mem_heap is initialized
		free( p_mem );
		return;
	}

	p_debugMemory = (debugMemory_t *) ( ( (byte *) p_mem ) - sizeof( debugMemory_t ) );

	if ( p_debugMemory->size < 0 ) {
		idLib::common->FatalError( "memory freed twice, first from %s, now from %s", idLib::sys->GetCallStackStr( p_debugMemory->callStack, MAX_CALLSTACK_DEPTH ), idLib::sys->GetCallStackCurStr( MAX_CALLSTACK_DEPTH ) );
	}

	Mem_UpdateFreeStats( p_debugMemory->size );

	if ( p_debugMemory->next ) {
		p_debugMemory->next->prev = p_debugMemory->prev;
	}
	if ( p_debugMemory->prev ) {
		p_debugMemory->prev->next = p_debugMemory->next;
	}
	else {
		mem_debugMemory = p_debugMemory->next;
	}

	p_debugMemory->fileName = fileName;
	p_debugMemory->lineNumber = lineNumber;
	p_debugMemory->frameNumber = idLib::frameNumber;
	p_debugMemory->size = -p_debugMemory->size;
	idLib::sys->GetCallStack( p_debugMemory->callStack, MAX_CALLSTACK_DEPTH );

	if ( align16 ) {
		mem_heap->Free16( p_debugMemory );
	}
	else {
		mem_heap->Free( p_debugMemory );
	}
}

/*
==================
Mem_Alloc
==================
*/
void *Mem_Alloc( const int size, const char *fileName, const int lineNumber ) {
	if ( !size ) {
		return NULL;
	}
	return Mem_AllocDebugMemory( size, fileName, lineNumber, false );
}

/*
==================
Mem_Free
==================
*/
void Mem_Free( void *p_ptr, const char *fileName, const int lineNumber ) {
	if ( !p_ptr ) {
		return;
	}
	Mem_FreeDebugMemory( p_ptr, fileName, lineNumber, false );
}

/*
==================
Mem_Alloc16
==================
*/
void *Mem_Alloc16( const int size, const char *fileName, const int lineNumber ) {
	if ( !size ) {
		return NULL;
	}
	void *p_mem = Mem_AllocDebugMemory( size, fileName, lineNumber, true );
	// make sure the memory is 16 byte aligned
	assert( ( reinterpret_cast<uintptr_t>( p_mem ) & 15 ) == 0 );
	return p_mem;
}

/*
==================
Mem_Free16
==================
*/
void Mem_Free16( void *p_ptr, const char *fileName, const int lineNumber ) {
	if ( !p_ptr ) {
		return;
	}
	// make sure the memory is 16 byte aligned
	assert( ( reinterpret_cast<uintptr_t>( p_ptr ) & 15 ) == 0 );
	Mem_FreeDebugMemory( p_ptr, fileName, lineNumber, true );
}

/*
==================
Mem_ClearedAlloc
==================
*/
void *Mem_ClearedAlloc( const int size, const char *fileName, const int lineNumber ) {
	void *p_mem = Mem_Alloc( size, fileName, lineNumber );
	SIMDProcessor->Memset( p_mem, 0, size );
	return p_mem;
}

/*
==================
Mem_CopyString
==================
*/
char *Mem_CopyString( const char *in, const char *fileName, const int lineNumber ) {
	char	*out;
	
	out = (char *)Mem_Alloc( strlen(in) + 1, fileName, lineNumber );
	strcpy( out, in );
	return out;
}

/*
==================
Mem_Init
==================
*/
void Mem_Init( void ) {
	mem_heap = new idHeap;
}

/*
==================
Mem_Shutdown
==================
*/
void Mem_Shutdown( void ) {

	if ( mem_leakName[0] != '\0' ) {
		Mem_DumpCompressed( va( "%s_leak_size.txt", mem_leakName ), MEMSORT_SIZE, 0, 0 );
		Mem_DumpCompressed( va( "%s_leak_location.txt", mem_leakName ), MEMSORT_LOCATION, 0, 0 );
		Mem_DumpCompressed( va( "%s_leak_cs1.txt", mem_leakName ), MEMSORT_CALLSTACK, 2, 0 );
	}

	idHeap *m = mem_heap;
	mem_heap = NULL;
	delete m;
}

/*
==================
Mem_EnableLeakTest
==================
*/
void Mem_EnableLeakTest( const char *name ) {
	idStr::Copynz( mem_leakName, name, sizeof( mem_leakName ) );
}

#endif /* !ID_DEBUG_MEMORY */
