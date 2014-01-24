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

#ifndef BATCHSTORAGEBASE_H
#define BATCHSTORAGEBASE_H

#include <string>
#include <vector>
#include <iterator>

using namespace std;

#include "BatchResolution.h"

namespace FinTP
{
	class ExportedObject BatchStorageBase
	{
		public:
		
			virtual ~BatchStorageBase();
			
			// add to storage
			virtual void enqueue( BatchResolution& resolution ) = 0;
			
			// get from storage
			virtual BatchItem dequeue() = 0;
			
			// open/close storage
			virtual void open( const string& storageId, ios_base::openmode openMode ) = 0;
			virtual void close( const string& storageId ) = 0;

			virtual void commit() = 0;
			virtual void rollback() = 0;
			
			// utils
			virtual long size() const = 0;
			
			static BatchItem begin() { return BatchItem::first(); }
			static BatchItem end() { return BatchItem::last(); }
			
			virtual void setBufferSize( const unsigned long size ) = 0;
		protected :
		
			BatchStorageBase();
			unsigned long m_Size;
	};
}

#endif // BATCHSTORAGEBASE_H
