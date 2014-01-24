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

#if ( defined( WIN32 ) && ( _MSC_VER <= 1400 ) )
	#ifdef _HAS_ITERATOR_DEBUGGING
		#undef _HAS_ITERATOR_DEBUGGING
	#endif
	#define _HAS_ITERATOR_DEBUGGING 0
#endif

#include "Trace.h"
#include "DataSet.h"
#include "StringUtil.h"

#include "RoutingKeyword.h"
//#include "RoutingExceptions.h"

#undef S_NORMAL

#define BOOST_REGEX_MATCH_EXTRA
#include <boost/regex.hpp>

using namespace FinTP;

pthread_once_t NotFoundKeywordMappings::NotFoundKeysCreate = PTHREAD_ONCE_INIT;
pthread_key_t NotFoundKeywordMappings::NotFoundKey;

RoutingKeyword::EVALUATOR_TYPE RoutingKeyword::parseType( const string& type )
{
	string typeStr = StringUtil::ToUpper( type );

	if( typeStr == "STRING" )
		return RoutingKeyword::STRING;
	if( typeStr == "DATE" )
		return RoutingKeyword::DATE;
	if( typeStr == "CURRENCY" )
		return RoutingKeyword::CURRENCY;
	
	stringstream errorMessage;
	errorMessage << "Invalid field type : [" << type << "]";
	throw logic_error( errorMessage.str() );
}

void RoutingKeyword::Dump()
{
	DEBUG( "Keyword [" << m_Name << "] has regex [" << m_Regex << "] and regex iso [" << m_RegexIso << "]" );

	vector< pair< string, EVALUATOR_TYPE > >::const_iterator fieldWalker = m_Fields.begin();
	while( fieldWalker != m_Fields.end() )
	{
		DEBUG( "\tField name : [" << fieldWalker->first << "] of type [" << fieldWalker->second << "]" );
		fieldWalker++;
	}

	fieldWalker = m_FieldsIso.begin();
	while( fieldWalker != m_FieldsIso.end() )
	{
		DEBUG( "\tISO field name : [" << fieldWalker->first << "] of type [" << fieldWalker->second << "]" );
		fieldWalker++;
	}
}

//RoutingKeyword implementation

RoutingKeyword::RoutingKeyword() : m_Regex( "" ), m_RegexIso( "" ), m_Name( "" )
{}

RoutingKeyword::~RoutingKeyword()
{}

RoutingKeyword::RoutingKeyword( const string& name, const string& comparer, const string& regex, const string& regexIso )
{
	DEBUG_GLOBAL( "Constructor [" << name << "] comparer [" << comparer << "], regex [" << regex << "], regex iso [" << regexIso << "]" );
	
	m_Name = name;
	string groupRegexStr = "\\((\\?<\\w+>)";

	// extract group names \(\?<(...)>
	
	boost::regex groupNamesRegex( groupRegexStr );//, boost::regex_constants::char_classes );
	boost::smatch groupsMatch;

	// Evaluate SMT field

	// split by ,
	StringUtil comparerSplit( comparer );
	comparerSplit.Split( "," );

	bool match = true;
	string newValue = regex;
	int length = 0;
	
	while ( match )
	{
		int matchStartPos = -1, matchEndPos = -1;

		match = boost::regex_search( newValue, groupsMatch, groupNamesRegex, boost::match_extra );
			
		matchStartPos = groupsMatch.position( 1 );
		matchEndPos = matchStartPos + groupsMatch.length( 1 );
		
		if ( !match || ( matchStartPos < 0 ) ) 
		{
			// we may have an iso field
			break;
		}

		string compType = StringUtil::Trim( comparerSplit.NextToken() );
		length = matchEndPos - matchStartPos;
		string fieldName = newValue.substr( matchStartPos + 2, length - 3 );

		m_Fields.push_back( pair< string, EVALUATOR_TYPE >( fieldName, parseType( compType ) ) );
		newValue = newValue.erase( matchStartPos, length );

		DEBUG_GLOBAL( "Field found [" << fieldName << "] comparison set to [" << compType << "]" );
	}

	m_Regex = newValue;

	// Evaluate ISO field
	
	// split by ,
	StringUtil comparerSplitIso( comparer );
	comparerSplitIso.Split( "," );

	match = true;
	newValue = regexIso;
	length = 0;
	
	while ( match )
	{
		int matchStartPos = -1, matchEndPos = -1;

		match = boost::regex_search( newValue, groupsMatch, groupNamesRegex, boost::match_extra );
			
		matchStartPos = groupsMatch.position( 1 );
		matchEndPos = matchStartPos + groupsMatch.length( 1 );
		
		if ( !match || ( matchStartPos < 0 ) ) 
		{
			// if no capture group was defined, no evaluation can be done on the keyword
			if ( ( m_Fields.size() == 0 ) && ( m_FieldsIso.size() == 0 ) )
			{
				stringstream errorMessage;
				errorMessage << "Keyword [" << m_Name << "] should have at least a capture group defined. ";
				errorMessage << "Check regex definition [" << regex << "]";

				throw logic_error( errorMessage.str() );
			}
			break;
		}

		string compType = StringUtil::Trim( comparerSplitIso.NextToken() );
		length = matchEndPos - matchStartPos;
		string fieldName = newValue.substr( matchStartPos + 2, length - 3 );

		m_FieldsIso.push_back( pair< string, EVALUATOR_TYPE >( fieldName, parseType( compType ) ) );
		newValue = newValue.erase( matchStartPos, length );

		DEBUG_GLOBAL( "ISO field found [" << fieldName << "] comparison set to [" << compType << "]" );
	}

	m_RegexIso = newValue;

	Dump();
}

pair< string, RoutingKeyword::EVALUATOR_TYPE > RoutingKeyword::Evaluate( const string& value, const string& field, bool iso )
{
	string crtRegex = ( iso ) ? m_RegexIso : m_Regex;

	DEBUG( "Trying to match [" << crtRegex << "] in [" << value << "]" );

	bool match = false;
	//int captureStartPos = -1;
	unsigned int groups = 0;
		
	// test current value

	boost::regex subfieldsRegex( crtRegex );
	boost::smatch groupsMatch;

	match = boost::regex_search( value, groupsMatch, subfieldsRegex, boost::match_extra );

	if ( match )
	{
		groups = groupsMatch.size();
		DEBUG( "Matched [" << groups << "] groups." );

		// find the field with the specified name
		unsigned int i=0;
		if ( !iso )
		{
			for ( ; i<m_Fields.size(); i++ )
			{
				if ( m_Fields[ i ].first == field )
					break;
			}
			// get the capture at index 
			if ( i+1 < groups )
				return pair< string, EVALUATOR_TYPE >( groupsMatch[ i+1 ], m_Fields[ i ].second );
		}
		else
		{
			for ( ; i<m_FieldsIso.size(); i++ )
			{
				if ( m_FieldsIso[ i ].first == field )
					break;
			}
			// get the capture at index 
			if ( i+1 < groups )
				return pair< string, EVALUATOR_TYPE >( groupsMatch[ i+1 ], m_FieldsIso[ i ].second );
		}
	}
	DEBUG( "Field [" << field << "] not matched in [" << value << "]" );
	return pair< string, RoutingKeyword::EVALUATOR_TYPE >( "", RoutingKeyword::STRING );
}

void NotFoundKeywordMappings::CreateKeys()
{
	cout << "Thread [" << pthread_self() << "] creating keyword mappings keys..." << endl;

	int keyCreateResult = pthread_key_create( &NotFoundKeywordMappings::NotFoundKey, &NotFoundKeywordMappings::DeleteCollections );
	if ( 0 != keyCreateResult )
	{
		TRACE( "An error occured while creating keyword mappings key [" << keyCreateResult << "]" );
	}
}

void NotFoundKeywordMappings::DeleteCollections( void* data )
{
	pthread_t selfId = pthread_self();
	RoutingKeywordMappings* notFoundKeywords = ( map< string, map< string, string > >* )data;

	//TRACE_NOLOG( "Deleting keyword mappings for thread [" << selfId << "]" );
	if ( data != NULL )
	{
		delete notFoundKeywords;
		data = NULL;
	}
	int setSpecificResult = pthread_setspecific( NotFoundKeywordMappings::NotFoundKey, NULL );
	if ( 0 != setSpecificResult )
	{
		TRACE_NOLOG( "Set thread specific keyword mappings failed [" << setSpecificResult << "]" );
	}
}

/*RoutingKeyword::EVALUATOR_TYPE RoutingKeyword::operator[]( const string& field )
{
	for ( unsigned int i=0; i<m_Fields.size(); i++ )
	{
		if ( m_Fields[ i ].first == field )
			return m_Fields[ i ].second;
	}
	return RoutingKeyword::STRING;
}*/

/*
//RoutingKeywordCollection implementation
RoutingKeywordCollection::RoutingKeywordCollection()
{
	// read keywords
	DEBUG( "Reading keywords" );
	DataSet* keywords = RoutingDbOp::GetKeywords();
	
	DEBUG( "Read " << keywords->size() << " keywords from the database." );
	
	try
	{
		for( unsigned int i=0; i<keywords->size(); i++ )
		{
			//get keyword name, com, sel
			string name = StringUtil::Trim( keywords->getCellValue( i, "KEYWORD" )->getString() );
			string comparer = StringUtil::Trim( keywords->getCellValue( i, "COMPARER" )->getString() );

			// the regex
			string valueSelector = StringUtil::Trim( keywords->getCellValue( i, "SELECTOR" )->getString() );
			string valueSelectorIso = StringUtil::Trim( keywords->getCellValue( i, "SELECTORISO" )->getString() );
			
			// insert the keyword
			RoutingKeyword keyword( name, comparer, valueSelector, valueSelectorIso );
			m_InnerCollection.insert( pair< string, RoutingKeyword >( name, keyword ) );
		}
	}
	catch( ... )
	{
		//cleanup
		if( keywords != NULL )
			delete keywords;
		throw;
	}
	
	if( keywords != NULL )
		delete keywords;
}
	
pair< string, RoutingKeyword::EVALUATOR_TYPE > RoutingKeywordCollection::Evaluate( const string& value, const string& keyword, const string& field, bool iso )
{
	map< string, RoutingKeyword >::iterator keywordFinder = m_InnerCollection.find( keyword );
	
	if ( keywordFinder == m_InnerCollection.end() )
	{
		stringstream errorMessage;
		errorMessage << "Can't evaluate keyword [" << keyword << "] because it is not defined in the configuration";
		throw logic_error( errorMessage.str() );
	}
	
	DEBUG( "About to evaluate [" << value << ", " << keyword << ", " << field << "]" );
	pair< string, RoutingKeyword::EVALUATOR_TYPE > evalResult = m_InnerCollection[ keyword ].Evaluate( value, field, iso );
	DEBUG( "Evaluate [" << value << ", " << keyword << ", " << field << "] = [" << evalResult.first << "]" );
	
	return evalResult;
}

RoutingKeyword RoutingKeywordCollection::operator[]( const string& keyword )
{
	map< string, RoutingKeyword >::iterator keywordFinder = m_InnerCollection.find( keyword );
	
	if ( keywordFinder == m_InnerCollection.end() )
	{
		stringstream errorMessage;
		errorMessage << "Can't evaluate keyword [" << keyword << "] because it is not defined in the configuration";
		throw logic_error( errorMessage.str() );
	}
	return m_InnerCollection[ keyword ];
}

//RoutingKeywordMappings implementation
RoutingKeywordMappings::RoutingKeywordMappings()
{
	// read keyword mappings
	DEBUG( "Reading keyword mappings" );
	DataSet* keywords = NULL;

	try
	{
		keywords = RoutingDbOp::GetKeywordMappings();
		if ( keywords == NULL )
			throw runtime_error( "No keywords found in database" );

		DEBUG( "Read " << keywords->size() << " keyword mappings from the database." );

		for( unsigned int i=0; i< keywords->size(); i++ )
		{
			//get keyword name
			string keyword = StringUtil::Trim( keywords->getCellValue( i, "KEYWORD" )->getString() );
			
			map< string, map< string, string > >::iterator keywordFinder = m_Keywords.find( keyword );
			
			// append it if not found
			if ( keywordFinder == m_Keywords.end() )
			{
				//DEBUG( "Creating keyword map for ["  << keyword << "]" );
				m_Keywords.insert( pair< string, map< string, string > >( keyword, map< string, string >() ) );
			}

			//get mt
			string messageType = StringUtil::Trim( keywords->getCellValue( i, "MT" )->getString() );
			map< string, map< string, string > >::iterator mtFinder = m_KeywordsMT.find( messageType );
			if( mtFinder == m_KeywordsMT.end() )
				m_KeywordsMT.insert( pair< string, map< string, string > >( messageType, map< string, string >() ) );
			
			//get tag
			string tag = StringUtil::Trim( keywords->getCellValue( i, "TAG" )->getString() );
			string selector = StringUtil::Trim( keywords->getCellValue( i, "SELECTOR" )->getString() );

			bool iso = ( StringUtil::ToUpper( selector ) == "SELECTORISO" );
						
			//DEBUG( "Finding a place for the keyword " << keyword << " with MT " << messageType );
			
			// find a place in sequence
			m_KeywordsMT[ messageType ].insert( pair< string, string >( keyword, tag ) );
			m_Keywords[ keyword ].insert( pair< string, string>( messageType, tag ) );
			m_IsoMessageTypes.insert( pair< string, bool >( messageType, iso ) );
		}
	}
	catch( ... )
	{
		// cleanup		
		if( keywords != NULL )
		{
			delete keywords;
			keywords = NULL;
		}
		throw;
	}
	
	if( keywords != NULL )
	{
		delete keywords;
		keywords = NULL;
	}

	int onceResult = pthread_once( &RoutingKeywordMappings::NotFoundKeysCreate, &RoutingKeywordMappings::CreateKeys );
	if ( 0 != onceResult )
	{
		TRACE( "One time key creation for keyword mappings failed [" << onceResult << "]" );
	}
}

string RoutingKeywordMappings::internalGetXPath( const string& messageType, const string& keyword )
{
	// try to find the keyword
	map< string, map< string, string > >::const_iterator keywordFinder = m_Keywords.find( keyword );
	if ( keywordFinder == m_Keywords.end() )
	{
		stringstream errorMessage;
		errorMessage << "Keyword [" << keyword << "] not found in configuration.";
		TRACE( errorMessage.str() );
		return "";
	}
	
	// find messageType 
	map< string, string > mappings = m_Keywords[ keyword ];
	map< string, string >::iterator mtFinder = mappings.find( messageType );
	
	// we may need a wildcard to match
	if( mtFinder == mappings.end() )
	{
		map< string, map< string, string > >* notFoundKeywords = ( map< string, map< string, string > >* )pthread_getspecific( RoutingKeywordMappings::NotFoundKey );
		if ( notFoundKeywords == NULL )
			notFoundKeywords = new map< string, map< string, string > >();

		// look if we haven't already searched for the keyword
		map< string, map< string, string > >::const_iterator notFoundKeyword = notFoundKeywords->find( keyword );
		if ( notFoundKeyword != notFoundKeywords->end() )
		{
			// we've added a keyword for this, find our messagetype
			if ( notFoundKeyword->second.find( messageType ) != notFoundKeyword->second.end() )
			{
				// got it
				throw KeywordMappingNotFound( keyword, messageType );
			}
		}

		string repKeyword = messageType;
		
		mtFinder = mappings.begin();
		while( mtFinder != mappings.end() ) 
		{
			DEBUG( "Trying keyword " << mtFinder->first );
			
			// replace keyword with wildcards
			string::size_type wildOffset = string::npos;
			
			do
			{
				wildOffset = mtFinder->first.find( '?', wildOffset + 1 );
								
				if ( wildOffset != string::npos )
				{
					repKeyword[ wildOffset ] = '?';
					DEBUG( "Wildcard offset : " << wildOffset << " result : " << repKeyword );
				}
			} while( wildOffset != string::npos );
			
			// check match
			if( repKeyword == mtFinder->first )
				return mtFinder->second;
			mtFinder++;
		}

		// if we didn't find it, add a dummy keyword so that we don't search again
		if ( notFoundKeyword == notFoundKeywords->end() )
		{
			notFoundKeywords->insert( pair< string, map< string, string > >( keyword, map< string, string >() ) );
		}
		( *notFoundKeywords )[ keyword ].insert( pair< string, string >( messageType, "DUMMY_KWORD" ) );
		int setSpecificResult = pthread_setspecific( RoutingKeywordMappings::NotFoundKey, notFoundKeywords );
		if ( 0 != setSpecificResult )
		{
			TRACE( "Set thread specific keyword mappings failed [" << setSpecificResult << "]" );
		}

		throw KeywordMappingNotFound( keyword, messageType );
	}
	else
		return mtFinder->second;

	//stringstream errorMessage;
	//errorMessage << "Keyword " << keyword <<" not mapped for message type " << messageType;
	//TRACE( errorMessage.str() );

	//return "";	
}

bool RoutingKeywordMappings::isIso( const string& messageType )
{
	// try to find the keyword
	map< string, bool >::const_iterator mtFinder = m_IsoMessageTypes.find( messageType );
	if ( mtFinder == m_IsoMessageTypes.end() )
	{
		stringstream errorMessage;
		errorMessage << "Message [" << messageType << "] not found in configuration.";
		TRACE( errorMessage.str() );
		return false;
	}
	return mtFinder->second;
}

string RoutingKeywordMappings::getXPath( const string& messageType, const string& keyword )
{
	//try
	//{
	//	string tag = internalGetXPath( messageType, keyword );
	//	if ( tag.size() > 0 )
	//	{
	//		stringstream xPath;
	//		xPath << "//smt:MessageText/smt:tag" << tag << "/@tagValue";
	//		return xPath.str();
	//	}
	//}
	//catch( const KeywordMappingNotFound& ex )
	//{
	//	DEBUG( ex.what() << " for keyword [" << ex.getKeyword() << "] and MT [" << ex.getMessageType() << "]" );
	//}
	//return "";

	string tag = "";
	try
	{
		tag = internalGetXPath( messageType, keyword );
	}
	catch( const KeywordMappingNotFound& ex )
	{
		DEBUG( ex.what() << " for keyword [" << ex.getKeyword() << "] and MT [" << ex.getMessageType() << "]" );
	}
	return tag;
}

void RoutingKeywordMappings::CreateKeys()
{
	cout << "Thread [" << pthread_self() << "] creating keyword mappings keys..." << endl;

	int keyCreateResult = pthread_key_create( &RoutingKeywordMappings::NotFoundKey, &RoutingKeywordMappings::DeleteCollections );
	if ( 0 != keyCreateResult )
	{
		TRACE( "An error occured while creating keyword mappings key [" << keyCreateResult << "]" );
	}
}

void RoutingKeywordMappings::DeleteCollections( void* data )
{
	pthread_t selfId = pthread_self();
	map< string, map< string, string > >* notFoundKeywords = ( map< string, map< string, string > >* )data;

	//TRACE_NOLOG( "Deleting keyword mappings for thread [" << selfId << "]" );
	if ( data != NULL )
	{
		delete notFoundKeywords;
		data = NULL;
	}
	int setSpecificResult = pthread_setspecific( RoutingKeywordMappings::NotFoundKey, NULL );
	if ( 0 != setSpecificResult )
	{
		TRACE_NOLOG( "Set thread specific keyword mappings failed [" << setSpecificResult << "]" );
	}
}

const RoutingKeywordMappings::MessageTypeKeywords RoutingKeywordMappings::getMessageTypeKeywords( const string& messageType )
{
	map< string, map< string, string > >::const_iterator findIterator = m_KeywordsMT.find( messageType );
	map< string, string > emptyMap;
	if( findIterator != m_KeywordsMT.end() )
		return m_Keywords[messageType];

	return emptyMap ;
}
*/