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

#include "XmlUtil.h"
#include "Trace.h"
#include "BatchItem.h"

using namespace std;
using namespace FinTP;

const int BatchItem::FIRST_IN_SEQUENCE = 1;
const int BatchItem::LAST_IN_SEQUENCE = INT_MAX;
const int BatchItem::INVALID_SEQUENCE = -1;

BatchItem::BatchItem( const int sequence, const string& batchId, const string& messageId, 
	const bool isLastItem, const string& eyeCatcher ) :
	m_Eyecatcher( eyeCatcher ), m_Payload( "" ), m_XmlPayload( NULL ), m_BinPayload( NULL ),
	m_PayloadType( BATCHITEM_TXT ),	m_Sequence( sequence ), m_BatchId( batchId ), 
	m_MessageId( messageId ), m_IsLast( isLastItem ), m_XmlOwner( false )
{
}

BatchItem::BatchItem( const BatchItem& source ) : 
	m_Eyecatcher( source.m_Eyecatcher ), m_Payload( "" ), m_XmlPayload( NULL ), m_BinPayload( NULL ), 
	m_PayloadType( source.m_PayloadType ), m_Sequence( source.m_Sequence ), m_BatchId( source.m_BatchId ),
	m_MessageId( source.m_MessageId ), m_IsLast( source.m_IsLast ), m_XmlOwner( false )	
{
	switch( source.getPayloadType() )
	{
		case BATCHITEM_TXT :
			setPayload( source.getPayload() );
			break;

		case BATCHITEM_XML :
			// if the source created this payload...
			if ( source.m_XmlOwner )
				throw logic_error( "Attempt to use dangling xml payload" );
			m_XmlPayload = source.m_XmlPayload;
			break;

		case BATCHITEM_BIN :
			m_BinPayload = new ManagedBuffer( *source.getBinPayload() );
			break;
	}
}

BatchItem& BatchItem::operator=( const BatchItem& source )
{
	if ( this == &source )
		return *this;

	m_Eyecatcher = source.getEyecatcher();
	m_Payload = "";
	m_XmlPayload = NULL;
	m_PayloadType = source.getPayloadType();
	m_Sequence = source.getSequence();
	m_BatchId = source.getBatchId();
	m_MessageId = source.getMessageId();
	m_IsLast = source.isLast();
	m_XmlOwner = false;
	
	switch( source.getPayloadType() )
	{
		case BATCHITEM_TXT :
			setPayload( source.getPayload() );
			break;

		case BATCHITEM_XML :
			// if the source created this payload...
			if ( source.m_XmlOwner )
				throw logic_error( "Attempt to use dangling xml payload" );
			m_XmlPayload = source.m_XmlPayload;
			break;

		case BATCHITEM_BIN :
			if ( m_BinPayload != NULL )
			{
				delete m_BinPayload;
				m_BinPayload = NULL;
			}
			m_BinPayload = new ManagedBuffer( *source.getBinPayload() );
			break;
	}		
	return *this;
}

BatchItem::~BatchItem()
{
	if ( m_BinPayload != NULL )
	{
		delete m_BinPayload;
		m_BinPayload = NULL;
	}

	if ( !m_XmlOwner )
		return;

	try
	{
		if ( m_XmlPayload != NULL )
		{
			m_XmlPayload->release();
			m_XmlPayload = NULL;
		}
	}
	catch( ... )
	{
		try
		{
			TRACE( "An error occured when releasing the payload" );
		} catch( ... ){}
	}
}

void BatchItem::setPayload( const string& payload )
{
	DEBUG( "Setting TXT payload" );
	if ( m_XmlOwner && ( m_XmlPayload != NULL ) )
	{
		m_XmlPayload->release();
		m_XmlPayload = NULL;
	}

	m_Payload = payload;
	m_PayloadType = BATCHITEM_TXT;
}

void BatchItem::setPayload( const char* payload )
{
	DEBUG( "Setting TXT payload" );
	if ( m_XmlOwner && ( m_XmlPayload != NULL ) )
	{
		m_XmlPayload->release();
		m_XmlPayload = NULL;
	}

	m_Payload = string( payload );
	m_PayloadType = BATCHITEM_TXT;
}

void BatchItem::setPayload( const ManagedBuffer* buffer )
{
	DEBUG( "Setting TXT payload" );
	if ( m_XmlOwner && ( m_XmlPayload != NULL ) )
	{
		m_XmlPayload->release();
		m_XmlPayload = NULL;
	}

	m_Payload = buffer->str();
	m_PayloadType = BATCHITEM_TXT;
}

void BatchItem::setBinPayload( const ManagedBuffer* buffer )
{
	DEBUG( "Setting BIN payload" );
	if ( m_XmlOwner && ( m_XmlPayload != NULL ) )
	{
		m_XmlPayload->release();
		m_XmlPayload = NULL;
	}

	if ( m_BinPayload != NULL )
	{
		delete m_BinPayload;
		m_BinPayload = NULL;
	}
	m_BinPayload = new ManagedBuffer( *buffer );
	m_PayloadType = BATCHITEM_BIN;
}

void BatchItem::setXmlPayload( XERCES_CPP_NAMESPACE_QUALIFIER DOMDocument* payload )
{
	DEBUG( "Setting XML payload" );
	if ( m_XmlOwner && ( m_XmlPayload != NULL ) )
	{
		m_XmlPayload->release();
		m_XmlPayload = NULL;
	}

	m_XmlOwner = false;
	m_XmlPayload = payload;
	m_PayloadType = BATCHITEM_XML;
}

string BatchItem::getPayload() const
{
	if ( m_PayloadType == BATCHITEM_TXT )
	{
		// payload is already string 
		return m_Payload;
	}
	else
	{
		return XmlUtil::SerializeToString( m_XmlPayload );
	}		
}

ManagedBuffer* BatchItem::getBinPayload() const
{
	if ( m_PayloadType == BATCHITEM_BIN )
	{
		// payload is already XML
		return m_BinPayload;
	}
	else
	{
		throw runtime_error( "Unable to return const BIN payload [TXT/XML payload]" );
	}
}

const XERCES_CPP_NAMESPACE_QUALIFIER DOMDocument* BatchItem::getXmlPayload() const
{
	if ( m_PayloadType == BATCHITEM_XML )
	{
		// payload is already XML
		return m_XmlPayload;
	}
	else
	{
		throw runtime_error( "Unable to return const XML payload [TXT/BIN payload]" );
	}
}

XERCES_CPP_NAMESPACE_QUALIFIER DOMDocument* BatchItem::getXmlPayload() 
{
	if ( m_PayloadType == BATCHITEM_XML )
	{
		// payload is already DOM 
		return m_XmlPayload;
	}
	else
	{
		if ( m_XmlPayload == NULL )
		{
			m_XmlOwner = true;
			m_XmlPayload = XmlUtil::DeserializeFromString( m_Payload );
		}
		return m_XmlPayload;
	}
}

bool operator !=( const BatchItem& lparamBI, const BatchItem& rparamBI )
{
	if ( lparamBI.getSequence() != rparamBI.getSequence() )
		return true;
	/*if ( m_BatchId != source.getBatchId() )
		return true;
	if ( m_MessageId != source.getMessageId() )
		return true;
	if ( m_Payload != source.str() )
		return true;*/
	return false;
}
