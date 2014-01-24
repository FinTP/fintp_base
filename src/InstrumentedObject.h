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

#ifndef INSTRUMENTEDOBJECT_H
#define INSTRUMENTEDOBJECT_H

#include "DllMain.h"

#include <vector>
#include <map>
#include <iostream>
#include <sstream>

#include <pthread.h>

#define INIT_COUNTERS( instrumented_obj, intrumented_object_name ) \
{\
	int mutexLockResult##intrumented_object_name = pthread_mutex_lock( &( InstrumentedObject::ObjMutex ) ); \
	if ( 0 != mutexLockResult##intrumented_object_name ) \
	{ \
		TRACE( "Unable to lock mutex for "#intrumented_object_name" [" << mutexLockResult##intrumented_object_name << "]" ); \
	}; \
	InstrumentedObject::Instance.registerCounter( typeid( *this ).name(), "_"#instrumented_obj, instrumented_obj ); \
	int mutexUnlockResult##intrumented_object_name = pthread_mutex_unlock( &( InstrumentedObject::ObjMutex ) ); \
	if ( 0 != mutexUnlockResult##intrumented_object_name ) \
	{ \
		TRACE( "Unable to unlock mutex for "#intrumented_object_name" [" << mutexUnlockResult##intrumented_object_name << "]" ); \
	}; \
}

typedef map< string, unsigned long > IntstrumentedObject_CounterType;

#define INIT_COUNTER( counterName ) ( void )m_Counters.insert( pair< string, unsigned long >( #counterName, 0 ) )
#define DESTROY_COUNTER( counterName ) if ( m_Counters.find( #counterName ) != m_Counters.end() ) ( void )m_Counters.erase( #counterName )
#define COUNTER( counterName ) m_Counters[ #counterName ]

#define INCREMENT_COUNTER( counterName ) COUNTER( counterName ) = COUNTER( counterName )+1
#define INCREMENT_COUNTER_ON( instance, counterName ) instance->COUNTER( counterName ) = instance->COUNTER( counterName )+1
#define INCREMENT_COUNTER_ON_T( instance, counterName ) 
#define RESET_COUNTER( counterName ) COUNTER( counterName ) = 0;
#define ASSIGN_COUNTER( counterName, value ) COUNTER( counterName ) = value;

namespace FinTP
{
	class ExportedObject InstrumentedObject
	{
		private :
			InstrumentedObject( const InstrumentedObject& obj );
			InstrumentedObject& operator=( const InstrumentedObject& obj );

		protected :
			map< string, unsigned long > m_Counters;

			map< string, const InstrumentedObject* > m_RegisteredObjects;
			unsigned int m_RegisterCounter;
			vector< string > m_CollectedReports;
					
			InstrumentedObject();
			virtual ~InstrumentedObject(){};

			string InternalGetCounters() const;

		public:

			static InstrumentedObject Instance;
			static pthread_mutex_t ObjMutex;

			static unsigned int GetIntrumentedInstanceNo()
			{
				return Instance.m_RegisteredObjects.size();
			}

			static void RemoveAllCounters()
			{
				Instance.m_RegisteredObjects.clear();
				Instance.m_RegisterCounter = 0;
			}

			void registerCounter( const string& ownerTypeName, const string& counterName, const InstrumentedObject* iobject );
			static string Report();
			static string Collect();
	};
}

#endif //INSTRUMENTEDOBJECT_H
