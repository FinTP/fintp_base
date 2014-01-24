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
	#define __MSXML_LIBRARY_DEFINED__
#endif

#include "XSDFilter.h"
#include "Trace.h"
#include "XmlUtil.h"
#include "XSDValidationException.h"

#include <sstream>
#include <iostream>
using namespace std;

#include <xercesc/util/PlatformUtils.hpp>
#include <xercesc/dom/DOMImplementation.hpp>
#include <xercesc/dom/DOMImplementationLS.hpp>
#include <xercesc/dom/DOMImplementationRegistry.hpp>
//#include <xercesc/dom/DOMWriter.hpp>

//#include <xercesc/util/XMLEntityResolver.hpp>
#include <xercesc/parsers/XercesDOMParser.hpp>
#include <xercesc/framework/MemBufInputSource.hpp>
//#include <xercesc/framework/MemBufFormatTarget.hpp>
//#include <xercesc/framework/LocalFileFormatTarget.hpp>

XERCES_CPP_NAMESPACE_USE
using namespace FinTP;

const string XSDFilter::XSDFILE = "XSDFILE";
const string XSDFilter::XSDNAMESPACE = "XSDNAMESPACE";

//Constructor
XSDFilter::XSDFilter() : AbstractFilter( FilterType::XSD ), m_XsdSchemaLocation( "" ), m_XsdNamespace( "" )
{
}

//Destructor 
XSDFilter::~XSDFilter()
{
}

//
// XML to XML
//
// Xerces-C++ couldn't validate the data contained in a DOMtree directly (see Xerces-C++ FAQ)
// So:
// 	the input DOM Document will be serialized into a memory buffer
// 	and then the buffer data will be processed and validate using filter ProcessMessage (buffer, XML) method
AbstractFilter::FilterResult XSDFilter::ProcessMessage( XERCES_CPP_NAMESPACE_QUALIFIER DOMDocument* inputOutputData, NameValueCollection& transportHeaders, bool asClient )
{	
	//Validate properties to see if contains XSD file
	ValidateProperties( transportHeaders );    
  
   //Serialize the inputOutputData DOM
	string theSerializedDOM = XmlUtil::SerializeToString( inputOutputData );
		
	//Process the buffer containing the serialized DOM tree
	ProcessMessage( ( unsigned char * )theSerializedDOM.data(), inputOutputData, transportHeaders, asClient);

	return AbstractFilter::Completed;	
}

//
//	Buffer to XML
//
AbstractFilter::FilterResult XSDFilter::ProcessMessage( unsigned char* inputData, XERCES_CPP_NAMESPACE_QUALIFIER DOMDocument* outputData, NameValueCollection& transportHeaders, bool asClient )
{
	//Validate properties to see if contains XSD file
	ValidateProperties( transportHeaders );
	DEBUG( "Schema location is [" << m_XsdSchemaLocation << "]" );
	DEBUG( "The schema namespace is [" << m_XsdNamespace << "]" );
	
	//Set Buffer ID	
	stringstream memBufIdStream;
	memBufIdStream << "XMLForValidate.id," << pthread_self();
	string memBufId = memBufIdStream.str();

  	string theInputDataString( ( const char * ) inputData );
    
   //Create memory input buffer
	MemBufInputSource* memBufIS = NULL;

	//Create parser
	XercesDOMParser *parser = NULL;

	//Create ErrorHandler for XercesDOMParser
	XercesDOMTreeErrorHandler *errReporter = NULL;

	try
	{
		memBufIS = new MemBufInputSource( ( const XMLByte* )inputData, theInputDataString.length(), memBufId.c_str(), false );
		parser = new XercesDOMParser;
   	
		//Set parser settings
		parser->setValidationScheme( XercesDOMParser::Val_Always );
		parser->setDoNamespaces( true );
		parser->setDoSchema( true );
		parser->setValidationSchemaFullChecking( true );
		parser->setCreateEntityReferenceNodes( true );
		parser->setIncludeIgnorableWhitespace( true );

		//Set external schema location
		if ( m_XsdNamespace.length() == 0 )
			parser->setExternalNoNamespaceSchemaLocation( m_XsdSchemaLocation.c_str() );
		else 
			parser->setExternalSchemaLocation( ( m_XsdNamespace + " " + m_XsdSchemaLocation ).c_str() );
   
		errReporter = new XercesDOMTreeErrorHandler();
		parser->setErrorHandler( errReporter );
	
		//  Parse the memory buffer, catching any exceptions that might propagate out from it	
		DEBUG( "DOMDocument address before validation : " << outputData );
		
		//Copy the content of parsed document to outputData DOMDocument parameter
		//Parse the memory buffer 
		parser->parse( *memBufIS );
		DEBUG( "Parse done." );
		
		//Get the root element for the parsed document
		DOMElement* theRootElement  = ( parser->getDocument() )->getDocumentElement();
		if ( theRootElement == NULL )
			throw runtime_error( "Validation error : The parsed document is NULL" );

		// if outout doc is set
		if ( outputData != NULL )
		{
			//If the outputData DOMDocument contains something, delete its content
			DOMElement* theOldRootElement = outputData->getDocumentElement();
			if ( theOldRootElement != NULL)
			{
	 			DOMNode* theRemovedChild = outputData->removeChild( theOldRootElement );
	 			theRemovedChild->release();
			}
		
			//Import the parsed document content (root sub-tree) to outputData DOMDocument
			//Append the imported node and its sub-tree to outputData DOMDocument
			outputData->appendChild( outputData->importNode( theRootElement, true ) );
#if ( XERCES_VERSION_MAJOR < 3 )
			outputData->setEncoding( unicodeForm( "UTF-8" ) );
#endif
			outputData->normalizeDocument();

			DEBUG( "DOMDocument address after validation : " << outputData );
		}
		else
		{
			DEBUG( "DOMDocument address after validation : [NULL]" );
		}
			
		//Release memory
		DEBUG( "Cleaning buffer... " );
		if ( memBufIS != NULL )
		{
			delete memBufIS;
			memBufIS = NULL;
		}
		DEBUG( "Cleaning error reporter... " );
		if ( errReporter != NULL )
		{
			delete errReporter;
			errReporter = NULL;
		}
		DEBUG( "Cleaning parser... " );
		if ( parser != NULL )
		{
			delete parser;
			parser = NULL;
		}
		DEBUG( "Parsing objects cleaned." );
	}
	catch( const std::exception& ex )
	{
		stringstream errorMessage;
		errorMessage << ex.what();

		TRACE( errorMessage.str() );
		if ( errReporter != NULL )
		{
			if( errReporter->getSawErrors() )
			{
				if ( errReporter->getLastError() != ex.what() )
					errorMessage << ". last error [" << errReporter->getLastError() << "]";
			}
			else
				errorMessage << ". last error (none) [" << errReporter->getLastError() << "]";
		}
		TRACE( errorMessage.str() );

		//Release memory
		if ( memBufIS != NULL )
			delete memBufIS;
		if ( errReporter != NULL )
			delete errReporter;
		if ( parser != NULL )
			delete parser;
			
		throw XSDValidationException( memBufId, m_XsdSchemaLocation, errorMessage.str() );
	}
	catch( ... )
	{
		stringstream errorMessage;
		errorMessage << "unknown exception";
		TRACE( errorMessage.str() );
		if ( errReporter != NULL )
		{
			if( errReporter->getSawErrors() )
				errorMessage << ". last error [" << errReporter->getLastError() << "]";
			else
				errorMessage << ". last error (none) [" << errReporter->getLastError() << "]";
		}
		TRACE( errorMessage.str() );

		//Release memory
		if ( memBufIS != NULL )
			delete memBufIS;
		if ( errReporter != NULL )
			delete errReporter;
		if ( parser != NULL )
			delete parser;
			
		throw XSDValidationException( memBufId, m_XsdSchemaLocation, errorMessage.str() );
    }
       
    return AbstractFilter::Completed;
}

bool XSDFilter::canLogPayload()
{
	return true;
//	throw logic_error( "You must override this function to let callers know if this filter can log the payload without disturbing operations" );
}


bool XSDFilter::isMethodSupported( AbstractFilter::FilterMethod method, bool asClient )
{
	switch( method )
	{
		case AbstractFilter::XmlToXml :
				return true;
		case AbstractFilter::BufferToXml :
				return true;
				
		default:
			return false;
	}
}

/// private methods implementation
void XSDFilter::ValidateProperties( NameValueCollection& transportHeaders )
{
	if ( m_Properties.ContainsKey( XSDFilter::XSDFILE ) )
		m_XsdSchemaLocation = m_Properties[ XSDFilter::XSDFILE ];
	else
	{
		if ( transportHeaders.ContainsKey( XSDFilter::XSDFILE ) )
			m_XsdSchemaLocation = transportHeaders[ XSDFilter::XSDFILE ];
		else
			throw invalid_argument( "Required parameter missing : XSDFILE" );
	}

	if ( m_Properties.ContainsKey( XSDFilter::XSDNAMESPACE ) )
		m_XsdNamespace = m_Properties[ XSDFilter::XSDNAMESPACE ];
	else
	{
		if ( transportHeaders.ContainsKey( XSDFilter::XSDNAMESPACE ) )
			m_XsdNamespace = transportHeaders[ XSDFilter::XSDNAMESPACE ];
		else
			m_XsdNamespace = "";
	}
}

