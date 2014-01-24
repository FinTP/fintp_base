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

#ifndef BATCHZIPSTORAGE_H
#define BATCHZIPSTORAGE_H

#include "../BatchStorageBase.h"
#include "../../Zip/zip.h"
#include "../../Zip/unzip.h"

//TODO: 
// 1. La eroare de formatare, pune in Mq pana la linia respectiva
// 		dupa care muta fisierul in ErrorPath si trimite mesaje de eroare
// 2. Daca este eroare pe prima linie dupa schimbarea Chunk-ului 
//		ultimul mesaj bun nu il pune
//

namespace FinTP
{
	class ExportedObject BatchZipStorage : public BatchStorageBase
	{
		public:
		
			BatchZipStorage();
			~BatchZipStorage();
			
			void enqueue( BatchResolution& resolution );
			BatchItem dequeue();
			
			// open/close storage
			void open( const string& storageId, ios_base::openmode openMode );	
			void close( const string& storageId );
			void removeTemp();
			
			void commit() {}
			void rollback() {};

			//nothing special TO DO
			long size() const { return 0; }
				
			//nothing special TO DO
			void setBufferSize( const unsigned long buffersize ){}
				
			void setBuffer( const unsigned char *zipContents, const unsigned int size );
			ManagedBuffer* getBuffer();
			
		private :
		
			// m_CrtStorage is the batch file under construction or under parse
			HZIP m_CrtUnzipStorage, m_CrtZipStorage;

			// current sequence in the current Storage
			long m_CrtSequence;
			
			string m_CrtStorageId;

			int m_NumberOfItems;
	};
}

#endif // BATCHZIPSTORAGE_H
