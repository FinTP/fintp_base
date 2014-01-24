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

#include "MemoryStatePersist.h"
#include "Trace.h"

using namespace FinTP;

void MemoryStatePersist::internalSet( const string& storageKey, DictionaryEntry storageData )
{
	DEBUG2( "Set ( " << storageKey << ", ( " << storageData.first << ", " << storageData.second << " ) )" );
	
	if( ( m_Data[ storageKey ] ).ContainsKey( storageData.first ) )
	{
		( m_Data[ storageKey ] ).Remove( storageData.first );
	}
	
	( m_Data[ storageKey ] ).Add( storageData.first, storageData.second );
	
#ifdef EXTENDED_DEBUG
	( m_Data[ storageKey ] ).Dump();
#endif
}

DictionaryEntry MemoryStatePersist::internalGet( const string& storageKey, const string& key ) 
{
	DEBUG2( "Get ( " << storageKey << ", " << key << " )" );
	if ( ( m_Data[ storageKey ] ).ContainsKey( key ) )
	{
		DEBUG2( "#Specific storage value : " << ( m_Data[ storageKey ] )[ key ] );
		return DictionaryEntry( key, ( m_Data[ storageKey ] )[ key ] );
	}

	return DictionaryEntry( key, "" );
}
		
void MemoryStatePersist::internalInitStorage( const string& storageKey )
{
	// ? does nothing.. the [] operator creates a new item by design
	//map< string, string > emptyStorage;
	
	//	m_Data.insert( key, emptyStorage );
	DEBUG( "InitStorage( " << storageKey << " )" );
}

void MemoryStatePersist::internalReleaseStorage( const string& storageKey )
{
	DEBUG( "ReleaseStorage( " << storageKey << " )" );
	
	// remove the storage
	( void )m_Data.erase( storageKey );
}

int MemoryStatePersist::internalGetItemCount( const string& storageKey ) const
{
	map< string, NameValueCollection >::const_iterator finder = m_Data.find( storageKey );
	if ( finder == m_Data.end() )
		return 0;

	return ( int )finder->second.getCount();
}
