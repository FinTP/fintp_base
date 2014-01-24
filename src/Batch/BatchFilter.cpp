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

#include "BatchFilter.h"
#include "Log/Trace.h"
#include "../XmlUtil.h"
#include "../StringUtil.h"



#include <sstream>
#include <iostream>
#include <strstream>
#include <string>
using namespace std;

#include <xercesc/util/PlatformUtils.hpp>

#include <xercesc/dom/DOMImplementation.hpp>
#include <xercesc/dom/DOMDocument.hpp>

#include <xalanc/Include/XalanAutoPtr.hpp>
#include <xalanc/Include/PlatformDefinitions.hpp>

#include <xalanc/XalanTransformer/XalanTransformer.hpp>
#include <xalanc/XalanTransformer/XercesDOMWrapperParsedSource.hpp>
#include <xalanc/XalanTransformer/XalanCompiledStylesheet.hpp>

#include <xalanc/XercesParserLiaison/XercesParserLiaison.hpp>
#include <xalanc/XercesParserLiaison/XercesDOMSupport.hpp>
#include <xalanc/XercesParserLiaison/FormatterToXercesDOM.hpp>

#include <xalanc/XSLT/XSLTResultTarget.hpp>
#include <xalanc/XSLT/StylesheetRoot.hpp>

#include <xalanc/XalanDOM/XalanDOMException.hpp>

XERCES_CPP_NAMESPACE_USE

const string BatchFilter::BatchFILE = "BatchFILE";
const string BatchFilter::BatchPARAM = "BatchPARAM";
const string BatchFilter::BatchUSEEXT = "BatchUSEEXT";
const string BatchFilter::BatchOUTPUTFORMAT = "BatchOUTPUTFORMAT";

const string BatchFilter::OUTPUT_METHOD_TEXT = "OUTPUT_METHOD_TEXT";
const string BatchFilter::OUTPUT_METHOD_XML = "OUTPUT_METHOD_XML";
const string BatchFilter::OUTPUT_METHOD_NONE = "OUTPUT_METHOD_NONE";

bool BatchFilter::m_ExtensionsInstalled = false;
XercesDOMTreeErrorHandler* BatchFilter::m_ErrorReporter = NULL;
XALAN_CPP_NAMESPACE_QUALIFIER XalanTransformer* BatchFilter::m_Transformer = NULL;
map< string, const XALAN_CPP_NAMESPACE_QUALIFIER XalanCompiledStylesheet* > BatchFilter::m_BatchCache;

BatchFilter::BatchFilter() : AbstractFilter()
{
}

BatchFilter::~BatchFilter()
{
}

void BatchFilter::Terminate()
{
	if( m_Transformer != NULL )
	{
		map< string, const XALAN_CPP_NAMESPACE_QUALIFIER XalanCompiledStylesheet* >::iterator finder;
		
		for( finder = m_BatchCache.begin(); finder != m_BatchCache.end(); finder++ )
		{
			int theResult = m_Transformer->destroyStylesheet( finder->second );
		}

		delete m_Transformer;
	}

	XALAN_USING_XERCES( XMLPlatformUtils );
	XALAN_USING_XALAN( XalanTransformer );
		
	// Terminate Xerces...
	//XMLPlatformUtils::Terminate();
	
	//XalanTransformer::terminate();
	
	// Clean up the ICU, if it's integrated...
	//XalanTransformer::ICUCleanUp();
	
	// Release memory 
	if ( m_ErrorReporter != NULL )
		delete m_ErrorReporter;
		
	if ( m_Transformer != NULL )
		delete m_Transformer;
}	

void BatchFilter::replyOutputFormat( NameValueCollection& transportHeaders, int format )
{
	XALAN_USING_XALAN( FormatterListener );
	string formatAsString = "";
	
	switch( format )
	{
		case FormatterListener::OUTPUT_METHOD_TEXT :
			formatAsString = BatchFilter::OUTPUT_METHOD_TEXT;
			break;
			
		case FormatterListener::OUTPUT_METHOD_XML :
		case FormatterListener::OUTPUT_METHOD_DOM :
			formatAsString = BatchFilter::OUTPUT_METHOD_XML;
			break;
			
		default :
			formatAsString = BatchFilter::OUTPUT_METHOD_NONE;
			break;
	}
	
	if ( transportHeaders.ContainsKey( BatchOUTPUTFORMAT ) )
		transportHeaders[ BatchOUTPUTFORMAT ] = formatAsString;
	else
		transportHeaders.Add( BatchOUTPUTFORMAT, formatAsString );
}

//
// XML to XML
//
AbstractFilter::FilterResult BatchFilter::ProcessMessage( XERCES_CPP_NAMESPACE_QUALIFIER DOMDocument* inputOutputData, NameValueCollection& transportHeaders, bool asClient )
{
	//Validate properties to see if contains Batch file
	validateProperties( transportHeaders );
	
	//Get the BatchFILE 
	string fileBatchLocation = m_Properties[ BatchFilter::BatchFILE ];
   	
	XALAN_USING_XALAN( XalanDOMException )
	try
	{
		XALAN_USING_XALAN( XercesParserLiaison )
		XALAN_USING_XALAN( XercesDOMSupport )
		XALAN_USING_XALAN( XercesDOMWrapperParsedSource )
		XALAN_USING_XALAN( XalanDOMString );
		XALAN_USING_XALAN( XalanCompiledStylesheet );
		
		XALAN_USING_XALAN( FormatterToXercesDOM )
		XALAN_USING_XALAN( XalanAutoPtr )
			
		XALAN_USING_XERCES( DOMImplementation )
		XALAN_USING_XERCES( DOMDocument )
		XALAN_USING_XALAN( FormatterListener )
		XALAN_USING_XALAN( StylesheetRoot )
		
		// reuse error reporter
		m_ErrorReporter->resetErrors();
		
		DEBUG( "About to apply Batch ["  << fileBatchLocation << "]" );		
		XalanCompiledStylesheet* stylesheet = getBatch( fileBatchLocation );
		
		//Prepare a DOM Document to be used as input for a transformation
		XercesDOMSupport theSupporter;
		XercesParserLiaison theLiaison;
		theLiaison.setDoNamespaces( true );
		
		//Let the wrapper know that it doesn't have an uri
		XalanDOMString uri( "" );
		
		//a XercesDOMWrapperParsedSource  will be used 
		XercesDOMWrapperParsedSource wrapper( inputOutputData, theLiaison, theSupporter, uri );
		
		//Prepare to get the transformation's result in a DOMDocument format
		const XalanAutoPtr<DOMDocument> theDocument( DOMImplementation::getImplementation()->createDocument() );

		// FormatterToXercesDOM will be used, which we'll hook up to Xalan's output stage...
		FormatterToXercesDOM theFormatter( theDocument.get(), 0 );
	      	
	  	//Transform the input DOMDocument using the compiled stylesheet. The result will be a DOMDocument
	  	int theResult = getTransformer()->transform( wrapper, stylesheet, theFormatter );
		if( theResult != 0 )
		{			
			stringstream messageBuffer;
			messageBuffer << "Xalan transformation error: " << getTransformer()->getLastError();
			throw runtime_error( messageBuffer.str() );
		}
		
		DEBUG( "Batch applied successfully. " );
		  
		// Copy a DOMDocument content ( source ) into another DOMDocument ( target ) 
		//Copy the transformation result DOMDocument into the inputOutputData DOMDocument parameter
		//Get the target document
		DOMDocument* doc = theFormatter.getDocument();
		if ( doc == NULL )
		{
			TRACE( "Transformation error: The resulted document is NULL" );
			throw runtime_error( "Transformation error: The resulted document is NULL" );
		}
			
		//Get the root element for the target document
		DOMElement* theRootElement  = doc->getDocumentElement();
		if ( theRootElement == NULL )
		{
			TRACE( "Transformation error: Root element of the resulted document is NULL " );
			throw runtime_error( "Transformation error: Root element of the resulted document is NULL " );
		}		
		
		//Clean the target document
		// 	- Delete the root element (and ? subtree) from the target document
		DOMElement* theOldRootElement  = inputOutputData->getDocumentElement();
	 	DOMNode* theRemovedChild = inputOutputData->removeChild( theOldRootElement );
		theRemovedChild->release();
		
		//Copy the source DOMDocument content to taget DOMDocument
		//		- Import the root sub-tree from the source DOMDocument  to inputOutputData DOMDocument
		//		- Append the imported node and his sub-tree to inputOutputData DOMDocument
		inputOutputData->appendChild( inputOutputData->importNode( theRootElement , true ) );
		
		replyOutputFormat( transportHeaders, stylesheet->getStylesheetRoot()->getOutputMethod() );
		DEBUG( "Transformation done. Output format is [" << transportHeaders[ BatchOUTPUTFORMAT ] 
			<< "]. Output address is ["  << inputOutputData << "]" );
	}
	catch( const XalanDOMException& e )
	{
		stringstream errorMessage;
		
		errorMessage << "Xalan transformation error: Code is " << e.getExceptionCode();
		TRACE( typeid( e ).name() << " exception." << errorMessage.str() );
				
		throw runtime_error( errorMessage.str() );
	}
	catch( const exception& e )
	{
		TRACE( typeid( e ).name() << " exception : " << e.what() );
		throw;
	}
	catch( ... )
	{
		TRACE( "Unhandled exception" );
		throw;
	}
   
   	DEBUG( "Batch Transform done" );
	return AbstractFilter::Completed;	
}

//
//  XML To Buffer
//
AbstractFilter::FilterResult BatchFilter::ProcessMessage( XERCES_CPP_NAMESPACE_QUALIFIER DOMDocument* inputData, unsigned char* outputData, NameValueCollection& transportHeaders, bool asClient )
{
	//Validate properties to see if contains Batch file
	validateProperties( transportHeaders );    
	
	//Get the BatchFILE 
	string fileBatchLocation = m_Properties[BatchFilter::BatchFILE];
	
	XALAN_USING_XALAN( XalanDOMException )
   	try
	{
		XALAN_USING_XALAN( XercesParserLiaison )
		XALAN_USING_XALAN( XercesDOMSupport )
		XALAN_USING_XALAN( XercesDOMWrapperParsedSource )
		XALAN_USING_XALAN( XalanDOMString );
		XALAN_USING_XALAN( XalanCompiledStylesheet );
	
		// reuse error reporter
		m_ErrorReporter->resetErrors();
		
		DEBUG( "About to apply Batch ["  << fileBatchLocation << "]" );
		XalanCompiledStylesheet* stylesheet = getBatch( fileBatchLocation );
		
		//Prepare a DOM Document to be used as input for a transformation
		XercesDOMSupport theSupporter;
		XercesParserLiaison theLiaison;
		theLiaison.setDoNamespaces( true );
		
		//Let the wrapper know that it doesn't have an uri
		XalanDOMString uri( "" );
		
		//a XercesDOMWrapperParsedSource  will be used 
		XercesDOMWrapperParsedSource wrapper( inputData, theLiaison, theSupporter, uri );
		
		//Create an outputStream to capture the transformation output
		ostrstream theOutputStream;

	  	//Transform the input DOMDocument into an output stream using the compiled stylesheet
	  	int theResult = getTransformer()->transform( wrapper, stylesheet, theOutputStream );
	  	if( theResult != 0 )
		{			
			stringstream messageBuffer;
			messageBuffer << "Xalan transformation error : " << getTransformer()->getLastError();
			throw runtime_error( messageBuffer.str() );
		} 
	 
		// append terminator
		theOutputStream << '\0';				
		DEBUG( "Batch applied successfully." );
		
		// create the buffer if necessary
		if( outputData == NULL )
		{
			DEBUG( "NULL buffer passed ... allocating "  << theOutputStream.tellp() + 1l << " bytes." );
			outputData = new unsigned char[ ( int )theOutputStream.tellp() + 1 ];
		}

		strcpy( ( char * )outputData, theOutputStream.str() );
	
		replyOutputFormat( transportHeaders, stylesheet->getStylesheetRoot()->getOutputMethod() );
		DEBUG( "Transformation of [" << inputData << "] done. Output format is [" << transportHeaders[ BatchOUTPUTFORMAT ] 
			<< "]. Output is ["  << outputData << "]" );
	}
	catch( const XalanDOMException& e )
	{
		stringstream errorMessage;
		errorMessage << "Xalan transformation error: Code is " << e.getExceptionCode();
		TRACE( typeid( e ).name() << " exception." << errorMessage.str() );
					
		throw runtime_error( errorMessage.str() );
	}
	catch( const exception& e )
	{
		TRACE( typeid( e ).name() << " exception : " << e.what() );
		throw;
	}
	catch( ... )
	{
		TRACE( "Unhandled exception" );
		throw;
	}
			
	DEBUG( "Batch transform done" );
	return AbstractFilter::Completed;
}

AbstractFilter::FilterResult BatchFilter::ProcessMessage( XERCES_CPP_NAMESPACE_QUALIFIER DOMDocument* inputData, unsigned char** outputDataPtr, NameValueCollection& transportHeaders, bool asClient )
{
	//Validate properties to see if contains Batch file
	validateProperties( transportHeaders );    
	
	//Get the BatchFILE 
	string fileBatchLocation = m_Properties[BatchFilter::BatchFILE];
	
	XALAN_USING_XALAN( XalanDOMException )
	try
	{
		XALAN_USING_XALAN( XercesParserLiaison )
		XALAN_USING_XALAN( XercesDOMSupport )
		XALAN_USING_XALAN( XercesDOMWrapperParsedSource )
		XALAN_USING_XALAN( XalanDOMString );
		XALAN_USING_XALAN( XalanCompiledStylesheet );
		
		// reuse error reporter
		m_ErrorReporter->resetErrors();		
		
		DEBUG( "About to apply Batch ["  << fileBatchLocation << "]" );
		XalanCompiledStylesheet* stylesheet = getBatch( fileBatchLocation );
		
		//Prepare a DOM Document to be used as input for a transformation
		XercesDOMSupport theSupporter;
		XercesParserLiaison theLiaison;
		theLiaison.setDoNamespaces( true );
		
		//Let the wrapper know that it doesn't have an uri
		XalanDOMString uri( "" );
		
		//a XercesDOMWrapperParsedSource  will be used 
		XercesDOMWrapperParsedSource wrapper( inputData, theLiaison, theSupporter, uri );
		
		//Create an outputStream to capture the transformation output
		ostrstream theOutputStream;
	    
	   	//Transform the input stream into an output stream using BatchFile
	  	int theResult = getTransformer()->transform( wrapper, stylesheet, theOutputStream );
		if( theResult != 0 )
		{			
			stringstream messageBuffer;
			messageBuffer << "Xalan Transformation error : " << getTransformer()->getLastError();
			throw runtime_error( messageBuffer.str() );
		} 
	 
		// append terminator
		theOutputStream << '\0';
		DEBUG( "Batch applied successfully." );
				
		// create the buffer if necessary
		DEBUG( "Creating buffer ... allocating "  << theOutputStream.tellp() + 1l << " bytes." );
		unsigned char* outputData = new unsigned char[ ( int )theOutputStream.tellp() + 1 ];
		*outputDataPtr = outputData;

		strcpy( ( char * )outputData, theOutputStream.str() );
		
		replyOutputFormat( transportHeaders, stylesheet->getStylesheetRoot()->getOutputMethod() );
		DEBUG( "Transformation of [" << inputData << "] done. Output format is [" << transportHeaders[ BatchOUTPUTFORMAT ] 
			<< "]. Output is ["  << outputData << "]" );
	}
	catch( const XalanDOMException& e )
	{
		stringstream errorMessage;
		errorMessage << "Xalan transformation error: Code is " << e.getExceptionCode();
		TRACE( typeid( e ).name() << " exception." << errorMessage.str() );
					
		throw runtime_error( errorMessage.str() );
	}
	catch( const exception& e )
	{
		TRACE( typeid( e ).name() << " exception : " << e.what() );
		throw;
	}
	catch( ... )
	{
		TRACE( "Unhandled exception" );
		throw;
	}
			
	DEBUG( "Batch transform done" );
	return AbstractFilter::Completed;	
}

bool BatchFilter::canLogPayload()
{
	return true;
//	throw logic_error( "You must override this function to let callers know if this filter can log the payload without disturbing operations" );
}


bool BatchFilter::isMethodSupported( AbstractFilter::FilterMethod method, bool asClient )
{
	
	switch( method )
	{
		case AbstractFilter::XmlToXml :
				return true;
		
		default:
			return false;
	}
}

/// private methods implementation
void BatchFilter::validateProperties( NameValueCollection& transportHeaders )
{
	if ( !m_Properties.ContainsKey( BatchFilter::BatchFILE ) )
		throw invalid_argument( "Required parameter missing : BatchFILE" );
		
	//Set Parameters for BatchFile
	int xlstParamsCount = transportHeaders.getCount();
		
  	if ( xlstParamsCount > 0 )
  	{
  		DEBUG( "The Batch has "  << xlstParamsCount << " parameters." );
  		vector< DictionaryEntry > params = transportHeaders.getData();
		for( int i = 0; i < xlstParamsCount; i++ )
		{
			if ( StringUtil::StartsWith( params[i].first, BatchFilter::BatchPARAM ) )
			{
				DEBUG( "Filter param [" << params[i].first << "] value is [" << params[i].second << "]" );
				getTransformer()->setStylesheetParam( params[i].first.data(), params[i].second.data() );		   		
			}
			else if ( ( params[i].first == BatchFilter::BatchUSEEXT ) && !m_ExtensionsInstalled )
			{
				DEBUG( "Using extension functions from http://extensions.bisnet.ro" );
				const XalanDOMString theNamespace( "http://extensions.bisnet.ro" );
				getTransformer()->installExternalFunction( theNamespace, XalanDOMString( "replace" ), FunctionReplace() );
				getTransformer()->installExternalFunction( theNamespace, XalanDOMString( "fill" ), FunctionFill() );
				m_ExtensionsInstalled = true;
			}
		}	
  	}
  	else
  		DEBUG( "The Batch doesn't have parameters" );
}

XALAN_CPP_NAMESPACE_QUALIFIER XalanTransformer* BatchFilter::getTransformer()
{
	if ( m_Transformer == NULL )
	{
		DEBUG_GLOBAL( "Creating transformer" );
		
		XALAN_USING_XERCES( XMLPlatformUtils );
		XALAN_USING_XALAN( XalanTransformer );
		
		//XMLPlatformUtils::Initialize();	
		
		// Create transformer and initialize Xalan.
		m_Transformer = new XalanTransformer();
//		XalanTransformer::initialize();
		
		// Create error reporter and set it to the transformer
		m_ErrorReporter = new XercesDOMTreeErrorHandler();
  		m_Transformer->setErrorHandler( m_ErrorReporter );
  		
  		m_ExtensionsInstalled = false;
	}
	return m_Transformer;
}

XALAN_CPP_NAMESPACE_QUALIFIER XalanCompiledStylesheet* BatchFilter::getBatch( const string filename )
{
	try
	{
		XALAN_USING_XALAN( XalanCompiledStylesheet );
		map< string, const XalanCompiledStylesheet* >::const_iterator finder = m_BatchCache.find( filename );
		
		if( finder != m_BatchCache.end() )
		{
			DEBUG( "Returning compiled stylesheet [" << filename << "] from cache." );
			return ( XalanCompiledStylesheet* )( finder->second );
		}		
	
		const XalanCompiledStylesheet* theCompiledStylesheet = NULL;
		int	theResult = getTransformer()->compileStylesheet( filename.data(), theCompiledStylesheet );
		
		if ( theResult == 0 ) // OK
		{
			m_BatchCache.insert( pair< string, const XalanCompiledStylesheet* >( filename, theCompiledStylesheet ) );
			
			DEBUG( "Inserting compiled stylesheet [" << filename << "] in cache." );
			return ( XalanCompiledStylesheet* )theCompiledStylesheet;
		}
		else
		{
			stringstream errorMessage;
			errorMessage << "Unable to compile stylesheet [" << filename << "]";
			
			throw runtime_error( errorMessage.str() );
		}
	}
	catch( ... )
	{
		TRACE( "Exception occured while getting Batch [" << filename << "] from cache." );
		throw;
	}	
}
