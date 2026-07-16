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

idCVar idDemoFile::com_logDemos( "com_logDemos", "0", CVAR_SYSTEM | CVAR_BOOL, "Write demo.log with debug information in it" );
idCVar idDemoFile::com_compressDemos( "com_compressDemos", "1", CVAR_SYSTEM | CVAR_INTEGER | CVAR_ARCHIVE, "Compression scheme for demo files\n0: None    (Fast, large files)\n1: LZW     (Fast to compress, Fast to decompress, medium/small files)\n2: LZSS    (Slow to compress, Fast to decompress, small files)\n3: Huffman (Fast to compress, Slow to decompress, medium files)\nSee also: The 'CompressDemo' command" );
idCVar idDemoFile::com_preloadDemos( "com_preloadDemos", "0", CVAR_SYSTEM | CVAR_BOOL | CVAR_ARCHIVE, "Load the whole demo in to RAM before running it" );

#define DEMO_MAGIC GAME_NAME " RDEMO"

/*
================
idDemoFile::idDemoFile
================
*/
idDemoFile::idDemoFile() {
	p_file = NULL;
	p_logFile = NULL;
	log = false;
	p_fileImage = NULL;
	p_compressor = NULL;
	writing = false;
}

/*
================
idDemoFile::~idDemoFile
================
*/
idDemoFile::~idDemoFile() {
	Close();
}

/*
================
idDemoFile::AllocCompressor
================
*/
idCompressor *idDemoFile::AllocCompressor( int type ) {
	switch ( type ) {
	case 0: return idCompressor::AllocNoCompression();
	default:
	case 1: return idCompressor::AllocLZW();
	case 2: return idCompressor::AllocLZSS();
	case 3: return idCompressor::AllocHuffman();
	}
}

/*
================
idDemoFile::OpenForReading
================
*/
bool idDemoFile::OpenForReading( const char *fileName ) {
	static const int magicLen = sizeof(DEMO_MAGIC) / sizeof(DEMO_MAGIC[0]);
	char magicBuffer[magicLen];
	int compression;
	int fileLength;

	Close();

	p_file = fileSystem->OpenFileRead( fileName );
	if ( !p_file ) {
		return false;
	}

	fileLength = p_file->Length();

	if ( com_preloadDemos.GetBool() ) {
		p_fileImage = (byte *)Mem_Alloc( fileLength );
		p_file->Read( p_fileImage, fileLength );
		fileSystem->CloseFile( p_file );
		p_file = new idFile_Memory( va( "preloaded(%s)", fileName ), (const char *)p_fileImage, fileLength );
	}

	if ( com_logDemos.GetBool() ) {
		p_logFile = fileSystem->OpenFileWrite( "demoread.log" );
	}

	writing = false;

	p_file->Read(magicBuffer, magicLen);
	if ( memcmp(magicBuffer, DEMO_MAGIC, magicLen) == 0 ) {
		p_file->ReadInt( compression );
	} else {
		// Ideally we would error out if the magic string isn't there,
		// but for backwards compatibility we are going to assume it's just an uncompressed demo file
		compression = 0;
		p_file->Rewind();
	}

	p_compressor = AllocCompressor( compression );
	p_compressor->Init( p_file, false, 8 );

	return true;
}

/*
================
idDemoFile::SetLog
================
*/
void idDemoFile::SetLog(bool b, const char *p_logName) {
	log = b;
	if (p_logName) {
		logStr = p_logName;
	}
}

/*
================
idDemoFile::Log
================
*/
void idDemoFile::Log(const char *p_text) {
	if ( p_logFile && p_text && *p_text ) {
		p_logFile->Write( p_text, strlen(p_text) );
	}
}

/*
================
idDemoFile::OpenForWriting
================
*/
bool idDemoFile::OpenForWriting( const char *fileName ) {
	Close();

	p_file = fileSystem->OpenFileWrite( fileName );
	if ( p_file == NULL ) {
		return false;
	}

	if ( com_logDemos.GetBool() ) {
		p_logFile = fileSystem->OpenFileWrite( "demowrite.log" );
	}

	writing = true;

	p_file->Write(DEMO_MAGIC, sizeof(DEMO_MAGIC));
	p_file->WriteInt( com_compressDemos.GetInteger() );
	p_file->Flush();

	p_compressor = AllocCompressor( com_compressDemos.GetInteger() );
	p_compressor->Init( p_file, true, 8 );

	return true;
}

/*
================
idDemoFile::Close
================
*/
void idDemoFile::Close() {
	if ( writing && p_compressor ) {
		p_compressor->FinishCompress();
	}

	if ( p_file ) {
		fileSystem->CloseFile( p_file );
		p_file = NULL;
	}
	if ( p_logFile ) {
		fileSystem->CloseFile( p_logFile );
		p_logFile = NULL;
	}
	if ( p_fileImage ) {
		Mem_Free( p_fileImage );
		p_fileImage = NULL;
	}
	if ( p_compressor ) {
		delete p_compressor;
		p_compressor = NULL;
	}

	demoStrings.DeleteContents( true );
}

/*
================
idDemoFile::ReadHashString
================
*/
const char *idDemoFile::ReadHashString() {
	int		index;

	if ( log && p_logFile ) {
		const char *text = va( "%s > Reading hash string\n", logStr.c_str() );
		p_logFile->Write( text, strlen( text ) );
	} 

	ReadInt( index );

	if ( index == -1 ) {
		// read a new string for the table
		idStr	*str = new idStr;
		
		idStr data;
		ReadString( data );
		*str = data;
		
		demoStrings.Append( str );

		return *str;
	}

	if ( index < -1 || index >= demoStrings.Num() ) {
		Close();
		common->Error( "demo hash index out of range" );
	}

	return demoStrings[index]->c_str();
}

/*
================
idDemoFile::WriteHashString
================
*/
void idDemoFile::WriteHashString( const char *str ) {
	if ( log && p_logFile ) {
		const char *text = va( "%s > Writing hash string\n", logStr.c_str() );
		p_logFile->Write( text, strlen( text ) );
	}
	// see if it is already in the has table
	for ( int i = 0 ; i < demoStrings.Num() ; i++ ) {
		if ( !strcmp( demoStrings[i]->c_str(), str ) ) {
			WriteInt( i );
			return;
		}
	}

	// add it to our table and the demo table
	idStr	*copy = new idStr( str );
//common->Printf( "hash:%i = %s\n", demoStrings.Num(), str );
	demoStrings.Append( copy );
	int cmd = -1;	
	WriteInt( cmd );
	WriteString( str );
}

/*
================
idDemoFile::ReadDict
================
*/
void idDemoFile::ReadDict( idDict &dict ) {
	int i, c;
	idStr key, val;

	dict.Clear();
	ReadInt( c );
	for ( i = 0; i < c; i++ ) {
		key = ReadHashString();
		val = ReadHashString();
		dict.Set( key, val );
	}
}

/*
================
idDemoFile::WriteDict
================
*/
void idDemoFile::WriteDict( const idDict &dict ) {
	int i, c;

	c = dict.GetNumKeyVals();
	WriteInt( c );
	for ( i = 0; i < c; i++ ) {
		WriteHashString( dict.GetKeyVal( i )->GetKey() );
		WriteHashString( dict.GetKeyVal( i )->GetValue() );
	}
}

/*
 ================
 idDemoFile::Read
 ================
 */
int idDemoFile::Read( void *p_buffer, int len ) {
	int read = p_compressor->Read( p_buffer, len );
	if ( read == 0 && len >= 4 ) {
		*(demoSystem_t *)p_buffer = DS_FINISHED;
	}
	return read;
}

/*
 ================
 idDemoFile::Write
 ================
 */
int idDemoFile::Write( const void *p_buffer, int len ) {
	return p_compressor->Write( p_buffer, len );
}

