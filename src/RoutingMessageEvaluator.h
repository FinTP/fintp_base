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

#ifndef ROUTINGMESSAGEEVALUATOR_H
#define ROUTINGMESSAGEEVALUATOR_H

#include <map>
#include <vector>
#include <string>

using namespace std;

#ifdef WIN32
#include <windows.h>
#endif

#include "XmlUtil.h"
#include "XPathHelper.h"
#include "Collections.h"
#include "RoutingAggregationCode.h"
#include "RoutingKeyword.h"

#include "WSRM/SequenceResponse.h"

#define FEEDBACK_PROVIDER_COUNT 8
#define INTERNALXMLPAYLOAD_FIELDCOUNT 19

//TODO rework this dependency
class RoutingMessage;
class Plugin;

class InternalXmlPayload
{
	public : 
		enum Fields
			{
				SENDER = 0,
				RECEIVER = 1,
				MESSAGETYPE = 2,
				MUR = 3, //TTC
				FINCOPY = 4,//Service
				//IOIDENTIFIER = 5, // va fi scos
				TRN = 5, //reference
				CURRENCY = 6,
				AMOUNT = 7,
				VALUEDATE = 8,
				RELATEDREF = 9,//RelRef
				IBAN = 10,//DbtAccount
				SEQ = 11,
				MAXSEQ = 12,
				IBANPL = 13,//CdtAccount
				SENDERCORR = 14,//OrdBank
				RECEIVERCORR = 15,//BenBank
				//OBATCHID = 17,
				//STATUS = 18,
				ORGTXID = 16,//OrigRef
				REASONCODE = 17,
				ORGINSTRID = 18,
				
			};

	private :
		static const string m_FieldNames[ INTERNALXMLPAYLOAD_FIELDCOUNT ];

	public :
		static string getFieldName( const int field );
};

class ExportedObject CorrelationOptions
{

	public:

		enum SAAACKMethod
		{
			SAA_METHOD_MQID = 100,
			SAA_METHOD_TRN = 101,
			SAA_METHOD_CORRELID = 102
		};

		enum SwiftACKMethod
		{
			METHOD_MQID = 150,
			METHOD_TRN = 151,
			METHOD_CORRELID = 152
		};

		enum TFDACKMethod
		{
			TMETHOD_MQID = 200,
			TMETHOD_MIR = 201,
			TMETHOD_CORRELID = 202
		};

		static string ToString( const SAAACKMethod method ) 
		{
			switch( method )
			{
				case CorrelationOptions::SAA_METHOD_TRN :
					return "TRN";
				case CorrelationOptions::SAA_METHOD_MQID :
					return "MQID";
				case CorrelationOptions::SAA_METHOD_CORRELID :
					return "CORRELID";
				default :
					throw invalid_argument( "SAA ACK method is invalid" );
			}
		}
		
		static string ToString( const SwiftACKMethod method ) 
		{
			switch( method )
			{
				case CorrelationOptions::METHOD_TRN :
					return "TRN";
				case CorrelationOptions::METHOD_MQID :
					return "MQID";
				case CorrelationOptions::METHOD_CORRELID :
					return "CORRELID";
				default :
					throw invalid_argument( "SWIFT ACK method is invalid" );
			}
		}
		
		static string ToString( const TFDACKMethod method ) 
		{
			switch( method )
			{
				case CorrelationOptions::TMETHOD_MIR :
					return "MIR";
				case CorrelationOptions::TMETHOD_MQID :
					return "MQID";
				case CorrelationOptions::TMETHOD_CORRELID :
					return "CORRELID";
				default :
					throw invalid_argument( "TFD ACK method is invalid" );
			}
		}
};

class ExportedObject RoutingMessageEvaluator 
{
	public :

#if defined( TESTDLL_EXPORT ) || defined ( TESTDLL_IMPORT )
		friend class RoutingPayloadEvalTest;
#endif

		enum EvaluatorType
		{
			SWIFTXML,
			ACHBLKACC,
			ACHBLKRJCT,
			ACHRECON,
			ACHCOREBLKDDBTRFL,
			GSRSXML,
			ISOXML,
			SAGSTS,
			SEPASTS
		};
		
		enum FeedbackProvider
		{
			FEEDBACKPROVIDER_FTP = 0,
			FEEDBACKPROVIDER_MQ = 1,
			FEEDBACKPROVIDER_TFD = 2,
			FEEDBACKPROVIDER_UNK = 3
		};

		static const string FEEDBACKFTP_ACK;
		static const string FEEDBACKFTP_APPROVED;
		static const string FEEDBACKFTP_CSMAPPROVED;
		static const string FEEDBACKFTP_RJCT;
		static const string FEEDBACKFTP_REACT;
		static const string FEEDBACKFTP_MSG;
		static const string FEEDBACKFTP_LATERJCT;
		static const string FEEDBACKFTP_REFUSE;
		static const string FEEDBACKFTP_LOSTRFD;
		static const string FEEDBACKFTP_NOREACT;
		
		static const string AGGREGATIONTOKEN_FTPID;
		static const string AGGREGATIONTOKEN_FTPCODE;
		static const string AGGREGATIONTOKEN_SAACODE;
		static const string AGGREGATIONTOKEN_TFDCODE;
		static const string AGGREGATIONTOKEN_SWIFTCODE;
		static const string AGGREGATIONTOKEN_MIR;
		static const string AGGREGATIONTOKEN_MQID;
		static const string AGGREGATIONTOKEN_MQCODE;
		static const string AGGREGATIONTOKEN_PAYLOAD;
		static const string AGGREGATIONTOKEN_REQUESTOR;
		static const string AGGREGATIONTOKEN_BATCHID;
		static const string AGGREGATIONTOKEN_OBATCHID;
		static const string AGGREGATIONTOKEN_BATCHSEQ;
		static const string AGGREGATIONTOKEN_TRN;
		static const string AGGREGATIONTOKEN_OSESSION;
		static const string AGGREGATIONTOKEN_ISESSION;
		static const string AGGREGATIONTOKEN_ISSUER;
		//same as AGGREGATIONTOKEN_FTPID but used to override bussines message info
		static const string AGGREGATIONTOKEN_CORRELID;
	
	protected :
		
		static const string m_FeedbackProviders[ FEEDBACK_PROVIDER_COUNT ];
		static const string m_FeedbackTokens[ FEEDBACK_PROVIDER_COUNT ];
		static const string m_FeedbackIds[ FEEDBACK_PROVIDER_COUNT ];

		/**
		 * Cache map containing message evaluators internal routing fields
		 * Values came from internal defined fields  
		**/
		map< string, string > m_Fields;

		/**
		 * Cache map containing xpath values used while processing message.
		 * Values came from:
		 * 1. Message evaluators implemetations who need specific xpaths values 
		 * 2. KEYWORD routing condition    
		**/
		map< string, string > m_XPathFields;

		/**
		 * All keywords definitions from Routing Engine
		**/		
		static RoutingKeywordCollection* m_Keywords;

		/**   
		 * Specific keyword mappings of current evaluator type
		**/
		map< string, string > m_KeywordMappings;

		/**
		 * Selector type to be consider for the current message.
		**/
		bool m_IsoMessageType;

		static CorrelationOptions::SwiftACKMethod m_SwiftACKMethod;
		static CorrelationOptions::TFDACKMethod m_TFDACKMethod;
		static CorrelationOptions::SAAACKMethod m_SAAACKMethod;
		static bool m_MarkACHApproved;

#ifdef WIN32
		static map<Plugin*, HMODULE> m_RegisteredPlugins;
#else
		static map<Plugin*, void*> m_RegisteredPlugins;
#endif

	protected : 

		RoutingMessageEvaluator::EvaluatorType m_EvaluatorType;	
		RoutingAggregationCode m_AggregationCode;
		bool m_Valid;
		string m_Namespace;

		virtual string internalToString() = 0;
		string internalGetField( const int field );
		string internalGetField( const string& field );

		RoutingMessageEvaluator& operator=( const RoutingMessageEvaluator& source );
		RoutingMessageEvaluator( const RoutingMessageEvaluator& source );
		RoutingMessageEvaluator( const XERCES_CPP_NAMESPACE_QUALIFIER DOMDocument* document, RoutingMessageEvaluator::EvaluatorType evaluatorType );
		
		XALAN_CPP_NAMESPACE_QUALIFIER XercesDocumentWrapper *m_DocumentWrapper;
		XALAN_CPP_NAMESPACE_QUALIFIER XalanDocument *m_XalanDocument;
		const XERCES_CPP_NAMESPACE_QUALIFIER DOMDocument *m_Document;

		void setNamespace( const string& namespaceValue ){ m_Namespace = namespaceValue; }

		static string getBatchType( const string& batchId, const string& tableName = "BATCHJOBS", const string& sender = "" );

	private:

		static string (*m_GetBatchTypeFunction)( const string& batchId, const string& tableName, const string& sender );
		virtual string getMessageType() = 0;

	public:

		static void setGetBatchTypeFunction( string (*function)(const string& batchId, const string& tableName, const string& sender) );

		//Keyword operations
		void setKeywordMappings( const map<string, string> keywordMappings ) { m_KeywordMappings = keywordMappings; }

		const vector<string> getKeywordNames(); 
		static void setKeywords( const RoutingKeywordCollection& keywords );
		void setIsoTypes( const bool isoMessageType ) { m_IsoMessageType = isoMessageType; }
		
		string GetKeywordXPath( const string& messageType, const string& keyword );
		string EvaluateKeywordValue( const string& messageType, const string& value, const string& keyword, const string& field = "value" );
		pair< string, RoutingKeyword::EVALUATOR_TYPE > Evaluate( const string& value, const string& keyword, const string& field );

		static void setSwiftACKMethod( const string& method )
		{
			if ( method == "TRN" )
				m_SwiftACKMethod = CorrelationOptions::METHOD_TRN;
			else if ( method == "CORRELID" )
				m_SwiftACKMethod = CorrelationOptions::METHOD_CORRELID;
			else m_SwiftACKMethod = CorrelationOptions::METHOD_MQID;
		}

		static void setSAAACKMethod( const string& method )
		{
			if ( method == "TRN" )
				m_SAAACKMethod = CorrelationOptions::SAA_METHOD_TRN;
			else if ( method == "CORRELID" )
				m_SAAACKMethod = CorrelationOptions::SAA_METHOD_CORRELID;
			else m_SAAACKMethod = CorrelationOptions::SAA_METHOD_MQID;
		}

		static void setTFDACKMethod( const string& method )
		{
			if ( method == "MQID" )
				m_TFDACKMethod = CorrelationOptions::TMETHOD_MQID;
			else if ( method == "CORRELID" )
				m_TFDACKMethod = CorrelationOptions::TMETHOD_CORRELID;
			else m_TFDACKMethod = CorrelationOptions::TMETHOD_MIR;
		}

		static void setMarkApproved( bool markAchApproved ){ m_MarkACHApproved = markAchApproved; }

		static string getSAAACKMethod() { return CorrelationOptions::ToString( m_SAAACKMethod ); }
		static string getSwiftACKMethod() { return CorrelationOptions::ToString( m_SwiftACKMethod ); }
		static string getTFDACKMethod() { return CorrelationOptions::ToString( m_TFDACKMethod ); }

		static void ReadEvaluators( const string& location );
		static void EvaluatorsCleanup();
	
		static RoutingMessageEvaluator::FeedbackProvider getProviderById( const string& coorelId );
		static RoutingMessageEvaluator::FeedbackProvider getProviderByCode( const string& code );
		static RoutingMessageEvaluator::FeedbackProvider getProviderByName( const string& coorelName );

		static string getNamespaceByMessageType( const string& messageType );

		static RoutingAggregationCode composeFeedback( const string& correlId, const string& errorCode, RoutingMessageEvaluator::FeedbackProvider );
		static string serializeFeedback( const RoutingAggregationCode& feedback );
		
		static RoutingMessageEvaluator* getEvaluator( const XERCES_CPP_NAMESPACE_QUALIFIER DOMDocument* document );
		static RoutingMessageEvaluator::EvaluatorType Parse( const string& evaluatorType );

		// returns true if the message is in SWIFT XML Format and can be evaluated
		bool CheckPayloadType( RoutingMessageEvaluator::EvaluatorType payloadtype ) const { return m_EvaluatorType == payloadtype; }

		static bool isIso( const string& msgNamespace );

		virtual bool isIso() const { return false; } 

		bool isBusinessFormat() const; 

		bool isValid() const { return m_Valid; }
		void setValid( const bool validStatus ) { m_Valid = validStatus; }
		
		string ToString();
		string getField( const int field );
		string getField( const string& field );
		string getCustomXPath( const string& xpath, const bool noCache = false );
		
		string getNamespace() const { return m_Namespace; }
		
		virtual const RoutingAggregationCode& getAggregationCode( const RoutingAggregationCode& feedback ) = 0;
		virtual RoutingAggregationCode getBusinessAggregationCode() 
		{
			return RoutingAggregationCode();
		}

		virtual FinTP::NameValueCollection getAddParams( const string& ref )
		{
			return FinTP::NameValueCollection();
		}
			
		virtual bool isReply() = 0;
		virtual bool isAck() = 0;
		virtual bool isNack() = 0;
		virtual bool isAck( const string ns )
		{
			if ( ns.length() == 0 )
				return isAck();
			return ( isAck() && ( ns == m_Namespace ) );
		}
		virtual bool isNack( const string ns )
		{
			if ( ns.length() == 0 )
				return isNack();
			return ( isNack() && ( ns == m_Namespace ) );
		}
		virtual bool isBatch() = 0;

		virtual bool updateRelatedMessages() { return false; }
		virtual bool isOriginalIncomingMessage() { return false; }
		virtual bool isDD() { return false; }
		virtual bool isID() { return false; }
		virtual string getIssuer() { return ""; }
		
		virtual string getOverrideFeedback() { return ""; }
		virtual string getOverrideFeedbackId() { return ""; }
		virtual RoutingMessageEvaluator::FeedbackProvider getOverrideFeedbackProvider() { return RoutingMessageEvaluator::FEEDBACKPROVIDER_MQ; }
				
		const XERCES_CPP_NAMESPACE_QUALIFIER DOMDocument* getDocument() const { return m_Document; }
		RoutingMessageEvaluator::EvaluatorType getEvaluatorType() const { return m_EvaluatorType; }
		
		// standards version
		virtual wsrm::SequenceResponse* getSequenceResponse() = 0;

		// Visitor method on message
		virtual void UpdateMessage( RoutingMessage* message );
		
		virtual bool checkReactivation(){ return true; }
		virtual bool delayReply() { return false; }

		virtual ~RoutingMessageEvaluator();
};

extern "C"
{
	class Plugin 
	{
		private:
			const std::string m_Name;
			const std::string m_Version;
		public:
			Plugin( const std::string& name, const std::string& version ): m_Name( name ), m_Version( version ){}
			const std::string& getName() { return m_Name; }
			const std::string& getVersion() { return m_Version; }
			virtual bool hasNamespace( const string& aNamespace ) = 0;
			virtual RoutingMessageEvaluator* newInstance( const XERCES_CPP_NAMESPACE_QUALIFIER DOMDocument* document ) = 0;
		};

		typedef Plugin* ( *getPlugin )( void );
}
#endif //ROUTINGMESSAGEEVALUATOR_H
