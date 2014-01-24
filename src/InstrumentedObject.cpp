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

#include "InstrumentedObject.h"
#include "TimeUtil.h"
#include "StringUtil.h"

#include <cstdlib>
#include <exception>
#include <stdexcept>
#include <typeinfo>

using namespace FinTP;

#if defined ( WIN32 ) && !defined( CRT_SECURE )
#define itoa( value, buffer, base ) _itoa( value, buffer, base )
#endif

pthread_mutex_t InstrumentedObject::ObjMutex = PTHREAD_MUTEX_INITIALIZER;
InstrumentedObject InstrumentedObject::Instance;

InstrumentedObject::InstrumentedObject() : m_RegisterCounter( 0 )
{
}

void InstrumentedObject::registerCounter( const string& ownerTypeName, const string& counterName, const InstrumentedObject* iobject )
{
	string _made_name = ownerTypeName + counterName;

#ifdef WIN32
	char buffer[ 3 ] = {0,0,0};
	#ifdef CRT_SECURE
		_itoa_s( m_RegisterCounter++, buffer, 3, 10 );
	#else
		itoa( m_RegisterCounter++, buffer, 10 );
	#endif

	_made_name.append( buffer );
#else
	_made_name.append( StringUtil::ToString( m_RegisterCounter++ ) );
#endif

	m_RegisteredObjects.insert( pair< string, const InstrumentedObject* >( _made_name, iobject ) );
}

string InstrumentedObject::InternalGetCounters() const
{
	stringstream report;

	map< string, unsigned long >::const_iterator counterWalker = m_Counters.begin();
	while( counterWalker != m_Counters.end() )
	{
		report << "<Counter name=\"" << counterWalker->first << "\">";
		report << counterWalker->second;
		report << "</Counter>";
		counterWalker++;
	}
	return report.str();
}

string InstrumentedObject::Report()
{
	stringstream report;
	report << "<Report>";// xmlns=\"http://tempuri.org/PerformanceReport.xsd\">";
	for( unsigned int i=0; i<Instance.m_CollectedReports.size(); i++ )
	{
		report << Instance.m_CollectedReports[ i ];
	}
	report << "</Report>";
	Instance.m_CollectedReports.clear();
	return report.str();
}

string InstrumentedObject::Collect()
{
	stringstream report;
	report << "<Snapshot time=\"" << TimeUtil::Get( "%d/%m/%Y-%H:%M:%S", 19 ) << "\">"; 
	map< string, const InstrumentedObject* >::const_iterator objectWalker = Instance.m_RegisteredObjects.begin();
	while( objectWalker != Instance.m_RegisteredObjects.end() )
	{
		try
		{
			report << "<Type type=\"" << typeid( objectWalker->second ).name() << 
				"\" name=\"" << StringUtil::Replace( objectWalker->first, "&", "&amp;" ) << "\">";
			report << objectWalker->second->InternalGetCounters();
			report << "</Type>";
		}	
#if !defined( LINUX ) 		
		catch( const __non_rtti_object& ex )
		{
			//cerr << ex.what() << endl;
			report << "<Type name=\"non_rtti_object\"/>";
		}
#endif // ( !LINUX )
		catch( const bad_typeid& ex )
		{
			//cerr << ex.what() << endl;
			report << "<Type name=\"bad_type\"/>";
		}
		objectWalker++;
	}
	report << "</Snapshot>";

	string partReport = report.str();
	Instance.m_CollectedReports.push_back( partReport );
	return partReport;
}
