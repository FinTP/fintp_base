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
#include "ExtensionRegex.h"

#include <string>
#include <sstream>

#include <xalanc/Include/PlatformDefinitions.hpp>
#include <xercesc/util/PlatformUtils.hpp>
#include <xalanc/XalanTransformer/XalanTransformer.hpp>
#include <xalanc/XPath/XObjectFactory.hpp>

#undef S_NORMAL

#define BOOST_REGEX_MATCH_EXTRA
#include <boost/regex.hpp>

#include "XmlUtil.h"
#include "TimeUtil.h"

using namespace std;
using namespace FinTP;
XALAN_CPP_NAMESPACE_USE

map< string, vector< string > > FunctionRegex::m_GroupNames;
map< string, string > FunctionRegex::m_Cache;

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
XObjectPtr FunctionRegex::execute( XPathExecutionContext& executionContext, XalanNode* context, const XObjectArgVectorType& args, const LocatorType* locator ) const
{
	if ( args.size() != 2 )
	{
		stringstream errorMessage;
		errorMessage << "The regex function takes 1 arguments! [value,regex], but received " << args.size();
#if (_XALAN_VERSION >= 11100)
		executionContext.problem( XPathExecutionContext::eXPath, XPathExecutionContext::eError, XalanDOMString( errorMessage.str().c_str() ), locator, context); 
#else
		executionContext.error( XalanDOMString( errorMessage.str().c_str() ), context );
#endif
	}
	
	// Use the XObjectFactory createNumber() method to create an XObject 
	// corresponding to the XSLT number data type.

	string initValue = localForm( ( const XMLCh* )( args[0]->str().data() ) );
	string regex = localForm( ( const XMLCh* )( args[1]->str().data() ) );
	string returnValue = "";

	// cached value
	map< string, string >::const_iterator cacheFinder = m_Cache.find( regex );
	if ( cacheFinder == m_Cache.end() )
	{
		buildNodeNames( regex );
		//returnValue.append( "<!-- build by regex -->\n" );
		cacheFinder = m_Cache.find( regex );
	}
	/*else
		returnValue.append( "<!-- returned from cache -->\n" );*/

	// test current value
	boost::regex groupNamesRegex( cacheFinder->second );
	//boost::smatch groupsMatch;
	boost::smatch groupsMatch;

	bool match = boost::regex_search( initValue, groupsMatch, groupNamesRegex, boost::match_extra );

	// no match or match start position negative
	if ( !match )
		return executionContext.getXObjectFactory().createString( unicodeForm( "" ) );
				
	unsigned int groups = groupsMatch.size();
	if ( groups <= 1 )
		return executionContext.getXObjectFactory().createString( unicodeForm( "" ) );
	
	// add all nodes to the tree
	//for( unsigned int i=0; i<groups; i++ )
	//{		
		//if ( i < m_GroupNames[ regex ].size() )
		//{
			//DEBUG( "Match #" << i << " [" << m_GroupNames[ i - 1 ] << "] = [" << foundValue << "] size : " << foundValue.length() );
			// append an attribute to the node
			//stringstream element;
			//element << "<" << m_GroupNames[ regex ][ i ] << ">" << groupsMatch.str( i + 1 ) << "</" << m_GroupNames[ regex ][ i ] << ">";
			//returnValue.append( element.str() );

		//}
	//}

	returnValue = groupsMatch.str( 1 );
	return executionContext.getXObjectFactory().createString( unicodeForm( returnValue ) );
}

void FunctionRegex::buildNodeNames( const string& regex )
{
	int length = 0;
	bool match = true;
	string newValue = regex;
	string groupRegexStr = "\\((\\?<\\w+>)";

	// extract group names (?<...>
	boost::regex groupNamesRegex( groupRegexStr );
	boost::smatch groupsMatch;

	( void )m_GroupNames.insert( pair< string, vector< string > >( regex, vector< string >() ) );

	while ( match )
	{		
		int matchStartPos = -1, matchEndPos = -1;

		match = boost::regex_search( newValue, groupsMatch, groupNamesRegex, boost::match_extra );
		
		matchStartPos = groupsMatch.position( 1 );
		matchEndPos = matchStartPos + groupsMatch.length( 1 );
	
		if ( !match || ( matchStartPos < 0 ) ) 
			break;
		
		length = matchEndPos - matchStartPos;
				
		// skip ?< in the returned value, go up to >
		m_GroupNames[ regex ].push_back( newValue.substr( matchStartPos + 2, length - 3 ) );
		
		newValue = newValue.erase( matchStartPos, length );
		
		//DEBUG( "Match #" << i++ << " " << m_GroupNames.back() << " in [" << m_Value << "]" );			
		//DEBUG_STREAM( "New value : [" ) << newValue << endl;
	}
	( void )m_Cache.insert( pair< string, string >( regex, newValue ) );
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
	FunctionRegex*
#endif
FunctionRegex::clone( MemoryManagerType& theManager ) const
{
    return XalanCopyConstruct(theManager, *this);
}

const XalanDOMString& FunctionRegex::getError( XalanDOMString& theResult ) const
{
	theResult.assign( "The regex function accepts only 2 arg [value,regex]" );
	return theResult;
}
#else //XALAN_1_9
#if defined( XALAN_NO_COVARIANT_RETURN_TYPE )
	Function*
#else
	FunctionRegex*
#endif
FunctionRegex::clone() const
{
	return new FunctionRegex( *this );
}

const XalanDOMString FunctionRegex::getError() const
{
	return StaticStringToDOMString( XALAN_STATIC_UCODE_STRING( "The regex function accepts only 2 arg [value,regex]" ) );
}
#endif //XALAN_1_9

//RegexMatch implementation
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
XObjectPtr FunctionRegexMatch::execute( XPathExecutionContext& executionContext, XalanNode* context, const XObjectArgVectorType& args, const LocatorType* locator ) const
{
	if ( args.size() != 2 )
	{
		stringstream errorMessage;
		errorMessage << "The regex-match function takes 2 arguments! [value,regex], but received " << args.size();
#if (_XALAN_VERSION >= 11100)
		executionContext.problem( XPathExecutionContext::eXPath, XPathExecutionContext::eError, XalanDOMString( errorMessage.str().c_str() ), locator, context); 
#else
		executionContext.error( XalanDOMString( errorMessage.str().c_str() ), context );
#endif
	}
	
	// Use the XObjectFactory createNumber() method to create an XObject 
	// corresponding to the XSLT number data type.

	string initValue = localForm( ( const XMLCh* )( args[0]->str().data() ) );
	string regex = localForm( ( const XMLCh* )( args[1]->str().data() ) );
	
	// test current value
	boost::regex groupNamesRegex( regex );
	//boost::smatch groupsMatch;
	boost::smatch groupsMatch;

	bool match = boost::regex_search( initValue, groupsMatch, groupNamesRegex, boost::match_extra );

	// no match or match start position negative
	return executionContext.getXObjectFactory().createBoolean( match );
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
	FunctionRegexMatch*
#endif
FunctionRegexMatch::clone( MemoryManagerType& theManager ) const
{
    return XalanCopyConstruct( theManager, *this );
}

const XalanDOMString& FunctionRegexMatch::getError( XalanDOMString& theResult ) const
{
	theResult.assign( "The regex-match function accepts only 2 arg [value,regex]" );
	return theResult;
}
#else //XALAN_1_9
#if defined( XALAN_NO_COVARIANT_RETURN_TYPE )
	Function*
#else
	FunctionRegexMatch*
#endif
FunctionRegexMatch::clone() const
{
	return new FunctionRegexMatch( *this );
}

const XalanDOMString FunctionRegexMatch::getError() const
{
	return StaticStringToDOMString( XALAN_STATIC_UCODE_STRING( "The regex-match function accepts only 2 arg [value,regex]" ) );
}
#endif //XALAN_1_9
