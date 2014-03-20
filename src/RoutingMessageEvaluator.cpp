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

#include <vector>
#include <deque>

#include <xalanc/PlatformSupport/XSLException.hpp>
#include <xalanc/XPath/NodeRefList.hpp>
#include <xalanc/XalanDOM/XalanDOMString.hpp>
#include <xalanc/XercesParserLiaison/XercesDocumentWrapper.hpp>

#include <boost/filesystem.hpp>

#include "WorkItemPool.h"
#include "PlatformDeps.h"
#include "XmlUtil.h"
#include "StringUtil.h"
#include "Trace.h"
#include "Base64.h"

#include "RoutingMessageEvaluator.h"

using namespace FinTP;

#ifdef WIN32
map<Plugin*, HMODULE> RoutingMessageEvaluator::m_RegisteredPlugins;
typedef map<Plugin*, HMODULE>::const_iterator pluginIterator;
#else
#include <dlfcn.h>
map<Plugin*, void*> RoutingMessageEvaluator::m_RegisteredPlugins;
typedef map<Plugin*, void*>::const_iterator pluginIterator;
#endif

string (*RoutingMessageEvaluator::m_GetBatchTypeFunction)( const string& batchId, const string& tableName, const string& sender ) = NULL;

RoutingKeywordCollection* RoutingMessageEvaluator::m_Keywords = NULL;

// Feedback codes
const string RoutingMessageEvaluator::FEEDBACKFTP_ACK = "FTP02";
const string RoutingMessageEvaluator::FEEDBACKFTP_APPROVED = "FTM00";
const string RoutingMessageEvaluator::FEEDBACKFTP_CSMAPPROVED = "CSM00";
const string RoutingMessageEvaluator::FEEDBACKFTP_RJCT = "FTP09";
const string RoutingMessageEvaluator::FEEDBACKFTP_REACT = "FTP12";
const string RoutingMessageEvaluator::FEEDBACKFTP_MSG = "FTP00";
const string RoutingMessageEvaluator::FEEDBACKFTP_LATERJCT = "FTP10";
const string RoutingMessageEvaluator::FEEDBACKFTP_REFUSE = "RFD";
const string RoutingMessageEvaluator::FEEDBACKFTP_LOSTRFD = "FTP99";
const string RoutingMessageEvaluator::FEEDBACKFTP_NOREACT = "FTP21";

// RoutingMessageEvaluator implementation 
// new dbv2
const string RoutingMessageEvaluator::AGGREGATIONTOKEN_FTPID = "CORRELID";
const string RoutingMessageEvaluator::AGGREGATIONTOKEN_FTPCODE = "APPCODE";
const string RoutingMessageEvaluator::AGGREGATIONTOKEN_SAACODE = "INTERFACECODE";
const string RoutingMessageEvaluator::AGGREGATIONTOKEN_TFDCODE = "CORRESPCODE";
const string RoutingMessageEvaluator::AGGREGATIONTOKEN_SWIFTCODE = "NETWORKCODE";
// old ones left in dbv2
const string RoutingMessageEvaluator::AGGREGATIONTOKEN_MIR = "SWIFTMIR";
const string RoutingMessageEvaluator::AGGREGATIONTOKEN_MQID = "MQID";
const string RoutingMessageEvaluator::AGGREGATIONTOKEN_PAYLOAD = "PAYLOAD";
const string RoutingMessageEvaluator::AGGREGATIONTOKEN_REQUESTOR = "REQUESTOR";
const string RoutingMessageEvaluator::AGGREGATIONTOKEN_MQCODE = "MQFEED";
const string RoutingMessageEvaluator::AGGREGATIONTOKEN_BATCHID = "BATCHID";
const string RoutingMessageEvaluator::AGGREGATIONTOKEN_OBATCHID = "OBATCHID";
const string RoutingMessageEvaluator::AGGREGATIONTOKEN_BATCHSEQ = "BATCHSEQ";
const string RoutingMessageEvaluator::AGGREGATIONTOKEN_TRN = "TRN";
const string RoutingMessageEvaluator::AGGREGATIONTOKEN_OSESSION = "OSESSION";
const string RoutingMessageEvaluator::AGGREGATIONTOKEN_ISESSION = "ISESSION";
const string RoutingMessageEvaluator::AGGREGATIONTOKEN_ISSUER = "ISSUER";
const string RoutingMessageEvaluator::AGGREGATIONTOKEN_CORRELID = "CORRELID";

const string RoutingMessageEvaluator::m_FeedbackProviders[ FEEDBACK_PROVIDER_COUNT ] = { "Q", "W", "T", "W", "T", "A", "S", "U" };

const string RoutingMessageEvaluator::m_FeedbackTokens[ FEEDBACK_PROVIDER_COUNT ] = { 
	RoutingMessageEvaluator::AGGREGATIONTOKEN_FTPCODE,
	RoutingMessageEvaluator::AGGREGATIONTOKEN_MQCODE,
	RoutingMessageEvaluator::AGGREGATIONTOKEN_TFDCODE, 
	RoutingMessageEvaluator::AGGREGATIONTOKEN_MQCODE,
	RoutingMessageEvaluator::AGGREGATIONTOKEN_TFDCODE, 
	RoutingMessageEvaluator::AGGREGATIONTOKEN_SAACODE, 
	RoutingMessageEvaluator::AGGREGATIONTOKEN_SWIFTCODE, 
	"UNKCODE" };

const string RoutingMessageEvaluator::m_FeedbackIds[ FEEDBACK_PROVIDER_COUNT ] = { 
	RoutingMessageEvaluator::AGGREGATIONTOKEN_FTPID,
	RoutingMessageEvaluator::AGGREGATIONTOKEN_MQID,
	RoutingMessageEvaluator::AGGREGATIONTOKEN_BATCHID, 
	RoutingMessageEvaluator::AGGREGATIONTOKEN_TRN, 
	RoutingMessageEvaluator::AGGREGATIONTOKEN_MIR,
	RoutingMessageEvaluator::AGGREGATIONTOKEN_MQID,
	RoutingMessageEvaluator::AGGREGATIONTOKEN_MQID,
	"UNKID" };

	bool RoutingMessageEvaluator::m_MarkACHApproved = false;
	CorrelationOptions::SAAACKMethod RoutingMessageEvaluator::m_SAAACKMethod = CorrelationOptions::SAA_METHOD_MQID;
	CorrelationOptions::SwiftACKMethod RoutingMessageEvaluator::m_SwiftACKMethod = CorrelationOptions::METHOD_MQID;
	CorrelationOptions::TFDACKMethod RoutingMessageEvaluator::m_TFDACKMethod = CorrelationOptions::TMETHOD_MIR;

	const string InternalXmlPayload::m_FieldNames[ INTERNALXMLPAYLOAD_FIELDCOUNT ] = {
		"Sender",
		"Receiver",
		"MessageType",
		"TTC", // ex MUR,
		"Service", //FINCOPY",
		//"IOIdentifier",// n/a - poate fi scos nu era
		"Reference", //TRN
		"Currency",
		"Amount",//new
		"ValueDate",
		"RelRef",//RelatedReference
		"DbtAccount"// ex IBAN",
		"SEQ",
		"MAXSEQ",
		"CdtAccount",// ex IBANPL",
		"OrdBank",//ex SenderCorresp,
		"BenBank",//"ReceiverCorresp",
		//"OBatchId", //?
		//"Status", //?
		"OrigRef",//OriginalTransactionId, 056, 029
		"RCode", // new ReasonCode
		//"EndToEndId" nu mai e folosit
		"OrigInstrID"// ex OriginalInstructedId
	};

//InternalXmlPayload implementation
string InternalXmlPayload::getFieldName( const int field )
{
	if( ( field >= INTERNALXMLPAYLOAD_FIELDCOUNT ) || ( field < 0 ) )
	{
		stringstream errorMessage;
		errorMessage << "Unknown field requested [" << field << "]";
		throw invalid_argument( errorMessage.str() );
	}
	return m_FieldNames[ field ];
}

RoutingMessageEvaluator::RoutingMessageEvaluator( const XERCES_CPP_NAMESPACE_QUALIFIER DOMDocument* document, RoutingMessageEvaluator::EvaluatorType evaluatorType )
{
	m_Document = document;
	m_Valid = ( m_Document != NULL );
	
	m_DocumentWrapper = NULL;
	m_XalanDocument = NULL;
	m_EvaluatorType = evaluatorType;
	//TODO initialize keywords maps?
}

RoutingMessageEvaluator::RoutingMessageEvaluator( const RoutingMessageEvaluator& source )
{
	DEBUG2( "Copy ctor" );
	m_Document = source.getDocument();
	m_Valid = ( m_Document != NULL );
	
	m_DocumentWrapper = NULL;
	m_XalanDocument = NULL;
	m_EvaluatorType = source.getEvaluatorType();
	//TODO copy keywords maps?
}

RoutingMessageEvaluator& RoutingMessageEvaluator::operator=( const RoutingMessageEvaluator& source )
{
	if ( this == &source )
		return *this;

	DEBUG2( "op=" );
	if ( m_DocumentWrapper != NULL )
	{
		//m_DocumentWrapper->destroyWrapper();
		DEBUG( "Destroying wrapper" );
		
		delete m_DocumentWrapper;
	}
	
	m_Document = source.getDocument();
	m_Valid = ( m_Document != NULL );
	m_Namespace = source.m_Namespace;
	m_XPathFields = source.m_XPathFields;
	m_Fields = source.m_Fields;
	m_AggregationCode = source.m_AggregationCode;
	
	m_DocumentWrapper = NULL;
	m_XalanDocument = NULL;
	m_EvaluatorType = source.getEvaluatorType();
	//TODO copy keywords maps?
	return *this;
}

void RoutingMessageEvaluator::UpdateMessage( RoutingMessage* message )
{
}

RoutingMessageEvaluator* RoutingMessageEvaluator::getEvaluator( const XERCES_CPP_NAMESPACE_QUALIFIER DOMDocument* document )
{
	if ( document == NULL )
		return NULL;

	RoutingMessageEvaluator* returnedEvaluator = NULL;

	try
	{
		DEBUG_GLOBAL( "Evaluating namespace ..." );   
		string ns = XmlUtil::getNamespace( document );

		for ( pluginIterator it = m_RegisteredPlugins.begin(); it != m_RegisteredPlugins.end(); ++it )
			if ( it->first->hasNamespace( ns ) )
			{
				returnedEvaluator = it->first->newInstance( document );
				break;
			}

		if ( returnedEvaluator != NULL )
		{
			returnedEvaluator->setNamespace( ns );
			return returnedEvaluator;
		}

		DEBUG( "No known evaluator matches this payload [" << ns << "]" );
		return NULL;
	}
	catch( const std::exception& ex )
	{
		if ( returnedEvaluator != NULL )
		{
			delete returnedEvaluator;
			returnedEvaluator = NULL;
		}

		TRACE_GLOBAL( "Namespace evaluation failed : [" << ex.what() << "]" );
		return NULL;
	}
	catch( ... )
	{
		if ( returnedEvaluator != NULL )
		{
			delete returnedEvaluator;
			returnedEvaluator = NULL;
		}

		TRACE_GLOBAL( "Namespace evaluation failed. [unknown exception]" );
		return NULL;
	}
}

bool RoutingMessageEvaluator::isIso( const string& msgNamespace )
{
	for ( pluginIterator it = m_RegisteredPlugins.begin(); it != m_RegisteredPlugins.end(); ++it )
		if ( /*m_RegisteredPlugins[i]->isIso() &&*/ it->first->hasNamespace( msgNamespace ) )
			return true;
	return false;
}

bool RoutingMessageEvaluator::isBusinessFormat() const
{
	return ( CheckPayloadType( RoutingMessageEvaluator::SWIFTXML ) || isIso( m_Namespace ) );
}

string RoutingMessageEvaluator::getNamespaceByMessageType( const string& messageType )
{
	if ( messageType == "104" )
		return "urn:swift:xs:CoreBlkLrgRmtDDbt";
	if ( ( messageType == "CQ" ) || ( messageType == "PN" ) || ( messageType == "BE" ) )
		return "urn:swift:xs:CoreBlkChq";
	/*if ( messageType == "PN" )
		return "urn:swift:xs:CoreBlkPrmsNt";
	if ( messageType == "BE" )
		return "urn:swift:xs:CoreBlkBillXch";*/
	return "";
}

string RoutingMessageEvaluator::getField( const int field )
{
	return getField( InternalXmlPayload::getFieldName( field ) );
}

string RoutingMessageEvaluator::getField( const string& field )
{
	if ( !m_Valid )
		throw logic_error( "The message evaluator is invalid. Unable to evaluate field" );
		
	if ( m_Document == NULL )
		throw logic_error( "Document is NULL and cannot be evaluated" );
		
	try 
	{
		XALAN_USING_XALAN( XercesDocumentWrapper );
		XALAN_USING_XALAN( XalanDocument );
		
		if ( m_Fields.find( field ) == m_Fields.end() )
		{
			// map xerces dom to xalan document
			if ( m_XalanDocument == NULL )
			{
#ifdef XALAN_1_9
				XALAN_USING_XERCES( XMLPlatformUtils )
				m_DocumentWrapper = new XercesDocumentWrapper( *XMLPlatformUtils::fgMemoryManager, m_Document, true, true, true );
#else
				m_DocumentWrapper = new XercesDocumentWrapper( m_Document, true, true, true );
#endif
				m_XalanDocument = ( XalanDocument* )m_DocumentWrapper;
				
				if( m_XalanDocument == NULL )
					throw runtime_error( "Unable to parse payload for evaluation." );
			}
			m_Fields.insert( pair< string, string >( field, internalGetField( field ) ) );
		}
		string result = m_Fields[ field ];		
		string fieldName = field;
		DEBUG( "Field [" << fieldName << "] = [" << result << "]" );
		return result;
	}
	catch( const std::exception& ex )
	{
		TRACE( "Error evaluating field [" << field << "] : " << ex.what() );
		return "";
	}
	catch( ... )
	{
		TRACE( "Error evaluating field [" << field << "] : Unknown reason" );
		return "";
	}
}

string RoutingMessageEvaluator::getCustomXPath( const string& xpath, const bool noCache )
{
	if ( !m_Valid )
		throw logic_error( "The message evaluator is invalid. Unable to evaluate custom xpath." );
	
	if ( m_Document == NULL )
		throw logic_error( "Document is NULL and cannot be evaluated" );
	
	try
	{
		XALAN_USING_XALAN( XercesDocumentWrapper );
		//XALAN_USING_XALAN( XalanDocument );
		
		string result = "";
		if ( !noCache )
			result = m_XPathFields[ xpath ];
			
		if ( result.length() == 0 )
		{
			// map xerces dom to xalan document
			if ( m_XalanDocument == NULL )
			{
#ifdef XALAN_1_9
				XALAN_USING_XERCES( XMLPlatformUtils )
				m_DocumentWrapper = new XercesDocumentWrapper( *XMLPlatformUtils::fgMemoryManager, m_Document, true, true, true );
#else
				m_DocumentWrapper = new XercesDocumentWrapper( m_Document, true, true, true );
#endif
				m_XalanDocument = ( XALAN_CPP_NAMESPACE_QUALIFIER XalanDocument* )m_DocumentWrapper;
				
				if( m_XalanDocument == NULL )
					throw runtime_error( "Unable to parse payload for evaluation." );
			}
			if ( noCache )
				return XPathHelper::SerializeToString( XPathHelper::Evaluate( xpath, m_XalanDocument ) );
				
			m_XPathFields[ xpath ] = XPathHelper::SerializeToString( XPathHelper::Evaluate( xpath, m_XalanDocument ) );
			result = m_XPathFields[ xpath ];
		}
		DEBUG( "Custom XPath evaluation of [" << xpath << "] = [" << result << "]" );
		return result;
	}
	catch( ... )
	{
		TRACE( "Error evaluating custom xpath [" << xpath << "]" );
		return "";
	}
}
	
RoutingMessageEvaluator::~RoutingMessageEvaluator()
{
	try
	{
		if ( m_DocumentWrapper != NULL )
		{
			//m_DocumentWrapper->destroyWrapper();
			DEBUG( "Destroying wrapper" );

			delete m_DocumentWrapper;
			m_DocumentWrapper = NULL;
		}
	}
	catch( ... )
	{
		try
		{
			TRACE( "An error occured while destroying m_DocumentWrapper" );
		}catch( ... ){};
	}

	try
	{
		m_Fields.clear();
	}
	catch( ... )
	{
		try
		{
			TRACE( "An error occured while clearing fields" );
		}catch( ... ){};
	}
}

RoutingMessageEvaluator::FeedbackProvider RoutingMessageEvaluator::getProviderById( const string& correlId )
{
	//string provider = "";
	
	for( int i=0; i<FEEDBACK_PROVIDER_COUNT; i++ )
		if ( correlId == m_FeedbackIds[ i ] )
			return ( RoutingMessageEvaluator::FeedbackProvider )i ;
	
	stringstream errorMessage;
	errorMessage << "Unknown provider for id [" << correlId << "]";
	throw runtime_error( errorMessage.str() );
}

RoutingMessageEvaluator::FeedbackProvider RoutingMessageEvaluator::getProviderByName( const string& name )
{
	//string provider = "";
	
	for( int i=0; i<FEEDBACK_PROVIDER_COUNT; i++ )
		if ( name == m_FeedbackProviders[ i ] )
			return ( RoutingMessageEvaluator::FeedbackProvider )i;
	
	stringstream errorMessage;
	errorMessage << "Unknown provider for name [" << name << "]";
	throw runtime_error( errorMessage.str() );
}

RoutingMessageEvaluator::FeedbackProvider RoutingMessageEvaluator::getProviderByCode( const string& code )
{
	//string provider = "";
	
	for( int i=0; i<FEEDBACK_PROVIDER_COUNT; i++ )
		if ( code == m_FeedbackTokens[ i ] )
			return ( RoutingMessageEvaluator::FeedbackProvider )i;
	
	stringstream errorMessage;
	errorMessage << "Unknown provider for code [" << code << "]";
	throw runtime_error( errorMessage.str() );
}

RoutingAggregationCode RoutingMessageEvaluator::composeFeedback( const string& correlId, const string& errorCode, RoutingMessageEvaluator::FeedbackProvider provider )
{
	RoutingAggregationCode feedbackCode;
	feedbackCode.setCorrelToken( m_FeedbackIds[ provider ] );
	feedbackCode.setCorrelId( correlId );
	feedbackCode.addAggregationField( m_FeedbackTokens[ provider ], errorCode );
	return feedbackCode;
}

string RoutingMessageEvaluator::serializeFeedback( const RoutingAggregationCode& feedback )
{
	RoutingMessageEvaluator::FeedbackProvider provider = getProviderById( feedback.getCorrelToken() );
	string retValue;

	// HACK - MQID may be used as correlation token, but TFD code is actually the feedback, so..
	try
	{
		retValue = m_FeedbackProviders[ provider ] + string( "|" ) + feedback.getCorrelId() + string( "|" ) + feedback.getAggregationField( m_FeedbackTokens[ provider ] );
	}
	catch( ... )
	{
		// should ckeck if it exists...
		provider = getProviderByCode( feedback.getAggregationFieldName( 0 ) );
		string actualValue = feedback.getAggregationField( m_FeedbackTokens[ provider ] );
		
		// we truncate the error code here ...
		if( actualValue.length() > 5 )
			actualValue = actualValue.substr( 1, 5 );

		retValue = m_FeedbackProviders[ provider ] + string( "|" ) + feedback.getCorrelId() + string( "|" ) + actualValue;
	}

	DEBUG_GLOBAL( "Formatted feedback is [" << retValue << "]" );
	return retValue;
}
/*
string RoutingMessageEvaluator::getKeyword( const string& keyword )
{
	if( m_Keywords.find( keyword ) != m_Keywords.end() )
		return m_Keywords[keyword];

	stringstream errorMessage;
	errorMessage << "Unknown keyword requested [" << keyword << "]";
	throw invalid_argument( errorMessage.str() );
}

void RoutingMessageEvaluator::initMethods( const FinTP::AppSettings& settings )
{
	m_SAAACKMETHOD = settings[ "SAAACKMethod" ];
	m_SWIFTACKMETHOD = settings[ "SwiftACKMethod" ];
	m_TFDACKMETHOD = settings[ "TFDACKMethod" ];

	const string& TFDACHApproved = settings[ "TFDACHApproved" ];
	if ( TFDACHApproved.empty() )
		DEBUG( "TFDACHApproved setting not found in config. Using default value [ignore approved messages]." );
	m_TFDACHAPPROVED = TFDACHApproved == "true";
}
*/
void RoutingMessageEvaluator::EvaluatorsCleanup()
{
	for ( pluginIterator it = m_RegisteredPlugins.begin(); it != m_RegisteredPlugins.end(); ++it )
	{
#ifdef WIN32
		FreeLibrary( it->second );
#else
		dlclose( it->second );
#endif
		delete it->first;
	}
	m_RegisteredPlugins.clear();
}

void RoutingMessageEvaluator::ReadEvaluators( const string& location )
{	
#ifdef WIN32
	const string pluginExtension = ".dll";
#else
	const string pluginExtension = ".so";
#endif

	boost::filesystem::path path( location );
	if ( !boost::filesystem::is_directory( path ) )
		throw runtime_error( "PluginLocation is not a valid directory" );
	boost::filesystem::directory_iterator it ( path );
	boost::filesystem::directory_iterator end;
	while ( it != end )
	{ 
		boost::filesystem::path child = it->path();
		if ( boost::filesystem::is_regular_file( child ) )
			if ( StringUtil::ToLower( boost::filesystem::extension( *it ) ) == pluginExtension )
			{
#if defined WIN32 && defined UNICODE
				wstring filename = child.wstring();
#else
				string filename = child.string();
#endif
#ifdef WIN32
				HMODULE lib = LoadLibrary( filename.c_str() );
#else
				void* lib = dlopen( filename.c_str(), RTLD_NOW );
#endif
				if ( lib == NULL )
					TRACE( "Cannot load plugin: " + filename )
				else
				{
#ifdef WIN32
				getPlugin func = (getPlugin)GetProcAddress( lib, "getPluginInfo" ); 
#else
				getPlugin func = (getPlugin)dlsym( lib, "getPluginInfo" );
#endif
				if ( func == NULL )
					TRACE( filename + " is not a valid FinTP plugin" )
				else
				{
					try
					{
						Plugin* pluginInfo = (*func)();
#ifdef WIN32
						m_RegisteredPlugins.insert(pair<Plugin*, HMODULE>(pluginInfo, lib));
#else
						m_RegisteredPlugins.insert(pair<Plugin*, void*>(pluginInfo, lib));
#endif
						DEBUG( "Registered plugin: " << pluginInfo->getName() <<" version: " << pluginInfo->getVersion() );
					}
					catch ( ... )
					{
						EvaluatorsCleanup();
						TRACE( "Unable to load evaluator from file: " + filename )
						throw;
					}
				}
				}
				  
			}
	++it;
	}
}

string RoutingMessageEvaluator::GetKeywordXPath( const string& messageType, const string& keyword )
{
	//TODO Check the message type
	map< string, string >::iterator mtFinder = m_KeywordMappings.find( keyword );
	
	// we may need a wildcard to match
	if( mtFinder == m_KeywordMappings.end() )
	{
		try
		{
			map< string, map< string, string > >* notFoundKeywords = ( map< string, map< string, string > >* )pthread_getspecific( NotFoundKeywordMappings::NotFoundKey );
			if ( notFoundKeywords == NULL )
				notFoundKeywords = new map< string, map< string, string > >();

			// look if we haven't already searched for the keyword
			map< string, map< string, string > >::const_iterator notFoundKeyword = notFoundKeywords->find( keyword );
			if ( notFoundKeyword != notFoundKeywords->end() )
			{
				// we've added a keyword for this, find our messagetype
				if ( notFoundKeyword->second.find( messageType ) != notFoundKeyword->second.end() )
				{
					throw KeywordMappingNotFound( keyword, messageType );
				}

			}

			mtFinder = m_KeywordMappings.begin();
			string repKeyword = keyword;
			while( mtFinder != m_KeywordMappings.end() ) 
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
			int setSpecificResult = pthread_setspecific( NotFoundKeywordMappings::NotFoundKey, notFoundKeywords );
			if ( 0 != setSpecificResult )
			{
				TRACE( "Set thread specific keyword mappings failed [" << setSpecificResult << "]" );
			}
			throw KeywordMappingNotFound( keyword, messageType );
		}
		catch( const KeywordMappingNotFound& ex )
		{
			DEBUG( ex.what() << " for keyword [" << ex.getKeyword() << "] and MT [" << ex.getMessageType() << "]" );
		}
		return "";
	}

	return mtFinder->second;

}

string RoutingMessageEvaluator::EvaluateKeywordValue( const string& messageType, const string& value, const string& keyword, const string& field )
{		
	pair< string, RoutingKeyword::EVALUATOR_TYPE > evalResult = Evaluate( value, keyword, field );
	return evalResult.first;
}

pair< string, RoutingKeyword::EVALUATOR_TYPE > RoutingMessageEvaluator::Evaluate( const string& value, const string& keyword, const string& field )
{
	map< string, RoutingKeyword >::iterator keywordFinder = m_Keywords->find( keyword );
	
	if ( keywordFinder == m_Keywords->end() )
	{
		stringstream errorMessage;
		errorMessage << "Can't evaluate keyword [" << keyword << "] because it is not defined in the configuration";
		throw logic_error( errorMessage.str() );
	}
	
	DEBUG( "About to evaluate [" << value << ", " << keyword << ", " << field << "]" );
	RoutingKeyword key = keywordFinder->second;
	pair< string, RoutingKeyword::EVALUATOR_TYPE > evalResult = key.Evaluate( value, field, m_IsoMessageType );
	DEBUG( "Evaluate [" << value << ", " << keyword << ", " << field << "] = [" << evalResult.first << "]" );
	
	return evalResult;
}

void RoutingMessageEvaluator::setKeywords( const RoutingKeywordCollection& keywords )
{
	if ( m_Keywords == NULL )
		m_Keywords = new RoutingKeywordCollection( keywords );
	else
		return;
}

string RoutingMessageEvaluator::internalGetField( const int field )
{
	return internalGetField( InternalXmlPayload::getFieldName( field ) );
}

string RoutingMessageEvaluator::internalGetField( const string& field )
{
	if ( m_Keywords->find( field ) == m_Keywords->end() )
	{
		if ( field == InternalXmlPayload::getFieldName( InternalXmlPayload::MESSAGETYPE ) )
			return getMessageType();
	}
	else
	{
		const string messageType = getMessageType();
		const string xpath = GetKeywordXPath( messageType, field );
		const string value = XPathHelper::SerializeToString( XPathHelper::Evaluate( xpath, m_XalanDocument, m_Namespace ) );
		return EvaluateKeywordValue( messageType, value, field );
	}

	throw invalid_argument( "Unknown field requested [" + field + "]" );
}

const vector<string> RoutingMessageEvaluator::getKeywordNames()
{
	vector<string> result;
	for ( map< string, string >::const_iterator it = m_KeywordMappings.begin(); it != m_KeywordMappings.end(); ++it )
		result.push_back( it->first );

	return result;
}


void RoutingMessageEvaluator::setGetBatchTypeFunction( string (*function)(const string& batchId, const string& tableName, const string& sender) )
{
	if ( m_GetBatchTypeFunction != NULL )
		throw runtime_error( "GetBatchType function already set." );

	m_GetBatchTypeFunction = function;
}

string RoutingMessageEvaluator::getBatchType( const string& batchId, const string& tableName, const string& sender )
{
	if ( m_GetBatchTypeFunction == NULL )
		throw runtime_error( "GetBatchType function not set." );

	return m_GetBatchTypeFunction( batchId, tableName, sender );
}