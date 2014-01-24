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
#include <sstream>
#include <typeinfo>

#if !defined( __GNUC__ )
	#include <strstream>
#endif

#ifdef WIN32
	#ifdef _DEBUG
		#include <stdlib.h>
		#include <crtdbg.h>
	#endif

	#define __MSXML_LIBRARY_DEFINED__
	#include <windows.h>
//	#define sleep(x) Sleep( (x)*1000 )

#else
	#include <unistd.h>
#endif

#include "XPathHelper.h"
#include "Trace.h"
#include "PlatformDeps.h"
#include "XmlUtil.h"

#include <xercesc/sax/SAXException.hpp>

#include <xalanc/XPath/XPathParserException.hpp>
#include <xalanc/XalanDOM/XalanDOMException.hpp>
#include <xalanc/PlatformSupport/XalanStdOutputStream.hpp>
#include <xalanc/PlatformSupport/XalanOutputStreamPrintWriter.hpp>
#include <xalanc/PlatformSupport/DOMStringPrintWriter.hpp>
#include <xalanc/PlatformSupport/XalanSimplePrefixResolver.hpp>
#include <xalanc/XMLSupport/FormatterToXML.hpp>
#include <xalanc/XMLSupport/FormatterTreeWalker.hpp>

using namespace FinTP;

//XPathHelper implementation
XPathHelper::XPathHelper()
{
	/*DEBUG( "Creating static instance" );
	// init platform
	//TODO remove this from every occ
	try
	{
		//XALAN_USING_XERCES( XMLPlatformUtils )
		XALAN_USING_XALAN( XPathEvaluator )

		// force xmlutil instance to be created ( initialize xmlplatform )
		//XmlUtil::getInstance();

		//XPathEvaluator::initialize();
	}
	catch( const exception ex )
	{
		TRACE( "Error initializing Xalan : " << ex.what() );
		//throw runtime_error( "Unknown exception while initializing Xalan" );
	}
	catch( ... )
	{
		TRACE( "Unknown exception while initializing Xalan" );
		//throw runtime_error( "Unknown exception while initializing Xalan" );
	}

	DEBUG( "Static instance created" );*/
}

XPathHelper::~XPathHelper()
{
	/*DEBUG( "Cleaning up Xalan platform" );

	XALAN_USING_XALAN( XPathEvaluator )

	try
	{
		XPathEvaluator::terminate();
	}
	catch( ... ){}
	*/
}

//typedef XALAN_CPP_NAMESPACE_QUALIFIER CharVectorType CVT;
/*#ifdef WIN32
	#pragma check_stack(off)
	#pragma runtime_checks( "", off )
#endif*/

XALAN_CPP_NAMESPACE_QUALIFIER NodeRefList const XPathHelper::EvaluateNodes( const string& xPath, XALAN_CPP_NAMESPACE_QUALIFIER XalanDocument* doc, const string& defaultPrefix )
{
	XALAN_USING_XALAN( XPathEvaluator )
	XALAN_USING_XALAN( XalanNode )
	XALAN_USING_XALAN( NodeRefList )
	XALAN_USING_XALAN( XalanDOMString )

	NodeRefList nodeset;

	XalanDOMString simplePRPrefix = XalanDOMString( "x" );
	XalanDOMString simplePRNSUri = XalanDOMString( defaultPrefix.c_str() );

	XPathEvaluator m_Evaluator;

	{
		XALAN_USING_XALAN( XObjectPtr )
		XALAN_USING_XALAN( PrefixResolver )
		XALAN_USING_XALAN( XalanSimplePrefixResolver )
		XALAN_USING_XALAN( XalanDocumentPrefixResolver )
		//XALAN_USING_XALAN( XPathEvaluator )

		DEBUG_GLOBAL( "Evaluating [" << xPath << "] ..." );
		if ( doc == NULL )
		{
			TRACE_GLOBAL( "Empty document received. Can't evaluate XPATH." );
			throw runtime_error( "Empty document received. Can't evaluate XPATH." );
		}

		PrefixResolver* thePrefixResolver = NULL;
		if ( defaultPrefix.length() > 0 )
		{
			DEBUG_GLOBAL( "Using simple prefix resolver : Prefix [x], NamespaceURI [" << defaultPrefix << "], URI []" );
			thePrefixResolver = new XalanSimplePrefixResolver( simplePRPrefix, simplePRNSUri, simplePRNSUri );
		}
		else
		{
			thePrefixResolver = new XalanDocumentPrefixResolver( doc );
		}

		unsigned char dummyXalanWorkaround[ 100 ];

		try
		{
			XALAN_USING_XALAN( XalanSourceTreeDOMSupport )
			XALAN_USING_XALAN( XalanSourceTreeParserLiaison )
			XALAN_USING_XALAN( XObjectPtr )

			XalanSourceTreeDOMSupport m_DOMSupport;
			XalanSourceTreeParserLiaison m_Liaison( m_DOMSupport );
			m_DOMSupport.setParserLiaison( &m_Liaison );

			DEBUG_GLOBAL( "Selecting a single node" );
			// OK, let's find the context node...
			XalanNode *const theContextNode = m_Evaluator.selectSingleNode( m_DOMSupport,
				doc, XalanDOMString( "/" ).c_str(), *thePrefixResolver );

			if ( theContextNode == NULL )
			{
				DEBUG_GLOBAL( "Context node [/] was not found" );

				if ( thePrefixResolver != NULL )
				{
					delete thePrefixResolver;
					thePrefixResolver = NULL;
				}

				return nodeset;
			}

			DEBUG_GLOBAL( "Context node found. Namespace URI is [" << theContextNode->getNamespaceURI()
				<< "]; prefix is [" << theContextNode->getPrefix() << "]; local name is [" << theContextNode->getLocalName() << "]" );

			const XObjectPtr theResult( m_Evaluator.evaluate( m_DOMSupport,
				theContextNode, XalanDOMString( xPath.data() ).c_str(), *thePrefixResolver ) );

			if ( thePrefixResolver != NULL )
			{
				delete thePrefixResolver;
				thePrefixResolver = NULL;
			}

			DEBUG_GLOBAL( "Result read." );

			if ( theResult.null() == true )
			{
				TRACE_GLOBAL( "Null result returned for xpath [" << xPath << "]" );

				return nodeset;
			}

			DEBUG_GLOBAL( "Result is available : [" << theResult->str() << "]" );

			nodeset = ( XALAN_CPP_NAMESPACE_QUALIFIER NodeRefList )theResult->nodeset();
			const XALAN_CPP_NAMESPACE_QUALIFIER NodeRefList::size_type theLength = nodeset.getLength();
			DEBUG_GLOBAL( "Node list contains [" << theLength << "] nodes" );

			DEBUG_GLOBAL( "Done" );
		}
		catch( const XALAN_CPP_NAMESPACE_QUALIFIER XalanDOMException& ex )
		{
			stringstream errorMessage;
			errorMessage << "DOM Exception received while evaluating xpath. Code is : " << ex.getExceptionCode();
			TRACE_GLOBAL( errorMessage.str() );

			if ( thePrefixResolver != NULL )
			{
				delete thePrefixResolver;
				thePrefixResolver = NULL;
			}

			throw runtime_error( errorMessage.str() );
		}
		catch( const XALAN_CPP_NAMESPACE_QUALIFIER XPathParserException & xex )
		{
			stringstream errorMessage;
			errorMessage << "XPath Exception reading xpath from context node : " <<
				xex.getMessage() << " at " << xex.getLineNumber() << ", " << xex.getColumnNumber();
			TRACE_GLOBAL( errorMessage.str() );

			if ( thePrefixResolver != NULL )
			{
				delete thePrefixResolver;
				thePrefixResolver = NULL;
			}

			throw runtime_error( errorMessage.str() );
		}
		catch( const XALAN_CPP_NAMESPACE_QUALIFIER XSLException& xex )
		{
			stringstream errorMessage;
			errorMessage << "XSL Exception reading xpath from context node : " <<
				xex.getMessage() << " at " << xex.getLineNumber() << ", " << xex.getColumnNumber();
			TRACE_GLOBAL( errorMessage.str() );

			if ( thePrefixResolver != NULL )
			{
				delete thePrefixResolver;
				thePrefixResolver = NULL;
			}

			throw runtime_error( errorMessage.str() );
		}
		catch( const std::exception& ex )
		{
			if ( thePrefixResolver != NULL )
			{
				delete thePrefixResolver;
				thePrefixResolver = NULL;
			}

			TRACE_GLOBAL( "Exception reading xpath from context node : " <<
				typeid( ex ).name() << " [" << ex.what() << "]" );

			throw;
		}
		catch( ... )
		{
			if ( thePrefixResolver != NULL )
			{
				delete thePrefixResolver;
				thePrefixResolver = NULL;
			}

			TRACE_GLOBAL( "Unexpected error reading xpath from context node." );

			throw runtime_error( "Unexpected error evaluating xpath from context node" );
		}
	}

	return nodeset;
}

XALAN_CPP_NAMESPACE_QUALIFIER XalanNode* const XPathHelper::Evaluate( const string& xPath, XALAN_CPP_NAMESPACE_QUALIFIER XalanDocument* doc, const string& defaultPrefix )
{
	XALAN_USING_XALAN( XPathEvaluator )
	XALAN_USING_XALAN( XalanNode )
	XALAN_USING_XALAN( XalanDOMString )

	XalanNode *crtNode = NULL;
	XPathEvaluator m_Evaluator;

	XalanDOMString simplePRPrefix = XalanDOMString( "x" );
	XalanDOMString simplePRNSUri = XalanDOMString( defaultPrefix.c_str() );

	{
		XALAN_USING_XALAN( XObjectPtr )
		XALAN_USING_XALAN( PrefixResolver )
		XALAN_USING_XALAN( XalanSimplePrefixResolver )
		XALAN_USING_XALAN( XalanDocumentPrefixResolver )
		//XALAN_USING_XALAN( XPathEvaluator )

		DEBUG_GLOBAL( "Evaluating [" << xPath << "] ..." );
		if ( doc == NULL )
		{
			TRACE_GLOBAL( "Empty document received. Can't evaluate XPATH." );
			throw runtime_error( "Empty document received. Can't evaluate XPATH." );
		}

		PrefixResolver* thePrefixResolver = NULL;
		if ( defaultPrefix.length() > 0 )
		{
			DEBUG_GLOBAL( "Using simple prefix resolver : Prefix [x], NamespaceURI [" << defaultPrefix << "], URI []" );
			thePrefixResolver = new XalanSimplePrefixResolver( simplePRPrefix, simplePRNSUri, simplePRNSUri );
		}
		else
		{
			thePrefixResolver = new XalanDocumentPrefixResolver( doc );
		}

		unsigned char dummyXalanWorkaround[ 100 ];

		try
		{
			XALAN_USING_XALAN( XalanSourceTreeDOMSupport )
			XALAN_USING_XALAN( XalanSourceTreeParserLiaison )
			XALAN_USING_XALAN( NodeRefList )
			XALAN_USING_XALAN( XObjectPtr )

			XalanSourceTreeDOMSupport m_DOMSupport;
			XalanSourceTreeParserLiaison m_Liaison( m_DOMSupport );
			m_DOMSupport.setParserLiaison( &m_Liaison );

			DEBUG2( "Selecting a single node" );
			// OK, let's find the context node...
			XalanNode *const theContextNode = m_Evaluator.selectSingleNode( m_DOMSupport,
				doc, XALAN_CPP_NAMESPACE_QUALIFIER XalanDOMString( "/" ).c_str(), *thePrefixResolver );

			if ( theContextNode == NULL )
			{
				DEBUG_GLOBAL( "Context node [/] was not found" );

				if ( thePrefixResolver != NULL )
				{
					delete thePrefixResolver;
					thePrefixResolver = NULL;
				}

				return NULL;
			}

			DEBUG2( "Context node found." );

			const XObjectPtr theResult( m_Evaluator.evaluate( m_DOMSupport,
				theContextNode, XALAN_CPP_NAMESPACE_QUALIFIER XalanDOMString( xPath.data() ).c_str(), *thePrefixResolver ) );

			if ( thePrefixResolver != NULL )
			{
				delete thePrefixResolver;
				thePrefixResolver = NULL;
			}

			DEBUG2( "Result read." );

			if ( theResult.null() == true )
			{
				TRACE_GLOBAL( "Null result returned for xpath [" << xPath << "]" );

				return NULL;
			}

			DEBUG2( "Result is available : [" << theResult->str() << "]" );

			NodeRefList nodeset = ( XALAN_CPP_NAMESPACE_QUALIFIER NodeRefList )theResult->nodeset();
			const XALAN_CPP_NAMESPACE_QUALIFIER NodeRefList::size_type theLength = nodeset.getLength();
			DEBUG2( "Node list contains [" << theLength << "] nodes" );

			// alloc result string
			//DEBUG_GLOBAL_STREAM( "Result length : " ) << theResult->stringLength();
			if ( theLength == 0 ) //theResult->stringLength() == 0 )
			{
				return NULL;
			}

			DEBUG2( "Nonempty result" );

			crtNode = nodeset.item( 0 );
			if ( crtNode == NULL ) //theResult->stringLength() == 0 )
			{
				return NULL;
			}

			DEBUG2( "Done" );
		}
		catch( const XALAN_CPP_NAMESPACE_QUALIFIER XalanDOMException& ex )
		{
			stringstream errorMessage;
			errorMessage << "DOM Exception received while evaluating xpath. Code is : " << ex.getExceptionCode();
			TRACE_GLOBAL( errorMessage.str() );

			if ( thePrefixResolver != NULL )
			{
				delete thePrefixResolver;
				thePrefixResolver = NULL;
			}

			throw runtime_error( errorMessage.str() );
		}
		catch( const XALAN_CPP_NAMESPACE_QUALIFIER XPathParserException & xex )
		{
			stringstream errorMessage;
			errorMessage << "XPath Exception reading xpath from context node : " <<
				xex.getMessage() << " at " << xex.getLineNumber() << ", " << xex.getColumnNumber();
			TRACE_GLOBAL( errorMessage.str() );

			if ( thePrefixResolver != NULL )
			{
				delete thePrefixResolver;
				thePrefixResolver = NULL;
			}

			throw runtime_error( errorMessage.str() );
		}
		catch( const XALAN_CPP_NAMESPACE_QUALIFIER XSLException& xex )
		{
			stringstream errorMessage;
			errorMessage << "XSL Exception reading xpath from context node : " <<
				xex.getMessage() << " at " << xex.getLineNumber() << ", " << xex.getColumnNumber();
			TRACE_GLOBAL( errorMessage.str() );

			if ( thePrefixResolver != NULL )
			{
				delete thePrefixResolver;
				thePrefixResolver = NULL;
			}

			throw runtime_error( errorMessage.str() );
		}
		catch( const std::exception& ex )
		{
			if ( thePrefixResolver != NULL )
			{
				delete thePrefixResolver;
				thePrefixResolver = NULL;
			}

			TRACE_GLOBAL( "Exception reading xpath from context node : " <<
				typeid( ex ).name() << " [" << ex.what() << "]" );
			throw;
		}
		catch( ... )
		{
			if ( thePrefixResolver != NULL )
			{
				delete thePrefixResolver;
				thePrefixResolver = NULL;
			}

			TRACE_GLOBAL( "Unexpected error reading xpath from context node." );
			throw runtime_error( "Unexpected error evaluating xpath from context node" );
		}
	}

	return crtNode;
}
/*#ifdef WIN32
	#pragma check_stack(on)
	#pragma runtime_checks( "", restore )
#endif*/

string XPathHelper::SerializeToString( XALAN_CPP_NAMESPACE_QUALIFIER XalanDOMString xString )
{
	XALAN_USING_XALAN( XalanDOMString );
	string retString = "";

	//XALAN_USING_XALAN( CharVectorType );
	//CharVectorType resultTranscode;

	try
	{
		/*xString.transcode( resultTranscode );
		retString.reserve( resultTranscode.size() );

		for( unsigned int i=0; i<resultTranscode.size(); i++ )
		{
			retString.append( 1, resultTranscode[ i ] );
		}*/
		for( unsigned int i=0; i<xString.size(); i++ )
		{
			/*if ( xString[ i ] > 0xFF )
			{
				TRACE( "Warning : unicode char overflow [" << ( unsigned int )( xString[ i ] ) << "]" );
				retString.append( 1, xString[ i ] & 0xFF );
				retString.append( 1, ( xString[ i ] & 0xFF00 ) >> 8 );
			}
			else
				retString.append( 1, ( char )xString[ i ] );*/
			if ( xString[ i ] < 0x80 )
			{
    			retString.append( 1, xString[ i ] );
			}
			else if ( xString[ i ] < 0x800 )
			{
				//convert to XalanDOMChar ( XalanDOMString::value_type ), because the result on linux was of type int
    			retString.append( 1, ( XalanDOMString::value_type ) ( 0xC0 | ( xString[ i ] >> 6 ) ) );
    			retString.append( 1, ( XalanDOMString::value_type ) ( 0x80 | ( xString[ i ] & 0x3F ) ) );
  			}
  			//NOT used because XalanDOMString is a vector of XalanDOMChar and this is defined
  			//as unsigned short if XALAN_USE_NATIVE_WCHAR_T not defined as compile flags
  			//not sure if wchar_t is bigger than 0xFFFF
#ifdef XALAN_USE_NATIVE_WCHAR_T
			else if ( xString[ i ] < 0x10000 )
			{
			    retString.append( 1, ( 0xE0 | ( xString[ i ] >> 12 ) ) );
			    retString.append( 1, ( 0x80 | ( ( xString[ i ] >> 6 ) & 0x3F ) ) );
			    retString.append( 1, ( 0x80 | ( xString[ i ] & 0x3F ) ) );
			}
			else if ( xString[ i ] < 0x200000 )
			{
				retString.append( 1, ( 0xF0 | ( xString[ i ] >> 18 ) ) );
				retString.append( 1, ( 0x80 | ( ( xString[ i ] >> 12 ) & 0x3F ) ) );
				retString.append( 1, ( 0x80 | ( ( xString[ i ] >> 6  ) & 0x3F ) ) );
				retString.append( 1, ( 0x80 | ( xString[ i ] & 0x3F ) ) );
			}
#endif
		}
	}
	catch( const XalanDOMString::TranscodingError &ex )
	{
		stringstream errorMessage;
		errorMessage << "Error transcoding [to local code page]. Code is : " << ex.getExceptionCode();
		TRACE_GLOBAL( errorMessage.str() );

		throw runtime_error( errorMessage.str() );
	}

	return retString;
}

/*
string XPathHelper::SerializeToString( XALAN_CPP_NAMESPACE_QUALIFIER XalanNode* crtNode )
{
	XALAN_USING_XALAN( XalanDOMString );

	string retString = "";

	if ( crtNode == NULL )
	{
		DEBUG_GLOBAL( "Empty result returned." );
		return "";
	}

	//retString.reserve( theResult->stringLength() );
	XalanDOMString xString;
	switch( crtNode->getNodeType() )
	{
		case XALAN_CPP_NAMESPACE_QUALIFIER XalanNode::TEXT_NODE :
			DEBUG2( "TEXT_NODE" );

			xString = crtNode->getNodeValue();
			break;

		case XALAN_CPP_NAMESPACE_QUALIFIER XalanNode::ATTRIBUTE_NODE :
			DEBUG2( "ATTRIBUTE_NODE" );

			xString = crtNode->getNodeValue();
			break;

		case XALAN_CPP_NAMESPACE_QUALIFIER XalanNode::DOCUMENT_NODE :
		case XALAN_CPP_NAMESPACE_QUALIFIER XalanNode::ELEMENT_NODE :
			{
				DEBUG2( "ELEMENT_NODE" );

				try
				{
					XALAN_CPP_NAMESPACE_QUALIFIER DOMStringPrintWriter thePrintWriter( xString );
					XALAN_CPP_NAMESPACE_QUALIFIER FormatterToXML theFormatter( thePrintWriter );
					XALAN_CPP_NAMESPACE_QUALIFIER FormatterTreeWalker theWalker( theFormatter );

					//theFormatter.setOutputEncoding( XalanDOMString( "us-ascii" ) );
					theFormatter.setShouldWriteXMLHeader( false );

					// It's required that we do this...
					theFormatter.startDocument();

					// Traverse the subtree of the document rooted at
					// each node we've selected...
					theWalker.traverseSubtree( crtNode );

					theFormatter.endDocument();
				}
				catch( const XalanDOMString::TranscodingError &ex )
				{
					stringstream errorMessage;
					errorMessage << "Error transcoding [to local code page] received while serializing node. Code is : " << ex.getExceptionCode();
					TRACE_GLOBAL( errorMessage.str() );

					throw runtime_error( errorMessage.str() );
				}
				catch( const XALAN_CPP_NAMESPACE_QUALIFIER XalanDOMException& ex )
				{
					stringstream errorMessage;
					errorMessage << "DOM Exception received while serializing node. Code is : " << ex.getExceptionCode();
					TRACE_GLOBAL( errorMessage.str() );

					throw runtime_error( errorMessage.str() );
				}
				catch( const XERCES_CPP_NAMESPACE_QUALIFIER SAXException& ex )
				{
					stringstream errorMessage;
					errorMessage << "SAX Exception received while serializing node. Message : " << localForm( ex.getMessage() );
					TRACE_GLOBAL( errorMessage.str() );

					throw runtime_error( errorMessage.str() );
				}
				catch( ... )
				{
					// unfreeze buffer( after str ) in order to allow deallocation of the buffer
					TRACE_GLOBAL( "Unknown exception received while serializing node." );
					throw;
				}
				break;
			}

		default:
			TRACE( "Serialization to string of [" << crtNode->getNodeType() << "] type node is not implemented" );
			break;
	}

	DEBUG2( "Transcoding node value" );
	retString = SerializeToString( xString );

	//retString = string( ( char* ) xString.c_str() );
	DEBUG2( "Nonempty result saved." );

	//evaluator.terminate();

	DEBUG_GLOBAL( "Returning bytes [" << retString << "]" );
	//return string( returnBuf );
	return retString;
}*/

string XPathHelper::SerializeToString( XALAN_CPP_NAMESPACE_QUALIFIER XalanNode* crtNode )
{
	XALAN_USING_XALAN( XalanDOMString );

	string retString = "";

	if ( crtNode == NULL )
	{
		DEBUG_GLOBAL( "Empty result returned." );
		return "";
	}

	//retString.reserve( theResult->stringLength() );
	XalanDOMString xString;
	switch( crtNode->getNodeType() )
	{
		case XALAN_CPP_NAMESPACE_QUALIFIER XalanNode::TEXT_NODE :
			DEBUG2( "TEXT_NODE" );

			xString = crtNode->getNodeValue();
			DEBUG2( "Transcoding node value" );
			retString = SerializeToString( xString );

			break;

		case XALAN_CPP_NAMESPACE_QUALIFIER XalanNode::ATTRIBUTE_NODE :
			DEBUG2( "ATTRIBUTE_NODE" );

			xString = crtNode->getNodeValue();
			DEBUG2( "Transcoding node value" );
			retString = SerializeToString( xString );

			break;

		case XALAN_CPP_NAMESPACE_QUALIFIER XalanNode::DOCUMENT_NODE :
		case XALAN_CPP_NAMESPACE_QUALIFIER XalanNode::ELEMENT_NODE :
			{
				DEBUG2( "ELEMENT_NODE" );

#if defined ( __GNUC__ )
				stringstream theOutputStream;
#else
				ostrstream theOutputStream;
#endif

				//XALAN_CPP_NAMESPACE_QUALIFIER DOMStringPrintWriter thePrintWriter( xString );
				try
				{
					XALAN_CPP_NAMESPACE_QUALIFIER XalanStdOutputStream theStream( theOutputStream );
					XALAN_CPP_NAMESPACE_QUALIFIER XalanOutputStreamPrintWriter thePrintWriter( theStream );
					XALAN_CPP_NAMESPACE_QUALIFIER FormatterToXML theFormatter( thePrintWriter );
					XALAN_CPP_NAMESPACE_QUALIFIER FormatterTreeWalker theWalker( theFormatter );

					//theFormatter.setOutputEncoding( XalanDOMString( "us-ascii" ) );
					theFormatter.setShouldWriteXMLHeader( false );

					// It's required that we do this...
					theFormatter.startDocument();

					// Traverse the subtree of the document rooted at
					// each node we've selected...
					theWalker.traverseSubtree( crtNode );

					theFormatter.endDocument();

#if !defined ( __GNUC__ )
					retString = string( theOutputStream.str(), theOutputStream.pcount() );
					// unfreeze buffer( after str ) in order to allow deallocation of the buffer
					theOutputStream.rdbuf()->freeze( false );
#else
					retString = theOutputStream.str();
#endif
				}
				catch( const XalanDOMString::TranscodingError &ex )
				{
					stringstream errorMessage;
					errorMessage << "Error transcoding [to local code page] received while serializing node. Code is : " << ex.getExceptionCode();
					TRACE_GLOBAL( errorMessage.str() );

#if !defined ( __GNUC__ )
					theOutputStream.rdbuf()->freeze( false );
#endif
					throw runtime_error( errorMessage.str() );
				}
				catch( const XALAN_CPP_NAMESPACE_QUALIFIER XalanDOMException& ex )
				{
					stringstream errorMessage;
					errorMessage << "DOM Exception received while serializing node. Code is : " << ex.getExceptionCode();
					TRACE_GLOBAL( errorMessage.str() );

#if !defined ( __GNUC__ )
					theOutputStream.rdbuf()->freeze( false );
#endif
					throw runtime_error( errorMessage.str() );
				}
				catch( const XERCES_CPP_NAMESPACE_QUALIFIER SAXException& ex )
				{
					stringstream errorMessage;
					errorMessage << "SAX Exception received while serializing node. Message : " << localForm( ex.getMessage() );
					TRACE_GLOBAL( errorMessage.str() );

#if !defined ( __GNUC__ )
					theOutputStream.rdbuf()->freeze( false );
#endif
					throw runtime_error( errorMessage.str() );
				}
				catch( ... )
				{
#if !defined ( __GNUC__ )
					// unfreeze buffer( after str ) in order to allow deallocation of the buffer
					theOutputStream.rdbuf()->freeze( false );
#endif
					throw;
				}

				break;
			}

		default:
			TRACE( "Serialization to string of [" << crtNode->getNodeType() << "] type node is not implemented" );
			break;
	}

	//retString = string( ( char* ) xString.c_str() );
	DEBUG_GLOBAL( "Nonempty result serialized." );

	//evaluator.terminate();

	DEBUG2( "Returning bytes [" << retString << "]" );
	//return string( returnBuf );
	return retString;
}
