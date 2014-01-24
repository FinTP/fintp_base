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

#include <sstream>
#include <cstdlib>
#include "AbstractStatePersistence.h"
#include "Trace.h"

using namespace FinTP;

AbstractStatePersistence::AbstractStatePersistence()
{
	DEBUG( "CONSTRUCTOR" );
}

AbstractStatePersistence::~AbstractStatePersistence()
{
	try
	{
		DEBUG( "DESTRUCTOR" );
		m_Storages.clear();
	} catch( ... ){}
}

void AbstractStatePersistence::InitStorage( const string& storageKey )
{
	internalInitStorage( storageKey );
}

void AbstractStatePersistence::ReleaseStorage( const string& storageKey )
{
	internalReleaseStorage( storageKey );
	( void )m_Storages.erase( storageKey );
}

void AbstractStatePersistence::Set( const string& storageKey, const string& key, const string& value )
{
	set< string >::const_iterator finder = m_Storages.find( storageKey );
	
	// no storage found
	if( finder == m_Storages.end() )
	{
		InitStorage( storageKey );
		( void )m_Storages.insert( storageKey );
	}
	
	internalSet( storageKey, DictionaryEntry( key, value ) );
}

string AbstractStatePersistence::GetString( const string& storageKey, const string& key ) 
{
	set< string >::const_iterator finder = m_Storages.find( storageKey );
	
	// no storage found
	if( finder == m_Storages.end() )
		return "";
	
	return internalGet( storageKey, key ).second;
}
		
void AbstractStatePersistence::Set( const string& storageKey, const string& key, const int value )
{
	// convert value to string
	stringstream valueAsString;
	valueAsString << value;
	
	// use string method
	Set( storageKey, key, valueAsString.str() );
}

int AbstractStatePersistence::GetInt( const string& storageKey, const string& key )
{
	string returnVal = GetString( storageKey, key ).data();
	
	// enmpty string
	if( returnVal.length() == 0 )
		return 0;
		
	return atoi( returnVal.data() );
}

int AbstractStatePersistence::getItemCount( const string& storageKey )
{
	set< string >::const_iterator finder = m_Storages.find( storageKey );
	
	// no storage found
	if( finder == m_Storages.end() )
		return -1;
		
	return internalGetItemCount( storageKey );
}
