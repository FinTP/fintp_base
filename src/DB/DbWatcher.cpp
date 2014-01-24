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

#include <string>
#include <sstream>
//#include <iostream>
#include "DbWatcher.h"
#include "DatabaseProvider.h"
#include "StringUtil.h"
#include "Trace.h"
#include "LogManager.h"
#include "XmlUtil.h"
#include "TimeUtil.h"

#ifdef WIN32
	#include <windows.h>
	#define sleep(x) Sleep( (x)*1000 )
#else
	#include <unistd.h>
#endif

using namespace FinTP;

DbWatcher::DbWatcher( void ( *callback )( const NotificationObject* ), const bool fullObjectNotif ) : InstrumentedObject(), AbstractWatcher( callback ),
m_WatchOptions( DbWatcher::ReturnDataSet ), m_FullObjectNotif( fullObjectNotif ), m_SelectSPName( "" ), m_DatabaseProvider( DatabaseProvider::None )
	
{
	INIT_COUNTER( INPUT_T_DEPTH );
}

DbWatcher::DbWatcher( NotificationPool* notificationPool, const bool fullObjectNotif ) : InstrumentedObject(), AbstractWatcher( notificationPool ), 
	m_WatchOptions( DbWatcher::ReturnDataSet ), m_FullObjectNotif( fullObjectNotif ), m_SelectSPName( "" ), m_DatabaseProvider( DatabaseProvider::None )
{
	INIT_COUNTER( INPUT_T_DEPTH );
}

DbWatcher::DbWatcher( NotificationPool* notificationPool, const ConnectionString& connectionString, const bool fullObjectNotif ) : 
	InstrumentedObject(), AbstractWatcher( notificationPool ), m_WatchOptions( DbWatcher::ReturnDataSet ), 
	m_FullObjectNotif( fullObjectNotif ), m_SelectSPName( "" ), m_DatabaseProvider( DatabaseProvider::None )
{
	INIT_COUNTER( INPUT_T_DEPTH );
	m_ConnectionString = connectionString;
}

DbWatcher::~DbWatcher()
{
	try
	{
		DESTROY_COUNTER( INPUT_T_DEPTH );
	}catch( ... ){};
}

void DbWatcher::internalScan()
{
	DEBUG_GLOBAL( "Scanning..." );

	// get SP to invoke in order to query the table for new records
	if ( m_SelectSPName.length() == 0 )
	{
		TRACE_GLOBAL( "No select SP for watcher " );
		throw runtime_error( "No select SP for watcher " );
	}

	DatabaseProviderFactory *provider = NULL;
	Database *queryDatabase = NULL;

	try
	{
		// obtain the database
		DEBUG_GLOBAL( "Creating database provider ... " );

		provider = DatabaseProvider::GetFactory( m_DatabaseProvider );
		if( provider == NULL )
		{
			stringstream errorMessage;
			errorMessage << "Database provider [" << DatabaseProvider::ToString( m_DatabaseProvider ) << "] is NULL";
			throw logic_error( errorMessage.str() );
		}
				
		DEBUG_GLOBAL( "Got DB provider. Creating database definition ... " );
		string watchDatabaseName = m_ConnectionString.getDatabaseName();

		queryDatabase = provider->createDatabase();	
		if( queryDatabase == NULL )
			throw logic_error( "Database definition is NULL." );

		// connect
		DEBUG_GLOBAL( "DB definition ok. Connecting to database ..." );
		
		unsigned int connectCount = 0;
		bool connected = false;

		do
		{
			try
			{
				queryDatabase->Connect( m_ConnectionString );
				connected = queryDatabase->IsConnected();
			}
			catch( const std::exception& ex )
			{
				// unable to connect ...
				if ( connectCount == 0 )
				{
					AppException aex( "Watcher unable to connect to database ... retrying", ex );
					aex.addAdditionalInfo( "Database name", m_ConnectionString.getDatabaseName() );
					aex.addAdditionalInfo( "User name", m_ConnectionString.getUserName() );

					LogManager::Publish( aex );
				}
			}
			catch( ... )
			{
				// unable to connect ...
				if ( connectCount == 0 )
				{
					AppException aex( "Watcher unable to connect to database ... retrying" );
					aex.addAdditionalInfo( "Database name", m_ConnectionString.getDatabaseName() );
					aex.addAdditionalInfo( "User name", m_ConnectionString.getUserName() );

					LogManager::Publish( aex );
				}
			}

			connectCount++;
			if( !connected && m_Enabled )
				sleep( 30 );

		} while ( !connected && m_Enabled );
		if ( !connected )
			throw runtime_error( "Unable to connect to database" );
		
		// only support one messahe in pool for now
		if ( ( m_NotificationPool != NULL ) && !m_FullObjectNotif )
			m_NotificationPool->reservePoolSize( 1 );

 		// exit condition. the upstream caller sets this when it is about to shutdown
		bool executeFailed = true;
		unsigned int idleTime = 0;

		while( m_Enabled )
		{
			// if the pool is shutting down, cancel
			if ( ( m_NotificationPool != NULL ) && ( !m_NotificationPool->IsRunning() ) )
				break;

			// isconnected will attempt to reconnect
			if ( executeFailed && !queryDatabase->IsConnected() )
			{
				// still disconnected
				sleep( 10 );
				continue;
			}
			
			executeFailed = true;
			DataSet* myDS = NULL;
			
			DEBUG_GLOBAL( TimeUtil::Get( "%d/%m/%Y %H:%M:%S", 19 ) << " - watcher is executing [" << m_SelectSPName << "] stored procedure to query database [" << watchDatabaseName << "]" );
				
			// invoke select SP
			try
			{
				// if full object is returned, the procedure is expected to update the record, setting a flag to "in process"
				// perform readonly transaction only when reading
				queryDatabase->BeginTransaction( !m_FullObjectNotif && ( m_NotificationType != NotificationObject::TYPE_XMLDOM ) );
				
				//TODO try to compile cursor version only for informix database
				/*if( !m_UseCursor )
					myDS = queryDatabase->ExecuteQueryCached( DataCommand::SP, m_SelectSPName );
				else
				myDS = queryDatabase->ExecuteQueryCached( DataCommand::CURSOR, m_SelectSPName );
				*/
				//myDS = queryDatabase->ExecuteQueryCached( DataCommand::INLINE, "select first 1 ROWID,* from output where coderoare ='0'" );
				//myDS = queryDatabase->ExecuteQueryCached( DataCommand::INLINE, m_SelectSPName );
				myDS = queryDatabase->ExecuteQueryCached( DataCommand::SP, m_SelectSPName );
				//update current row if informix
				/*if( m_DatabaseProvider == DatabaseProvider::Informix )
				{
					long currentRowId;
					currentRowId = myDS->getCellValue( 0, "ROWID" )->getLong();
					ParametersVector myParams;
					DataParameterBase *param = createParameter( DataType::LONGINT_TYPE );
					//param->setDimension(  );	  	
                    param->setLong( currentRowId );
                    param->setName( "ROWID" );

		            myParams.push_back( param );
					string statement = "update output set coderoare='1' where rowid= ?" + StringUtil::ToString( currentRowId );
					//string spName = "spmarkforprocess";
					queryDatabase->ExecuteNonQueryCached( DataCommand::SP, statement.c_str() ); 
				}*/

				queryDatabase->EndTransaction( TransactionType::COMMIT );
				executeFailed = false;
			}
			catch( const DBConnectionLostException& ex )
			{
				TRACE_GLOBAL( "Connection lost. " << ex.getMessage() );
				
				// set nothrow on endtransaction since we are in a catch block
				queryDatabase->EndTransaction( TransactionType::ROLLBACK, false );
			}
			catch( const std::exception& error )
			{
				TRACE_GLOBAL( "ExecuteQuery failed [" << error.what() << "]" );

				// set nothrow on endtransaction since we are in a catch block
				queryDatabase->EndTransaction( TransactionType::ROLLBACK, false );
			}
			catch( ... )
			{
				TRACE_GLOBAL( "ExecuteQuery failed [unknown error]" );

				// set nothrow on endtransaction since we are in a catch block
				queryDatabase->EndTransaction( TransactionType::ROLLBACK, false );
			}
			
			unsigned long newRecords = 0;
			
			// check if we can continue
			if ( ( executeFailed ) || ( myDS == NULL ) || ( myDS->size() == 0 ) )
			{
				DEBUG_GLOBAL( "No records available !" );
			}
			else
			{
				try
				{	
					unsigned int rowsToProcess = myDS->size();
					DEBUG2( "Result dataset size : " << rowsToProcess );

					// if the watcher is set to get the full message, not just the notification,
					// newRecords has another meaning ( a row is returned if there are new rows )
					if ( m_FullObjectNotif || ( m_NotificationType == NotificationObject::TYPE_XMLDOM ) )
						newRecords = rowsToProcess;
					else
						newRecords = myDS->getCellValue( 0, 0 )->getLong();
								
					ASSIGN_COUNTER( INPUT_T_DEPTH, newRecords );
					if ( newRecords > 0 )
					{
						idleTime = 0;
						for ( unsigned int i=0; i<rowsToProcess; i++ )
						{
							DEBUG_GLOBAL( "New records available ! Notifying caller .. " );
							
							// invoke callback
							try
							{
								string messageId = "newrowid";
								NotificationObject* crtNotification = NULL;

								// if the owner of this watcher requested full object notification ( whole message, not just rowid ), convert
								// the resulted rowset to XML
								if ( m_FullObjectNotif )
								{
#if defined ( INFORMIX_ONLY ) 
									messageId = StringUtil::ToString( myDS->getCellValue( i, "ROWID" )->getLong() );
#elif defined ( SQLSERVER_ONLY )
									DataColumnBase *rowidCell = myDS->getCellValue( i, "ROWID" );
									if( rowidCell == NULL )
										throw logic_error( "Query result used by watcher must include a ROWID named column" );
									else if ( rowidCell->getType() == DataType::CHAR_TYPE )
										messageId = StringUtil::Trim( myDS->getCellValue( i, "ROWID" )->getString() );
									else
										messageId = StringUtil::ToString( myDS->getCellValue( i, "ROWID" )->getLong() );
#else
									messageId = StringUtil::Trim( myDS->getCellValue( i, "ROWID" )->getString() );
#endif
									DEBUG2( "New message available [" << messageId << "] !" );

									XERCES_CPP_NAMESPACE_QUALIFIER DOMDocument *doc = NULL;
									ManagedBuffer* notifPtr = NULL;
									try
									{
										if ( ( m_WatchOptions & DbWatcher::ReturnCursorXml ) == DbWatcher::ReturnCursorXml )
										{
											if ( m_NotificationType != NotificationObject::TYPE_XMLDOM )
											{
												string notifValue = StringUtil::Trim( myDS->getCellValue( i, "XMLELEM" )->getString() );
												notifPtr = new ManagedBuffer( ( unsigned char* )notifValue.c_str(), ManagedBuffer::Copy, notifValue.length() );
											}
											else
											{
												doc = XmlUtil::DeserializeFromString( StringUtil::Trim( myDS->getCellValue( i, "XMLELEM" )->getString() ) );
											}
										}
										else
										{
											doc = Database::ConvertToXML( myDS );
										}

										if ( doc == NULL )
										{
											if ( notifPtr == NULL )
												throw runtime_error( "Empty document" );
											crtNotification = new NotificationObject( messageId, notifPtr, string( "datagroup" ), 0, m_NotificationType );
										}
										else
										{
											crtNotification = new NotificationObject( messageId, doc, string( "datagroup" ), 0, m_NotificationType );
										}
									}
									catch( const std::exception& error )
									{
										if( doc != NULL )
										{
											doc->release();
											doc = NULL;
										}
								  			
										stringstream messageBuffer;
										messageBuffer << "Conversion recordset to XML failed [" << typeid( error ).name() << "] exception [" << error.what() << "]";
										TRACE( messageBuffer.str() );

										throw;
									}
									catch( ... )
									{
										if( doc != NULL )
										{
											doc->release();
											doc = NULL;
										}

										stringstream messageBuffer;
										messageBuffer << "Conversion recordset to XML failed [unknown error]";
										TRACE( messageBuffer.str() );

										throw;
									}
								}
								else
								{
									if( m_NotificationType == NotificationObject::TYPE_XMLDOM )
										messageId = StringUtil::Trim( myDS->getCellValue( i, "ROWID" )->getString() );
										
									DEBUG_GLOBAL( "New message available [" << messageId << "] !" );
									crtNotification = new NotificationObject( messageId, string( "datagroup" ), 0 );
								}

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
							catch( const WorkPoolShutdown& shutdownError )
							{
								TRACE_GLOBAL( shutdownError.what() );
								break;
							}
							catch( const AppException& error )
							{
								TRACE_GLOBAL( "Callback exception : [" << error.getMessage() << "]" );
								executeFailed = true;
							}
							catch( const std::exception& error )
							{
								TRACE_GLOBAL( "Callback exception : [" << error.what() << "]" );
								executeFailed = true;
							}	
							catch( ... )
							{
								TRACE_GLOBAL( "Callback exception : [unknown reason]" );
								executeFailed = true;
							}
						}
					}
					else
						DEBUG_GLOBAL( "No records available !" );
				}	
				catch( const std::exception& error )
				{
					DEBUG_GLOBAL( "Callback failed : " << error.what() );
					executeFailed = true;
				}
				catch( ... )
				{
					DEBUG_GLOBAL( "Callback failed : unknown error"  );
					executeFailed = true;
				}
			}
			
			if ( myDS != NULL )
			{
				delete myDS;
				myDS = NULL;
			}
				
			if ( !newRecords && !executeFailed )
			{
				sleep( 5 );
				idleTime += 5;

				if ( m_IdleTimeout && ( idleTime > m_IdleTimeout ) && ( m_IdleCallback != NULL ) )
				{
					m_IdleCallback();
					idleTime = 0;
				}				
			}
			
			if ( executeFailed )
			{
				DEBUG_GLOBAL( "Previous attempt to query the database returned an error. Trying again in 30 seconds." );
				sleep( 30 );
			}
		}
		
		queryDatabase->Disconnect();
	
	}
	catch( ... )
	{
		if ( queryDatabase != NULL )
		{
			delete queryDatabase;
			queryDatabase = NULL;
		}

		if( provider != NULL )
		{
			delete provider;
			provider = NULL;
		}
			
		throw;
	}

	if ( queryDatabase != NULL )
	{
		delete queryDatabase;
		queryDatabase = NULL;
	}

	if( provider != NULL )
	{
		delete provider;
		provider = NULL;
	}

	TRACE_SERVICE( "DB watcher terminated." );
}

void DbWatcher::setConnectionString( const ConnectionString& connectionString )
{
	DEBUG( "ConnectionString set." );
	bool shouldReenable = m_Enabled;

	setEnableRaisingEvents( false );
	m_ConnectionString = connectionString;
	setEnableRaisingEvents( shouldReenable );
}

void DbWatcher::setSelectSPName( const string& spName )
{
	m_SelectSPName = spName;
}

string DbWatcher::getSelectSPName() const
{
	return m_SelectSPName;
}

void DbWatcher::setProvider( const string& provider )
{
	m_DatabaseProvider = DatabaseProvider::Parse( provider );
}

DatabaseProvider::PROVIDER_TYPE DbWatcher::getProvider() const
{
	return m_DatabaseProvider;
}

