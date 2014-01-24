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
#include "Trace.h"
#include "PlatformDeps.h"
#include "boost/regex.hpp"

#include <iostream>
#include <string>

#ifndef WIN32
#include <unistd.h>
#include <dirent.h>
#endif

#ifdef WIN32
#include "windows.h"
#include "dirent_w32.h"
#define sleep(x) Sleep( (x)*1000 )
#endif

#include "TimeUtil.h"
#include "StringUtil.h"
using namespace std;
using namespace FinTP;

FsWatcher::FsWatcher( NotificationPool* notificationPool, const string& path, const string& filter ) 
	: AbstractWatcher( notificationPool ), m_Folder( path ), m_Filter( filter ), m_WatchOptions( 0 )
{
	initScanPaths( path );
}

FsWatcher::FsWatcher( void ( *callback )( const NotificationObject* ), const string& path, const string& filter )
	: AbstractWatcher( callback ), m_Folder( path ), m_Filter( filter ), m_WatchOptions( 0 )
{
	initScanPaths( path );
}

FsWatcher::~FsWatcher()
{
}

void FsWatcher::internalScan()
{
#ifndef WIN32
	struct dirent entry;
#endif

	struct dirent* result;

	unsigned int filterSize = m_Filter.length();

	// synchronous processing...
	if ( m_NotificationPool != NULL )
		m_NotificationPool->reservePoolSize( 1 );

	DIR* directoryPointer = NULL;

	while( m_Enabled )
	{
		try
		{
			vector< string >::iterator pathIterator = m_ScanPaths.begin();
			while( pathIterator < m_ScanPaths.end() )
			{
				DEBUG_GLOBAL( TimeUtil::Get( "%d/%m/%Y %H:%M:%S", 19 ) << " - watcher checking folder [" << *pathIterator << "]" );
				directoryPointer = opendir( ( *pathIterator ).data() );
				string messageId = "";

				#ifndef WIN32

				for ( readdir_r( directoryPointer, &entry, &result ); result != NULL;
					readdir_r( directoryPointer, &entry, &result ) )
				{
					messageId = entry.d_name;

				#else
				for ( readdir( directoryPointer, &result ); result != NULL;	readdir( directoryPointer, &result ) )
				{
					messageId = result->d_name;

				#endif

					// skip crt folder and parent entries
					if ( ( messageId == "." ) || ( messageId == ".." ) )
						continue;
					
					if ( pathIterator == m_ScanPaths.begin() )
					{
						boost::regex expression( m_Filter ); 
						boost::cmatch what;
						if ( !( boost::regex_match( messageId.data(), what, expression ) ) )
						{
							DEBUG_GLOBAL( "Skipping file [" << messageId << "] because it doesn't match the filter [" << m_Filter << "]" );
							continue;
						}
					}
					if ( ( m_WatchOptions & FsWatcher::SkipEmptyFiles ) == FsWatcher::SkipEmptyFiles )
					{
						ifstream testEmptyFile;
						try
						{
							testEmptyFile.exceptions( ifstream::failbit | ifstream::badbit );
							string filenameWithPath = Path::Combine( *pathIterator, messageId );
							testEmptyFile.open( filenameWithPath.c_str(), ios::in | ios::binary | ios::ate );
							if ( 0 == testEmptyFile.tellg() )
								throw runtime_error( "empty file" );

							try
							{
								testEmptyFile.close();
							}catch( ... ){};
						}
						catch( const std::exception& ex )
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
					messageId = Path::Combine( *pathIterator, messageId );
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

				closedir( directoryPointer );
				sleep( 5 );
				DEBUG_GLOBAL( "[" << *pathIterator << "] -> End of file list." );
				pathIterator++;
			}
		}
		catch( const std::exception& error )
		{
			try
			{
				closedir( directoryPointer );
			} catch( ... ){}
			DEBUG_GLOBAL( "Callback failed : " << error.what() << ". Trying again in 30 seconds." );
			sleep( 30 );
		}
		catch( ... )
		{
			try
			{
				closedir( directoryPointer );
			} catch( ... ){}
			DEBUG_GLOBAL( "Callback failed : unknown error. Trying again in 30 seconds." );
			sleep( 30 );
		}
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
	initScanPaths( path );
	setEnableRaisingEvents( shouldReenable );
}
void FsWatcher::initScanPaths( const string& paths )
{
	if( paths.length() >1 )
	{
		m_ScanPaths.clear();
		StringUtil watchPaths = StringUtil( paths ); 
		watchPaths.Split( ";" );
		string pathToken = "";
		while ( watchPaths.MoreTokens() )
		{
			pathToken = watchPaths.NextToken();
			m_ScanPaths.push_back( pathToken );
		}
	}
}

