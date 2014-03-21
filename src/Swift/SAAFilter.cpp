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

#include "SAAFilter.h"
#include "Trace.h"
#include "AppExceptions.h"
#include "TimeUtil.h"
#include "Base64.h"
#include "MQ/MqFilter.h"
#include "Swift/SwiftFormatFilter.h"

namespace FinTP
{

const string SAAFilter::SAAReplyQueue = "SAAReplyQueue";
const string SAAFilter::MESSAGE_DATE = "MessageDate";

bool SAAFilter::isMethodSupported( FilterMethod method, bool asClient )
{
	switch ( method )
	{
	case SAAFilter::BufferToBuffer :
		return true;
	default:
		return false;
	}
}

AbstractFilter::FilterResult SAAFilter::ProcessMessage( AbstractFilter::buffer_type inputData, unsigned char** outputData, NameValueCollection& transportHeaders, bool asClient )
{
	throw FilterInvalidMethod( AbstractFilter::BufferToBuffer );
}

AbstractFilter::FilterResult SAAFilter::ProcessMessage( AbstractFilter::buffer_type inputData, AbstractFilter::buffer_type outputData, NameValueCollection& transportHeaders, bool asClient )
{
	ManagedBuffer *inputBuffer = inputData.get();
	string groupId = transportHeaders[ MqFilter::MQGROUPID ];
	if( !asClient )
	{
		if ( inputBuffer )
		{
			DEBUG( "Applying SAA filter..." );

			XERCES_CPP_NAMESPACE_QUALIFIER DOMDocument* doc = NULL;
			char* outputXslt = NULL;

			try
			{
				const string& payload = inputBuffer->str();
				doc = XmlUtil::DeserializeFromString( payload );
				XSLTFilter xsltFilter;
				xsltFilter.ProcessMessage( doc, reinterpret_cast< unsigned char** >( &outputXslt), transportHeaders, true );
				size_t outputXsltSize = strlen( outputXslt );
				WorkItem< ManagedBuffer > inBuffer( new ManagedBuffer( reinterpret_cast<unsigned char*> ( outputXslt ), ManagedBuffer::Ref, outputXsltSize) );
				WorkItem< ManagedBuffer > outBuffer( new ManagedBuffer() );
				SwiftFormatFilter swiftFormatFilter;
				swiftFormatFilter.ProcessMessage( inBuffer, outBuffer, transportHeaders, false );
				BatchItem item;
				m_BatchStorage->open( groupId, ios_base::out );
				item.setBinPayload( outBuffer.get() );
				item.setBatchId( groupId );
				item.setSequence( 1 );
				*m_BatchStorage << item;
				item.setSequence( 2 );
				item.setLast();
				item.setPayload( payload );
				*m_BatchStorage << item;
			}
			catch( ... )
			{
				if ( doc != NULL )
					doc->release();
				delete outputXslt;
				TRACE( "Unhandled exception while applying SAA filter");
				throw;
			}
			doc->release();
			delete outputXslt;

			return SAAFilter::Completed;
		}
	}
	else
	{
		DEBUG( "Applying SAA filter ..." );

		WorkItem< ManagedBuffer > outputBuffer( new ManagedBuffer() );
		ManagedBuffer* managedOutputData = outputData.get();
		WorkItem< ManagedBuffer > inputBuffer( new ManagedBuffer() );
		ManagedBuffer* managedInputBuffer = inputBuffer.get();

		m_BatchStorage->open( Base64::decode( groupId ), ios_base::in );
		BatchItem batchItem;
		bool backoutCountExceeds = false;
		try
		{
			//get partner message
			*m_BatchStorage >> batchItem;
			managedInputBuffer->copyFrom( batchItem.getPayload() );

			//check backout count
			// TODO: Change backout count expose mechanism to avoid cast
			BatchManager<BatchMQStorage>* castStorage = dynamic_cast< BatchManager<BatchMQStorage> * >( m_BatchStorage.get() );
			if( castStorage != NULL )
			{
				//backout count exceeded, try Abort
				if( castStorage->storage().getCleaningUp() )
				{
					*m_BatchStorage >> batchItem;
					managedOutputData->copyFrom( batchItem.getPayload() );
					// Dont't  rely on filter user to Abort;
					// The commit is save because SAAFilter-server is always the first stage of message processing
					// ( nothing is partial commited when backout count exceeded)
					castStorage->storage().setCleaningUp( false );
					m_BatchStorage->commit();

					AppException aex( "FileAct message moved to dead letter queue because backout count exceeded" );
					aex.setSeverity( EventSeverity::Fatal );
					throw aex;
				}
			}
			else
				TRACE( "Backout counts should be check here..." )

			//check signature in partner message
			SwiftFormatFilter swiftFormatFilter;
			swiftFormatFilter.ProcessMessage( inputBuffer, outputBuffer, transportHeaders, true );
		}
		catch( runtime_error &ex )
		{
			// Probably no message there, rely on filter user to Abort
			if( ( batchItem.getPayload() ).empty() )
			{
				AppException aex( "No message matching the specified ids was found.", ex, EventType::Warning );
				aex.setSeverity( EventSeverity::Fatal );
				throw aex;
			}
			//increase backout count and rely om filter user to Rollback
			*m_BatchStorage >> batchItem;
			managedOutputData->copyFrom( batchItem.getPayload() );
			throw ex;
		}

		//ignore partner message and get paylod ( second message in wmq group )
		*m_BatchStorage >> batchItem;

		managedOutputData->copyFrom( batchItem.getPayload() );
		time_t createDate = batchItem.getCreateDate();
		transportHeaders.Add( SAAFilter::MESSAGE_DATE, TimeUtil::Get( "%d/%m/%Y %H:%M:%S", 19, &createDate ) );

		return SAAFilter::Completed;
	}
	TRACE( "SAAFilter not applied!" )
	return SAAFilter::Completed;
}

AbstractFilter::FilterResult SAAFilter::ProcessMessage( XERCES_CPP_NAMESPACE_QUALIFIER DOMDocument* inputOutputData, NameValueCollection& transportHeaders, bool asClient )
{
	throw FilterInvalidMethod( AbstractFilter::XmlToXml );
}

AbstractFilter::FilterResult SAAFilter::ProcessMessage( const XERCES_CPP_NAMESPACE_QUALIFIER DOMDocument* inputData, AbstractFilter::buffer_type outputData, NameValueCollection& transportHeaders, bool asClient )
{
	throw FilterInvalidMethod( AbstractFilter::XmlToBuffer );
}

AbstractFilter::FilterResult SAAFilter::ProcessMessage( const XERCES_CPP_NAMESPACE_QUALIFIER DOMDocument* inputData, unsigned char** outputData, NameValueCollection& transportHeaders, bool asClient )
{
	throw FilterInvalidMethod( AbstractFilter::XmlToBuffer );
}

AbstractFilter::FilterResult SAAFilter::ProcessMessage( AbstractFilter::buffer_type inputData, XERCES_CPP_NAMESPACE_QUALIFIER DOMDocument* outputData, NameValueCollection& transportHeaders, bool asClient )
{
	throw FilterInvalidMethod( AbstractFilter::BufferToXml );
}

AbstractFilter::FilterResult SAAFilter::ProcessMessage( unsigned char* inputData, XERCES_CPP_NAMESPACE_QUALIFIER DOMDocument* outputData, NameValueCollection& transportHeaders, bool asClient )
{
	throw FilterInvalidMethod( AbstractFilter::BufferToXml );
}

void SAAFilter::Commit()
{
	if(  m_BatchStorage == NULL )
		throw runtime_error ( "Null SAA BatchManager " );
	else
		 m_BatchStorage->commit();
}

void SAAFilter::Rollback()
{
	if(  m_BatchStorage == NULL )
		throw runtime_error ( "Null SAA BatchManager " );
	else
		 m_BatchStorage->rollback();
}

}
