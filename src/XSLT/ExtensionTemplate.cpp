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
#include "ExtensionTemplate.h"

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
XObjectPtr FunctionFill::execute( XPathExecutionContext& executionContext, XalanNode* context, const XObjectArgVectorType& args, const LocatorType* locator ) const
{
	if ( args.size() != 4 )
	{
		//DEBUG( "No. of arguments specified : " << args.size() );
		return executionContext.getXObjectFactory().createString( unicodeForm( "bad" ) );

		//executionContext.error( "Invalid number of arguments : the fill() function takes four arguments!  ( value, how many chars in the result, fill char, fill type )", context );
	}
	
	// Use the XObjectFactory createNumber() method to create an XObject 
	// corresponding to the XSLT number data type.
	
	string::size_type replacedSize = ( string::size_type )( args[1]->num() );
	
	// create a string of the requested size, filled with 'fill char'
	string result( replacedSize, localForm( ( const XMLCh* )( args[2]->str().data() ) )[ 0 ] );
	
	// original value 
	string value = localForm( ( const XMLCh* )( args[0]->str().data() ) );
	
	int fillType = ( int )( args[3]->num() );
	
	string::size_type originalSize = value.length();
	string replVal;
	
	// fill right/left -> truncate value
	if( fillType < 2 )
	{
		if ( originalSize > replacedSize )
		{
			value = value.substr( 0, replacedSize );
			originalSize = replacedSize;
		}
		
		//DEBUG( "Replacement [" << ( fillType ? "left" : "right" ) << "] string set to " << result.length() << " blanks" );
		switch( fillType )
		{
			case 0 :
				replVal = result.replace( 0, originalSize, value );
				break;
				
			case 1 :
				replVal = result.replace( replacedSize - originalSize, originalSize, value );
				break;
		}
	}
	else // fill multiple of replacedSize
	{
		int remainder = replacedSize - ( originalSize % replacedSize );
		if ( remainder == replacedSize )
			remainder = 0;
			
		//DEBUG( "Need to append " << remainder << " blanks" );
		string strReminder( remainder, ' ' );		
				
		replVal = result.append( strReminder );
	}
	
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
	FunctionFill*
#endif
FunctionFill::clone( MemoryManagerType& theManager ) const
{
    return XalanCopyConstruct(theManager, *this);
}

const XalanDOMString& FunctionFill::getError( XalanDOMString& theResult ) const
{
	theResult.assign( "The fill function accepts only 4 args. ( value, how many chars in the result, fill char, fill type )" );
	return theResult;
}
#else
#if defined( XALAN_NO_COVARIANT_RETURN_TYPE )
	Function*
#else
	FunctionFill*
#endif
FunctionFill::clone() const
{
	return new FunctionFill( *this );
}

const XalanDOMString FunctionFill::getError() const
{
	return StaticStringToDOMString( XALAN_STATIC_UCODE_STRING( "The fill function accepts only 4 args. ( value, how many chars in the result, fill char, fill type )" ) );
}
#endif
