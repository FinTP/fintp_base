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

#include <sstream>

#include "StringUtil.h"
#include "Trace.h"
#include "BatchXMLfileStorage.h"

#define _DEFAULT_NAMESPACE_ "_DEFAULT_NAMESPACE_"

using namespace FinTP;

BatchXMLfileStorage::BatchXMLfileStorage() : BatchStorageBase(), m_CrtStorage( NULL ), m_CrtStorageWrapper( NULL ), m_CrtStorageRoot( NULL ), 
	m_CrtInsertNode( NULL ), m_BatchId( "" ), m_NumberOfItems( 0 ), m_CrtSequence( 0 ), m_XsltFileName( "" ), m_XPath( "" ), m_XPathCallback( NULL )
{	
}

BatchXMLfileStorage::~BatchXMLfileStorage()
{
	try
	{
		if ( m_CrtStorageWrapper != NULL )
		{
			XSLTFilter::releaseSource( m_CrtStorageWrapper );
			m_CrtStorageWrapper = NULL;
		}
	} 
	catch( ... )
	{
		try
		{
			TRACE( "An error occured when releasing Storage wrapper" );
		}catch( ... ){}
	}

	try
	{
		if ( m_CrtStorage != NULL )
		{
			m_CrtStorage->release();
			m_CrtStorage = NULL;
		}
	} 
	catch( ... )
	{
		try
		{
			TRACE( "An error occured when releasing Storage" );
		}catch( ... ){}
	}
}

//Enqueue an element to current storage
void BatchXMLfileStorage::enqueue( BatchResolution& resolution )
{
	DEBUG( "Enqueue" );
	
	BatchItem item = resolution.getItem();
	XERCES_CPP_NAMESPACE_QUALIFIER DOMDocument* currentItem = NULL;
	
	DEBUG( "Current item sequence is " << item.getSequence() );
	
	try
	{
		// if m_CrtSequence is first sequence create the BatchXml DOM 
		// the item will become the main document
		if ( m_CrtSequence == BatchItem::FIRST_IN_SEQUENCE )
		{			
			m_CrtStorage = XmlUtil::DeserializeFromString( resolution.getItem().getPayload() );
			m_CrtStorageRoot = m_CrtStorage->getDocumentElement();
			
			DEBUG( "Root BATCH XML added." );		
		}
		else
		// if not first sequence append to BatchXml DOM 
		{
			// serialize the current item
			// item will be added as a child to the main document
			currentItem = XmlUtil::DeserializeFromString( resolution.getItem().getPayload() );
			DOMElement* currentItemRoot = currentItem->getDocumentElement();
			string itemNamespace = XmlUtil::getNamespace( m_CrtStorage );
			string curXPath = "";
			if ( m_XPath.length() > 0 ) 
			{
				DEBUG( "Add current item [" << resolution.getItem().getPayload() << "]" );

				// look for insert node if not found before
				if ( m_CrtInsertNode == NULL )
				{
					m_CrtInsertNode = m_CrtStorageRoot;
					if ( m_CrtInsertNode == NULL )
						throw runtime_error( "Storage root not created" );
					/*
					StringUtil splitter( m_XPath );

					// simple xpath support only 
					splitter.Split( "/" );

					while( splitter.MoreTokens() )
					{
						string nextToken = splitter.NextToken();
						DOMNodeList* result = m_CrtInsertNode->getElementsByTagName( unicodeForm( nextToken ) );
						if ( ( result == NULL ) || ( result->getLength() == 0 ) )
						{
							stringstream errorMessage;
							errorMessage << "Missing required [" << nextToken << "] element in [" << localForm( m_CrtInsertNode->getNodeName() ) << "] node";
							throw runtime_error( errorMessage.str() );
						}

						m_CrtInsertNode = dynamic_cast< DOMElement* >( result->item( 0 ) );
					}
					*/
					vector< string > splitXPath = getXPath( itemNamespace );
					vector< string >::iterator itToken;
					for( itToken = splitXPath.begin(); itToken < splitXPath.end() ; itToken++ )
					{
						DOMNodeList* result = m_CrtInsertNode->getElementsByTagName( unicodeForm( *itToken ) );
						if ( ( result == NULL ) || ( result->getLength() == 0 ) )
						{
							stringstream errorMessage;
							errorMessage << "Missing required [" << *itToken << "] element in [" << localForm( m_CrtInsertNode->getNodeName() ) << "] node";
							throw runtime_error( errorMessage.str() );
						}
						m_CrtInsertNode = dynamic_cast< DOMElement* >( result->item( 0 ) );
						curXPath += *itToken + "/";
					}
					curXPath.erase( curXPath.size()-2 );
					// move insert node up a level 
					m_CrtInsertNode = dynamic_cast< DOMElement* >( m_CrtInsertNode->getParentNode() );
					DEBUG( "Add current item on element [" << curXPath << "]" );
				}
				
				// try to find the item in the child too
				bool xpathOnChild = true;
				/*
				StringUtil splitter( m_XPath );

				// simple xpath support only 
				splitter.Split( "/" );
				DOMElement* currentItemMovedRoot = currentItemRoot;

				while( xpathOnChild && splitter.MoreTokens() )
				{
					string nextToken = splitter.NextToken();
					DOMNodeList* resultItem = currentItemMovedRoot->getElementsByTagName( unicodeForm( nextToken ) );
					if ( ( resultItem == NULL ) || ( resultItem->getLength() == 0 ) )
					{
						DEBUG( "Attempt to apply [" << m_XPath << "] on child element failed. Root will be added." );
						xpathOnChild = false;
						break;
					}
					else
						currentItemMovedRoot = dynamic_cast< DOMElement* >( resultItem->item( 0 ) );
				}
				*/
				DOMElement* currentItemMovedRoot = currentItemRoot;
				vector< string > splitXPath = getXPath( itemNamespace );
				vector< string >::iterator itToken;
				itToken = splitXPath.begin();
				while( xpathOnChild && ( itToken != splitXPath.end() ) )
				{
					DOMNodeList* resultItem = currentItemMovedRoot->getElementsByTagName( unicodeForm( *itToken ) );
					if ( ( resultItem == NULL ) || ( resultItem->getLength() == 0 ) )
					{
						DEBUG( "Attempt to apply [" << *itToken << "] on child element failed. Root will be added." );
						xpathOnChild = false;
						break;
					}
					else
						currentItemMovedRoot = dynamic_cast< DOMElement* >( resultItem->item( 0 ) );
					itToken++;
				}
				if ( xpathOnChild )
				{
					DEBUG( "Applying [" << curXPath << "] on child element. Selected node will be added as child." );
					currentItemRoot = currentItemMovedRoot;
				}
			}
			else
			{
				DEBUG( "Add current item on element [/] : " << resolution.getItem().getPayload() );
				m_CrtInsertNode = m_CrtStorageRoot;
			}
			
			if ( m_CrtStorage == NULL )
				throw runtime_error( "Storage not created" );

			if ( m_CrtInsertNode == NULL )
				throw runtime_error( "Storage root not created" );

			m_CrtInsertNode->appendChild( m_CrtStorage->importNode( currentItemRoot, true ) );
	
			DEBUG( "Current item added" );
			if ( currentItem != NULL )
			{
  				currentItem->release();
  				currentItem = NULL;
			}	
		}
	}
	catch( const XMLException& e )
  	{
  		if( m_CrtStorage != NULL )
  		{
			m_CrtStorage->release();
  			m_CrtStorage = NULL;
  		}
  			
  		if( currentItem != NULL )
  		{
  			currentItem->release();
  			currentItem = NULL;
  		}
		m_CrtStorageRoot = NULL;
		m_CrtInsertNode = NULL;
  		TRACE_GLOBAL( "XMLException"  << localForm( e.getMessage() ) );  		
  		throw runtime_error( localForm( e.getMessage() ) );
  	}
  	catch( const DOMException& e )
  	{
		if( m_CrtStorage != NULL )
  		{
			m_CrtStorage->release();
  			m_CrtStorage = NULL;
  		}
  			
  		if( currentItem != NULL )
  		{
  			currentItem->release();
  			currentItem = NULL;
  		}
		m_CrtStorageRoot = NULL;
		m_CrtInsertNode = NULL;
  		TRACE_GLOBAL( "DOMException "  << e.code );
  		string message;
  		
  		const unsigned int maxChars = 2047;
		
		XMLCh errText[maxChars + 1];
      
		// attemt to read message text
    	if ( DOMImplementation::loadDOMExceptionMsg( e.code, errText, maxChars ) )
    	{
			message = localForm( errText );
		}
  		else
  		{
  			// format error code
  			stringstream messageBuffer;
  			messageBuffer << "DOMException error code : [ " << ( int )e.code << "]";
    		message = messageBuffer.str();
    	}		
 		throw runtime_error( message );
 	}
	catch( const std::exception& e )
	{
		if( m_CrtStorage != NULL )
  		{
			m_CrtStorage->release();
  			m_CrtStorage = NULL;
  		}
  			
  		if( currentItem != NULL )
  		{
  			currentItem->release();
  			currentItem = NULL;
  		}
  		m_CrtStorageRoot = NULL;
		m_CrtInsertNode = NULL;		
		stringstream messageBuffer;
	  	messageBuffer << typeid( e ).name() << " exception [" << e.what() << "]";
		TRACE_GLOBAL( messageBuffer.str() );

    	//throw runtime_error( "Parse error" );
    	throw runtime_error( messageBuffer.str() );
  	}
	catch( ... )
	{
		if( m_CrtStorage != NULL )
  		{
			m_CrtStorage->release();
  			m_CrtStorage = NULL;
  		}
  			
  		if( currentItem != NULL )
  		{
  			currentItem->release();
  			currentItem = NULL;
  		}	
		m_CrtStorageRoot = NULL;
		m_CrtInsertNode = NULL;
		TRACE_GLOBAL( "Unhandled exception" );
		
		throw;
	}    
	
	m_CrtSequence++;
	DEBUG( "Next sequence is " << m_CrtSequence );
	DEBUG_GLOBAL( "Current storage address [" << m_CrtStorage << "]" );
}

vector< string > BatchXMLfileStorage::getXPath( const string& itemNamespace )
{
	if ( itemNamespace.length() <= 0 )
		return getXPath( _DEFAULT_NAMESPACE_ );

	if ( m_XPathTokens.find( itemNamespace ) == m_XPathTokens.end() )
	{
		string crtXPath = m_XPath;
		m_XPathTokens.insert( pair< string, vector< string > >( itemNamespace, vector< string >() ) );
		
		if ( m_XPathCallback != NULL ) 
			crtXPath = ( *m_XPathCallback )( itemNamespace );

		if ( crtXPath.length() <= 0 )
			crtXPath = m_XPath;

		StringUtil splitter( crtXPath );

		// simple xpath support only 
		splitter.Split( "/" );

		while( splitter.MoreTokens() )
		{
			m_XPathTokens[ itemNamespace ].push_back( splitter.NextToken() );
		}
	}

	return m_XPathTokens[ itemNamespace ];
}

BatchItem BatchXMLfileStorage::dequeue()
{
	DEBUG( "Dequeue" );
	
	// Xslt will be applied on m_CrtStorage , parameter passed will be item position 
	// will return the m_CrtStorage item with position specified
		
	// using Xslt will offer the possibility to do a specialized selection

   	//Prepare the XSLTFilter ProcessMessage parameters
	bool asClient = false;
	NameValueCollection transportHeaderCollection;
  	
	//Prepare XSLT params
	if ( transportHeaderCollection.ContainsKey( "XSLTPARAMPOSITION" ) )
		transportHeaderCollection.ChangeValue( "XSLTPARAMPOSITION", StringUtil::ToString( m_CrtSequence ) );
	else
		transportHeaderCollection.Add( "XSLTPARAMPOSITION", StringUtil::ToString( m_CrtSequence ) );
	transportHeaderCollection.Add( XSLTFilter::XSLTFILE, m_XsltFileName );
					
  	//ProcessMessage
  	// expect Header + ordinary element ( with the position precised by param )
	
	WorkItem< ManagedBuffer > output( new ManagedBuffer() );
	ManagedBuffer* outputBuffer = output.get();

	// create a wrapper for source ( it will be reused )
	XALAN_USING_XALAN( XalanDocument );

	m_XsltFilter.ProcessMessage( m_CrtStorageWrapper, output, transportHeaderCollection, true );
	DEBUG2( "Got batch item : [" << outputXslt << "]" );		
		
	// create item 
	BatchItem item;
	item.setBatchId( m_BatchId );
	item.setSequence( m_CrtSequence );
	item.setPayload( outputBuffer->str() );
	if ( m_CrtSequence >= m_NumberOfItems )
		item.setLast( true );
	
	m_CrtSequence++;		
	return item;
}

void BatchXMLfileStorage::close( const string& storageId )
{
	DEBUG( "Close" );
	if ( m_CrtStorage != NULL )
	{
		m_CrtStorage->release();	
		m_CrtStorage = NULL;
	}
	m_CrtStorageRoot = NULL;
	m_CrtInsertNode = NULL;
}

// open storage
void BatchXMLfileStorage::open( const string& storageId, ios_base::openmode openMode )
{
	DEBUG( "Open" );

	// if openmode = inputMode    => TODO: decompose the DOM from the BatchXML
	// if openmode = outputMode   => TODO: build the DOM for the BatchXML
	
	if ( ( openMode & ios_base::in ) == ios_base::in )
	{		
		// Will dequeue to get XML messages from the Batch XML
		DEBUG( "XML Batch open options set for dequeueing... " );
			
		m_CrtSequence = 0;
		bool asClient = false;
		NameValueCollection transportHeaderCollection;
		
		//Prepare XSLT params
		transportHeaderCollection.Add( "XSLTPARAMPOSITION", StringUtil::ToString( m_CrtSequence ) );

		// tell it to create a parsed source 
		transportHeaderCollection.Add( XSLTFilter::XSLTCRTPARSESOURCE, "true" );

		DEBUG( "XSLT filename [" << m_XsltFileName << "]" );
		transportHeaderCollection.Add( XSLTFilter::XSLTFILE, m_XsltFileName );
	
		// first time filter is applied to extract header information
		//    - groupId
		//    - number of ordinary elements
		//ProcessHeader
		WorkItem< ManagedBuffer > output( new ManagedBuffer() );
		ManagedBuffer* outputBuffer = output.get();
		
		try
		{
			// create a wrapper for source ( it will be reused )
			XALAN_USING_XERCES( XMLPlatformUtils );
			XALAN_USING_XALAN( XercesDocumentWrapper );
			XALAN_USING_XALAN( XalanDOMString );
			XALAN_USING_XALAN( XalanDocument );

			if ( m_CrtStorageWrapper != NULL )
			{
				XSLTFilter::releaseSource( m_CrtStorageWrapper );
				m_CrtStorageWrapper = NULL;
			}

			m_CrtStorageWrapper = XSLTFilter::parseSource( m_CrtStorage );
			if ( m_CrtStorageWrapper == NULL )
				throw runtime_error( "Unable to create xerces wrapper" );
			
			m_XsltFilter.ProcessMessage( m_CrtStorageWrapper, output, transportHeaderCollection, true );

			DEBUG( "Got batch header" );
		}
		catch( ... )
		{
			TRACE( "Unable to get batch header" );				
			throw;
		}

		//expected result after tranformation is: <Batch Id=4 Count=6 />
		XERCES_CPP_NAMESPACE_QUALIFIER DOMDocument* batchHeader = NULL;
		DOMNode* rootNode = NULL;
    
	    try
		{	
			batchHeader = XmlUtil::DeserializeFromString( outputBuffer->str() );
			
			if ( batchHeader == NULL ) 
				throw runtime_error( "Empty document received." );
			
			rootNode = ( DOMNode* )( batchHeader->getDocumentElement() );	
			if( rootNode == NULL )
				throw runtime_error( "Document element missing." );				
		}
		catch( ... )
		{
			if ( batchHeader != NULL )
			{
				batchHeader->release();
				batchHeader = NULL;
			}
			
			throw;
		}
		   
		DOMAttr* attributeBatchId, *attributeCount;         
		try
		{
			if  ( ( string )( localForm( rootNode->getNodeName() ) ) == "Batch" ) 	
			{		
				DOMElement* rootElem = dynamic_cast< DOMElement* >( rootNode );
				if ( rootElem == NULL )
					throw runtime_error( "Bad type : expected root to be an element" );

				//get Batch Id
				attributeBatchId = rootElem->getAttributeNode( unicodeForm( "Id" ) );
				if ( attributeBatchId == NULL )
					throw runtime_error( "Attribute [Id] was expected as child of element [Batch]" );
					
				m_BatchId = localForm( attributeBatchId->getValue() );
				DEBUG( "Batch Id is [" << m_BatchId << "]" );
				
				//get Number of items
				attributeCount = rootElem->getAttributeNode( unicodeForm( "Count" ) );
				if ( attributeCount == NULL )
					throw runtime_error( "Attribute [Count] was expected as child of element [Batch]" );
				
				m_NumberOfItems = StringUtil::ParseInt( localForm( attributeCount->getValue() ) );
				DEBUG( "Total number of items is [" << m_NumberOfItems << "]" );
			}
		}				
		catch( ... )
		{
			if ( batchHeader != NULL )
			{
				batchHeader->release();
				batchHeader = NULL;
			}
			throw;
		}

		if ( batchHeader != NULL )
		{
			batchHeader->release();
			batchHeader = NULL;
		}
	}
	else 
	{
		// Batch open options set for enqueueing
		DEBUG( "XML Batch open options set for enqueueing." );	
	}
	
	m_CrtSequence = BatchItem::FIRST_IN_SEQUENCE;
	//Need it in enqueue to see if first sequence or not
	
	DEBUG( "Current sequence is FIRST" );	
}

string BatchXMLfileStorage::getSerializedXml() const
{
	if ( m_CrtStorage == NULL )
		throw logic_error( "Current storage empty." );

	string batchXml = XmlUtil::SerializeToString( m_CrtStorage );	
	return batchXml;
}

void BatchXMLfileStorage::setSerializedXml( const string& document ) 
{
	if ( m_CrtStorage != NULL )
	{
		m_CrtStorage->release();
		m_CrtStorage = NULL;
	}
	m_CrtStorage = XmlUtil::DeserializeFromString( document );	
}

void BatchXMLfileStorage::setSerializedXml( const unsigned char *document, const unsigned long isize ) 
{
	if ( m_CrtStorage != NULL )
	{
		m_CrtStorage->release();
		m_CrtStorage = NULL;
	}
	m_CrtStorage = XmlUtil::DeserializeFromString( document, isize );
}
