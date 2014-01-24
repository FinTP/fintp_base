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

#ifndef BATCHMQSTORAGE_H
#define BATCHMQSTORAGE_H

#include "../BatchStorageBase.h"
#include "TransportHelper.h"

namespace FinTP
{
	class ExportedObject BatchMQStorage : public BatchStorageBase
	{
		public:
		
			BatchMQStorage( TransportHelper::TRANSPORT_HELPER_TYPE helperType = TransportHelper::WMQ );
			~BatchMQStorage();
			
			void enqueue( BatchResolution& resolution );
			BatchItem dequeue();
			
			// open/close storage
			void open( const string& storageId, ios_base::openmode openMode );	
			void close( const string& storageId );

			void commit() { m_CrtHelper->commit(); }
			void rollback() { m_CrtHelper->rollback(); };

			long size() const { return 0; }
			
			// particular sets/flags
			void setQueue( const string& queue ){ m_Queue = queue; }
			void setQueueManager( const string& queueManager ){ m_QueueManager = queueManager; }
			void setBackupQueue( const string& queue ){ m_BackupQueue = queue; }
			void setTransportURI( const string& chDef ){ m_ChDef = chDef; }
			
			void setAutoAbandon( const int& retries ){ m_CrtHelper->setAutoAbandon( retries ); }
			void setCleaningUp( bool cleanFlag ){ m_IsCleaningUp = cleanFlag; }
			bool getCleaningUp(){ return m_IsCleaningUp; }
			void setBufferSize( const unsigned long buffersize ){ m_BufferSize = buffersize; }
			
		private :
		
			TransportHelper* m_CrtHelper;
			unsigned long m_BufferSize;
			string m_CrtStorageId;
			string m_Queue, m_BackupQueue, m_QueueManager, m_ChDef;
			bool m_IsCleaningUp;
	};
}

#endif // BATCHMQSTORAGE_H
