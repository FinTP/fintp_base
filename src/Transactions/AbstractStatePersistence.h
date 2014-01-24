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

#ifndef ABSTRACTSTATEPERSISTENCE_H
#define ABSTRACTSTATEPERSISTENCE_H

#include <string>
#include <set>

#include "Collections.h"
#include "DllMain.h"

using namespace std;

namespace FinTP
{
	//EXPIMP_TEMPLATE template class ExportedObject std::set< string >;

	class ExportedObject AbstractStatePersistence
	{
		protected :
			set< string > m_Storages;
		
			// methods to be overriden by derived classes to implement the actual data storage/retrieval
			virtual void internalSet( const string& storageKey, DictionaryEntry storageData ) = 0;
			virtual DictionaryEntry internalGet( const string& storageKey, const string& key ) = 0;

			virtual int internalGetItemCount( const string& storageKey ) const = 0;
			
			virtual void internalInitStorage( const string& storageKey ) = 0;
			virtual void internalReleaseStorage( const string& storageKey ) = 0;
			
			AbstractStatePersistence();
			
		public :
		
			virtual ~AbstractStatePersistence();

			AbstractStatePersistence( const AbstractStatePersistence& source )
			{
				m_Storages.clear();
				m_Storages = source.m_Storages;
			}

			AbstractStatePersistence& operator=( const AbstractStatePersistence& source )
			{
				if ( this == &source )
					return *this;
				m_Storages.clear();
				m_Storages = source.m_Storages;
				return *this;
			}
			
			// storage init/release
			void InitStorage( const string& storageKey );
			void ReleaseStorage( const string& storageKey );
		
			// accessors for string data
			void Set( const string& storageKey, const string& key, const string& value );
			string GetString( const string& storageKey, const string& key );
			
			// accessors for int data
			void Set( const string& storageKey, const string& key, const int value );
			int GetInt( const string& storageKey, const string& key );
			
			int getItemCount( const string& storageKey );
	};
}

#endif // ABSTRACTSTATEPERSISTENCE_H
