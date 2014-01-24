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

#ifndef BATCHACTION_H
#define BATCHACTION_H

#include <string>
using namespace std;

#include "DllMain.h"
#include "BatchItem.h"

namespace FinTP
{
	class ExportedObject BatchResolution
	{
		public:
		
			enum BatchAction
			{
				Release,
				Hold
			};
			
			enum BatchThreadModel
			{
				ASYNCHRONOUS,
				SYNCHRONOUS
			};
			
			BatchResolution();
			BatchResolution( BatchResolution::BatchAction resolution, const BatchItem& item );
			~BatchResolution();
			
			// resolution accessors
			BatchResolution::BatchAction getResolution() const { return m_Resolution; }
			void setResolution( const BatchResolution::BatchAction action ) { m_Resolution = action; }

			const BatchItem& getItem() const { return m_Item; }
			void setItem( const BatchItem& item ) { m_Item = item; }

			string getTransform() const { return m_Transform; }
			void setTransform( const string& batchTransform ) { m_Transform = batchTransform; }

			string getIntendedBatchId() const { return m_IntendedBatchId; }
			void setIntendedBatchId( const string& batchid ) { m_IntendedBatchId = batchid; }

			static string ToString( BatchResolution::BatchAction action );

			void setItemPayload( const string& payload ) { m_Item.setPayload( payload ); }
			void setItemXmlPayload( XERCES_CPP_NAMESPACE_QUALIFIER DOMDocument* payload ) { m_Item.setXmlPayload( payload ); }

			void setItemMessageId( const string& messageId ) { m_Item.setMessageId( messageId ); }
			void setItemLast( const bool flag = true ) { m_Item.setLast( flag ); }
			/*void setItemPayload( const char* payload );
			void setItemPayload( const ManagedBuffer *payload );*/
			
		private :
		
			BatchResolution::BatchAction m_Resolution;
			string m_Transform;
			string m_IntendedBatchId;
			BatchItem m_Item;
	};
}

#endif // BATCHACTION_H
