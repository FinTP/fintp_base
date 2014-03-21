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

#ifndef BATCHITEM_H
#define BATCHITEM_H

#include <xercesc/dom/DOMDocument.hpp>
#include "Collaboration.h"
#include "WorkItemPool.h"
#include "DllMain.h"

XERCES_CPP_NAMESPACE_USE

namespace FinTP
{
	class ExportedObject BatchItem
	{
		public:
			typedef enum 
			{
				BATCHITEM_TXT = 1,
				BATCHITEM_XML = 2,
				BATCHITEM_BIN = 3
			} BATCHITEM_TYPE;
		
			explicit BatchItem( const int sequence = BatchItem::FIRST_IN_SEQUENCE, 
				const string& batchId = Collaboration::EmptyGuid(), 
				const string& messageId = Collaboration::EmptyGuid(),
				const bool isLast = false, const string& eyecatcher = "" );
			BatchItem( const BatchItem& source );
			~BatchItem();
			
			BatchItem& operator=( const BatchItem& source );
			
			void setPayload( const string& payload );
			void setPayload( const char* payload );
			void setPayload( const ManagedBuffer *payload );
			void setBinPayload( const ManagedBuffer *payload );
			void setXmlPayload( XERCES_CPP_NAMESPACE_QUALIFIER DOMDocument* payload );
			
			string getPayload() const;		
			XERCES_CPP_NAMESPACE_QUALIFIER DOMDocument* getXmlPayload();
			const XERCES_CPP_NAMESPACE_QUALIFIER DOMDocument* getXmlPayload() const;
			ManagedBuffer* getBinPayload() const;
			
			BatchItem::BATCHITEM_TYPE getPayloadType() const { return m_PayloadType; }
			
			int getSequence() const { return m_Sequence; }
			void setSequence( const int sequence ) { m_Sequence = sequence; }
			
			string getBatchId() const { return m_BatchId; }
			void setBatchId( const string& batchId ) { m_BatchId = batchId; }
			
			string getMessageId() const { return m_MessageId; }
			void setMessageId( const string& messageId ) { m_MessageId = messageId; }

			string getEyecatcher() const { return m_Eyecatcher; }	
			void setEyecatcher( const string& eyecatcher ) { m_Eyecatcher = eyecatcher; }	
					
			void setLast( const bool flag = true ) { m_IsLast = flag; }
			bool isLast() const { return m_IsLast; }

			time_t getCreateDate() const { return m_CreateDate; }
			void setCreateDate( const time_t createDate ) { m_CreateDate = createDate; }
			
			friend bool operator !=( const BatchItem& lparamBI, const BatchItem& rparamBI );
			
			static const int FIRST_IN_SEQUENCE;
			static const int LAST_IN_SEQUENCE;
			static const int INVALID_SEQUENCE;
			
			static BatchItem first() { return BatchItem( BatchItem::FIRST_IN_SEQUENCE, Collaboration::EmptyGuid(), Collaboration::EmptyGuid() ); }
			static BatchItem last() { return BatchItem( BatchItem::FIRST_IN_SEQUENCE, Collaboration::EmptyGuid(), Collaboration::EmptyGuid() ); }

		private :
		
			string m_Eyecatcher;
			string m_Payload;
			ManagedBuffer* m_BinPayload;
			
			XERCES_CPP_NAMESPACE_QUALIFIER DOMDocument* m_XmlPayload;
			// m_PayloadTupe = false - string, true - XML
			BatchItem::BATCHITEM_TYPE m_PayloadType;
			
			time_t m_CreateDate;
			int m_Sequence;
			string m_BatchId;
			string m_MessageId;
			bool m_IsLast;
			bool m_XmlOwner;
	};
}

#endif // BATCHITEM_H
