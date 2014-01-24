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

#ifdef WIN32
	/*#ifdef _DEBUG
		//#define _CRTDBG_MAP_ALLOC
		#include <stdlib.h>
		#include <crtdbg.h>
	#endif*/
#endif

#include "MqWatcher.h"
#include "TransportHelper.h"

#include <string>
#include <sstream>
#include <iostream>
//#include <iomanip>

#ifdef WIN32
	#define __MSXML_LIBRARY_DEFINED__
	#include <windows.h>
	#define sleep(x) Sleep( (x)*1000 )
#else
	#include <unistd.h>
#endif

#include "Trace.h"
#include "TimeUtil.h"

using namespace std;
using namespace FinTP;

MqWatcher::MqWatcher( NotificationPool* notificationPool, const string& queue, const string& queueManager, const string& transportURI, const string& connectionString )
	: InstrumentedObject(), AbstractWatcher( notificationPool ), m_WatchOptions( MqWatcher::NotifyMessage ), m_Queue( queue ), 
	m_QueueManager( queueManager ), m_TransportURI( transportURI ), m_SSLKeyRepos( "" ), m_SSLCypherSpec( "" ), m_SSLPeerName( "" ), m_HelperType( TransportHelper::NONE )
{
	INIT_COUNTER( INPUT_Q_DEPTH );
}

MqWatcher::~MqWatcher()
{
	try
	{
		DESTROY_COUNTER( INPUT_Q_DEPTH );
	}
	catch( ... )
	{
		try
		{
			TRACE( "An error occured while destroying INPUT_Q_DEPTH counter" );
		} catch( ... ){}
	}
}

void MqWatcher::internalScan()
{
	if ( m_NotificationPool == NULL )
	{
		TRACE( "Message pool not created." )
		throw logic_error( "Message pool not created." );
	}

	TransportHelper* myHelper = TransportHelper::CreateHelper( m_HelperType );
	
	bool force = true;
		
	while( m_Enabled && m_NotificationPool->IsRunning() )
	{
		bool succeeded = true;
		bool first = true;
		
		try
		{
			DEBUG_GLOBAL( TimeUtil::Get( "%d/%m/%Y %H:%M:%S", 19 ) << " - watcher checking queue [" << m_Queue << "] for new messages ... " );
			
			// TODO remove connect/disconnect form pool
			if ( m_SSLKeyRepos.length() > 0 )
			{
			/*	NameValueCollection sslOptions;
	
				sslOptions.Add( "KEYREPOS", m_SSLKeyRepos);
				sslOptions.Add( "SSLCYPHERSPEC", m_SSLCypherSpec );
				sslOptions.Add( "SSLPEERNAME", m_SSLPeerName );
			*/
				myHelper->connect( m_QueueManager, m_TransportURI,m_SSLKeyRepos, m_SSLCypherSpec, m_SSLPeerName, force );
			}
			else
				myHelper->connect( m_QueueManager, m_TransportURI, force );
			
			// reset forced connect
			if( force )
				force = false;
				
			DEBUG_GLOBAL( "Connected to queue manager" );
			
			while ( ( myHelper->peek( m_Queue, first ) == 0 ) && ( succeeded ) && m_Enabled )
			{
				first = false;

				string messageId = myHelper->getLastMessageId();
				string groupId = myHelper->getLastGroupId();
				unsigned long messageSize = myHelper->getLastMessageLength();
				
				DEBUG_GLOBAL( "New message available [" << messageId << "], [" << groupId << "] !" );

				if ( ( ( m_WatchOptions & MqWatcher::NotifyGroups ) == MqWatcher::NotifyGroups ) && !myHelper->isLastInGroup() )
				{
					DEBUG( "Skipping group message ( notifications set for groups )" );
					continue;
				}

				WorkItem< NotificationObject > notification( new NotificationObject( messageId, groupId, messageSize ) );
				
				// enqueue notification
				DEBUG_GLOBAL( "Waiting on notification pool to allow inserts - thread [" << m_ScanThreadId << "]." );
				
				if ( ( m_WatchOptions & MqWatcher::NotifyUnique ) != MqWatcher::NotifyUnique )
					m_NotificationPool->addPoolItem( messageId, notification );
				else
					m_NotificationPool->addUniquePoolItem( messageId, notification );

				DEBUG_GLOBAL( "Inserted notification in pool [" << notification.get()->getObjectId() << "], [" << notification.get()->getObjectGroupId() << "]" );

				ASSIGN_COUNTER( INPUT_Q_DEPTH, myHelper->getQueueDepth(m_Queue) );
			}
		}
		catch( const WorkPoolShutdown& shutdownError )
		{
			TRACE_GLOBAL( shutdownError.what() );
			break;
		}
		catch( const AppException& ex )
		{
			string exceptionType = typeid( ex ) .name();
			string errorMessage = ex.getMessage();
			
			TRACE_GLOBAL( exceptionType << " encountered when scanning : " << errorMessage );
			sleep( 30 );
		}
		catch( const std::exception& ex )
		{
			string exceptionType = typeid( ex ) .name();
			string errorMessage = ex.what();
			
			TRACE_GLOBAL( exceptionType << " encountered when scanning : " << errorMessage );
			sleep( 30 );
		}
		catch( ... )
		{
			TRACE_GLOBAL( "Unhandled exception encountered when scanning. " );
			sleep( 30 );
		}
		
		if ( !succeeded )
		{
			TRACE_GLOBAL( "Last attempt to scan received an error. The next attempt will be made in 30 seconds" );
			try
			{
				force = true;
				myHelper->closeQueue();
				myHelper->disconnect();
			}
			catch( ... ){}
			sleep( 30 );
		}
		else
			ASSIGN_COUNTER( INPUT_Q_DEPTH, 0 );
		
		DEBUG_GLOBAL( "Wake and try again" );
	}
	delete myHelper;
	TRACE_SERVICE( "MQ watcher terminated." );
}

void MqWatcher::setQueue( const string& queue )
{
	DEBUG( "Set queue : " << queue );
	
	bool shouldReenable = m_Enabled;

	setEnableRaisingEvents( false );
	m_Queue = queue;
	setEnableRaisingEvents( shouldReenable );
}

void MqWatcher::setQueueManager( const string& queueManager )
{
	bool shouldReenable = m_Enabled;

	setEnableRaisingEvents( false );
	m_QueueManager = queueManager;
	setEnableRaisingEvents( shouldReenable );
}

void MqWatcher::setTransportURI( const string& transportURI )
{
	bool shouldReenable = m_Enabled;

	setEnableRaisingEvents( false );
	m_TransportURI = transportURI;
	setEnableRaisingEvents( shouldReenable );
}

void MqWatcher::setHelperType ( const TransportHelper::TRANSPORT_HELPER_TYPE& helperType )
{
	bool shouldReenable = m_Enabled;

	setEnableRaisingEvents( false );
	m_HelperType = helperType;
	setEnableRaisingEvents( shouldReenable );
}
