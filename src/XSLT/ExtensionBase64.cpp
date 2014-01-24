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

// Base header file.  Must be first.
#include <xalanc/Include/PlatformDefinitions.hpp>

#include <xercesc/util/PlatformUtils.hpp>
#include <xalanc/XalanTransformer/XalanTransformer.hpp>
#include <xalanc/XPath/XObjectFactory.hpp>
#include <xalanc/XPath/XPathParserException.hpp>
#include <xalanc/XalanDOM/XalanDOMException.hpp>
#include <xalanc/PlatformSupport/XalanStdOutputStream.hpp>
#include <xalanc/PlatformSupport/XalanOutputStreamPrintWriter.hpp>
#include <xalanc/PlatformSupport/DOMStringPrintWriter.hpp>
#include <xalanc/PlatformSupport/XalanSimplePrefixResolver.hpp>
#include <xalanc/XMLSupport/FormatterToXML.hpp>
#include <xalanc/XMLSupport/FormatterTreeWalker.hpp>
#include "ExtensionBase64.h"

#include <string>
#include <sstream>
#include <iostream>
//#include <strstream>
#include "XmlUtil.h"

//#include <boost/crc.hpp>
//#include <boost/cstdint.hpp>
#include "../XPathHelper.h"
#include "Base64.h"


using namespace std;
using namespace FinTP;
XALAN_CPP_NAMESPACE_USE

/**
* Execute an XPath function object.  The function must return a valid
* XObject.
*
* @param executionContext executing context
* @param context          current context node
* @param opPos            current op position
* @param args             vector of pointers to XObject arguments
* @return                 pointer to the result XObject
*/
XObjectPtr FunctionBase64::execute( XPathExecutionContext& executionContext, XalanNode* context, const XObjectArgVectorType& args, const LocatorType* locator ) const
{
	XALAN_USING_XALAN( XalanDOMString );

	if ( args.size() != 3 )
	{
		stringstream errorMessage;
		errorMessage << "The Base64() function takes 3 arguments [ nodeset, trailer, envelope ], but received " << args.size();
#if (_XALAN_VERSION >= 11100)
		executionContext.problem( XPathExecutionContext::eXPath, XPathExecutionContext::eError, XalanDOMString( errorMessage.str().c_str() ), locator, context); 
#else
		executionContext.error( XalanDOMString( errorMessage.str().c_str() ), context );
#endif
	}

	stringstream stringToEncode;
	string envelopeName = localForm( ( const XMLCh* )( args[ 2 ]->str().data() ) );

	stringToEncode << "<" << envelopeName << ">";
	for( unsigned int i=0; i<args[ 0 ]->nodeset().getLength(); i++ )
	{
		stringToEncode << XPathHelper::SerializeToString( args[ 0 ]->nodeset().item( i ) );
	}
	stringToEncode << localForm( ( const XMLCh* )( args[ 1 ]->str().data() ) ) << "</" << envelopeName << ">";

	string encodedString = Base64::encode( stringToEncode.str() ); 

	return executionContext.getXObjectFactory().createString( unicodeForm( encodedString ) );
}

/**
* Implement clone() so Xalan can copy the function into
* its own function table.
*
* @return pointer to the new object
*/
// For compilers that do not support covariant return types,
// clone() must be declared to return the base type.
#ifdef XALAN_1_9
#if defined( XALAN_NO_COVARIANT_RETURN_TYPE )
	Function*
#else
	FunctionBase64*
#endif
FunctionBase64::clone( MemoryManagerType& theManager ) const
{
    return XalanCopyConstruct(theManager, *this);
}

const XalanDOMString& FunctionBase64::getError( XalanDOMString& theResult ) const
{
	theResult.assign( "The base64 function accepts 3 arguments [ nodeset, trailer, envelope ]" );
	return theResult;
}
#else
#if defined( XALAN_NO_COVARIANT_RETURN_TYPE )
	Function*
#else
	FunctionBase64*
#endif
FunctionBase64::clone() const
{
	return new FunctionBase64( *this );
}

const XalanDOMString FunctionBase64::getError() const
{
	return StaticStringToDOMString( XALAN_STATIC_UCODE_STRING( "The base64 function accepts only 3 arguments [ nodeset, trailer, envelope ]" ) );
}
#endif
