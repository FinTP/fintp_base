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

#ifndef MQFILTER_H
#define MQFILTER_H

#include <string>

#include "../DllMain.h"
#include "../AbstractFilter.h"
#include "Collections.h"
#include <xercesc/util/PlatformUtils.hpp>
#include <xercesc/framework/StdOutFormatTarget.hpp>
#include "TransportHelper.h"
#include "../BatchManager/BatchManager.h"
#include "../BatchManager/Storages/BatchMQStorage.h"

using namespace std;

namespace FinTP
{
	class ExportedObject MqFilter : public AbstractFilter
	{
		public:
		
			//config section parameters
			static const string MQQUEUE;
			static const string MQBACKUPQUEUE;
			static const string MQMANAGER;
			static const string MQURI;
			static const string MQHELPERTYPE;
			static const string MQBATCH;
			
			static const string MQMSGSIZE;
			static const string MQMSGID;
			static const string MQGROUPID;  // GroupId = BatchId
			static const string MQLASTMSG;	// Last message in Group 0 = No, 1 = Yes
			static const string MQMSGCORELID;
			static const string MQREPLYQUEUE;
			static const string MQREPLYQUEUEMANAGER;
			static const string MQREPLYOPTIONS;
			static const string MQRPLTransportURI;
			static const string MQRPLMESSAGETYPE;
			static const string MQRPLUSRDATA;
			static const string MQRPLMESSAGEFORMAT;
			static const string MQMESSAGETYPE;
			static const string MQFEEDBACK;
			static const string MQFORMAT;
			static const string MQAPPNAME;
			static const string MQSEQUENCE;

			static const string MQSSLKEYREPOSITORY;
			static const string MQSSLCYPHERSPEC;
			static const string MQSSLPEERNAME;
			static const string MQHELPERRETRIES;

			MqFilter();
			~MqFilter();
			
			// return true if the filter supports logging payload to a file
			// note for overrides : return true/false if the payload can be read without rewinding ( not a stream )
			bool canLogPayload();
			// return true if the filter can execute the requested operation in client/server context
			bool isMethodSupported( FilterMethod method, bool asClient );
			
			FilterResult ProcessMessage( XERCES_CPP_NAMESPACE_QUALIFIER DOMDocument* inputOutputData, NameValueCollection& transportHeaders, bool asClient );
			FilterResult ProcessMessage( const XERCES_CPP_NAMESPACE_QUALIFIER DOMDocument* inputData, AbstractFilter::buffer_type outputData, NameValueCollection& transportHeaders, bool asClient );
			FilterResult ProcessMessage( AbstractFilter::buffer_type inputData, XERCES_CPP_NAMESPACE_QUALIFIER DOMDocument* outputData, NameValueCollection& transportHeaders, bool asClient );
			FilterResult ProcessMessage( AbstractFilter::buffer_type inputData, AbstractFilter::buffer_type outputData, NameValueCollection& transportHeaders, bool asClient );
			
			FilterResult ProcessMessage( const XERCES_CPP_NAMESPACE_QUALIFIER DOMDocument* inputData, unsigned char** outputData, NameValueCollection& transportHeaders, bool asClient )
			{
				throw FilterInvalidMethod( AbstractFilter::XmlToBuffer );
			}

			FilterResult ProcessMessage( AbstractFilter::buffer_type inputData, unsigned char** outputData, NameValueCollection& transportHeaders, bool asClient )
			{
				throw FilterInvalidMethod( AbstractFilter::BufferToBuffer );
			}

			void Init();

			/**
			 * \brief Use this method to send a reply MQ message as response to a request
			 * \details This method is part of a scenario of sending back report messages via Connector:FilterChain:MqFilter.
			 * 			Scenario not fully implemented yet
			 */
			void Reply( const string& buffer, const NameValueCollection& transportHeaders, long feedback );
			
			void Rollback();	// rolls back the current get/put
			void Commit();		// commits get/put
			void Abort();		// moves the message to system.dead.letter.queue
			void Cleanup();   // Current batch action that get messages from current position till the last message
			
			// returns true if the filter is running in batch mode
			bool isBatch() const;

			// returns the WMQ queue manager name
			string getQueueManagerName() const;

			// returns the WMQ channel definition
			string getTransportURI() const;

			// returns the WMQ queue name used to put/get messages from
			string getQueueName() const;

			TransportHelper::TRANSPORT_HELPER_TYPE getHelperType() const;

			bool isTransportType() { return true; }

		private :
		
			string m_QueueManagerName;
			string m_QueueName;
			string m_BackupQueueName;
			string m_MessageId;
			string m_GroupId;
			string m_CorrelationId;

			string m_CrtBatchId;

			//will be WMQ BatchWMQ
			BatchManager<BatchMQStorage> *m_BatchManager;
			
			// validates required properties
			void ValidateProperties();
			
			TransportHelper* m_CrtHelper;
			TransportHelper::TRANSPORT_HELPER_TYPE m_HelperType;
					
			string getQueueManagerName( NameValueCollection& transportHeaders ) const;
			string getTransportURI( const NameValueCollection& transportHeaders ) const;
			string getReplyTransportURI( const NameValueCollection& transportHeaders ) const;
			string getQueueName( NameValueCollection& transportHeaders ) const;
			string getBackupQueueName( NameValueCollection& transportHeaders ) const;
			string getMessageId( NameValueCollection& transportHeaders ) const;
			string getGroupId( NameValueCollection& transportHeaders ) const;
			string getCorrelationId( NameValueCollection& transportHeaders ) const;
			
			string getSSLKeyRepository( const NameValueCollection& transportHeaders ) const;
			string getSSLCypherSpec( const NameValueCollection& transportHeaders ) const;
			string getSSLPeerName( const NameValueCollection& transportHeaders ) const;

			void setTransportHeaders( NameValueCollection& transportHeaders, bool asClient );
			string getFormat();
			
			bool isBatch( NameValueCollection& transportHeaders ) const;
	};
}

#endif // MQFILTER_H
