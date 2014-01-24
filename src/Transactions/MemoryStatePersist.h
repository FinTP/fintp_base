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

#ifndef MEMORYSTATEPERSIST_H
#define MEMORYSTATEPERSIST_H

#include "AbstractStatePersistence.h"
#include <map>

namespace FinTP
{
	//EXPIMP_TEMPLATE template class ExportedObject std::map< string, NameValueCollection >;

	class ExportedObject MemoryStatePersist : public AbstractStatePersistence
	{
		private :	
			
			map< string, NameValueCollection > m_Data;
			
		protected :
		
			void internalSet( const string& storageKey, DictionaryEntry storageData );
			DictionaryEntry internalGet( const string& storageKey, const string& key );
			int internalGetItemCount( const string& storageKey ) const;
		
			void internalInitStorage( const string& storageKey );
			void internalReleaseStorage( const string& storageKey );
			
		public :
				
			MemoryStatePersist() : AbstractStatePersistence()
			{
			}

			MemoryStatePersist( const MemoryStatePersist& source ) : AbstractStatePersistence( source )
			{
				m_Data = source.m_Data;
			}

			MemoryStatePersist& operator=( const MemoryStatePersist& source )
			{
				if ( this == &source )
					return *this;
				AbstractStatePersistence::operator=( source );

				m_Data.clear();
				m_Data = source.m_Data;
				return *this;
			}

			~MemoryStatePersist()
			{
				try
				{
					m_Data.clear();
				}
				catch( ... ){}
			}
	};
}

#endif // MEMORYSTATEPERSIST_H
