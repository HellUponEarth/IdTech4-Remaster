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

idCVar idEventLoop::com_journal( "com_journal", "0", CVAR_INIT|CVAR_SYSTEM, "1 = record journal, 2 = play back journal", 0, 2, idCmdSystem::ArgCompletion_Integer<0,2> );

idEventLoop eventLoopLocal;
idEventLoop *eventLoop = &eventLoopLocal;

static const int JOURNAL_EVENT_MAGIC = ( 'J' << 0 ) | ( 'R' << 8 ) | ( 'N' << 16 ) | ( '2' << 24 );
static const int JOURNAL_EVENT_VERSION = 1;

typedef struct journalEventRecord_s {
	int evType;
	int evValue;
	int evValue2;
	int evPtrLength;
} journalEventRecord_t;

static_assert( sizeof( int ) == 4, "journal event fields must stay fixed-width" );
static_assert( sizeof( journalEventRecord_t ) == 16, "journal event record must stay fixed-width" );

static void Journal_FatalRead( void ) {
	common->FatalError( "Error reading from journal file" );
}

static void Journal_FatalWrite( void ) {
	common->FatalError( "Error writing to journal file" );
}

static void Journal_ReadPayload( idFile *p_journalFile, sysEvent_t &ev ) {
	if ( ev.evPtrLength < 0 ) {
		Journal_FatalRead();
	}
	ev.p_evPtr = NULL;
	if ( ev.evPtrLength > 0 ) {
		ev.p_evPtr = Mem_ClearedAlloc( ev.evPtrLength );
		if ( p_journalFile->Read( ev.p_evPtr, ev.evPtrLength ) != ev.evPtrLength ) {
			Journal_FatalRead();
		}
	}
}

static sysEvent_t Journal_ReadFixedEvent( idFile *p_journalFile ) {
	journalEventRecord_t record;
	sysEvent_t ev;

	if ( p_journalFile->ReadInt( record.evType ) != sizeof( record.evType ) ||
			p_journalFile->ReadInt( record.evValue ) != sizeof( record.evValue ) ||
			p_journalFile->ReadInt( record.evValue2 ) != sizeof( record.evValue2 ) ||
			p_journalFile->ReadInt( record.evPtrLength ) != sizeof( record.evPtrLength ) ) {
		Journal_FatalRead();
	}

	ev.evType = static_cast<sysEventType_t>( record.evType );
	ev.evValue = record.evValue;
	ev.evValue2 = record.evValue2;
	ev.evPtrLength = record.evPtrLength;
	Journal_ReadPayload( p_journalFile, ev );
	return ev;
}

static sysEvent_t Journal_ReadLegacy32Event( idFile *p_journalFile ) {
	int evType;
	int legacyPointerToken;
	sysEvent_t ev;

	if ( p_journalFile->ReadInt( evType ) != sizeof( evType ) ||
			p_journalFile->ReadInt( ev.evValue ) != sizeof( ev.evValue ) ||
			p_journalFile->ReadInt( ev.evValue2 ) != sizeof( ev.evValue2 ) ||
			p_journalFile->ReadInt( ev.evPtrLength ) != sizeof( ev.evPtrLength ) ||
			p_journalFile->ReadInt( legacyPointerToken ) != sizeof( legacyPointerToken ) ) {
		Journal_FatalRead();
	}

	ev.evType = static_cast<sysEventType_t>( evType );
	Journal_ReadPayload( p_journalFile, ev );
	return ev;
}

static void Journal_WriteFixedEvent( idFile *p_journalFile, const sysEvent_t &ev ) {
	if ( p_journalFile->WriteInt( static_cast<int>( ev.evType ) ) != sizeof( int ) ||
			p_journalFile->WriteInt( ev.evValue ) != sizeof( ev.evValue ) ||
			p_journalFile->WriteInt( ev.evValue2 ) != sizeof( ev.evValue2 ) ||
			p_journalFile->WriteInt( ev.evPtrLength ) != sizeof( ev.evPtrLength ) ) {
		Journal_FatalWrite();
	}

	if ( ev.evPtrLength > 0 ) {
		if ( p_journalFile->Write( ev.p_evPtr, ev.evPtrLength ) != ev.evPtrLength ) {
			Journal_FatalWrite();
		}
	}
}


/*
=================
idEventLoop::idEventLoop
=================
*/
idEventLoop::idEventLoop( void ) {
	com_journalFile = NULL;
	com_journalDataFile = NULL;
	initialTimeOffset = 0;
	com_journalReadFormat = JOURNAL_FORMAT_NONE;
}

/*
=================
idEventLoop::~idEventLoop
=================
*/
idEventLoop::~idEventLoop( void ) {
}

/*
=================
idEventLoop::GetRealEvent
=================
*/
sysEvent_t	idEventLoop::GetRealEvent( void ) {
	sysEvent_t	ev;

	// either get an event from the system or the journal file
	if ( com_journal.GetInteger() == 2 ) {
		if ( com_journalReadFormat == JOURNAL_FORMAT_FIXED ) {
			ev = Journal_ReadFixedEvent( com_journalFile );
		} else if ( com_journalReadFormat == JOURNAL_FORMAT_LEGACY_32 ) {
			ev = Journal_ReadLegacy32Event( com_journalFile );
		} else {
			Journal_FatalRead();
		}
	} else {
		ev = Sys_GetEvent();

		// write the journal value out if needed
		if ( com_journal.GetInteger() == 1 ) {
			Journal_WriteFixedEvent( com_journalFile, ev );
		}
	}

	return ev;
}

/*
=================
idEventLoop::PushEvent
=================
*/
void idEventLoop::PushEvent( sysEvent_t *p_event ) {
	sysEvent_t		*p_ev;
	static			bool printedWarning;

	p_ev = &com_pushedEvents[ com_pushedEventsHead & (MAX_PUSHED_EVENTS-1) ];

	if ( com_pushedEventsHead - com_pushedEventsTail >= MAX_PUSHED_EVENTS ) {

		// don't print the warning constantly, or it can give time for more...
		if ( !printedWarning ) {
			printedWarning = true;
			common->Printf( "WARNING: Com_PushEvent overflow\n" );
		}

		if ( p_ev->p_evPtr ) {
			Mem_Free( p_ev->p_evPtr );
		}
		com_pushedEventsTail++;
	} else {
		printedWarning = false;
	}

	*p_ev = *p_event;
	com_pushedEventsHead++;
}

/*
=================
idEventLoop::GetEvent
=================
*/
sysEvent_t idEventLoop::GetEvent( void ) {
	if ( com_pushedEventsHead > com_pushedEventsTail ) {
		com_pushedEventsTail++;
		return com_pushedEvents[ (com_pushedEventsTail-1) & (MAX_PUSHED_EVENTS-1) ];
	}
	return GetRealEvent();
}

/*
=================
idEventLoop::ProcessEvent
=================
*/
void idEventLoop::ProcessEvent( sysEvent_t ev ) {
	// track key up / down states
	if ( ev.evType == SE_KEY ) {
		idKeyInput::PreliminaryKeyEvent( ev.evValue, ( ev.evValue2 != 0 ) );
	}

	if ( ev.evType == SE_CONSOLE ) {
		// from a text console outside the game window
		cmdSystem->BufferCommandText( CMD_EXEC_APPEND, (char *)ev.p_evPtr );
		cmdSystem->BufferCommandText( CMD_EXEC_APPEND, "\n" );
	} else {
		session->ProcessEvent( &ev );
	}

	// free any block data
	if ( ev.p_evPtr ) {
		Mem_Free( ev.p_evPtr );
	}
}

/*
===============
idEventLoop::RunEventLoop
===============
*/
int idEventLoop::RunEventLoop( bool commandExecution ) {
	sysEvent_t	ev;

	while ( 1 ) {

		if ( commandExecution ) {
			// execute any bound commands before processing another event
			cmdSystem->ExecuteCommandBuffer();
		}

		ev = GetEvent();

		// if no more events are available
		if ( ev.evType == SE_NONE ) {
			return 0;
		}
		ProcessEvent( ev );
	}

	return 0;	// never reached
}

/*
=============
idEventLoop::Init
=============
*/
void idEventLoop::Init( void ) {

	initialTimeOffset = Sys_Milliseconds();

	common->StartupVariable( "journal", false );

	if ( com_journal.GetInteger() == 1 ) {
		common->Printf( "Journaling events\n" );
		com_journalFile = fileSystem->OpenFileWrite( "journal.dat" );
		com_journalDataFile = fileSystem->OpenFileWrite( "journaldata.dat" );
		com_journalReadFormat = JOURNAL_FORMAT_NONE;
	} else if ( com_journal.GetInteger() == 2 ) {
		common->Printf( "Replaying journaled events\n" );
		com_journalFile = fileSystem->OpenFileRead( "journal.dat" );
		com_journalDataFile = fileSystem->OpenFileRead( "journaldata.dat" );
		com_journalReadFormat = JOURNAL_FORMAT_NONE;
	}

	if ( !com_journalFile || !com_journalDataFile ) {
		com_journal.SetInteger( 0 );
		com_journalFile = 0;
		com_journalDataFile = 0;
		com_journalReadFormat = JOURNAL_FORMAT_NONE;
		common->Printf( "Couldn't open journal files\n" );
	} else if ( com_journal.GetInteger() == 1 ) {
		if ( com_journalFile->WriteInt( JOURNAL_EVENT_MAGIC ) != sizeof( int ) ||
				com_journalFile->WriteInt( JOURNAL_EVENT_VERSION ) != sizeof( int ) ) {
			common->FatalError( "Error writing journal header" );
		}
	} else if ( com_journal.GetInteger() == 2 ) {
		int magic = 0;
		int version = 0;
		if ( com_journalFile->ReadInt( magic ) != sizeof( magic ) ) {
			common->FatalError( "Error reading journal header" );
		}
		if ( magic == JOURNAL_EVENT_MAGIC ) {
			if ( com_journalFile->ReadInt( version ) != sizeof( version ) || version != JOURNAL_EVENT_VERSION ) {
				common->FatalError( "Unsupported journal event version" );
			}
			com_journalReadFormat = JOURNAL_FORMAT_FIXED;
		} else {
			com_journalFile->Seek( 0, FS_SEEK_SET );
			com_journalReadFormat = JOURNAL_FORMAT_LEGACY_32;
		}
	}
}

/*
=============
idEventLoop::Shutdown
=============
*/
void idEventLoop::Shutdown( void ) {
	if ( com_journalFile ) {
		fileSystem->CloseFile( com_journalFile );
		com_journalFile = NULL;
	}
	if ( com_journalDataFile ) {
		fileSystem->CloseFile( com_journalDataFile );
		com_journalDataFile = NULL;
	}
}

/*
================
idEventLoop::Milliseconds

Can be used for profiling, but will be journaled accurately
================
*/
int idEventLoop::Milliseconds( void ) {
#if 1	// FIXME!
	return Sys_Milliseconds() - initialTimeOffset;
#else
	sysEvent_t	ev;

	// get events and push them until we get a null event with the current time
	do {

		ev = Com_GetRealEvent();
		if ( ev.evType != SE_NONE ) {
			Com_PushEvent( &ev );
		}
	} while ( ev.evType != SE_NONE );
	
	return ev.evTime;
#endif
}

/*
================
idEventLoop::JournalLevel
================
*/
int idEventLoop::JournalLevel( void ) const {
	return com_journal.GetInteger();
}
