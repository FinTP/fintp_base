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

#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <cassert>

//When build on linux , the compiler throw the warning that this is a deprecated header
//and to use sstream instead of this
#ifndef LINUX
#include <strstream>
#endif

#include "XmlUtil.h"
#include "XPathHelper.h"
#include "Trace.h"
#include "AppExceptions.h"
#include "Base64Filter.h"
#include "Base64.h"

/*#include <xercesc/framework/LocalFileInputSource.hpp>
#include <xercesc/dom/DOMWriter.hpp>
#include <xercesc/framework/MemBufFormatTarget.hpp>*/

#include <xalanc/XPath/NodeRefList.hpp>
#include <xalanc/PlatformSupport/XalanOutputStreamPrintWriter.hpp>
#include <xalanc/PlatformSupport/XalanStdOutputStream.hpp>
#include <xalanc/XMLSupport/FormatterToXML.hpp>
#include <xalanc/XMLSupport/FormatterTreeWalker.hpp>
#include <xalanc/XercesParserLiaison/XercesDocumentWrapper.hpp>

using namespace std;

#ifdef XERCES_CPP_NAMESPACE_USE
XERCES_CPP_NAMESPACE_USE
#endif

using namespace FinTP;

const string Base64Filter::XPATH = "XPATH";
const string Base64Filter::KEEP_ORIGINAL = "KeepOriginal";
const string Base64Filter::ENCODED_NODE_NAME = "EncodedNodeName";
		
Base64Filter::Base64Filter() : AbstractFilter( FilterType::BASE64 ), m_KeepOriginal( false )
{
}

Base64Filter::~Base64Filter()
{	
}

AbstractFilter::FilterResult Base64Filter::ProcessMessage( XERCES_CPP_NAMESPACE_QUALIFIER DOMDocument* inputOutputData, NameValueCollection& transportHeaders, bool asClient )
{
	ValidateProperties();

	DEBUG( "XPath is " << m_XPath );
	
	string encodedValue, decodedValue;

	if( inputOutputData == NULL )
	{
		TRACE( "Input document is empty" );
		throw runtime_error( "Input document is empty" );
	}
		
	try
	{	
		XALAN_USING_XALAN( XercesDocumentWrapper )
		XALAN_USING_XALAN( XalanDocument )
		XALAN_USING_XALAN( XalanNode )
	
		XERCES_CPP_NAMESPACE_QUALIFIER DOMNode* domNode = NULL;
		string theXml = "";
		
		// this space is intentionally left blank
		// allow wrapper to be destroyed before modifying the underlying DOM Document
		{	
			DEBUG( "About to create wrapper" );	

#ifdef XALAN_1_9
			XALAN_USING_XERCES( XMLPlatformUtils )
			XercesDocumentWrapper docWrapper( *XMLPlatformUtils::fgMemoryManager, inputOutputData, true, true, true );
#else
			XercesDocumentWrapper docWrapper( inputOutputData, true, true, true );
#endif
			XalanDocument* const theDocument = ( XalanDocument* )&docWrapper;
				
			DEBUG( "About to evaluate xpath" );
			XalanNode* refNode = XPathHelper::Evaluate( m_XPath, theDocument );
			if ( refNode == NULL )
			{
				AppException aex( "XPath evaluation returned an empty node. Check the input document and the XPath." );
				aex.addAdditionalInfo( "XPath", m_XPath );
				aex.addAdditionalInfo( "Document", XmlUtil::SerializeToString( inputOutputData ) );

				throw aex;
			}
			
			DEBUG( "Serializing Xalan node" );
			theXml = XPathHelper::SerializeToString( refNode );
			
			DEBUG2( "Output of XPath eval is : [" << theXml << "]" );
			DEBUG_GLOBAL( "Node is : [" << refNode << "]" );
			
			domNode = ( DOMNode* )( docWrapper.mapNode( refNode ) );
		}
			
		if ( asClient ) 	
		// Client - get data with theXPath form inputOutputData,
		{
			// ENCODE it base64 and put it back in inputOutputData
			encodedValue = Base64::encode( string( theXml ) ).data();
			//strcpy( charValue, ( char* )( Base64::encode( string( theXml ) ) ).data() );
			DEBUG2( "Encoded value is : [" << encodedValue << "]" );
			DEBUG( "Value encoded. DOM node is : [" << domNode << "]" );
		} 
		else  		
		// Server - get data with theXPath form inputOutputData,
		{				
			// DECODE it base64 and put it back in inputOutputData
			decodedValue = Base64::decode( string( theXml ) ).data();
			//strcpy( charValue, ( char* )( Base64::decode( string( theXml ) ) ).data() );
			DEBUG2( "Decoded value is : [" << decodedValue << "]" );
			DEBUG( "Value decoded." );
		}
	
		//=================================================
		//Replace the element with the Base64 encoded value
		//Get ParentNode of my serialized node
		DOMNode* theParentNode = domNode->getParentNode();
		string parentNodeName = "";
		bool rootReplaced = false;
		
		if ( theParentNode == NULL )
		{
			DEBUG( "Error obtaining parent node ( ending root element ? ) : a [root] element will be created. " );
			// request made on root : create a node named root, add content to it
			
			theParentNode = inputOutputData->createElement( unicodeForm( "root" ) );
			
			DOMElement* theOldRootElement = inputOutputData->getDocumentElement();
	 		DOMNode* theRemovedChild = inputOutputData->removeChild( theOldRootElement );
	 		theRemovedChild->release();
		
			inputOutputData->appendChild( theParentNode );
			
			rootReplaced = true;
			//throw runtime_error( "Unable to obtain parent node of the node being replaced. Check the XPath." );
		}

		parentNodeName = localForm( theParentNode->getNodeName() );
		DEBUG( "Parent node is [" << parentNodeName << "]" );

		//Remove the serialized Node		
		if( !m_KeepOriginal )
		{
			if ( !rootReplaced )
			{
				DOMNode* theRemovedChild = theParentNode->removeChild( domNode );
				theRemovedChild->release();
			}
		}
		else
		{
			if( m_EncodedNodeName.length() <= 0 )
				m_EncodedNodeName = string( "Encoded_" ) + parentNodeName;
			
			// append original
			DOMElement* encodedElement = inputOutputData->createElement( unicodeForm( m_EncodedNodeName ) );
			DEBUG( "Encoded element [" << m_EncodedNodeName << "] created." );
			
			theParentNode->appendChild( encodedElement );
			theParentNode = encodedElement;
		}

		//Create a text node as child of ParentNode
		//Text Node will be the old element encoded Base64 
		DOMText* encodedTextNode = inputOutputData->createTextNode( unicodeForm( encodedValue ) );
		theParentNode->appendChild( encodedTextNode );	
		
		//Release the memory
		DEBUG2( "Document after encoding/decoding : [" << XmlUtil::SerializeToString( inputOutputData ) << "]" );
	}
	catch( const XMLException& e )
	{
		stringstream errorMessage;
		
		errorMessage << "XmlException type : " << ( int )e.getErrorType() << 
			" Code : [" << e.getCode() << "] Message = " << localForm( e.getMessage() ) <<
			" Line : " << e.getSrcLine();
		
		TRACE( errorMessage.str() );
		throw runtime_error( errorMessage.str() );
	}
	catch( const std::exception& e )
	{
		stringstream messageBuffer;
		messageBuffer << typeid( e ).name() << " exception [" << e.what() << "]";
		TRACE( messageBuffer.str() );
	
		throw;
	}
	catch( ... )
	{  	
		TRACE( "Unhandled exception while processing message in Base64 filter" );

		throw;
	}
	return AbstractFilter::Completed;
}

AbstractFilter::FilterResult Base64Filter::ProcessMessage( const XERCES_CPP_NAMESPACE_QUALIFIER DOMDocument* inputData, AbstractFilter::buffer_type outputData, NameValueCollection& transportHeaders, bool asClient )
{
	AbstractFilter::FilterResult result = ProcessMessage( const_cast< XERCES_CPP_NAMESPACE_QUALIFIER DOMDocument* >( inputData ), transportHeaders, asClient );
	
	string serializedDOM = XmlUtil::SerializeToString( inputData );
	outputData.get()->copyFrom( serializedDOM );

	return AbstractFilter::Completed;
}

AbstractFilter::FilterResult Base64Filter::ProcessMessage( AbstractFilter::buffer_type inputData, XERCES_CPP_NAMESPACE_QUALIFIER DOMDocument* outputData, NameValueCollection& transportHeaders, bool asClient )
{
	ManagedBuffer* inputBuffer = inputData.get();

	if ( outputData == NULL )
	{
		throw logic_error( "Output document is NULL" );
	}

	ValidateProperties();
		
	//Clean the target document
	// 	- Delete the root element (and ? subtree) from the target document
	DOMElement* theOldRootElement = outputData->getDocumentElement();
	DOMNode* theRemovedChild = outputData->removeChild( theOldRootElement );
	theRemovedChild->release();

	if ( m_XPath.length() > 0 )
	{
		XERCES_CPP_NAMESPACE_QUALIFIER DOMDocument *tempData = NULL;
		
		try
		{
			tempData = XmlUtil::DeserializeFromString( inputBuffer->buffer(), inputBuffer->size() );
			AbstractFilter::FilterResult result = ProcessMessage( tempData, transportHeaders, asClient );
			
			//Copy the source DOMDocument content to taget DOMDocument
			//		- Import the root sub-tree from the source DOMDocument  to inputOutputData DOMDocument
			//		- Append the imported node and his sub-tree to inputOutputData DOMDocument
			outputData->appendChild( outputData->importNode( tempData->getDocumentElement() , true ) );

			try
			{
				tempData->release();
			}catch( ... ){};

			tempData = NULL;
		}
		catch( ... )
		{
			if ( tempData != NULL )
				tempData->release();
			throw;
		}
	}
	else
	{
		string encdecValue = "";
		if( m_KeepOriginal )
			throw logic_error( "Unable to keep original of a non-DOM argument" );

		DOMNode* theParentNode = outputData->createElement( unicodeForm( "root" ) );
		outputData->appendChild( theParentNode );

		if ( asClient ) 	
		// Client - get data with theXPath form inputOutputData,
		{
			// ENCODE it base64 and put it back in inputOutputData
			encdecValue = Base64::encode( inputBuffer->str() ).data();
			DEBUG( "Encoded value is : [" << encdecValue << "]" );			
		} 
		else  		
		// Server - get data with theXPath form inputOutputData,
		{				
			// DECODE it base64 and put it back in inputOutputData
			encdecValue = Base64::decode( inputBuffer->str() ).data();
			DEBUG( "Decoded value is : [" << encdecValue << "]" );
		}

		//Create a text node as child of ParentNode
		//Text Node will be the old element encoded Base64 
		DOMText* encodedTextNode = outputData->createTextNode( unicodeForm( encdecValue ) );
		theParentNode->appendChild( encodedTextNode );
	}
		
	return AbstractFilter::Completed;
}

/*AbstractFilter::FilterResult Base64Filter::ProcessMessage( unsigned char* inputData, unsigned char* outputData, NameValueCollection& transportHeaders, bool asClient )
{
	throw logic_error( "Not implemented" );
}
*/

// ------------------------------------------------------------------------------------
// END Buffer to Buffer
// ------------------------------------------------------------------------------------

/// private methods implementation
void Base64Filter::ValidateProperties()
{
	if ( !m_Properties.ContainsKey( Base64Filter::XPATH ) )
		//throw invalid_argument( "Required parameter missing : XPATH" );
		m_XPath = "";
	else
		m_XPath = m_Properties[ Base64Filter::XPATH ].data();
	
	if ( !m_Properties.ContainsKey( Base64Filter::KEEP_ORIGINAL ) )
		m_KeepOriginal = false;
	else 
		m_KeepOriginal = ( m_Properties[ Base64Filter::KEEP_ORIGINAL ] == "yes" );

	if ( !m_Properties.ContainsKey( Base64Filter::ENCODED_NODE_NAME ) )
		m_EncodedNodeName = "";
	else
		m_EncodedNodeName = m_Properties[ Base64Filter::ENCODED_NODE_NAME ];
}

bool Base64Filter::isMethodSupported( AbstractFilter::FilterMethod method, bool asClient )
{
/*	switch( method )
	{
		case AbstractFilter::XmlToXml :
			return true;
		case AbstractFilter::XmlToBuffer :
			return true;
		default :
			return false;
	}	*/
	return true;
}

bool Base64Filter::canLogPayload()
{
	return true;
//	throw logic_error( "You must override this function to let callers know if this filter can log the payload without disturbing operations" );
}
