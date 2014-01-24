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

#ifdef ZIPARCHIVE
#include "Storages/BatchZipArchiveStorage.h"
#else
#include "Storages/BatchZipStorage.h"
#endif

#include "BatchManager.h"
#include "BatchItemEval.h"

#include "Trace.h"
#include "StringUtil.h"

#include "Storages/BatchFlatfileStorage.h"
#include "Storages/BatchXMLfileStorage.h"
#include "Storages/BatchMQStorage.h"

#ifdef XALAN_1_9
#include <xalanc/Include/XalanMemoryManagement.hpp>
#endif

#include <xalanc/XercesParserLiaison/XercesDocumentWrapper.hpp>

#include <sstream>
#include <errno.h>
#include <iostream>
#include <string>

// globals
using namespace std;
using namespace BatchManip;
using namespace FinTP;

BufferSize setw( int size )
{
	return BufferSize( size );
}

BatchMetadata setmeta( XERCES_CPP_NAMESPACE_QUALIFIER DOMDocument* const document )
{
	return BatchMetadata( document );
}

//BatchManagerBase implementation
BatchManagerBase::BatchManagerBase( const BatchManagerBase::StorageCategory storageCategory ) :
	m_XPath( "" ), m_StorageCategory( storageCategory ), m_MoreMessages( true ), m_MetaDoc( NULL )
{
}

string BatchManagerBase::ToString( const BatchManagerBase::BATCH_STATUS status )
{
	switch( status )
	{
		case BatchManagerBase::BATCH_NEW :
			return "BATCH_NEW";
		case BatchManagerBase::BATCH_INPROGRESS :
			return "BATCH_INPROGRESS";
		case BATCH_READY :
			return "BATCH_READY";
		case BATCH_FAILED :
			return "BATCH_FAILED";
		case BATCH_COMPLETED :
			return "BATCH_COMPLETED";
		default:
			{
				stringstream errorMessage;
				errorMessage << "Invalid value for batch status [" << ( long )status << "]";
				throw logic_error( errorMessage.str() );
			}
	};
}

BatchManagerBase* BatchManagerBase::CreateBatchManager( const BatchManagerBase::StorageCategory storageCategory )
{
	switch( storageCategory )
	{
		case BatchManagerBase::Flatfile :
			return new BatchManager< BatchFlatfileStorage >( storageCategory, BatchResolution::SYNCHRONOUS );
			break;
		case BatchManagerBase::XMLfile :
			return new BatchManager< BatchXMLfileStorage >( storageCategory, BatchResolution::SYNCHRONOUS );
			break;
		case BatchManagerBase::WMQ :
			return new BatchManager< BatchMQStorage >( storageCategory, BatchResolution::SYNCHRONOUS );
			break;

		case BatchManagerBase::ZIP :
#ifdef ZIPARCHIVE
			return new BatchManager< BatchZipArchiveStorage >( storageCategory, BatchResolution::SYNCHRONOUS );
#else
			return new BatchManager< BatchZipStorage >( storageCategory, BatchResolution::SYNCHRONOUS );
#endif
			break;
	}
	throw runtime_error( "Invalid batch storage specified." );
}

const BatchManagerBase& BatchManagerBase::operator << ( const string& document )
{
	this->internalEnqueue( document );
	return *this;
}

const BatchManagerBase& BatchManagerBase::operator << ( BatchManip::BatchMetadata metadata )
{
	this->internalEnqueue( metadata );
	return *this;
}

const BatchManagerBase& BatchManagerBase::operator << ( const BatchItem& batchItem )
{
	this->internalEnqueue( batchItem );
	return *this;
}

const BatchManagerBase& BatchManagerBase::operator << ( const BatchManip::BufferSize size )
{
	this->internalEnqueue( size );
	return *this;
}

void BatchManagerBase::operator >> ( BatchItem &item )
{
	try
	{
		this->internalDequeue( item );
	}
	catch( ... )
	{
		m_MoreMessages = false;
		throw;
	}
}

// BatchManager implementation
template < class T >
BatchManager< T >::BatchManager( const BatchManagerBase::StorageCategory storageCategory, const BatchResolution::BatchThreadModel threadModel ) :
	BatchManagerBase( storageCategory ), m_StorageId( "" ), m_ThreadModel( threadModel )
{
}

template < class T >
BatchManager< T >::~BatchManager()
{
	try
	{
		DEBUG2( "~BatchManager" );
	}
	catch( ... ){}
}

// open storage
template < class T >
void BatchManager< T >::open( const string& storageId, ios_base::openmode openMode )
{
	DEBUG( "Opening batch [" << storageId.c_str() << "]" );

	m_StorageId = storageId;
	m_OpenMode = openMode;

	m_MoreMessages = false;
	m_Storage.open( storageId, openMode );
	m_MoreMessages = true;
}

template < class T >
void BatchManager< T >::close( const string& storageId )
{
	DEBUG( "Closing batch [" << storageId.c_str() << "]" );

	m_MoreMessages = false;
	//m_MetaStorage.ReleaseStorage( storageId );
	m_Storage.close( storageId );
}

template < class T >
void BatchManager< T >::internalDequeue( BatchItem &item )
{
	DEBUG( "Dequeue" );
	item = m_Storage.dequeue();

	m_MoreMessages = !item.isLast();
}

// manip
template < class T >
void BatchManager< T >::internalEnqueue( BatchManip::BatchMetadata metadata )
{
	DEBUG_GLOBAL( "Set metadata for evaluation ... " );
	{
		m_MetaDoc = metadata.getDocument();

		// Evaluation of NULL will return a default resolution
		if( m_MetaDoc == NULL )
		{
			m_CrtResolution = BatchItemEval::Evaluate( NULL, m_XPath, metadata.getEyeCatcher() );
		}
		else
		{
			XALAN_USING_XALAN( XercesDocumentWrapper )
			XALAN_USING_XALAN( XalanDocument )

			// map xerces dom to xalan document
	#ifdef XALAN_1_9
			XALAN_USING_XERCES( XMLPlatformUtils )
			XercesDocumentWrapper docWrapper( *XMLPlatformUtils::fgMemoryManager, m_MetaDoc, true, true, true );
	#else
			XercesDocumentWrapper docWrapper( m_MetaDoc, true, true, true );
	#endif
			XalanDocument* const theDocument = ( XalanDocument* )&docWrapper;

			m_CrtResolution = BatchItemEval::Evaluate( theDocument, m_XPath, metadata.getEyeCatcher() );
		}
		m_CrtResolution.setIntendedBatchId( metadata.getBatchId() );
		m_CrtResolution.setTransform( metadata.getTransform() );
	}
	DEBUG_GLOBAL( "Resolution set ... " );
}

template < class T >
void BatchManager< T >::internalEnqueue( const BatchManip::BufferSize size )
{
	DEBUG_GLOBAL( "Set size for buffers ... " );
	m_Storage.setBufferSize( size.getSize() );
}

// add to storage
template < class T >
void BatchManager< T >::internalEnqueue( const BatchItem& item )
{
	string payloadTooLong = ( item.getPayload().length() > 100 ) ? item.getPayload().substr( 0, 100 ) : item.getPayload();
	DEBUG( "Adding a batch item ... ( first 100 bytes ) [" << payloadTooLong << "]" );

	// add it to storage
	//m_MetaStorage.Set( item.getBatchId(), StringUtil::ToString( item.getSequence() ), item.getMessageId() );
	DEBUG( "Item added to temporary storage" );

	m_CrtResolution.setResolution( BatchResolution::Release );
	m_CrtResolution.setItem( item );

	m_Storage.enqueue( m_CrtResolution );
	DEBUG( "Item added to storage" );

	// last batch item .. relase batch, storage
	if ( item.isLast() )
	{
		// if it is also the first ( only 1 message in batch )
		//m_MetaStorage.ReleaseStorage( item.getBatchId() );
	}
}

template < class T >
void BatchManager< T >::internalEnqueue( const string& doc )
{
	string eyecatcher = m_CrtResolution.getItem().getEyecatcher();
	DEBUG( "EyeCatcher for batch item is : [" << eyecatcher << "]" );

	try
	{
		/*string transform = m_CrtResolution.getTransform();
		if( transform.length() > 0 )
		{
			NameValueCollection transportHeaders;
			transportHeaders.Add( XSLTFilter::XSLTFILE, transform );
			transportHeaders.Add( "XSLTPARAMPAYLOAD", StringUtil::Pad( doc, "\'","\'" ) );

			// use this because the evaluation may have modified the batchid
			transportHeaders.Add( "XSLTPARAMINTENDEDBATCHID", StringUtil::Pad( m_CrtResolution.getIntendedBatchId(), "\'","\'" ) );

			WorkItem< ManagedBuffer > managedBuffer( new ManagedBuffer() );
			m_TransformFilter.ProcessMessage( m_MetaDoc, managedBuffer, transportHeaders, true );
			m_CrtResolution.getItem().setPayload( managedBuffer.get() );
		}
		else
			*/
		m_CrtResolution.setItemPayload( doc );
	}
	catch( ... )
	{
		DEBUG( "No request found for EyeCatcher [" << eyecatcher << "]" );
	}

	DEBUG( "Adding a batch item... [" << m_CrtResolution.getItem().getPayload() << "]" );

	BatchResolution resolution = m_CrtResolution;
	DEBUG( "Resolution from previous metadata exchange was : Sequence [" << m_CrtResolution.getItem().getSequence() << "]" );

	// add it to storage
	//m_MetaStorage.Set( m_CrtResolution.getItem().getBatchId(),
	//	StringUtil::ToString( m_CrtResolution.getItem().getSequence() ),
	//	m_CrtResolution.getItem().getMessageId() );
	DEBUG( "Item added to temporary storage" );

	m_Storage.enqueue( m_CrtResolution );
	DEBUG( "Item added to storage" );

	// last batch item .. relase batch, storage
	if ( m_CrtResolution.getItem().isLast() )
	{
		// if it is also the first ( only 1 message in batch )
		//m_MetaStorage.ReleaseStorage( m_CrtResolution.getItem().getBatchId() );
	}
}

template < class T >
void BatchManager< T >::internalEnqueue( XERCES_CPP_NAMESPACE_QUALIFIER DOMDocument* const document )
{
	string eyecatcher = m_CrtResolution.getItem().getEyecatcher();
	DEBUG( "EyeCatcher for batch item is : [" << eyecatcher << "]" );

	try
	{
		//BatchRequest request = m_Config.GetRequest( "*", eyecatcher );
		//BatchRequestTransform transform = request.GetTransform();

		//document = transform.Apply( document );
	}
	catch( ... )
	{
		DEBUG( "No request found for EyeCatcher [" << eyecatcher << "]" );
	}

	m_CrtResolution.setItemXmlPayload( document );

	DEBUG( "Adding a batch item... [" << m_CrtResolution.getItem().getXmlPayload() << "]" );

	BatchResolution resolution = m_CrtResolution;
	DEBUG( "Resolution from previous metadata exchange was : Sequence[" << m_CrtResolution.getItem().getSequence() << "]" );

	// add it to storage
	//m_MetaStorage.Set( m_CrtResolution.getItem().getBatchId(),
	//	StringUtil::ToString( m_CrtResolution.getItem().getSequence() ),
	//	m_CrtResolution.getItem().getMessageId() );
	DEBUG( "Item added to temporary storage" );

	m_Storage.enqueue( m_CrtResolution );
	DEBUG( "Item added to storage" );

	// last batch item .. relase batch, storage
	if ( m_CrtResolution.getItem().isLast() )
	{
		// if it is also the first ( only 1 message in batch )
		//m_MetaStorage.ReleaseStorage( m_CrtResolution.getItem().getBatchId() );
	}
}

template < class T >
void BatchManager< T >::setConfig( const string& configFile )
{
	m_Config.setConfigFile( configFile );
}

// explicit template instantiation required as this is a dll
template class FinTP::BatchManager< BatchFlatfileStorage >;
template class FinTP::BatchManager< BatchXMLfileStorage >;
template class FinTP::BatchManager< BatchMQStorage >;
#ifdef ZIPARCHIVE
template class FinTP::BatchManager< BatchZipArchiveStorage >;
#else
template class FinTP::BatchManager< BatchZipStorage >;
#endif
