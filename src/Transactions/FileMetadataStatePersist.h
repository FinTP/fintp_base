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

#ifndef FILEMETADATASTATEPERSIST_H
#define FILEMETADATASTATEPERSIST_H

#include "AbstractStatePersistence.h"

namespace FinTP
{
	// persists metadata to a file.
	class ExportedObject FileMetadataStatePersist : public AbstractStatePersistence
	{
		protected :

			// persists a setting ( storageData ) to a file container
			// storageKey = name of the metadata file ( container )
			// storageData = key-value pair to persist
			void internalSet( const string& storageKey, DictionaryEntry storageData );
			
			// retrieves a name-value pair from the file container
			// storageKey = name of the metadata file ( container )
			// key = key of the pair to retrieve
			DictionaryEntry internalGet( const string& storageKey, const string& key );
			
			int internalGetItemCount( const string& storageKey ) const { return -1; }
			
			// creates a new storage ( metadata file )
			void internalInitStorage( const string& storageKey );
			
			// destroys the storage ( removes metadata file )
			void internalReleaseStorage( const string& storageKey );
			
		public : 
			
			FileMetadataStatePersist();
			~FileMetadataStatePersist();
	};
}

#endif // FILEMETADATASTATEPERSIST_H
