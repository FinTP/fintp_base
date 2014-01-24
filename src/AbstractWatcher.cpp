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

#include "AbstractWatcher.h"
#include "LogManager.h"
#include "Trace.h"

#include <errno.h>
using namespace FinTP;

AbstractWatcher::~AbstractWatcher()
{
	m_ScanThreadId = 0;
}

void AbstractWatcher::waitForExit()
{
	int joinResult = pthread_join( m_ScanThreadId, NULL ); 
	if ( 0 != joinResult )
	{
		TRACE( "An error occured when joining the scan thread [" << joinResult << "]" );
	}
}

void AbstractWatcher::setEnableRaisingEvents( bool val )
{
	if ( m_Enabled == val )
		return;

	m_Enabled = val;
	
	if( m_Enabled )
	{
		pthread_attr_t scanThreadAttr;

		int attrInitResult = pthread_attr_init( &scanThreadAttr );
		if ( 0 != attrInitResult )
		{
			TRACE( "Error initializing scan thread attribute [" << attrInitResult << "]" );
			throw runtime_error( "Error initializing scan thread attribute" );
		}

		int setDetachResult = pthread_attr_setdetachstate( &scanThreadAttr, PTHREAD_CREATE_JOINABLE );
		if ( 0 != setDetachResult )
		{
			TRACE( "Error setting joinable option to scan thread attribute [" << setDetachResult << "]" );
			throw runtime_error( "Error setting joinable option to scan thread attribute" );
		}
				
		// if running in debugger, reattempt to create thread on failed request
		int threadStatus = 0, errCodeCreate = 0;
		do
		{
			threadStatus = pthread_create( &m_ScanThreadId, &scanThreadAttr, AbstractWatcher::ScanInNewThread, this );
			errCodeCreate = errno;
			DEBUG( "Scan thread create result [" << threadStatus << "] errno [" << errCodeCreate << "]" );
		} while( ( threadStatus != 0 ) && ( errCodeCreate == EINTR ) );

		if ( threadStatus != 0 )
		{
			int errCode = errno;
			int attrDestroyResult = pthread_attr_destroy( &scanThreadAttr );
			if ( 0 != attrDestroyResult )
			{
				TRACE( "Unable to destroy scan thread attribute [" << attrDestroyResult << "]" );
			}	
			
			stringstream errorMessage;
#ifdef CRT_SECURE
			char errBuffer[ 95 ];
			strerror_s( errBuffer, sizeof( errBuffer ), errCode );
			errorMessage << "Unable to create scan thread. [" << errBuffer << "]";
#else
			errorMessage << "Unable to create scan thread. [" << strerror( errCode ) << "]";
#endif	
			throw runtime_error( errorMessage.str() );
		}
				
		int attrDestroyResult2 = pthread_attr_destroy( &scanThreadAttr );
		if ( 0 != attrDestroyResult2 )
		{
			TRACE( "Unable to destroy scan thread attribute [" << attrDestroyResult2 << "]" );
		}
	}
	else
	{
		DEBUG( "Stopping scan thread..." );
		int joinResult = pthread_join( m_ScanThreadId, NULL ); 
		if ( 0 != joinResult )
		{
			TRACE( "An error occured when joining the scan thread [" << joinResult << "]" );
		}
		DEBUG( "Scan thread stopped." );
	}
}

void* AbstractWatcher::ScanInNewThread( void* pThis )
{
	AbstractWatcher* me = static_cast< AbstractWatcher* >( pThis );
	TRACE( "Watcher thread" );

	try
	{
		me->internalScan();
	}
	catch( const std::exception& ex )
	{
		stringstream errorMessage;
		errorMessage << typeid( ex ).name() << " exception encountered by Watcher [" << ex.what() << "]";
		TRACE( errorMessage.str() );

		try
		{
			LogManager::Publish( AppException( "Error encountered by watcher", ex ) );
		}
		catch( ... ){}
	}
	catch( ... )
	{
		stringstream errorMessage;
		errorMessage << "Exception encountered by watcher [unhandled exception]";
		TRACE( errorMessage.str() );

		try
		{
			LogManager::Publish( errorMessage.str() );
		}
		catch( ... ){}
	}

	pthread_exit( NULL );
	return NULL;
}
