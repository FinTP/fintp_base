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

#include "XSLTFilter.h"
#include "CacheManager.h"
#include "../DllMain.h"
#include "Trace.h"
#include "XmlUtil.h"
#include "StringUtil.h"

// include extensions
#if !defined( NO_DB )
#include "ExtensionLookup.h"
#endif
#include "ExtensionNewLine.h"
#include "ExtensionTemplate.h"
#include "ExtensionTime.h"
#include "ExtensionAscii.h"
#include "ExtensionRegex.h"
#include "ExtensionHash.h"
#include "ExtensionBase64.h"
#include "ExtensionUrl.h"
using namespace FinTP;

#include <sstream>
#include <iostream>
#if !defined( __DEPRECATEDGNUC__ )
	#include <strstream>
#endif

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

const string XSLTFilter::XSLTFILE = "XSLTFILE";
const string XSLTFilter::XSLTPARAM = "XSLTPARAM";
const string XSLTFilter::XSLTUSEEXT = "XSLTUSEEXT";
const string XSLTFilter::XSLTOUTPUTFORMAT = "XSLTOUTPUTFORMAT";
const string XSLTFilter::XSLTCRTPARSESOURCE = "XSLTPSRC";

const string XSLTFilter::OUTPUT_METHOD_TEXT = "OUTPUT_METHOD_TEXT";
const string XSLTFilter::OUTPUT_METHOD_XML = "OUTPUT_METHOD_XML";
const string XSLTFilter::OUTPUT_METHOD_NONE = "OUTPUT_METHOD_NONE";

pthread_once_t XSLTFilter::KeysCreate = PTHREAD_ONCE_INIT;
pthread_key_t XSLTFilter::CompiledXSLTsKey;
pthread_key_t XSLTFilter::TransformerKey;

XALAN_CPP_NAMESPACE_QUALIFIER XercesParserLiaison* XSLTFilter::m_Liaison = NULL;
XALAN_CPP_NAMESPACE_QUALIFIER XercesDOMSupport* XSLTFilter::m_DOMSupport = NULL;

//XercesDOMTreeErrorHandler* XSLTFilter::m_ErrorReporter = NULL;
//map< pthread_t, XALAN_CPP_NAMESPACE_QUALIFIER XalanTransformer* > XSLTFilter::m_Transformer;
//map< pthread_t, map< string, const XALAN_CPP_NAMESPACE_QUALIFIER XalanCompiledStylesheet* > > XSLTFilter::m_XSLTCache;

XSLTFilter::XSLTFilter() : AbstractFilter( FilterType::XSLT )//, m_ExtensionsInstalled( false )
{
	int onceResult = pthread_once( &XSLTFilter::KeysCreate, &XSLTFilter::CreateKeys );
	if ( 0 != onceResult )
	{
		TRACE( "One time key creation for XSLT transformer threads failed [" << onceResult << "]" );
	}
}

XSLTFilter::~XSLTFilter()
{
}

void XSLTFilter::CreateKeys()
{
	cout << "Thread [" << pthread_self() << "] creating transformer keys..." << endl;

	int keyCreateResult = pthread_key_create( &XSLTFilter::TransformerKey, &XSLTFilter::DeleteTransformers );
	if ( 0 != keyCreateResult )
	{
		TRACE( "An error occured while creating transformer thread key [" << keyCreateResult << "]" );
	}

	keyCreateResult = pthread_key_create( &XSLTFilter::CompiledXSLTsKey, NULL );
	if ( 0 != keyCreateResult )
	{
		TRACE( "An error occured while creating transformer compiled XSLTs key [" << keyCreateResult << "]" );
	}

	m_Liaison = new XALAN_CPP_NAMESPACE_QUALIFIER XercesParserLiaison();
	m_Liaison->setDoNamespaces( true );

#if (_XALAN_VERSION >= 11100)
	m_DOMSupport = new XALAN_CPP_NAMESPACE_QUALIFIER XercesDOMSupport(*m_Liaison);
#else
	m_DOMSupport = new XALAN_CPP_NAMESPACE_QUALIFIER XercesDOMSupport();
#endif

		// install extensions
	DEBUG( "Using extension functions from http://extensions.bisnet.ro" );

	XALAN_USING_XALAN( XalanTransformer );

#if !defined( NO_DB )
	XalanTransformer::installExternalFunctionGlobal( XalanDOMString( "http://extensions.bisnet.ro" ), XalanDOMString( "lookup" ), FunctionLookup() );
#endif
	XalanTransformer::installExternalFunctionGlobal( XalanDOMString( "http://extensions.bisnet.ro" ), XalanDOMString( "replace" ), FunctionReplace() );
	XalanTransformer::installExternalFunctionGlobal( XalanDOMString( "http://extensions.bisnet.ro" ), XalanDOMString( "fill" ), FunctionFill() );
	XalanTransformer::installExternalFunctionGlobal( XalanDOMString( "http://extensions.bisnet.ro" ), XalanDOMString( "time" ), FunctionTime() );
	XalanTransformer::installExternalFunctionGlobal( XalanDOMString( "http://extensions.bisnet.ro" ), XalanDOMString( "ascii" ), FunctionAscii() );
	XalanTransformer::installExternalFunctionGlobal( XalanDOMString( "http://extensions.bisnet.ro" ), XalanDOMString( "regex" ), FunctionRegex() );
	XalanTransformer::installExternalFunctionGlobal( XalanDOMString( "http://extensions.bisnet.ro" ), XalanDOMString( "regex-match" ), FunctionRegexMatch() );
	XalanTransformer::installExternalFunctionGlobal( XalanDOMString( "http://extensions.bisnet.ro" ), XalanDOMString( "hash" ), FunctionHash() );
	XalanTransformer::installExternalFunctionGlobal( XalanDOMString( "http://extensions.bisnet.ro" ), XalanDOMString( "base64" ), FunctionBase64() );
	XalanTransformer::installExternalFunctionGlobal( XalanDOMString( "http://extensions.bisnet.ro" ), XalanDOMString( "url-get" ), FunctionUrl() );
}

void XSLTFilter::DeleteTransformers( void* data )
{
	XALAN_USING_XALAN( XalanTransformer );
	XALAN_USING_XALAN( XalanCompiledStylesheet );

	XalanTransformer* threadTransformer = ( XalanTransformer* )data;
	if ( threadTransformer != NULL )
	{
		// delete compiled XSLTs
		CacheManager< string, const XalanCompiledStylesheet* >* xsltMap = ( CacheManager< string, const XalanCompiledStylesheet* >* )pthread_getspecific( XSLTFilter::CompiledXSLTsKey );
		if ( xsltMap != NULL ) 
		{
			if ( xsltMap->size() > 0 )
			{
				map< string, const XalanCompiledStylesheet* >& xsltCache = xsltMap->data();
				map< string, const XalanCompiledStylesheet* >::iterator xsltCacheWalker = xsltCache.begin();
				for( ;xsltCacheWalker != xsltCache.end(); xsltCacheWalker++ )
				{
					try
					{
						if ( xsltCacheWalker->second != NULL )
						{
							int destroyResult = threadTransformer->destroyStylesheet( xsltCacheWalker->second );
							xsltCacheWalker->second = NULL;
							if ( 0 != destroyResult )
							{
								TRACE( "Error releasing xslt from cache [" << threadTransformer << "], [" << xsltCacheWalker->first << "] error code : " << destroyResult );
							}
						}
					}
					catch( ... )
					{
						TRACE( "Error releasing xslt from cache [" << threadTransformer << "], [" << xsltCacheWalker->first << "]" );
					}
				}
				xsltMap->clear();
			}

			delete xsltMap;
		}
		int setSpecificResult = pthread_setspecific( XSLTFilter::CompiledXSLTsKey, NULL );
		if ( 0 != setSpecificResult )
		{
			TRACE( "Set thread specific CompiledXSLTsKey failed [" << setSpecificResult << "]" );
		}

		try
		{
			// delete transformer
			delete threadTransformer;
		}
		catch( ... )
		{
			TRACE( "Error releasing transformer" );
		}
	}

	int setSpecificResult = pthread_setspecific( XSLTFilter::TransformerKey, NULL );
	if ( 0 != setSpecificResult )
	{
		TRACE( "Set thread specific TransformerKey failed [" << setSpecificResult << "]" );
	}

	if( m_Liaison != NULL )
	{
		delete m_Liaison;
		m_Liaison = NULL;
	}

	if( m_DOMSupport != NULL )
	{
		delete m_DOMSupport;
		m_DOMSupport = NULL;
	}
}

void XSLTFilter::replyOutputFormat( NameValueCollection& transportHeaders, int outFormat ) const
{
	XALAN_USING_XALAN( FormatterListener );
	string formatAsString = "";
	
	switch( outFormat )
	{
		case FormatterListener::OUTPUT_METHOD_TEXT :
			formatAsString = XSLTFilter::OUTPUT_METHOD_TEXT;
			break;
			
		case FormatterListener::OUTPUT_METHOD_XML :
		case FormatterListener::OUTPUT_METHOD_DOM :
			formatAsString = XSLTFilter::OUTPUT_METHOD_XML;
			break;
			
		default :
			formatAsString = XSLTFilter::OUTPUT_METHOD_NONE;
			break;
	}
	
	if ( transportHeaders.ContainsKey( XSLTOUTPUTFORMAT ) )
		transportHeaders.ChangeValue( XSLTOUTPUTFORMAT, formatAsString );
	else
		transportHeaders.Add( XSLTOUTPUTFORMAT, formatAsString );
}

//
// XML to XML
//
AbstractFilter::FilterResult XSLTFilter::ProcessMessage( XERCES_CPP_NAMESPACE_QUALIFIER DOMDocument* inputOutputData, NameValueCollection& transportHeaders, bool asClient )
{
	//Validate properties to see if contains XSLT file
	string xsltFilename = getTransform( transportHeaders );
   	
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
		//XALAN_USING_XALAN( FormatterListener )
		//XALAN_USING_XALAN( StylesheetRoot )
		
		// reuse error reporter
		//m_ErrorReporter->resetErrors();
		
		DEBUG( "About to apply XSLT [" << xsltFilename << "] ..." );		
		const XalanCompiledStylesheet* stylesheet = getXSLT( xsltFilename );
		
		//Prepare a DOM Document to be used as input for a transform
		XercesParserLiaison theLiaison;
		theLiaison.setDoNamespaces( true );
#if (_XALAN_VERSION >= 11100)
		XercesDOMSupport theSupporter(theLiaison);
#else
		XercesDOMSupport theSupporter;
#endif
		
		//Let the wrapper know that it doesn't have an uri
		XalanDOMString uri( "" );
		
		//a XercesDOMWrapperParsedSource  will be used 
		XercesDOMWrapperParsedSource wrapper( inputOutputData, theLiaison, theSupporter, uri );
		
		//Prepare to get the transform result in a DOMDocument format
		const XalanAutoPtr<DOMDocument> theDocument( DOMImplementation::getImplementation()->createDocument() );

		// FormatterToXercesDOM will be used, which we'll hook up to Xalan's output stage...
		FormatterToXercesDOM theFormatter( theDocument.get(), 0 );
	      	
	  	//Transform the input DOMDocument using the compiled stylesheet. The result will be a DOMDocument
	  	int theResult = getTransformer()->transform( wrapper, stylesheet, theFormatter );
		if( theResult != 0 )
		{			
			stringstream messageBuffer;
			messageBuffer << "Xalan transform error: " << getTransformer()->getLastError();
			throw runtime_error( messageBuffer.str() );
		}
		
		DEBUG( "XSLT applied successfully. " );
		  
		// Copy a DOMDocument content ( source ) into another DOMDocument ( target ) 
		//Copy the transform result DOMDocument into the inputOutputData DOMDocument parameter
		//Get the target document
		DOMDocument* doc = theFormatter.getDocument();
		if ( doc == NULL )
		{
			TRACE( "Transform error : The resulted document is NULL" );
			throw runtime_error( "Transform error: The resulted document is NULL" );
		}
		
		replyOutputFormat( transportHeaders, stylesheet->getStylesheetRoot()->getOutputMethod() );

		//Get the root element for the target document
		DOMElement* theRootElement  = doc->getDocumentElement();
		if ( theRootElement == NULL )
		{
			TRACE( "Transform error: Root element of the resulted document is NULL " );
			throw runtime_error( "Transform error: Root element of the resulted document is NULL " );
		}		
		
		//Clean the target document
		// 	- Delete the root element (and ? subtree) from the target document
		DOMElement* theOldRootElement = inputOutputData->getDocumentElement();
	 	DOMNode* theRemovedChild = inputOutputData->removeChild( theOldRootElement );
		theRemovedChild->release();
		
		//Copy the source DOMDocument content to taget DOMDocument
		//		- Import the root sub-tree from the source DOMDocument  to inputOutputData DOMDocument
		//		- Append the imported node and his sub-tree to inputOutputData DOMDocument
		inputOutputData->appendChild( inputOutputData->importNode( theRootElement , true ) );

		DEBUG( "Transform done. Output format is [" << transportHeaders[ XSLTOUTPUTFORMAT ] 
			<< "]. Output address is [" << inputOutputData << "]" );
	}
	catch( const XalanDOMException& e )
	{
		stringstream errorMessage;
		
		errorMessage << "Xalan transform error: Code is " << e.getExceptionCode();
		TRACE( typeid( e ).name() << " exception." << errorMessage.str() );
				
		throw runtime_error( errorMessage.str() );
	}
	catch( const std::exception& e )
	{
		TRACE( typeid( e ).name() << " exception : " << e.what() );
		throw;
	}
	catch( ... )
	{
		TRACE( "Unhandled exception" );
		throw;
	}
   
   	DEBUG2( "XSLT Transform done" );
	return AbstractFilter::Completed;	
}

//
//  XML To Buffer
//
AbstractFilter::FilterResult XSLTFilter::ProcessMessage( const XERCES_CPP_NAMESPACE_QUALIFIER DOMDocument* inputData, AbstractFilter::buffer_type outputData, NameValueCollection& transportHeaders, bool asClient )
{
	//Validate properties to see if contains XSLT file
	string xsltFilename = getTransform( transportHeaders );    
	
	//Create an outputStream to capture the transform output
#if defined ( __DEPRECATEDGNUC__ )
	stringstream theOutputStream;
#else
	ostrstream theOutputStream;
#endif

	XALAN_USING_XALAN( XalanDOMException )
   	try
	{
		XALAN_USING_XALAN( XercesParserLiaison )
		XALAN_USING_XALAN( XercesDOMSupport )
		XALAN_USING_XALAN( XercesDOMWrapperParsedSource )
		XALAN_USING_XALAN( XalanDOMString );
		XALAN_USING_XALAN( XalanCompiledStylesheet );
	
		// reuse error reporter
		//m_ErrorReporter->resetErrors();
		
		DEBUG( "About to apply XSLT [" << xsltFilename << "] ..." );
		const XalanCompiledStylesheet* stylesheet = getXSLT( xsltFilename );
		
		//Prepare a DOM Document to be used as input for a transform
		XercesParserLiaison theLiaison;
		theLiaison.setDoNamespaces( true );

#if (_XALAN_VERSION >= 11100)
		XercesDOMSupport theSupporter(theLiaison);
#else
		XercesDOMSupport theSupporter;
#endif
		
		//Let the wrapper know that it doesn't have an uri
		XalanDOMString uri( "" );
		
		//a XercesDOMWrapperParsedSource  will be used 
		XercesDOMWrapperParsedSource wrapper( inputData, theLiaison, theSupporter, uri );
		
	  	//Transform the input DOMDocument into an output stream using the compiled stylesheet
	  	int theResult = getTransformer()->transform( wrapper, stylesheet, theOutputStream );
	  	if( theResult != 0 )
		{			
			stringstream messageBuffer;
			messageBuffer << "Xalan transform error : " << getTransformer()->getLastError();
			throw runtime_error( messageBuffer.str() );
		} 
	 
		// append terminator
		theOutputStream << '\0';				
		DEBUG( "XSLT applied successfully." );
		
		// create the buffer if necessary
		outputData.get()->copyFrom( theOutputStream.str() );

#if !defined ( __DEPRECATEDGNUC__ )
		// unfreeze buffer( after str ) in order to allow deallocation of the buffer
		theOutputStream.rdbuf()->freeze( false );
#endif
	
		replyOutputFormat( transportHeaders, stylesheet->getStylesheetRoot()->getOutputMethod() );
		DEBUG( "Transform of [" << inputData << "] done. Output format is [" << transportHeaders[ XSLTOUTPUTFORMAT ] 
			<< "]. Output is ( first 100 bytes ) [" << outputData.get()->str( 100 ) << "]" );
	}
	catch( const XalanDOMException& e )
	{
#if !defined ( __DEPRECATEDGNUC__ )
		// unfreeze buffer( after str ) in order to allow deallocation of the buffer
		theOutputStream.rdbuf()->freeze( false );
#endif
		
		stringstream errorMessage;
		errorMessage << "Xalan transform error: Code is " << e.getExceptionCode();
		TRACE( typeid( e ).name() << " exception encountered while transforming message : " << errorMessage.str() );
					
		throw runtime_error( errorMessage.str() );
	}
	catch( const std::exception& e )
	{
#if !defined ( __DEPRECATEDGNUC__ )
		// unfreeze buffer( after str ) in order to allow deallocation of the buffer
		theOutputStream.rdbuf()->freeze( false );
#endif
		
		TRACE( typeid( e ).name() << " exception encountered while transforming message : " << e.what() );
		throw;
	}
	catch( ... )
	{
#if !defined ( __DEPRECATEDGNUC__ )
		// unfreeze buffer( after str ) in order to allow deallocation of the buffer
		theOutputStream.rdbuf()->freeze( false );
#endif
		
		TRACE( "Unknown exception encountered while transforming message" );
		throw;
	}
		
	DEBUG2( "XSLT transform done" );
	return AbstractFilter::Completed;
}

AbstractFilter::FilterResult XSLTFilter::ProcessMessage( const XERCES_CPP_NAMESPACE_QUALIFIER DOMDocument* inputData, unsigned char** outputDataPtr, NameValueCollection& transportHeaders, bool asClient )
{
	//Validate properties to see if contains XSLT file
	string xsltFilename = getTransform( transportHeaders );    
	
	//Create an outputStream to capture the transform output
#if defined ( __DEPRECATEDGNUC__ )
	stringstream theOutputStream;
#else
	ostrstream theOutputStream;
#endif
			
	XALAN_USING_XALAN( XalanDOMException )
	try
	{
		XALAN_USING_XALAN( XercesParserLiaison )
		XALAN_USING_XALAN( XercesDOMSupport )
		XALAN_USING_XALAN( XercesDOMWrapperParsedSource )
		XALAN_USING_XALAN( XalanDOMString );
		XALAN_USING_XALAN( XalanCompiledStylesheet );
		
		// reuse error reporter
		//m_ErrorReporter->resetErrors();		
		
		DEBUG( "About to apply XSLT [" << xsltFilename << "] ..." );
		const XalanCompiledStylesheet* stylesheet = getXSLT( xsltFilename );
		
		//Prepare a DOM Document to be used as input for a transform
		XercesParserLiaison theLiaison;
		theLiaison.setDoNamespaces( true );
#if (_XALAN_VERSION >= 11100)
		XercesDOMSupport theSupporter(theLiaison);
#else
		XercesDOMSupport theSupporter;
#endif
		
		//Let the wrapper know that it doesn't have an uri
		XalanDOMString uri( "" );
		
		//a XercesDOMWrapperParsedSource  will be used 
		XercesDOMWrapperParsedSource wrapper( inputData, theLiaison, theSupporter, uri );
		
		//Transform the input stream into an output stream using XSLTFile
		int theResult = getTransformer()->transform( wrapper, stylesheet, theOutputStream );
		if( theResult != 0 )
		{			
			stringstream messageBuffer;
			messageBuffer << "Xalan transform error : " << getTransformer()->getLastError();
			throw runtime_error( messageBuffer.str() );
		} 
	 
		// append terminator
		theOutputStream << '\0';
		DEBUG( "XSLT applied successfully." );
				
		// create the buffer if necessary
		unsigned long bufferSize = ( unsigned long )( theOutputStream.tellp() ) + 1L;

		DEBUG( "Creating buffer ... allocating " << bufferSize << " bytes." );
		unsigned char* outputData = new unsigned char[ bufferSize ];
		*outputDataPtr = outputData;

#ifdef CRT_SECURE
		int strcpyResult = strcpy_s( ( char * )outputData, bufferSize, theOutputStream.str() );
		if ( 0 != strcpyResult )
		{
			stringstream errorMessage;
			errorMessage << "Unable to copy data to output buffer. buffer size [" << bufferSize << "] error code [" << strcpyResult << "]";
			TRACE( errorMessage.str() )
			throw runtime_error( errorMessage.str() );
		}
#else
	#if defined ( __DEPRECATEDGNUC__ )
		strcpy( ( char * )outputData, theOutputStream.str().c_str() );
	#else
		strcpy( ( char * )outputData, theOutputStream.str() );
	#endif
#endif
		
#if !defined ( __DEPRECATEDGNUC__ )
		// unfreeze buffer( after str ) in order to allow deallocation of the buffer
		theOutputStream.rdbuf()->freeze( false );
#endif
		
		replyOutputFormat( transportHeaders, stylesheet->getStylesheetRoot()->getOutputMethod() );
		DEBUG( "Transform of [" << inputData << "] done. Output format is [" << transportHeaders[ XSLTOUTPUTFORMAT ] 
			<< "]. Output is [" << outputData << "]" );
	}
	catch( const XalanDOMException& e )
	{
#if !defined ( __DEPRECATEDGNUC__ )
		// unfreeze buffer( after str ) in order to allow deallocation of the buffer
		theOutputStream.rdbuf()->freeze( false );
#endif
		stringstream errorMessage;
		errorMessage << "Xalan transform error: Code is " << e.getExceptionCode();
		TRACE( typeid( e ).name() << " exception encountered while transforming message : " << errorMessage.str() );
					
		throw runtime_error( errorMessage.str() );
	}
	catch( const std::exception& e )
	{
#if !defined ( __DEPRECATEDGNUC__ )
		// unfreeze buffer( after str ) in order to allow deallocation of the buffer
		theOutputStream.rdbuf()->freeze( false );
#endif
		
		TRACE( typeid( e ).name() << " exception encountered while transforming message : " << e.what() );
		throw;
	}
	catch( ... )
	{
#if !defined ( __DEPRECATEDGNUC__ )
		// unfreeze buffer( after str ) in order to allow deallocation of the buffer
		theOutputStream.rdbuf()->freeze( false );
#endif
		
		TRACE( "Unknown exception encountered while transforming message" );
		throw;
	}
	
	DEBUG2( "XSLT transform done" );
	return AbstractFilter::Completed;	
}

//
//  Buffer To Buffer
//
AbstractFilter::FilterResult XSLTFilter::ProcessMessage( AbstractFilter::buffer_type inputData, AbstractFilter::buffer_type outputData, NameValueCollection& transportHeaders, bool asClient )
{
	//Validate properties to see if contains XSLT file
	string xsltFilename = getTransform( transportHeaders );    
	
	//Create an outputStream to capture the transform output
#if defined ( __DEPRECATEDGNUC__ )
	stringstream theOutputStream;
#else
	ostrstream theOutputStream;
#endif
	
	// input buffer
	ManagedBuffer* inputBuffer = inputData.get();
	if ( inputBuffer == NULL )
		throw runtime_error( "Input document is empty" );
	
	// input source stream
#if defined ( __DEPRECATEDGNUC__ )
	stringstream theInputStream( inputBuffer->str() );
#else
	istrstream theInputStream( ( char* )( inputBuffer->buffer() ), inputBuffer->size() );
#endif

	XALAN_USING_XALAN( XalanDOMException )
	try
	{
		//XALAN_USING_XALAN( XalanDOMString );
		XALAN_USING_XALAN( XalanCompiledStylesheet );
	
		// reuse error reporter
		//m_ErrorReporter->resetErrors();
		
		DEBUG( "About to apply XSLT [" << xsltFilename << "] ..." );
		const XalanCompiledStylesheet* stylesheet = getXSLT( xsltFilename );
				
	  	//Transform the input DOMDocument into an output stream using the compiled stylesheet
	  	int theResult = getTransformer()->transform( theInputStream, stylesheet, theOutputStream );
	  	if( theResult != 0 ) 
		{			
			stringstream messageBuffer;
			messageBuffer << "Xalan transform error : " << getTransformer()->getLastError();
			throw runtime_error( messageBuffer.str() );
		} 
	 
		// append terminator
		theOutputStream << '\0';				
		DEBUG( "XSLT applied successfully." );
		
		// create the buffer if necessary
		outputData.get()->copyFrom( theOutputStream.str() );

#if !defined ( __DEPRECATEDGNUC__ )
		// unfreeze buffer( after str ) in order to allow deallocation of the buffer
		theInputStream.rdbuf()->freeze( false );
		theOutputStream.rdbuf()->freeze( false );
#endif

		replyOutputFormat( transportHeaders, stylesheet->getStylesheetRoot()->getOutputMethod() );
		DEBUG( "Transform of [...] done. Output format is [" << transportHeaders[ XSLTOUTPUTFORMAT ] 
			<< "]. Output is [" << outputData.get()->str() << "]" );
	}
	catch( const XalanDOMException& e )
	{
#if !defined ( __DEPRECATEDGNUC__ )
		// unfreeze buffer( after str ) in order to allow deallocation of the buffer
		theInputStream.rdbuf()->freeze( false );
		theOutputStream.rdbuf()->freeze( false );
#endif

		stringstream errorMessage;
		errorMessage << "Xalan transform error: Code is " << e.getExceptionCode();
		TRACE( typeid( e ).name() << " exception encountered while transforming message : " << errorMessage.str() );
					
		throw runtime_error( errorMessage.str() );
	}
	catch( const std::exception& e )
	{
#if !defined ( __DEPRECATEDGNUC__ )
		// unfreeze buffer( after str ) in order to allow deallocation of the buffer
		theInputStream.rdbuf()->freeze( false );
		theOutputStream.rdbuf()->freeze( false );
#endif

		TRACE( typeid( e ).name() << " exception encountered while transforming message : " << e.what() );
		throw;
	}
	catch( ... )
	{
#if !defined ( __DEPRECATEDGNUC__ )
		// unfreeze buffer( after str ) in order to allow deallocation of the buffer
		theInputStream.rdbuf()->freeze( false);
		theOutputStream.rdbuf()->freeze( false );
#endif

		TRACE( "Unknown exception encountered while transforming message" );
		throw;
	}

	DEBUG2( "XSLT transform done" );
	return AbstractFilter::Completed;
}

//
//  XalanDoc To Buffer
//
AbstractFilter::FilterResult XSLTFilter::ProcessMessage( XALAN_CPP_NAMESPACE_QUALIFIER XercesDOMWrapperParsedSource* inputData, AbstractFilter::buffer_type outputData, NameValueCollection& transportHeaders, bool asClient )
{
	//Validate properties to see if contains XSLT file
	string xsltFilename = getTransform( transportHeaders );    
	
	//Create an outputStream to capture the transform output
#if defined ( __DEPRECATEDGNUC__ )
	stringstream theOutputStream;
#else
	ostrstream theOutputStream;
#endif

	// validate input
	if ( inputData == NULL )
		throw runtime_error( "Input document is empty" );

	XALAN_USING_XALAN( XalanDOMException )
   	try
	{
		//XALAN_USING_XALAN( XalanDOMString );
		XALAN_USING_XALAN( XalanCompiledStylesheet );

		// reuse error reporter
		//m_ErrorReporter->resetErrors();

		DEBUG( "About to apply XSLT [" << xsltFilename << "] ..." );
		const XalanCompiledStylesheet* stylesheet = getXSLT( xsltFilename );

	  	//Transform the input DOMDocument into an output stream using the compiled stylesheet
	  	if( 0 != getTransformer()->transform( *inputData, stylesheet, theOutputStream ) )
		{			
			stringstream messageBuffer;
			messageBuffer << "Xalan transform error : " << getTransformer()->getLastError();
			throw runtime_error( messageBuffer.str() );
		}

		// append terminator
		theOutputStream << '\0';
		DEBUG( "XSLT applied successfully." );
		
		// create the buffer if necessary
		outputData.get()->copyFrom( theOutputStream.str() );

#if !defined ( __DEPRECATEDGNUC__ )
		// unfreeze buffer( after str ) in order to allow deallocation of the buffer
		theOutputStream.rdbuf()->freeze( false );
#endif

		replyOutputFormat( transportHeaders, stylesheet->getStylesheetRoot()->getOutputMethod() );
		
		if ( ( outputData.get() )->size() > 100 )
		{
			ManagedBuffer* managedBuffer = outputData.get();
			string partialBuffer = string( ( char* )managedBuffer->buffer(), 100 );
			DEBUG( "Transform of [" << inputData << "] done. Output format is [" << transportHeaders[ XSLTOUTPUTFORMAT ] 
			<< "]. Output is ( first 100 bytes ) [" << partialBuffer << "]" );
		}
		else
		{
			DEBUG( "Transform of [" << inputData << "] done. Output format is [" << transportHeaders[ XSLTOUTPUTFORMAT ] 
			<< "]. Output is [" << outputData.get()->str() << "]" );
		}
	}
	catch( const XalanDOMException& e )
	{
#if !defined ( __DEPRECATEDGNUC__ )
		// unfreeze buffer( after str ) in order to allow deallocation of the buffer
		theOutputStream.rdbuf()->freeze( false );
#endif
		
		stringstream errorMessage;
		errorMessage << "Xalan transform error: Code is " << e.getExceptionCode();
		TRACE( typeid( e ).name() << " exception encountered while transforming message : " << errorMessage.str() );
					
		throw runtime_error( errorMessage.str() );
	}
	catch( const std::exception& e )
	{
#if !defined ( __DEPRECATEDGNUC__ )
		// unfreeze buffer( after str ) in order to allow deallocation of the buffer
		theOutputStream.rdbuf()->freeze( false );
#endif
		
		TRACE( typeid( e ).name() << " exception encountered while transforming message : " << e.what() );
		throw;
	}
	catch( ... )
	{
#if !defined ( __DEPRECATEDGNUC__ )
		// unfreeze buffer( after str ) in order to allow deallocation of the buffer
		theOutputStream.rdbuf()->freeze( false );
#endif
		
		TRACE( "Unknown exception encountered while transforming message" );
		throw;
	}
		
	DEBUG2( "XSLT transform done" );
	return AbstractFilter::Completed;
}

XALAN_CPP_NAMESPACE_QUALIFIER XercesDOMWrapperParsedSource* XSLTFilter::parseSource( const XERCES_CPP_NAMESPACE_QUALIFIER DOMDocument* inputData )
{
	XALAN_USING_XALAN( XalanDOMException )
	try
	{
		//XALAN_USING_XALAN( XercesParserLiaison )
		//XALAN_USING_XALAN( XercesDOMSupport )
		XALAN_USING_XALAN( XercesDOMWrapperParsedSource )
		XALAN_USING_XALAN( XalanDOMString );
		
		//Let the wrapper know that it doesn't have an uri
		XalanDOMString uri( "" );

		m_Liaison->resetErrors();
		m_DOMSupport->reset();
		
		//a XercesDOMWrapperParsedSource  will be used 
		return new XercesDOMWrapperParsedSource( inputData, *m_Liaison, *m_DOMSupport, uri );
	}
	catch( const XalanDOMException& e )
	{		
		stringstream errorMessage;
		errorMessage << "Xalan parse error: Code is " << e.getExceptionCode();
		TRACE( typeid( e ).name() << " exception encountered while parsing message : " << errorMessage.str() );
					
		throw runtime_error( errorMessage.str() );
	}
	catch( ... )
	{		
		TRACE( "Unknown exception encountered while parsing message" );
		throw;
	}
}

void XSLTFilter::releaseSource( XALAN_CPP_NAMESPACE_QUALIFIER XercesDOMWrapperParsedSource* source )
{
	if( source != NULL )
	{
		delete source;
		source = NULL;
	}
}

bool XSLTFilter::canLogPayload()
{
	return true;
//	throw logic_error( "You must override this function to let callers know if this filter can log the payload without disturbing operations" );
}

bool XSLTFilter::isMethodSupported( AbstractFilter::FilterMethod method, bool asClient )
{	
	switch( method )
	{
		case AbstractFilter::XmlToXml :
			return true;
		case AbstractFilter::XmlToBuffer :
			return true;
		case AbstractFilter::BufferToBuffer :
			return true;
		default:
			return false;
	}
}

/// private methods implementation
string XSLTFilter::getTransform( NameValueCollection& transportHeaders )
{
	string xsltFilename = "";
	// m_Properties is not thread safe !!!
	if( transportHeaders.ContainsKey( XSLTFilter::XSLTFILE ) )
		xsltFilename = transportHeaders[ XSLTFilter::XSLTFILE ];
	else
	{
		if ( m_Properties.ContainsKey( XSLTFilter::XSLTFILE ) )
		{
			TRACE( "This construct is not thread-safe" );
			xsltFilename = m_Properties[ XSLTFilter::XSLTFILE ];
		}
		else
			throw invalid_argument( "Required parameter missing : XSLTFILE" );
	}
	
	XALAN_USING_XALAN( XalanTransformer );
	
	// Ensure the transformer is created and get a hold on it
	XalanTransformer *returnedTransformer = getTransformer();

	//Set Parameters for XSLTFile
	unsigned int xlstParamsCount = transportHeaders.getCount();
		
  	if ( xlstParamsCount > 0 )
  	{
		DEBUG( "XSLT [" << xsltFilename << "] has " << xlstParamsCount << " parameters :" );
  		vector< DictionaryEntry > params = transportHeaders.getData();

		// add all params to the transform
		for( unsigned int i=0; i<xlstParamsCount; i++ )
		{
			if ( StringUtil::StartsWith( params[i].first, XSLTFilter::XSLTPARAM ) )
			{
				DEBUG( "\tFilter param [" << params[ i ].first << "] value is [" << params[ i ].second << "]" );
				returnedTransformer->setStylesheetParam( params[ i ].first.data(), params[ i ].second.data() );		   		
			}
		}
  	}
  	else
	{
		DEBUG( "XSLT [" << xsltFilename << "] doesn't have parameters" );
	}
	return xsltFilename;
}

XALAN_CPP_NAMESPACE_QUALIFIER XalanTransformer* XSLTFilter::getTransformer()
{
	XALAN_USING_XALAN( XalanCompiledStylesheet );
	XALAN_USING_XALAN( XalanTransformer );

	XalanTransformer *returnedTransformer = ( XalanTransformer * )pthread_getspecific( XSLTFilter::TransformerKey );
	if ( returnedTransformer == NULL )
	{
		try
		{
			pthread_t selfId = pthread_self();
			DEBUG( "Creating transformer for thread [" << selfId << "] ..." );

			// Create transformer 
			returnedTransformer = new XalanTransformer();
				
			if ( returnedTransformer == NULL )
				throw runtime_error( "Error occured during creation of XalanTransformer" );

			DEBUG( "Transformer created; setting thread specific transformer instance [" << selfId << "]" );

			// insert the transfomer to collection 
			int setSpecificResult = pthread_setspecific( XSLTFilter::TransformerKey, returnedTransformer );
			if ( 0 != setSpecificResult )
			{
				TRACE( "Set thread specific TransformerKey failed [" << setSpecificResult << "]" );
			}

			// create XSLT map
			CacheManager< string, const XalanCompiledStylesheet* >* xsltMap = ( CacheManager< string, const XalanCompiledStylesheet* >* )pthread_getspecific( XSLTFilter::CompiledXSLTsKey );
			if( xsltMap == NULL )
			{
				xsltMap = new CacheManager< string, const XalanCompiledStylesheet* >();
				setSpecificResult = pthread_setspecific( XSLTFilter::CompiledXSLTsKey, xsltMap );
				if ( 0 != setSpecificResult )
				{
					TRACE( "Set thread specific CompiledXSLTsKey failed [" << setSpecificResult << "]" );
				}
			}
		}
		catch( const std::exception& e )
		{
			TRACE( "Exception occured while creating transformer. " << typeid( e ).name() << " : " << e.what() );
			throw;
		}
		catch( ... )
		{
			TRACE( "Exception occured while creating transformer." );
			throw;
		}
	}

	return returnedTransformer;
}

const XALAN_CPP_NAMESPACE_QUALIFIER XalanCompiledStylesheet* XSLTFilter::getXSLT( const string& filename ) 
{
	XALAN_USING_XALAN( XalanCompiledStylesheet );
	//XALAN_USING_XALAN( XSLTInputSource );
	
	const XalanCompiledStylesheet *returnedXSLT = NULL;

	try
	{
		CacheManager< string, const XalanCompiledStylesheet* >* xsltMap = ( CacheManager< string, const XalanCompiledStylesheet* >* )pthread_getspecific( XSLTFilter::CompiledXSLTsKey );
		if( xsltMap == NULL )
		{
			xsltMap = new CacheManager< string, const XalanCompiledStylesheet* >();
			int setSpecificResult = pthread_setspecific( XSLTFilter::CompiledXSLTsKey, xsltMap );
			if ( 0 != setSpecificResult )
			{
				stringstream errorMessage;
				errorMessage << "Set thread specific CompiledXSLTsKey failed [" << setSpecificResult << "]";
				TRACE( errorMessage.str() );
				throw runtime_error( errorMessage.str() );
			}
		}

		if( xsltMap->Contains( filename ) )
		{
			DEBUG( "Returning compiled stylesheet [" << filename << "] from cache." );
			returnedXSLT = ( *xsltMap )[ filename ];
		}
		else
		{
			if ( 0 == getTransformer()->compileStylesheet( filename.c_str(), returnedXSLT ) ) // OK
			{
				DEBUG( "Inserting compiled stylesheet [" << filename << "] in cache." );
				xsltMap->Add( filename, returnedXSLT );
			}
			else
			{
				stringstream errorMessage;
				errorMessage << "Unable to compile stylesheet [" << filename << "] : [" << getTransformer()->getLastError() << "]";
				
				throw runtime_error( errorMessage.str() );
			}
		}
	}
	catch( ... )
	{
		TRACE( "Exception occured while getting XSLT [" << filename << "] from cache." );
		throw;
	}	
	return returnedXSLT;
}
