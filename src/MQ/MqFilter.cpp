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

#include "MqFilter.h"
#include "XmlUtil.h"
#include "Trace.h"
#include "StringUtil.h"

#include <iostream>
#include <sstream>
#include <string>

using namespace std;
using namespace FinTP;
XERCES_CPP_NAMESPACE_USE

// config section parameters
const string MqFilter::MQQUEUE = "WMQQueue";
const string MqFilter::MQBACKUPQUEUE = "WMQBackupQueue";
const string MqFilter::MQMANAGER = "WMQQMgr";
const string MqFilter::MQURI = "MQURI";
const string MqFilter::MQHELPERTYPE= "MQManagerType";
const string MqFilter::MQRPLTransportURI = "WMQRplChDef";
const string MqFilter::MQBATCH = "batch";
const string MqFilter::MQFORMAT = "format";
const string MqFilter::MQRPLMESSAGETYPE = "WMQRplMsgType";
const string MqFilter::MQRPLUSRDATA = "WMQRplUsrData";
const string MqFilter::MQRPLMESSAGEFORMAT = "WMQRplMsgFmt";

//transportHeaders parameters
const string MqFilter::MQMSGID = "MQMSGID";
const string MqFilter::MQGROUPID = "MQGROUPID"; //notified by callback with GroupId
const string MqFilter::MQLASTMSG = "MQLASTMSG";
const string MqFilter::MQSEQUENCE = "MQSEQUENCE";
const string MqFilter::MQMSGCORELID = "MQMSGCORELID";
const string MqFilter::MQMSGSIZE = "MQMSGSIZE";
const string MqFilter::MQREPLYQUEUE = "MQREPLYQUEUE";
const string MqFilter::MQREPLYQUEUEMANAGER = "MQREPLYQUEUEMANAGER";
const string MqFilter::MQREPLYOPTIONS = "MQREPLYOPTIONS";
const string MqFilter::MQMESSAGETYPE = "MQMESSAGETYPE";
const string MqFilter::MQFEEDBACK = "MQFEEDBACK";
const string MqFilter::MQAPPNAME = "APPNAME";

const string MqFilter::MQSSLKEYREPOSITORY = "MQKEYREPOSITORY";
const string MqFilter::MQSSLCYPHERSPEC = "MQSSLCYPHERSPEC";
const string MqFilter::MQSSLPEERNAME = "MQSSLPEERNAME";
const string MqFilter::MQHELPERRETRIES = "MQHELPERRETRIES";

MqFilter::MqFilter() : AbstractFilter( FilterType::MQ ), m_BatchManager( NULL ), m_CrtHelper( NULL )
{
}

MqFilter::~MqFilter()
{
	try
	{
		if( m_BatchManager != NULL )
		{
			m_BatchManager->close( m_CrtBatchId );
			delete m_BatchManager;
		}
		delete m_CrtHelper;
	}
	catch( ... ){}
}

AbstractFilter::FilterResult MqFilter::ProcessMessage( XERCES_CPP_NAMESPACE_QUALIFIER DOMDocument* inputOutputData, NameValueCollection& transportHeaders, bool asClient )
{
	// reuse prev. method	
	string crtBackupQueue = getBackupQueueName( transportHeaders );
	if( asClient )
	{
		string serializedDOM = XmlUtil::SerializeToString( inputOutputData );

		ManagedBuffer* inputBuffer = new ManagedBuffer();
		inputBuffer->copyFrom( serializedDOM );

		return ProcessMessage( AbstractFilter::buffer_type( inputBuffer ), AbstractFilter::buffer_type( NULL ), transportHeaders, asClient );
	}
	else
	{
		if ( isBatch( transportHeaders ) )
		{
			DEBUG( "MQFilter running in batch mode.");
			int lastId = 0;
			if ( ( m_BatchManager != NULL ) && ( m_CrtBatchId != getGroupId( transportHeaders ) ) )
			{
				m_BatchManager->close( m_CrtBatchId );
				delete m_BatchManager;
				m_BatchManager = NULL;
			}
			// if first in Batch
			if ( m_BatchManager == NULL )
			{
				//create WMQ BatchManager
				m_BatchManager = new BatchManager<BatchMQStorage>( BatchManagerBase::WMQ, BatchResolution::SYNCHRONOUS );
				m_BatchManager->storage().setQueue( getQueueName( transportHeaders ) );
				m_BatchManager->storage().setBackupQueue( getBackupQueueName( transportHeaders ) );
				m_BatchManager->storage().setQueueManager( getQueueManagerName( transportHeaders ) );
				m_BatchManager->storage().setTransportURI( getTransportURI( transportHeaders ) );
				m_BatchManager->storage().setAutoAbandon( 3 );
				m_BatchManager->storage().setCleaningUp( false );
				//open WMQ BatchManager
				//
				// connector is Notified with groupId, 
				// groupId will be passed to next Filters using TransportHeaders
				
				//storage ( group ) is open for output 
				
				DEBUG( "Opening WMQ Storage ..." );
				
				if ( !transportHeaders.ContainsKey( MqFilter::MQGROUPID ) )
					throw logic_error( "Expected parameter of WMQ batch [MQGROUPID] missing" );

				m_CrtBatchId = getGroupId( transportHeaders );
				DEBUG( "Transport headers param MQGROUPID is [" << m_CrtBatchId.c_str() << "]" );
				m_BatchManager->open( getGroupId( transportHeaders ), ios_base::in );
			}
				
			if ( transportHeaders.ContainsKey( MQMSGSIZE ) )
			{
				unsigned long passedSize = StringUtil::ParseLong( transportHeaders[ MQMSGSIZE ] );
				m_BatchManager->storage().setBufferSize( passedSize );
			}
			
			//dequeue a message 
			BatchItem batchItem ;
			DEBUG( "Dequeuing ..." );
			*m_BatchManager >> batchItem; 
			if( m_BatchManager->storage().getCleaningUp() )
			{
				DEBUG( "Filter performe last batch cleaning up..." )
				// item exceed  backoutcount, BatchManager was set for cleaning up
				Cleanup();
				AppException aex( "Undeliverable message sent to dead letter queue because the backout count was exceeded", EventType::Warning );
				aex.setSeverity( EventSeverity::Fatal );
				throw aex;
			}
			// message will be sent by filters
			// to preserve the pointer passed by caller,  REPLACE document element with the message content
			// existing content is replaced with current message
			if ( inputOutputData == NULL )
				throw logic_error( "You must supply a valid DOMDocument to ProcessMessage" );
				
			XERCES_CPP_NAMESPACE_QUALIFIER DOMNode* docRootElement = inputOutputData->getDocumentElement();
			if( docRootElement != NULL )
			{
				DEBUG( "Removing current document content" );
				//remove current document
				inputOutputData->removeChild( docRootElement );
				docRootElement->release();
			}
				
			DEBUG( "Append new root node" );
			// current message will be passed as DOMDocument
			// it is serialized and append in the document passed as parameter
			// as root; document will contain only current message serialized
			
			XERCES_CPP_NAMESPACE_QUALIFIER DOMDocument* xmlPayload = NULL;
			xmlPayload = batchItem.getXmlPayload();
			if ( xmlPayload == NULL )
				throw runtime_error( "payload is empty" );
			inputOutputData->appendChild( inputOutputData->importNode( xmlPayload->getDocumentElement() , true  ) );
			DEBUG( "Object replaced" );				
			
			//if last item in group ( batch ), close batch		
			if ( batchItem.isLast() )
			{
				DEBUG( "Last item in batch" );
				lastId = 1;
			}
			else
			{
				lastId = 0;
				DEBUG( "There are more messages in this batch..." );
			}
			
			// pass groupId ( batch Id ) to next filters or connector using transportHeaders			
			if ( transportHeaders.ContainsKey( MQGROUPID ) )
				transportHeaders.ChangeValue( MQGROUPID, getGroupId( transportHeaders ) );
			else
				transportHeaders.Add( MQGROUPID, getGroupId( transportHeaders ) );
				
			// pass information if last Item to next filters or connector using transportHeaders											
			if ( transportHeaders.ContainsKey( MQLASTMSG ) )
				transportHeaders.ChangeValue( MQLASTMSG, StringUtil::ToString( lastId ) );
			else
				transportHeaders.Add( MQLASTMSG, StringUtil::ToString( lastId ) );
			
			setTransportHeaders( transportHeaders, asClient );
			return AbstractFilter::Completed;
		}
		else
		{
			return ProcessMessage( AbstractFilter::buffer_type( NULL ), inputOutputData, transportHeaders, asClient );
		}
	}
}

AbstractFilter::FilterResult MqFilter::ProcessMessage( const XERCES_CPP_NAMESPACE_QUALIFIER DOMDocument* inputData, AbstractFilter::buffer_type outputData, NameValueCollection& transportHeaders, bool asClient )
{
	// reuse prev. method
	if( asClient )
	{
		string serializedDOM = XmlUtil::SerializeToString( inputData );

		ManagedBuffer* inputBuffer = new ManagedBuffer();
		inputBuffer->copyFrom( serializedDOM );

		return ProcessMessage( AbstractFilter::buffer_type( inputBuffer ), AbstractFilter::buffer_type( NULL ), transportHeaders, asClient );
	}
	else
	{
		return ProcessMessage( AbstractFilter::buffer_type( NULL ), outputData, transportHeaders, asClient );
	}
}

AbstractFilter::FilterResult MqFilter::ProcessMessage( AbstractFilter::buffer_type inputData, XERCES_CPP_NAMESPACE_QUALIFIER DOMDocument* outputData, NameValueCollection& transportHeaders, bool asClient )
{
	if ( asClient )
	{
		return ProcessMessage( inputData, AbstractFilter::buffer_type( NULL ), transportHeaders, asClient );
	}
	else
	{
		//reuse defined method, but alloc buffer
		ManagedBuffer* outputBuffer = new ManagedBuffer();
		AbstractFilter::buffer_type outputDataBuffer( outputBuffer );

		AbstractFilter::FilterResult result = ProcessMessage( AbstractFilter::buffer_type( NULL ), outputDataBuffer, transportHeaders, asClient );
		
		// get dom implementation
		XERCES_CPP_NAMESPACE_QUALIFIER DOMDocument* tempDoc = NULL;
		try		
		{
			DEBUG( "Deserializing document" );
			if ( outputBuffer == NULL )
				throw runtime_error( "Empty document" );

			tempDoc = XmlUtil::DeserializeFromString( outputBuffer->buffer(), outputBuffer->size() );
			DEBUG( "Deserialization OK" );

			// replace document element
			if ( outputData == NULL )
				throw logic_error( "You must supply a valid DOMDocument to ProcessMessage" );
				
			DOMNode* docElement = outputData->getDocumentElement();
			if( docElement != NULL )
			{
				DEBUG( "Removing current document element" );
				outputData->removeChild( docElement );
				docElement->release();
			}
				
			DEBUG( "Append new root node" );
			outputData->appendChild( outputData->importNode( tempDoc->getDocumentElement(), true ) );
			DEBUG( "Object replaced" );
		}
		catch( ... )
		{
			
			//release document
			if( tempDoc != NULL )
				tempDoc->release();
			
			throw;
		}
		
		//release document
		if( tempDoc != NULL )
			tempDoc->release();
									
		return result;
	}
}

AbstractFilter::FilterResult MqFilter::ProcessMessage( AbstractFilter::buffer_type inputData, AbstractFilter::buffer_type outputData, NameValueCollection& transportHeaders, bool asClient )
{
	ValidateProperties();
	
	string crtQueue = getQueueName( transportHeaders );
	string crtBackupQueue = getBackupQueueName( transportHeaders );
	string crtQueueManager = getQueueManagerName( transportHeaders );

	DEBUG( "Using queue manager [" << crtQueueManager << "]" );
	DEBUG( "Using queue [" << crtQueue << "]" );
	
	if ( asClient )  // Client - put inputData in queue
	{
		//m_ProcessedMessageSize = 0;
		if ( isBatch( transportHeaders ) )
		{
			if ( !transportHeaders.ContainsKey( MQGROUPID ) )
				throw logic_error( "Expected parameter of WMQ batch [MQGROUPID] missing" );

			if ( !transportHeaders.ContainsKey( MQSEQUENCE ) )
				throw logic_error( "Expected parameter of WMQ batch [MQSEQUENCE] missing" );

			string batchId = getGroupId( transportHeaders );
			string messageId = getMessageId( transportHeaders );

			DEBUG( "MQFilter running in batch mode.");
			int lastId = 0;
			if ( ( m_BatchManager != NULL ) && ( m_CrtBatchId != getGroupId( transportHeaders ) ) )
			{
				m_BatchManager->close( m_CrtBatchId );
				delete m_BatchManager;
				m_BatchManager = NULL;
			}

			// if first in Batch
			if ( m_BatchManager == NULL )
			{
				//create WMQ BatchManager
				m_BatchManager = new BatchManager<BatchMQStorage>( BatchManagerBase::WMQ, BatchResolution::SYNCHRONOUS );
				m_BatchManager->storage().setQueue( crtQueue );
				m_BatchManager->storage().setBackupQueue( crtBackupQueue );
				m_BatchManager->storage().setQueueManager( crtQueueManager );
				m_BatchManager->storage().setTransportURI( getTransportURI( transportHeaders ) );
				m_BatchManager->storage().setAutoAbandon( 3 );
				m_BatchManager->storage().setCleaningUp( false );

				//open WMQ BatchManager
				m_BatchManager->open( batchId, ios_base::out );
			}

			bool isLast = transportHeaders.ContainsKey( MQLASTMSG );
			int batchSequence = StringUtil::ParseInt( transportHeaders[ MQSEQUENCE ] );

			ManagedBuffer* inputBuffer = inputData.get();

			BatchItem item( batchSequence, batchId, messageId, isLast );
			item.setPayload( inputBuffer );

			*m_BatchManager << item;

			if ( isLast )
				m_BatchManager->close( batchId );
		}
		else
		{
			//m_CrtHelper->setOpenQueueOptions( TransportHelper::OUTPUT );
			m_CrtHelper->connect( crtQueueManager, getTransportURI( transportHeaders ) );
			m_CrtHelper->openQueue( crtQueue );
		
			if( transportHeaders.ContainsKey( MQMSGID ) )
				m_CrtHelper->setMessageId( transportHeaders[ MQMSGID ] );
		
			if( transportHeaders.ContainsKey( MQMSGCORELID ) )
				m_CrtHelper->setCorrelationId( transportHeaders[ MQMSGCORELID ] );	
		
			if( transportHeaders.ContainsKey( MQAPPNAME ) )
				m_CrtHelper->setApplicationName( transportHeaders[ MQAPPNAME ] );

			ManagedBuffer* inputBuffer = inputData.get();
			m_CrtHelper->setMessageFormat( getFormat() );
			m_CrtHelper->putOne( inputBuffer->buffer(), inputBuffer->size() );
		}
	}
	else  // Server - get one message and put it in outputData
	{
		//m_CrtHelper->setOpenQueueOptions(TransportHelper::INPUT );
		m_CrtHelper->connect( crtQueueManager, getTransportURI( transportHeaders ) );
		m_CrtHelper->setBackupQueue( crtBackupQueue );
		m_CrtHelper->openQueue( crtQueue );
		
		// compute max buffer size
		ManagedBuffer* outputBuffer = outputData.get();

		if( transportHeaders.ContainsKey( MQMSGID ) )
		{
			DEBUG( "Passing message id [" << transportHeaders[ MQMSGID ] << "]" );
			m_CrtHelper->setMessageId( transportHeaders[ MQMSGID ] );
		}

		if ( transportHeaders.ContainsKey( MQMSGSIZE ) )
		{
			DEBUG( "Passing size [" << transportHeaders[ MQMSGSIZE ] << "]" );
			int passedSize = atoi( transportHeaders[ MQMSGSIZE ].data() );
			if ( outputBuffer->size() == 0 )
				outputBuffer->allocate( passedSize );
		}
		
		DEBUG( "Getting message ..." );
		long result = m_CrtHelper->getOne( outputBuffer->buffer(), outputBuffer->size() );

		// no message 
		if ( result == -1 )
		{
			AppException aex( "No message matching the specified ids was found.", EventType::Warning );
			aex.setSeverity( EventSeverity::Fatal );
			
			throw aex;
		}

		// the message was moved to dead letter
		if ( result == -2 )
		{
			AppException aex( "Undeliverable message sent to dead letter queue because the backout count was exceeded", EventType::Warning );
			aex.setSeverity( EventSeverity::Fatal );

			throw aex;
		}

		outputBuffer->truncate( m_CrtHelper->getLastMessageLength() );

		if ( outputBuffer->size() > 100 )
		{
			string partialBuffer = string( ( char* )outputBuffer->buffer(), 100 );
			DEBUG( "Partial buffer ( first 100 bytes ) [" << partialBuffer << "]" );
		}
		else
		{
			DEBUG( "Buffer [" << outputBuffer->str() << "]" );
		}
	}
	setTransportHeaders( transportHeaders, asClient );	
	return AbstractFilter::Completed;	
}

void MqFilter::Reply( const string& buffer, const NameValueCollection& transportHeaders, long feedback )
{
/*
	if ( transportHeaders.ContainsKey( MqFilter::MQMESSAGETYPE ) && ( transportHeaders[ MqFilter::MQMESSAGETYPE ] == "REQUEST" ) )
	{
		DEBUG( "Put one reply as requested" );
	
		string rtqName = "SYSTEM.DEAD.LETTER.QUEUE", rtqmName = "";
		string msgId = "", correlationId = ""; 
		string replyUsrData = "", replyMsgFormat = MQFMT_STRING;
		long replyOptions = 0;
		long replyMessageType = MQMT_REPLY;
				
		if ( transportHeaders.ContainsKey( MqFilter::MQREPLYOPTIONS ) )
		{
			replyOptions = atol( ( transportHeaders[ MqFilter::MQREPLYOPTIONS ] ).data() );
			if ( ( ( ( replyOptions & MQRO_PAN ) == MQRO_PAN ) && ( ( feedback & MQFB_PAN ) != MQFB_PAN ) ) ||
				( ( ( replyOptions & MQRO_NAN ) == MQRO_NAN ) && ( ( feedback & MQFB_NAN ) != MQFB_NAN ) ) )
			{
				DEBUG( "Reply feedback doesn't match requested reply options. No reply will be sent." );
				return;
			}
		}
		DEBUG( "ReplyOptions : [" << replyOptions << "]" );

		if ( transportHeaders.ContainsKey( MqFilter::MQREPLYQUEUE ) )
			rtqName = transportHeaders[ MqFilter::MQREPLYQUEUE ];
		DEBUG( "Reply-to Q : [" << rtqName << "]" );
					
		if ( transportHeaders.ContainsKey( MqFilter::MQREPLYQUEUEMANAGER ) )
			rtqmName = transportHeaders[ MqFilter::MQREPLYQUEUEMANAGER ];
		DEBUG( "Reply-to QM : [" << rtqmName << "]" );
		
		if( transportHeaders.ContainsKey( MqFilter::MQMSGID ) )
			msgId = transportHeaders[ MqFilter::MQMSGID ];
		DEBUG( "Message ID : [" << msgId << "]" );
		
		if( transportHeaders.ContainsKey( MqFilter::MQMSGCORELID ) )
			correlationId = transportHeaders[ MqFilter::MQMSGCORELID ];
		DEBUG( "CorrelationID : [" << correlationId << "]" );
		
		if ( m_Properties.ContainsKey( MqFilter::MQRPLMESSAGETYPE ) )
			replyMessageType = atol( ( m_Properties[ MqFilter::MQRPLMESSAGETYPE ] ).data() );
		DEBUG( "ReplyMessageType : [" << replyMessageType << "]" );
		
		if ( m_Properties.ContainsKey( MqFilter::MQRPLUSRDATA ) )
			replyUsrData = m_Properties[ MqFilter::MQRPLUSRDATA ];
		DEBUG( "ReplyUsrData : [" << replyUsrData << "]" );
		
		if ( m_Properties.ContainsKey( MqFilter::MQRPLMESSAGEFORMAT ) )
			replyMsgFormat = m_Properties[ MqFilter::MQRPLMESSAGEFORMAT ];
		DEBUG( "ReplyMessageFormat : [" << replyMsgFormat << "]" );
			
		if( ( replyOptions & MQRO_COPY_MSG_ID_TO_CORREL_ID ) == MQRO_COPY_MSG_ID_TO_CORREL_ID )
			correlationId = msgId;
		
		m_CrtHelper->disconnect();

		TransportHelper* myHelper = TransportHelper::CreateHelper( m_HelperType );
		//myHelper->setOpenQueueOptions( TransportHelper::OUTPUT );
		myHelper->connect( rtqmName, getReplyTransportURI( transportHeaders ), 
			getSSLKeyRepository( transportHeaders ),
			getSSLCypherSpec( transportHeaders ), 
			getSSLPeerName( transportHeaders ) );
		myHelper->openQueue( rtqName );
		
		myHelper->setCorrelationId( correlationId );

		myHelper->setReplyUserData( replyUsrData );
		myHelper->setMessageFormat( replyMsgFormat );
		myHelper->putOneReply( ( unsigned char* )buffer.c_str(), buffer.length(), feedback, replyMessageType );
		myHelper->setReplyUserData( "" );
		myHelper->clearSSLOptions();
		
		myHelper->commit();
		myHelper->closeQueue();
		myHelper->disconnect();
		delete myHelper;
	}
	else 
	{
		DEBUG( "Message type is not [REQUEST], so we don't send a reply " );
	}
*/
}

void MqFilter::setTransportHeaders( NameValueCollection& transportHeaders, bool asClient )
{
	if ( asClient )  // Client - put inputData in queue
	{
		// push ids to tr. headers
		if ( transportHeaders.ContainsKey( MQMSGID ) )
			transportHeaders.ChangeValue( MQMSGID, m_CrtHelper->getLastMessageId() );
		else
			transportHeaders.Add( MQMSGID, m_CrtHelper->getLastMessageId() );
			
		if ( transportHeaders.ContainsKey( MQMSGCORELID ) )
			transportHeaders.ChangeValue( MQMSGCORELID, m_CrtHelper->getLastCorrelId() );
		else
			transportHeaders.Add( MQMSGCORELID, m_CrtHelper->getLastCorrelId() );
	}
	else
	{		
		// push ids to tr. headers
		if ( transportHeaders.ContainsKey( MQMSGID ) )
			transportHeaders.ChangeValue( MQMSGID, m_CrtHelper->getLastMessageId() );
		else
			transportHeaders.Add( MQMSGID, m_CrtHelper->getLastMessageId() );
			
		if ( transportHeaders.ContainsKey( MQMSGCORELID ) )
			transportHeaders.ChangeValue( MQMSGCORELID, m_CrtHelper->getLastCorrelId() );
		else
			transportHeaders.Add( MQMSGCORELID, m_CrtHelper->getLastCorrelId() );
			
		// only supported if request
		if ( m_CrtHelper->getLastMessageType() == TransportHelper::TMT_REQUEST )
		{			
			if ( transportHeaders.ContainsKey( MQMESSAGETYPE ) )
				transportHeaders.ChangeValue( MQMESSAGETYPE, "REQUEST" );
			else
				transportHeaders.Add( MQMESSAGETYPE, "REQUEST" );
			
			if ( transportHeaders.ContainsKey( MQREPLYQUEUE ) )
				transportHeaders.ChangeValue( MQREPLYQUEUE, m_CrtHelper->getLastReplyQueue() );
			else
				transportHeaders.Add( MQREPLYQUEUE, m_CrtHelper->getLastReplyQueue() );
				
			if ( transportHeaders.ContainsKey( MQREPLYQUEUEMANAGER ) )
				transportHeaders.ChangeValue( MQREPLYQUEUEMANAGER, m_CrtHelper->getLastReplyQueueManager() );
			else
				transportHeaders.Add( MQREPLYQUEUEMANAGER, m_CrtHelper->getLastReplyQueueManager() );
				
			if ( transportHeaders.ContainsKey( MQREPLYOPTIONS ) )
				transportHeaders.ChangeValue( MQREPLYOPTIONS, m_CrtHelper->getLastReplyOptions().ToString() );
			else
				transportHeaders.Add( MQREPLYOPTIONS, m_CrtHelper->getLastReplyOptions().ToString() );
		}
		else if ( m_CrtHelper->getLastMessageType() == TransportHelper::TMT_REPLY )
		{
			if ( transportHeaders.ContainsKey( MQMESSAGETYPE ) )
				transportHeaders.ChangeValue( MQMESSAGETYPE, "REPLY" );
			else
				transportHeaders.Add( MQMESSAGETYPE, "REPLY" );
				
			if ( transportHeaders.ContainsKey( MQFEEDBACK ) )
				transportHeaders.ChangeValue( MQFEEDBACK, StringUtil::ToString( m_CrtHelper->getLastFeedback() ) );
			else
				transportHeaders.Add( MQFEEDBACK, StringUtil::ToString( m_CrtHelper->getLastFeedback() ) );
		}
		else
		{
			if ( transportHeaders.ContainsKey( MQMESSAGETYPE ) )
				transportHeaders.ChangeValue( MQMESSAGETYPE, "DATAGRAM" );
			else
				transportHeaders.Add( MQMESSAGETYPE, "DATAGRAM" );
		}
	}
}

void MqFilter::Rollback()
{
	m_CrtHelper->rollback();
	if ( m_BatchManager != NULL )
	{
		//BatchItem prevItem = m_BatchManager->moreMessagesm();
		Cleanup();
		m_BatchManager->rollback(); 
	}
}

void MqFilter::Commit()
{
	m_CrtHelper->commit();
	if ( m_BatchManager != NULL )
	{
		m_BatchManager->commit();
		DEBUG( "Closing WMQ batch" );
		m_BatchManager->close( m_CrtBatchId );
		
		DEBUG( "Deleting batch manager..." );
		delete m_BatchManager;
		m_BatchManager = NULL;
	}
}

/// <summary>If filter proccess batches the entire batch is moved to DEAD.LETTER by Cleanup()
/// for further details see MaFilter::CleanUp()
/// </summary>
void MqFilter::Abort()
{
	m_CrtHelper->commit();
	if ( m_BatchManager != NULL )
	{	
		m_BatchManager->commit();

		DEBUG( "Closing WMQ batch" );
		m_BatchManager->close( m_CrtBatchId );
		
		DEBUG( "Deleting batch manager..." );
		delete m_BatchManager;
		m_BatchManager = NULL;
	}
}
		
/// private methods implementation
void MqFilter::ValidateProperties()
{

}

bool MqFilter::isMethodSupported( AbstractFilter::FilterMethod method, bool asClient )
{
	return true;
}

bool MqFilter::canLogPayload()
{
	return true;
}

string MqFilter::getQueueManagerName() const
{
	// now try properties
	if ( m_Properties.ContainsKey( MqFilter::MQMANAGER ) )
		return m_Properties[ MqFilter::MQMANAGER ];
	return "";
}

string MqFilter::getQueueName() const
{
	if ( m_Properties.ContainsKey( MqFilter::MQQUEUE ) )
		return m_Properties[ MqFilter::MQQUEUE ];
	return "";
}

string MqFilter::getTransportURI() const
{
	if ( m_Properties.ContainsKey( MqFilter::MQURI ) )
		return m_Properties[ MqFilter::MQURI ];
	return "";
}

string MqFilter::getReplyTransportURI( const NameValueCollection& transportHeaders ) const
{
	if ( transportHeaders.ContainsKey( MqFilter::MQRPLTransportURI ) )
		return transportHeaders[ MqFilter::MQRPLTransportURI ];

	if ( m_Properties.ContainsKey( MqFilter::MQRPLTransportURI ) )
		return m_Properties[ MqFilter::MQRPLTransportURI ];
		
	return getTransportURI( transportHeaders );
}

string MqFilter::getSSLKeyRepository( const NameValueCollection& transportHeaders ) const
{
	if ( transportHeaders.ContainsKey( MqFilter::MQSSLKEYREPOSITORY ) )
		return transportHeaders[ MqFilter::MQSSLKEYREPOSITORY ];

	if ( m_Properties.ContainsKey( MqFilter::MQSSLKEYREPOSITORY ) )
		return m_Properties[ MqFilter::MQSSLKEYREPOSITORY ];
	return "";
}

string MqFilter::getSSLCypherSpec( const NameValueCollection& transportHeaders ) const
{
	if ( transportHeaders.ContainsKey( MqFilter::MQSSLCYPHERSPEC ) )
		return transportHeaders[ MqFilter::MQSSLCYPHERSPEC ];

	if ( m_Properties.ContainsKey( MqFilter::MQSSLCYPHERSPEC ) )
		return m_Properties[ MqFilter::MQSSLCYPHERSPEC ];
	return "";
}

string MqFilter::getSSLPeerName( const NameValueCollection& transportHeaders ) const
{
	if ( transportHeaders.ContainsKey( MqFilter::MQSSLPEERNAME ) )
		return transportHeaders[ MqFilter::MQSSLPEERNAME ];

	if ( m_Properties.ContainsKey( MqFilter::MQSSLPEERNAME ) )
		return m_Properties[ MqFilter::MQSSLPEERNAME ];
	return "";
}

string MqFilter::getQueueManagerName( NameValueCollection& transportHeaders ) const
{
	// try tr headers
	if ( transportHeaders.ContainsKey( MQMANAGER ) )
		return transportHeaders[ MQMANAGER ];
		
	// now try properties
	if ( m_Properties.ContainsKey( MqFilter::MQMANAGER ) )
		return m_Properties[ MqFilter::MQMANAGER ];
		
	// queue manager is a must
	throw invalid_argument( "Required parameter missing : MQMANAGER" );
}

string MqFilter::getQueueName( NameValueCollection& transportHeaders ) const
{
	// try tr headers
	if ( transportHeaders.ContainsKey( MqFilter::MQQUEUE ) )
		return transportHeaders[ MqFilter::MQQUEUE ];
	
	// now try properties
	if ( m_Properties.ContainsKey( MqFilter::MQQUEUE ) )
		return m_Properties[ MqFilter::MQQUEUE ];
		
	// queue is a must
	throw invalid_argument( "Required parameter missing : MQQUEUE" );
}

string MqFilter::getBackupQueueName( NameValueCollection& transportHeaders ) const
{
	// try tr headers
	if ( transportHeaders.ContainsKey( MqFilter::MQBACKUPQUEUE ) )
		return transportHeaders[ MqFilter::MQBACKUPQUEUE ];

	if ( m_Properties.ContainsKey( MqFilter::MQBACKUPQUEUE ) )
		return m_Properties[ MqFilter::MQBACKUPQUEUE ];
	return "";
}

TransportHelper::TRANSPORT_HELPER_TYPE MqFilter::getHelperType() const
{
	if ( m_Properties.ContainsKey( MqFilter::MQHELPERTYPE ) )
	{
		return TransportHelper::parseTransportType( m_Properties[ MqFilter::MQHELPERTYPE ] ); 
	}
	return TransportHelper::NONE;
}

string MqFilter::getTransportURI( const NameValueCollection& transportHeaders ) const
{
	if ( transportHeaders.ContainsKey( MqFilter::MQURI ) )
		return transportHeaders[ MqFilter::MQURI ];
		
	// now try properties
	if ( m_Properties.ContainsKey( MqFilter::MQURI ) )
		return m_Properties[ MqFilter::MQURI ];
	
	return "";
}

bool MqFilter::isBatch( NameValueCollection& transportHeaders ) const
{
	if ( transportHeaders.ContainsKey( MqFilter::MQBATCH ) && ( transportHeaders[ MqFilter::MQBATCH ] == "true" ) )
	{	
		DEBUG( "Batch mode processing ..." );	
		return true;
	}
	
	// now try properties
	if ( m_Properties.ContainsKey( MqFilter::MQBATCH ) && ( m_Properties[ MqFilter::MQBATCH ] == "true" ) )
	{
		DEBUG( "Batch mode processing ..." );
		return true;
	}
		
	DEBUG( "Single mode processing ..." );
	return false ;
}

bool MqFilter::isBatch() const
{
	return ( m_Properties.ContainsKey( MqFilter::MQBATCH ) && ( m_Properties[ MqFilter::MQBATCH ] == "true" ) );
}

string MqFilter::getFormat()
{
	if ( m_Properties.ContainsKey( MqFilter::MQFORMAT ) )
	{
		DEBUG( "Format set to [" << m_Properties[ MqFilter::MQFORMAT ] << "]" );
		return m_Properties[ MqFilter::MQFORMAT ];
	}
	DEBUG( "Format set to [" << TransportHelper::TMT_STRING << "]" );
	return TransportHelper::TMT_STRING;
}

string MqFilter::getMessageId( NameValueCollection& transportHeaders ) const
{
	if ( transportHeaders.ContainsKey( MqFilter::MQMSGID ) )
		return transportHeaders[ MqFilter::MQMSGID ];
	
	return "";
}

string MqFilter::getGroupId( NameValueCollection& transportHeaders ) const
{
	if ( transportHeaders.ContainsKey( MqFilter::MQGROUPID ) )
		return transportHeaders[ MqFilter::MQGROUPID ];
	
	return "";
}

string MqFilter::getCorrelationId( NameValueCollection& transportHeaders ) const
{
	if ( transportHeaders.ContainsKey( MqFilter::MQMSGCORELID ) )
		return transportHeaders[ MqFilter::MQMSGCORELID ];
	
	return "";
}
/// <summary>This Method is used by MqFilter::Abort()to move al messages to DEAD.LETTER. 
/// ::setCleaningUp() tell the storage to use the suitable get() method that move message to DEAD.LETTER
/// see BatchManager<BatchMQStorage>::deqeue()
/// In MqPublisher MqPublisher::Abort() was called only if entire batch was retried for proper MqPublisher::m_BackoutCont.
/// When the code is getting here the batch will not be proccess any more
/// </summary>
void MqFilter::Cleanup()
{
	DEBUG ( "Cleaning up batch..." );
	if ( m_BatchManager != NULL )
	{
		while( m_BatchManager->moreMessages() )
		{
			BatchItem batchItem ;
			*m_BatchManager >> batchItem; 
		}	
	}
}

void MqFilter::Init()
{
	if ( m_CrtHelper == NULL )
	{
		m_HelperType = getHelperType();
		m_CrtHelper = TransportHelper::CreateHelper( m_HelperType  );
		m_CrtHelper->setAutoAbandon( 3 );
	}
}
