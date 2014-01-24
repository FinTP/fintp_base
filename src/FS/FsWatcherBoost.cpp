/*
* FinTP - Financial Transactions Processing Application
* Copyright (C) 2013 Business Information Systems (Allevo) S.R.L.
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program. If not, see <http://www.gnu.org/licenses/>
* or contact Allevo at : 031281 Bucuresti, 23C Calea Vitan, Romania,
* phone +40212554577, office@allevo.ro <mailto:office@allevo.ro>, www.allevo.ro.
*/

#include "string.h"
#include "FsWatcher.h"
#include "../Log/Trace.h"
#include <boost/regex.hpp>

#include <iostream>
#include <string>

#ifndef WIN32
#include <unistd.h>
#include <dirent.h>
#endif

#ifdef WIN32
#include <windows.h>

//#include "dirent_w32.h"
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/filesystem/convenience.hpp>

namespace fs = boost::filesystem;

#define sleep(x) Sleep( (x)*1000 )

#endif

#include "../TimeUtil.h"
using namespace std;

FsWatcher::FsWatcher( NotificationPool* notificationPool, const string& path, const string& filter ) 
	: AbstractWatcher( notificationPool ), m_WatchOptions( 0 ), m_Folder( path ), m_Filter( filter )
{
}

FsWatcher::FsWatcher( void ( *callback )( const NotificationObject* ), const string& path, const string& filter )
	: AbstractWatcher( callback ), m_WatchOptions( 0 ), m_Folder( path ), m_Filter( filter )
{
}

FsWatcher::~FsWatcher()
{
}

void FsWatcher::internalScan()
{
#ifndef WIN32
	struct dirent entry;

	struct dirent* result;
#endif	

	//unsigned int filterSize = m_Filter.length();

	// synchronous processing...
	if ( m_NotificationPool != NULL )
		m_NotificationPool->reservePoolSize( 1 );

	while( m_Enabled )
	{
		DEBUG_GLOBAL( TimeUtil::Get( "%d/%m/%Y %H:%M:%S", 20 ) << " - watcher checking folder [" << m_Folder << "]" );
			
		string messageId = "";

		#ifndef WIN32

		DIR* directoryPointer = opendir( m_Folder.data() );

		for ( readdir_r( directoryPointer, &entry, &result ); result != NULL;
			readdir_r( directoryPointer, &entry, &result ) )
		{
			messageId = entry.d_name;

			// skip crt folder and parent entries
			if ( ( messageId == "." ) || ( messageId == ".." ) )
				continue;

		#else
		/*for ( readdir( directoryPointer, &result ); result != NULL;	readdir( directoryPointer, &result ) )
		{
			messageId = result->d_name;*/

		fs::directory_iterator dirIterator( m_Folder );
		fs::directory_iterator dirEnd;

		//unsigned int fileCount = 0;

		for ( ;dirIterator != dirEnd; ++dirIterator/*, fileCount++*/ )
		{
			// skip folders 
			if ( fs::is_directory( *dirIterator ) )
				continue;

			messageId = dirIterator->leaf();

		#endif
			
			boost::regex expression( m_Filter );
			boost::cmatch what;
			if ( !( boost::regex_match( messageId.data(), what, expression ) ) )
			{
				DEBUG_GLOBAL( "Skipping file [" << messageId << "] because it doesn't match the filter [" << m_Filter << "]" );
				continue;
			}

			if ( ( m_WatchOptions & FsWatcher::SkipEmptyFiles ) == FsWatcher::SkipEmptyFiles )
			{
				ifstream testEmptyFile;
				try
				{
					testEmptyFile.exceptions( ifstream::failbit | ifstream::badbit );
					string filenameWithPath = Path::Combine( m_Folder, messageId );
					testEmptyFile.open( filenameWithPath.c_str(), ios::in | ios::binary | ios::ate );
					if ( 0 == testEmptyFile.tellg() )
						throw runtime_error( "empty file" );
					
					try
					{
						testEmptyFile.close();
					}catch( ... ){};
				}
				catch( const exception& ex )
				{
					try
					{
						testEmptyFile.close();
					}catch( ... ){};
					DEBUG_GLOBAL( "Skipping file [" << messageId << "] error opening file [" << ex.what() << "]" );
					continue;
				}
				catch( ... )
				{
					try
					{
						testEmptyFile.close();
					}catch( ... ){};
					testEmptyFile.close();
					DEBUG_GLOBAL( "Skipping file [" << messageId << "] error opening file [unknown error]" );
					continue;
				}
			}

			DEBUG_GLOBAL( "New file available [" << messageId << "] !" );
			
			NotificationObject* crtNotification = new NotificationObject( messageId, string( "." ), 0 );
			WorkItem< NotificationObject > notification( crtNotification );
			
			if ( m_NotificationPool != NULL )
			{
				// enqueue notification
				DEBUG_GLOBAL( "Waiting on notification pool to allow inserts - thread [" << m_ScanThreadId << "]." );
				m_NotificationPool->addPoolItem( messageId, notification );
				DEBUG_GLOBAL( "Inserted notification in pool [" << messageId << "]" );
			}
			if ( m_Callback != NULL )
			{
				( *AbstractWatcher::m_Callback )( crtNotification );
			}
		}

		#ifndef WIN32
		closedir( directoryPointer );
		#endif //WIN32

		sleep( 5 );
		DEBUG_GLOBAL( "[" << m_Folder << "] -> End of file list." );
	}
}

void FsWatcher::setFilter( const string& filter )
{
	bool shouldReenable = m_Enabled;

	setEnableRaisingEvents( false );
	m_Filter = filter;
	setEnableRaisingEvents( shouldReenable );
}

void FsWatcher::setPath( const string& path )
{
	bool shouldReenable = m_Enabled;

	setEnableRaisingEvents( false );
	m_Folder = path;
	setEnableRaisingEvents( shouldReenable );
}
