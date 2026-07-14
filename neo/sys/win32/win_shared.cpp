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

#include "../../idlib/precompiled.h"
#pragma hdrstop

#include "win_local.h"
#include <lmerr.h>
#include <lmcons.h>
#include <lmwksta.h>
#include <errno.h>
#include <fcntl.h>
#include <direct.h>
#include <io.h>
#include <conio.h>

#ifndef	ID_DEDICATED
#include <comdef.h>
#include <comutil.h>
#include <Wbemidl.h>

#pragma comment (lib, "wbemuuid.lib")
#endif

/*
================
Sys_Milliseconds
================
*/
int Sys_Milliseconds( void ) {
	int sys_curtime;
	static int sys_timeBase;
	static bool	initialized = false;

	if ( !initialized ) {
		sys_timeBase = timeGetTime();
		initialized = true;
	}
	sys_curtime = timeGetTime() - sys_timeBase;

	return sys_curtime;
}

/*
================
Sys_GetSystemRam

	returns amount of physical memory in MB
================
*/
int Sys_GetSystemRam( void ) {
	MEMORYSTATUSEX statex;
	statex.dwLength = sizeof ( statex );
	GlobalMemoryStatusEx (&statex);
	int physRam = statex.ullTotalPhys / ( 1024 * 1024 );
	// HACK: For some reason, ullTotalPhys is sometimes off by a meg or two, so we round up to the nearest 16 megs
	physRam = ( physRam + 8 ) & ~15;
	return physRam;
}


/*
================
Sys_GetDriveFreeSpace
returns in megabytes
================
*/
int Sys_GetDriveFreeSpace( const char *path ) {
	DWORDLONG lpFreeBytesAvailable;
	DWORDLONG lpTotalNumberOfBytes;
	DWORDLONG lpTotalNumberOfFreeBytes;
	int ret = 26;
	//FIXME: see why this is failing on some machines
	if ( ::GetDiskFreeSpaceEx( path, (PULARGE_INTEGER)&lpFreeBytesAvailable, (PULARGE_INTEGER)&lpTotalNumberOfBytes, (PULARGE_INTEGER)&lpTotalNumberOfFreeBytes ) ) {
		ret = ( double )( lpFreeBytesAvailable ) / ( 1024.0 * 1024.0 );
	}
	return ret;
}


/*
================
Sys_GetVideoRam
returns in megabytes
================
*/
int Sys_GetVideoRam( void ) {
#ifdef	ID_DEDICATED
	return 0;
#else
	unsigned int retSize = 64;

	IWbemLocator *p_locator = NULL;
	IWbemServices *p_services = NULL;
	IEnumWbemClassObject *p_enumInstances = NULL;
	IWbemClassObject *p_instance = NULL;
	BSTR p_namespace = NULL;
	BSTR p_className = NULL;
	BSTR p_adapterRam = NULL;
	VARIANT varSize;
	ULONG numInstances = 0;
	HRESULT hr;

	VariantInit( &varSize );

	hr = CoCreateInstance( CLSID_WbemLocator, 0, CLSCTX_SERVER, IID_IWbemLocator, reinterpret_cast<LPVOID *>( &p_locator ) );
	if ( hr != S_OK || p_locator == NULL ) {
		return retSize;
	}

	p_namespace = SysAllocString( L"\\\\.\\root\\CIMV2" );
	if ( p_namespace == NULL ) {
		p_locator->Release();
		return retSize;
	}

	// Connect to CIM
	hr = p_locator->ConnectServer( p_namespace, NULL, NULL, 0, NULL, 0, 0, &p_services );
	SysFreeString( p_namespace );
	p_namespace = NULL;
	if ( hr != WBEM_S_NO_ERROR ) {
		p_locator->Release();
		return retSize;
	}

	// Switch the security level to IMPERSONATE so that provider will grant access to system-level objects.  
	hr = CoSetProxyBlanket( p_services, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, NULL, RPC_C_AUTHN_LEVEL_CALL, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE );
	if ( hr != S_OK ) {
		p_services->Release();
		p_locator->Release();
		return retSize;
	}

	// Get the vid controller
	p_className = SysAllocString( L"Win32_VideoController" );
	if ( p_className == NULL ) {
		p_services->Release();
		p_locator->Release();
		return retSize;
	}
	hr = p_services->CreateInstanceEnum( p_className, WBEM_FLAG_SHALLOW, NULL, &p_enumInstances );
	SysFreeString( p_className );
	p_className = NULL;
	if ( hr != WBEM_S_NO_ERROR || p_enumInstances == NULL ) {
		p_services->Release();
		p_locator->Release();
		return retSize;
	}

	hr = p_enumInstances->Next( 10000, 1, &p_instance, &numInstances );

	if ( hr == S_OK && p_instance != NULL ) {
		// Get properties from the object
		p_adapterRam = SysAllocString( L"AdapterRAM" );
		hr = p_adapterRam != NULL ? p_instance->Get( p_adapterRam, 0, &varSize, 0, 0 ) : E_OUTOFMEMORY;
		if ( p_adapterRam != NULL ) {
			SysFreeString( p_adapterRam );
		}
		if ( hr == S_OK ) {
			retSize = varSize.ulVal / ( 1024 * 1024 );
			if ( retSize == 0 ) {
				retSize = 64;
			}
		}
	}
	VariantClear( &varSize );
	if ( p_instance != NULL ) {
		p_instance->Release();
	}
	p_enumInstances->Release();
	p_services->Release();
	p_locator->Release();
	return retSize;
#endif
}

/*
================
Sys_GetCurrentMemoryStatus

	returns OS mem info
	all values are in kB except the memoryload
================
*/
void Sys_GetCurrentMemoryStatus( sysMemoryStats_t &stats ) {
	MEMORYSTATUSEX statex;
	unsigned __int64 work;

	memset( &statex, sizeof( statex ), 0 );
	statex.dwLength = sizeof( statex );
	GlobalMemoryStatusEx( &statex );

	memset( &stats, 0, sizeof( stats ) );

	stats.memoryLoad = statex.dwMemoryLoad;

	work = statex.ullTotalPhys >> 20;
	stats.totalPhysical = *(int*)&work;

	work = statex.ullAvailPhys >> 20;
	stats.availPhysical = *(int*)&work;

	work = statex.ullAvailPageFile >> 20;
	stats.availPageFile = *(int*)&work;

	work = statex.ullTotalPageFile >> 20;
	stats.totalPageFile = *(int*)&work;

	work = statex.ullTotalVirtual >> 20;
	stats.totalVirtual = *(int*)&work;

	work = statex.ullAvailVirtual >> 20;
	stats.availVirtual = *(int*)&work;

	work = statex.ullAvailExtendedVirtual >> 20;
	stats.availExtendedVirtual = *(int*)&work;
}

/*
================
Sys_LockMemory
================
*/
bool Sys_LockMemory( void *p_ptr, int bytes ) {
	return ( VirtualLock( p_ptr, (SIZE_T)bytes ) != FALSE );
}

/*
================
Sys_UnlockMemory
================
*/
bool Sys_UnlockMemory( void *p_ptr, int bytes ) {
	return ( VirtualUnlock( p_ptr, (SIZE_T)bytes ) != FALSE );
}

/*
================
Sys_SetPhysicalWorkMemory
================
*/
void Sys_SetPhysicalWorkMemory( int minBytes, int maxBytes ) {
	::SetProcessWorkingSetSize( GetCurrentProcess(), minBytes, maxBytes );
}

/*
================
Sys_GetCurrentUser
================
*/
char *Sys_GetCurrentUser( void ) {
	static char s_userName[1024];
	unsigned long size = sizeof( s_userName );


	if ( !GetUserName( s_userName, &size ) ) {
		strcpy( s_userName, "player" );
	}

	if ( !s_userName[0] ) {
		strcpy( s_userName, "player" );
	}

	return s_userName;
}	


/*
===============================================================================

	Call stack

===============================================================================
*/


#define PROLOGUE_SIGNATURE 0x00EC8B55

#include <dbghelp.h>

const int UNDECORATE_FLAGS =	UNDNAME_NO_MS_KEYWORDS |
								UNDNAME_NO_ACCESS_SPECIFIERS |
								UNDNAME_NO_FUNCTION_RETURNS |
								UNDNAME_NO_ALLOCATION_MODEL |
								UNDNAME_NO_ALLOCATION_LANGUAGE |
								UNDNAME_NO_MEMBER_TYPE;

#if defined(_DEBUG) && 1

typedef struct symbol_s {
	address_t			address;
	char *				name;
	struct symbol_s *	next;
} symbol_t;

typedef struct module_s {
	address_t			address;
	char *				name;
	symbol_t *			symbols;
	struct module_s *	next;
} module_t;

module_t *modules;

/*
==================
SkipRestOfLine
==================
*/
void SkipRestOfLine( const char **ptr ) {
	while( (**ptr) != '\0' && (**ptr) != '\n' && (**ptr) != '\r' ) {
		(*ptr)++;
	}
	while( (**ptr) == '\n' || (**ptr) == '\r' ) {
		(*ptr)++;
	}
}

/*
==================
SkipWhiteSpace
==================
*/
void SkipWhiteSpace( const char **ptr ) {
	while( (**ptr) == ' ' ) {
		(*ptr)++;
	}
}

/*
==================
ParseHexNumber
==================
*/
int ParseHexNumber( const char **ptr ) {
	int n = 0;
	while( (**ptr) >= '0' && (**ptr) <= '9' || (**ptr) >= 'a' && (**ptr) <= 'f' ) {
		n <<= 4;
		if ( **ptr >= '0' && **ptr <= '9' ) {
			n |= ( (**ptr) - '0' );
		} else {
			n |= 10 + ( (**ptr) - 'a' );
		}
		(*ptr)++;
	}
	return n;
}

/*
==================
Sym_Init
==================
*/
void Sym_Init( address_t addr ) {
	TCHAR moduleName[MAX_STRING_CHARS];
	MEMORY_BASIC_INFORMATION mbi;

	VirtualQuery( reinterpret_cast<void *>( addr ), &mbi, sizeof(mbi) );

	GetModuleFileName( (HMODULE)mbi.AllocationBase, moduleName, sizeof( moduleName ) );

	char *ext = moduleName + strlen( moduleName );
	while( ext > moduleName && *ext != '.' ) {
		ext--;
	}
	if ( ext == moduleName ) {
		strcat( moduleName, ".map" );
	} else {
		strcpy( ext, ".map" );
	}

	module_t *module = (module_t *) malloc( sizeof( module_t ) );
	module->name = (char *) malloc( strlen( moduleName ) + 1 );
	strcpy( module->name, moduleName );
	module->address = reinterpret_cast<address_t>( mbi.AllocationBase );
	module->symbols = NULL;
	module->next = modules;
	modules = module;

	FILE *fp = fopen( moduleName, "rb" );
	if ( fp == NULL ) {
		return;
	}

	int pos = ftell( fp );
	fseek( fp, 0, SEEK_END );
	int length = ftell( fp );
	fseek( fp, pos, SEEK_SET );

	char *text = (char *) malloc( length+1 );
	fread( text, 1, length, fp );
	text[length] = '\0';
	fclose( fp );

	const char *ptr = text;

	// skip up to " Address" on a new line
	while( *ptr != '\0' ) {
		SkipWhiteSpace( &ptr );
		if ( idStr::Cmpn( ptr, "Address", 7 ) == 0 ) {
			SkipRestOfLine( &ptr );
			break;
		}
		SkipRestOfLine( &ptr );
	}

	int symbolAddress;
	int symbolLength;
	char symbolName[MAX_STRING_CHARS];
	symbol_t *symbol;

	// parse symbols
	while( *ptr != '\0' ) {

		SkipWhiteSpace( &ptr );

		ParseHexNumber( &ptr );
		if ( *ptr == ':' ) {
			ptr++;
		} else {
			break;
		}
		ParseHexNumber( &ptr );

		SkipWhiteSpace( &ptr );

		// parse symbol name
		symbolLength = 0;
		while( *ptr != '\0' && *ptr != ' ' ) {
			symbolName[symbolLength++] = *ptr++;
			if ( symbolLength >= sizeof( symbolName ) - 1 ) {
				break;
			}
		}
		symbolName[symbolLength++] = '\0';

		SkipWhiteSpace( &ptr );

		// parse symbol address
		symbolAddress = ParseHexNumber( &ptr );

		SkipRestOfLine( &ptr );

		symbol = (symbol_t *) malloc( sizeof( symbol_t ) );
		symbol->name = (char *) malloc( symbolLength );
		strcpy( symbol->name, symbolName );
		symbol->address = symbolAddress;
		symbol->next = module->symbols;
		module->symbols = symbol;
	}

	free( text );
}

/*
==================
Sym_Shutdown
==================
*/
void Sym_Shutdown( void ) {
	module_t *m;
	symbol_t *s;

	for ( m = modules; m != NULL; m = modules ) {
		modules = m->next;
		for ( s = m->symbols; s != NULL; s = m->symbols ) {
			m->symbols = s->next;
			free( s->name );
			free( s );
		}
		free( m->name );
		free( m );
	}
	modules = NULL;
}

/*
==================
Sym_GetFuncInfo
==================
*/
void Sym_GetFuncInfo( address_t addr, idStr &module, idStr &funcName ) {
	MEMORY_BASIC_INFORMATION mbi;
	module_t *m;
	symbol_t *s;

	VirtualQuery( reinterpret_cast<void *>( addr ), &mbi, sizeof(mbi) );

	for ( m = modules; m != NULL; m = m->next ) {
		if ( m->address == reinterpret_cast<address_t>( mbi.AllocationBase ) ) {
			break;
		}
	}
	if ( !m ) {
		Sym_Init( addr );
		m = modules;
	}

	for ( s = m->symbols; s != NULL; s = s->next ) {
		if ( s->address == addr ) {

			char undName[MAX_STRING_CHARS];
			if ( UnDecorateSymbolName( s->name, undName, sizeof(undName), UNDECORATE_FLAGS ) ) {
				funcName = undName;
			} else {
				funcName = s->name;
			}
			for ( int i = 0; i < funcName.Length(); i++ ) {
				if ( funcName[i] == '(' ) {
					funcName.CapLength( i );
					break;
				}
			}
			module = m->name;
			return;
		}
	}

	funcName = va( "0x%p", reinterpret_cast<void *>( addr ) );
	module = "";
}

#elif defined(_DEBUG)

address_t lastAllocationBase = static_cast<address_t>( -1 );
HANDLE processHandle;
idStr lastModule;

/*
==================
Sym_Init
==================
*/
void Sym_Init( address_t addr ) {
	TCHAR moduleName[MAX_STRING_CHARS];
	TCHAR modShortNameBuf[MAX_STRING_CHARS];
	MEMORY_BASIC_INFORMATION mbi;

	if ( lastAllocationBase != static_cast<address_t>( -1 ) ) {
		Sym_Shutdown();
	}

	VirtualQuery( reinterpret_cast<void *>( addr ), &mbi, sizeof(mbi) );

	GetModuleFileName( (HMODULE)mbi.AllocationBase, moduleName, sizeof( moduleName ) );
	_splitpath( moduleName, NULL, NULL, modShortNameBuf, NULL );
	lastModule = modShortNameBuf;

	processHandle = GetCurrentProcess();
	if ( !SymInitialize( processHandle, NULL, FALSE ) ) {
		return;
	}
	if ( !SymLoadModule64( processHandle, NULL, moduleName, NULL, reinterpret_cast<DWORD64>( mbi.AllocationBase ), 0 ) ) {
		SymCleanup( processHandle );
		return;
	}

	SymSetOptions( SymGetOptions() & ~SYMOPT_UNDNAME );

	lastAllocationBase = reinterpret_cast<address_t>( mbi.AllocationBase );
}

/*
==================
Sym_Shutdown
==================
*/
void Sym_Shutdown( void ) {
	SymUnloadModule64( GetCurrentProcess(), static_cast<DWORD64>( lastAllocationBase ) );
	SymCleanup( GetCurrentProcess() );
	lastAllocationBase = static_cast<address_t>( -1 );
}

/*
==================
Sym_GetFuncInfo
==================
*/
void Sym_GetFuncInfo( address_t addr, idStr &module, idStr &funcName ) {
	MEMORY_BASIC_INFORMATION mbi;

	VirtualQuery( reinterpret_cast<void *>( addr ), &mbi, sizeof(mbi) );

	if ( reinterpret_cast<address_t>( mbi.AllocationBase ) != lastAllocationBase ) {
		Sym_Init( addr );
	}

	BYTE symbolBuffer[ sizeof(IMAGEHLP_SYMBOL64) + MAX_STRING_CHARS ];
	PIMAGEHLP_SYMBOL64 pSymbol = reinterpret_cast<PIMAGEHLP_SYMBOL64>( &symbolBuffer[0] );
	pSymbol->SizeOfStruct = sizeof(IMAGEHLP_SYMBOL64);
	pSymbol->MaxNameLength = MAX_STRING_CHARS - 1;
	pSymbol->Address = 0;
	pSymbol->Flags = 0;
	pSymbol->Size =0;

	DWORD64 symDisplacement = 0;
	if ( SymGetSymFromAddr64( processHandle, static_cast<DWORD64>( addr ), &symDisplacement, pSymbol ) ) {
		// clean up name, throwing away decorations that don't affect uniqueness
	    char undName[MAX_STRING_CHARS];
		if ( UnDecorateSymbolName( pSymbol->Name, undName, sizeof(undName), UNDECORATE_FLAGS ) ) {
			funcName = undName;
		} else {
			funcName = pSymbol->Name;
		}
		module = lastModule;
	}
	else {
		LPVOID lpMsgBuf;
		FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
						NULL,
						GetLastError(),
						MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
						(LPTSTR) &lpMsgBuf,
						0,
						NULL 
						);
		LocalFree( lpMsgBuf );

		// Couldn't retrieve symbol (no debug info?, can't load dbghelp.dll?)
		funcName = va( "0x%p", reinterpret_cast<void *>( addr ) );
		module = "";
    }
}

#else

/*
==================
Sym_Init
==================
*/
void Sym_Init( address_t addr ) {
}

/*
==================
Sym_Shutdown
==================
*/
void Sym_Shutdown( void ) {
}

/*
==================
Sym_GetFuncInfo
==================
*/
void Sym_GetFuncInfo( address_t addr, idStr &module, idStr &funcName ) {
	module = "";
	funcName = va( "0x%p", reinterpret_cast<void *>( addr ) );
}

#endif

/*
==================
GetFuncAddr
==================
*/
address_t GetFuncAddr( address_t midPtPtr ) {
#if defined( _M_X64 )
	return midPtPtr;
#else
	long temp;
	do {
		temp = (long)(*(long*)midPtPtr);
		if ( (temp&0x00FFFFFF) == PROLOGUE_SIGNATURE ) {
			break;
		}
		midPtPtr--;
	} while(true);

	return midPtPtr;
#endif
}

/*
==================
GetCallerAddr
==================
*/
address_t GetCallerAddr( address_t _ebp ) {
#if defined( _M_X64 )
	return 0;
#else
	long midPtPtr;
	long res = 0;

	__asm {
		mov		eax, _ebp
		mov		ecx, [eax]		// check for end of stack frames list
		test	ecx, ecx		// check for zero stack frame
		jz		label
		mov		eax, [eax+4]	// get the ret address
		test	eax, eax		// check for zero return address
		jz		label
		mov		midPtPtr, eax
	}
	res = GetFuncAddr( midPtPtr );
label:
	return res;
#endif
}

/*
==================
Sys_GetCallStack

 use /Oy option
==================
*/
void Sys_GetCallStack( address_t *p_callStack, const int callStackSize ) {
#if defined( _M_X64 )
	USHORT capturedFrames;
	void *p_backTrace[64];
	int i;
	int framesToCapture;

	framesToCapture = Min( callStackSize, static_cast<int>( sizeof( p_backTrace ) / sizeof( p_backTrace[0] ) ) );
	capturedFrames = CaptureStackBackTrace( 2, framesToCapture, p_backTrace, NULL );
	for ( i = 0; i < capturedFrames; i++ ) {
		p_callStack[i] = reinterpret_cast<address_t>( p_backTrace[i] );
	}
	while ( i < callStackSize ) {
		p_callStack[i++] = 0;
	}
#else
#if 1 //def _DEBUG
	int i;
	long m_ebp;

	__asm {
		mov eax, ebp
		mov m_ebp, eax
	}
	// skip last two functions
	m_ebp = *((long*)m_ebp);
	m_ebp = *((long*)m_ebp);
	// list functions
	for ( i = 0; i < callStackSize; i++ ) {
		p_callStack[i] = GetCallerAddr( m_ebp );
		if ( p_callStack[i] == 0 ) {
			break;
		}
		m_ebp = *((long*)m_ebp);
	}
#else
	int i = 0;
#endif
	while( i < callStackSize ) {
		p_callStack[i++] = 0;
	}
#endif
}

/*
==================
Sys_GetCallStackStr
==================
*/
const char *Sys_GetCallStackStr( const address_t *p_callStack, const int callStackSize ) {
	static char string[MAX_STRING_CHARS*2];
	int index, i;
	idStr module, funcName;

	index = 0;
	for ( i = callStackSize-1; i >= 0; i-- ) {
		Sym_GetFuncInfo( p_callStack[i], module, funcName );
		index += sprintf( string+index, " -> %s", funcName.c_str() );
	}
	return string;
}

/*
==================
Sys_GetCallStackCurStr
==================
*/
const char *Sys_GetCallStackCurStr( int depth ) {
	address_t *callStack;

	callStack = (address_t *) _alloca( depth * sizeof( address_t ) );
	Sys_GetCallStack( callStack, depth );
	return Sys_GetCallStackStr( callStack, depth );
}

/*
==================
Sys_GetCallStackCurAddressStr
==================
*/
const char *Sys_GetCallStackCurAddressStr( int depth ) {
	static char string[MAX_STRING_CHARS*2];
	address_t *callStack;
	int index, i;

	callStack = (address_t *) _alloca( depth * sizeof( address_t ) );
	Sys_GetCallStack( callStack, depth );

	index = 0;
	for ( i = depth-1; i >= 0; i-- ) {
		index += sprintf( string+index, " -> 0x%p", reinterpret_cast<void *>( callStack[i] ) );
	}
	return string;
}

/*
==================
Sys_ShutdownSymbols
==================
*/
void Sys_ShutdownSymbols( void ) {
	Sym_Shutdown();
}
