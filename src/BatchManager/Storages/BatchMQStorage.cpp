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

#include "BatchMQStorage.h"
#include "Trace.h"
#include "WorkItemPool.h"

using namespace FinTP;

BatchMQStorage::BatchMQStorage():BatchStorageBase(),
	m_CrtHelper( NULL ), m_BufferSize( 1000 ), m_IsCleaningUp( false )
{
}

BatchMQStorage::~BatchMQStorage()
{
	try
	{
		DEBUG( "Destructor" );
	} catch ( ... ) {}

	try
	{
		m_CrtHelper->disconnect();
	} 
	catch( ... )
	{
		try
		{
			TRACE( "An error occured while disconnecting from MQ" );
		} catch ( ... ){}
	}

	try
	{
		//shouldn't throw but just in case ...
		delete m_CrtHelper;
	} catch ( ... ) {}
}

void BatchMQStorage::initialize( TransportHelper::TRANSPORT_HELPER_TYPE helperType )
{
	if ( m_CrtHelper == NULL )
		m_CrtHelper = TransportHelper::CreateHelper( helperType );
	else
		throw runtime_error( "MQ storage already initialized" );
}

void BatchMQStorage::enqueue( BatchResolution& resolution )
{
	if ( m_CrtHelper == NULL )
		throw runtime_error( "MQ storage not initialized" );
	DEBUG( "Enqueue" );
	BatchItem item = resolution.getItem();

	DEBUG( "Storage : BatchId [" << item.getBatchId() << "], current sequence [" <<  item.getSequence() << "]" );

	switch ( item.getPayloadType() )
	{
	case BatchItem::BATCHITEM_BIN:
	{
		if ( m_ReplyOptions.empty() )
			m_CrtHelper->putGroupMessage( item.getBinPayload(), item.getBatchId(), item.getSequence(), item.isLast() );
		else
		{
			TransportReplyOptions replyOptions( m_ReplyOptions );
			m_CrtHelper->putSAAmessage( replyOptions, m_ReplyQueue, item.getBinPayload(), item.getBatchId(), item.getSequence(), item.isLast() );
		}
		break;
	}
	case BatchItem::BATCHITEM_TXT:
	{
		const string& payload = item.getPayload();
		ManagedBuffer buffer( ( unsigned char* )payload.c_str(), ManagedBuffer::Ref, payload.size() );
		if ( m_ReplyOptions.empty() )
			m_CrtHelper->putGroupMessage( &buffer, item.getBatchId(), item.getSequence(), item.isLast() );
		else
		{
			TransportReplyOptions replyOptions( m_ReplyOptions );
			m_CrtHelper->putSAAmessage( replyOptions, m_ReplyQueue, &buffer, item.getBatchId(), item.getSequence(), item.isLast() ); 
		}
		break;
	}
	default:
		throw runtime_error( "Could not enque batch item");
	}
	
	resolution.setItemMessageId( m_CrtHelper->getLastMessageId() );
}

BatchItem BatchMQStorage::dequeue()
{
	if ( m_CrtHelper == NULL )
		throw runtime_error( "MQ storage not initialized" );
	DEBUG( "Dequeue" );

	BatchItem item;
	ManagedBuffer buffer;
	long result = -1;

	result = m_CrtHelper->getGroupMessage( &buffer, m_CrtStorageId, m_IsCleaningUp);

	// no message
	if ( result == -1 )
	{
		DEBUG( "No message matching the current ids found." );
		throw runtime_error( "Attempt to read a message from queue failed [no matching messages]" );
	}
	if ( result == -2 )
	{
		DEBUG( "Message in batch exceed backoutcount, sequence for dequeued mesage is [" <<  m_CrtHelper->getLastGroupSequence() << "]" );
		m_IsCleaningUp = true;
	}
	item.setSequence( m_CrtHelper->getLastGroupSequence() );
	item.setMessageId( m_CrtHelper->getLastMessageId() );
	item.setPayload(&buffer);

	if ( m_CrtHelper->isLastInGroup() )
		item.setLast();

	return item;
}

void BatchMQStorage::close( const string& storageId )
{
	if ( m_CrtHelper == NULL )
		throw runtime_error( "MQ storage not initialized" );
	m_CrtHelper->closeQueue(); 
}

// open storage
void BatchMQStorage::open( const string& storageId, ios_base::openmode openMode )
{	
	if ( m_CrtHelper == NULL )
		throw runtime_error( "MQ storage not initialized" );
	m_CrtStorageId = storageId;
	m_CrtHelper->connect( m_QueueManager, m_ChDef );
	m_CrtHelper->openQueue( m_Queue );
	m_CrtHelper->setBackupQueue( m_BackupQueue );
}
