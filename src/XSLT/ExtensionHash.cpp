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
#include "ExtensionHash.h"

#include <string>
#include <sstream>
#include "XmlUtil.h"

//#include <boost/crc.hpp>
//#include <boost/cstdint.hpp>
#include "../SSL/MD5.h"

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
XObjectPtr FunctionHash::execute( XPathExecutionContext& executionContext, XalanNode* context, const XObjectArgVectorType& args, const LocatorType* locator ) const
{
	if ( args.size() != 1 )
	{
		stringstream errorMessage;
		errorMessage << "The Hash() function takes one argument! [ stringToCRC ], but received " << args.size();
#if (_XALAN_VERSION >= 11100)
		executionContext.problem( XPathExecutionContext::eXPath, XPathExecutionContext::eError, XalanDOMString( errorMessage.str().c_str() ), locator, context); 
#else
		executionContext.error( XalanDOMString( errorMessage.str().c_str() ), context );
#endif
	}

	string stringToCalculateCRC = localForm( ( const XMLCh* )( args[0]->str().data() ) );

	//DEBUG( "Expression to calculate crc for : [" << stringToCalculateCRC << "]" );
	stringstream messageHash;

	/*MD5 md5Value;
	md5Value.update( ( unsigned char* )&stringToCalculateCRC , stringToCalculateCRC.length() );
	md5Value.finalize ();
*/
	//DEBUG( "MD5 = [" << md5Value.hex_digest() << "]" );

	//return executionContext.getXObjectFactory().createString( unicodeForm( md5Value.hex_digest() ) );
	return executionContext.getXObjectFactory().createString( unicodeForm( md5( stringToCalculateCRC ).c_str() ) );
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
	FunctionHash*
#endif
FunctionHash::clone( MemoryManagerType& theManager ) const
{
    return XalanCopyConstruct(theManager, *this);
}

const XalanDOMString& FunctionHash::getError( XalanDOMString& theResult ) const
{
	theResult.assign( "The hash function accepts only 1 argument [ stringToCRC ]" );
	return theResult;
}
#else
#if defined( XALAN_NO_COVARIANT_RETURN_TYPE )
	Function*
#else
	FunctionHash*
#endif
FunctionHash::clone() const
{
	return new FunctionHash( *this );
}

const XalanDOMString FunctionHash::getError() const
{
	return StaticStringToDOMString( XALAN_STATIC_UCODE_STRING( "The hash function accepts only 1 argument [ stringToCRC ]" ) );
}
#endif
