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
#include "ExtensionNewLine.h"

#include <string>
#include "XmlUtil.h"
#include "StringUtil.h"

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
XObjectPtr FunctionReplace::execute( XPathExecutionContext& executionContext, XalanNode* context, const XObjectArgVectorType& args, const LocatorType* locator ) const
{
	if ( args.size() != 3 )
	{
#ifdef XALAN_1_9
		XPathExecutionContext::GetAndReleaseCachedString theGuard( executionContext );
#if (_XALAN_VERSION >= 11100)
		executionContext.problem( XPathExecutionContext::eXPath, XPathExecutionContext::eError, getError( theGuard.get() ), locator, context); 
#else
		executionContext.error( getError( theGuard.get() ), context, locator );
#endif
#else
		executionContext.error( getError(), context );
#endif
	}
	
	// Use the XObjectFactory createNumber() method to create an XObject 
	// corresponding to the XSLT number data type.
#ifdef NDEBUG_HXA
	string inputValue = localForm( (  const XMLCh* )( args[0]->str().data() ) );
	string what = localForm( (  const XMLCh* )( args[1]->str().data() ) );
	string with = localForm( (  const XMLCh* )( args[2]->str().data() ) );
	string::size_type off = inputValue.find( what );
	
	string replVal = inputValue;
	while( off != string::npos )
	{
		replVal = replVal.replace( off, what.size(), with, 0, with.size() );
		off = replVal.find( what, off + with.size() );
	}
#else
	string replVal = StringUtil::Replace( localForm( ( XMLCh* )( args[0]->str().data() ) ),
		localForm( ( XMLCh* )( args[1]->str().data() ) ), localForm( ( XMLCh* )( args[2]->str().data() ) ) );
#endif

	return executionContext.getXObjectFactory().createString( unicodeForm( replVal ) );
}

/**
* Implement clone() so Xalan can copy the square-root function into
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
	FunctionReplace*
#endif
FunctionReplace::clone( MemoryManagerType& theManager ) const
{
    return XalanCopyConstruct(theManager, *this);
}

const XalanDOMString& FunctionReplace::getError( XalanDOMString& theResult ) const
{
	theResult.assign( "The replace function accepts only 3 args." );
	return theResult;
}
#else
#if defined( XALAN_NO_COVARIANT_RETURN_TYPE )
	Function*
#else
	FunctionReplace*
#endif
FunctionReplace::clone() const
{
	return new FunctionReplace( *this );
}

const XalanDOMString FunctionReplace::getError() const
{
	return StaticStringToDOMString( XALAN_STATIC_UCODE_STRING( "The replace function accepts only 3 args." ) );
}
#endif
