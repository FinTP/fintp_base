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

#ifndef BATCHMANAGER_H
#define BATCHMANAGER_H

//#include "../XmlUtil.h"
//#include "../XPathHelper.h"
#include "../XSLT/XSLTFilter.h"
#include "../Transactions/MemoryStatePersist.h"

//#include "DllMain.h"

#include "BatchConfig.h"
#include "BatchItem.h"
#include "BatchResolution.h"

// // usage scenario :
// // 1. get messages from flatfile ( CEC - input flatfile use case, ACH[FPM] )
//
// BatchManager<BatchFlatfileStorage> batchManager( BatchResolution::SYNCHRONOUS );
// batchManager.open( flatFilename, ios_base::in );
//
// BatchManager<BatchFlatfileStorage>::iterator enumerator = batchManager.first();
//
// while( enumerator != batchManager.end() )
// {
//		ProcessMessage( enumerator.str() );
//		enumerator++;
// }

// // 2. get messages from XML file ( ACH[FTM] - input XML use case )
//
// BatchManager<BatchXMLStorage> batchManager( BatchResolution::SYNCHRONOUS );
// batchManager.open( xmlFilename, ios_base::in );
//
// BatchManager<BatchXMLStorage>::iterator enumerator = batchManager.first();
//
// while( enumerator != batchManager.end() )
// {
//		ProcessMessage( enumerator.str() );
//		enumerator++;
// }

// // 3. get messages from WMQ ( BCR - output MT950 use case )
// // the BatchManager will get only the complete message ( all in group ). the app will process 
// //	all messages in group one by one. It is the reponsability of the BatchManager to serve them in order
//
// BatchManager<BatchMQStorage> batchManager( BatchResolution::SYNCHRONOUS );
// batchManager.storage().setQueue( batchQueue );
// batchManager.storage().setQueueManager( batchQueueManager );
// batchManager.storage().setTransportURI( batchChannelDefinition );
// batchManager.open( messageGroupId, ios_base::in );
//
// BatchManager<BatchMQStorage>::iterator enumerator = batchManager.first();
// 
// while( enumerator != batchManager.end() )
// {
//		ProcessMessage( enumerator.str() );
//		enumerator++;
// }

// // 4. synchronously write messages to flatfile ( CEC - output flatfile use case ?, ACH[FMP] )
//
// BatchManager<BatchFlatfileStorage> batchManager( BatchResolution::SYNCHRONOUS );
// batchManager.open( flatFilename, ios_base::out );
//
// while( messagesLeft )
// {
//		batchManager << messageAsString;
// }

// // 5. synchronously write messages to XML file ( ACH[FTM] - output use case )
//
// BatchManager<BatchXMLStorage> batchManager( BatchResolution::SYNCHRONOUS );
// batchManager.open( xmlFilename, ios_base::out );
//
// while( messagesLeft )
// {
//		batchManager << messageAsString;
// }

// // 6. synchronously write messages to message queue
//
// BatchManager<BatchMQStorage> batchManager( BatchResolution::SYNCHRONOUS );
// batchManager.storage().setQueue( batchQueue );
// batchManager.storage().setQueueManager( batchQueueManager );
// batchManager.storage().setTransportURI( batchChannelDefinition );
// batchManager.open( messageGroupId, ios_base::in );
//
// while( messagesLeft )
// {
//		batchManager << messageAsString;
// }

// RoutingEngine use case

// // 7. ACH connector[ FPM ] use case
// ... in fetcher Process ...
// see use case 1.
// 
// ... in publisher Process ...
// see use case 4.

// // 8. CEC connector use case 
// ... see use case 7.

// // asynchronously write messages to 
//
// void ReleaseBatch( string batchId )
//
// BatchManager<BatchFlatfileStorage> batchManager( BatchResolution::ASYNCHRONOUS );
// batchManager.open( flatFilename, ios_base::out );
//
// batchManager.setReleaseCallback( &releaseCallback ); // check race conditions
//
// while( messagesLeft )
// {
//		BatchResolution resolution = batchManager << messageAsString;
//		if 
// }

// ::setReleaseCallback( &releaseFunction );
namespace BatchManip
{
	class BufferSize
	{
		public : 
			explicit BufferSize( const int size ){ m_Size = size; };
			int getSize() const { return m_Size; }
				
		private :	
			int m_Size;
	};
	
	class BatchMetadata
	{
		public :
			explicit BatchMetadata( const XERCES_CPP_NAMESPACE_QUALIFIER DOMDocument* const document, const string eyecatcher = "", const string batchId = "", const string batchtransform = "" ) :
			   m_Transform( batchtransform ), m_EyeCatcher( eyecatcher ), m_BatchId( batchId )
			{
				m_Document = document; 
			}
			
			const XERCES_CPP_NAMESPACE_QUALIFIER DOMDocument* getDocument() const { return m_Document; }
			string getEyeCatcher() const { return m_EyeCatcher; }
			string getTransform() const { return m_Transform; }
			string getBatchId() const { return m_BatchId; }

		private :

			const XERCES_CPP_NAMESPACE_QUALIFIER DOMDocument* m_Document;
			string m_Transform;
			string m_EyeCatcher;
			string m_BatchId;
	};
	
	// stream modifiers 
	ExportedObject BatchManip::BufferSize setw( int size );
	ExportedObject BatchManip::BatchMetadata setmeta( XERCES_CPP_NAMESPACE_QUALIFIER DOMDocument* const document );
}

namespace FinTP
{
	class ExportedObject BatchManagerBase
	{
		public :

			typedef enum 
			{
				BATCH_NEW = 0,
				BATCH_INPROGRESS = 10,
				BATCH_READY = 15,
				BATCH_FAILED = 20,
				BATCH_COMPLETED = 30
			} BATCH_STATUS;

			enum StorageCategory
			{
				Flatfile,
				XMLfile,
				WMQ,
				AMQ,
				ZIP
			};
			
			static BatchManagerBase* CreateBatchManager( const BatchManagerBase::StorageCategory storageCategory );

			virtual ~BatchManagerBase(){};

			// open storage
			virtual void open( const string& storageId, ios_base::openmode openMode ) = 0;
			
			//close storage
			virtual void close( const string& storageId ) = 0;

			//transaction support
			virtual void commit() = 0;
			virtual void rollback() = 0;

			// add to storage
			const BatchManagerBase& operator << ( const string& document );
			
			// manip inserts
			const BatchManagerBase& operator << ( BatchManip::BatchMetadata metadata );
			const BatchManagerBase& operator << ( const BatchManip::BufferSize size );
			const BatchManagerBase& operator << ( const BatchItem& item );
			
			// get from storage
			void operator >> ( BatchItem &item );
			
			// returns true if more messages can be got/put from/in the storage
			bool moreMessages() const { return m_MoreMessages; }
					
			// item discrimination
			void setXPath( const string& xpath ) { m_XPath = xpath; };
			string getXPath() const { return m_XPath; };

			BatchManagerBase::StorageCategory getStorageCategory() const { return m_StorageCategory; }

			static string ToString( const BatchManagerBase::BATCH_STATUS status );
			
		protected :

			virtual void internalEnqueue( const string& document ) = 0;		
			virtual void internalEnqueue( XERCES_CPP_NAMESPACE_QUALIFIER DOMDocument* const document ) = 0;
			virtual void internalEnqueue( BatchManip::BatchMetadata metadata ) = 0;
			virtual void internalEnqueue( const BatchManip::BufferSize size ) = 0;
			virtual void internalEnqueue( const BatchItem &item ) = 0;

			virtual void internalDequeue( BatchItem &item  ) = 0;
			
			string m_XPath;
			BatchManagerBase::StorageCategory m_StorageCategory;
			bool m_MoreMessages;
			XSLTFilter m_TransformFilter;
			const XERCES_CPP_NAMESPACE_QUALIFIER DOMDocument* m_MetaDoc;
			
			explicit BatchManagerBase( const BatchManagerBase::StorageCategory storageCategory );
	};

	template < class T >
	class ExportedObject BatchManager : public BatchManagerBase
	{		
		public:

			BatchManager( const BatchManagerBase::StorageCategory storageCategory, const BatchResolution::BatchThreadModel threadModel );
			~BatchManager();
			
			// open/close storage
			void open( const string& storageId, ios_base::openmode openMode );
			void close( const string& storageId );

			void commit() { m_Storage.commit(); }
			void rollback() { m_Storage.rollback(); }
			
			// accessors
			T& storage() { return m_Storage; }
			
			// iterators
			BatchItem begin() const { return T::begin(); }
			BatchItem end() const { return T::end(); }
			
			// utils
			long size() const { return m_Storage.size(); }
			
			void setConfig( const string& configFile );
			const BatchResolution& getResolution() { return m_CrtResolution; }
			void setResolution( const BatchResolution& resolution ) { m_CrtResolution = resolution; }
			
		protected :

			void internalEnqueue( const string& document );
			void internalEnqueue( XERCES_CPP_NAMESPACE_QUALIFIER DOMDocument* const document );
			void internalEnqueue( BatchManip::BatchMetadata metadata );
			void internalEnqueue( const BatchManip::BufferSize size );
			void internalEnqueue( const BatchItem& item );
			
			void internalDequeue( BatchItem &item );
			
		private :
					
			BatchResolution::BatchThreadModel m_ThreadModel;
			ios_base::openmode m_OpenMode;
			BatchConfig m_Config;
			
			T m_Storage;
			string m_StorageId;
			
			BatchResolution m_CrtResolution;
			//MemoryStatePersist m_MetaStorage;
	};
}

#endif // BATCHMANAGER_H
