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
#include "ExtensionAscii.h"

#include <string>
#include <sstream>
#include "XmlUtil.h"
#include "TimeUtil.h"

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
XObjectPtr FunctionAscii::execute( XPathExecutionContext& executionContext, XalanNode* context, const XObjectArgVectorType& args, const LocatorType* locator ) const
{
	if ( args.size() != 1 )
	{
		stringstream errorMessage;
		errorMessage << "The Ascii() function takes 1 arguments! [code], but received " << args.size();
#if (_XALAN_VERSION >= 11100)
		executionContext.problem( XPathExecutionContext::eXPath, XPathExecutionContext::eError, XalanDOMString( errorMessage.str().c_str() ), locator, context); 
#else
		executionContext.error( XalanDOMString( errorMessage.str().c_str() ), context );
#endif
	}
	
	// Use the XObjectFactory createNumber() method to create an XObject 
	// corresponding to the XSLT number data type.
	string replVal = string( 1, ( char )( args[0]->num() ) );

	return executionContext.getXObjectFactory().createString( unicodeForm( replVal ) );
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
	FunctionAscii*
#endif
FunctionAscii::clone( MemoryManagerType& theManager ) const
{
    return XalanCopyConstruct(theManager, *this);
}

const XalanDOMString& FunctionAscii::getError( XalanDOMString& theResult ) const
{
	theResult.assign( "The ascii function accepts only 1 arg [code]" );
	return theResult;
}
#else
#if defined( XALAN_NO_COVARIANT_RETURN_TYPE )
	Function*
#else
	FunctionAscii*
#endif
FunctionAscii::clone() const
{
	return new FunctionAscii( *this );
}

const XalanDOMString FunctionAscii::getError() const
{
	return StaticStringToDOMString( XALAN_STATIC_UCODE_STRING( "The ascii function accepts only 1 arg [code]" ) );
}
#endif
