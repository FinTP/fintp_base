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

#ifndef ABSTRACTWATCHER_H
#define ABSTRACTWATCHER_H

#include "DllMain.h"
#include "WorkItemPool.h"

namespace FinTP
{
	class ExportedObject AbstractWatcher
	{
		public : // inner class definitions
			class ExportedObject NotificationObject
			{
				public :
					enum NotificationType
					{
						TYPE_XMLDOM,
						TYPE_CHAR
					};

				private : 
					string m_ObjectId, m_ObjectGroupId;
					unsigned long m_ObjectSize;
					void* m_Object;
					NotificationType m_ObjectType;

				public :

					explicit NotificationObject( const string& objectId, const string& objectGroupId = "", 
						const unsigned long objectSize = 0, NotificationType objectType = NotificationObject::TYPE_XMLDOM ) : 
						m_ObjectId( objectId ), m_ObjectGroupId( objectGroupId ), m_ObjectSize( objectSize ), 
						m_Object( NULL ), m_ObjectType( objectType )
					{}

					NotificationObject( const string& objectId, void* object, const string& objectGroupId = "",
						const unsigned long objectSize = 0, NotificationType objectType = NotificationObject::TYPE_XMLDOM ) :
						m_ObjectId( objectId ), m_ObjectGroupId( objectGroupId ), m_ObjectSize( objectSize ), 
						m_Object( object ), m_ObjectType( objectType )
					{}

					NotificationObject( const NotificationObject& source ) :
						m_ObjectId( source.m_ObjectId ), m_ObjectGroupId( source.m_ObjectGroupId ), m_ObjectSize( source.m_ObjectSize ),
						m_Object( source.m_Object ), m_ObjectType( source.m_ObjectType )
					{
					}

					NotificationObject& operator=( const NotificationObject& source )
					{
						if ( this == &source )
							return *this;

						m_ObjectId = source.m_ObjectId;
						m_ObjectGroupId = source.m_ObjectGroupId;
						m_ObjectSize = source.m_ObjectSize;
						m_Object = source.m_Object;
						m_ObjectType = source.m_ObjectType;

						return *this;
					}

					string getObjectId() const { return m_ObjectId; }
					
					void* getObject() const { return m_Object; }
					template< typename T >
					T* getObject() const { return dynamic_cast< T* >( m_Object ); }

					NotificationType getObjectType() const { return m_ObjectType; }
					string getObjectGroupId() const { return m_ObjectGroupId; }
					unsigned long getObjectSize() const { return m_ObjectSize; }
			};

		public :
			typedef WorkItemPool< NotificationObject > NotificationPool;

			void setCallback( void ( *callback )( const NotificationObject* ) )
			{
				m_Callback = callback;
			}

			void setNotificationPool( NotificationPool* notificationPool )
			{
				m_NotificationPool = notificationPool;
			}

			void setIdleCallback( void ( *callback )( void ), unsigned int seconds )
			{
				m_IdleTimeout = seconds;
				m_IdleCallback = callback;
			}

		protected : 

			explicit AbstractWatcher( NotificationPool* notificationPool, void *( *prepareCallback )( void *object ) = NULL ) : 
				m_Callback( NULL ), m_PrepareCallback( prepareCallback ), m_IdleCallback( NULL ),
				m_ScanThreadId( 0 ), m_Enabled( false ), m_NotificationType( NotificationObject::TYPE_XMLDOM ), 
				m_IdleTimeout( 0 ), m_NotificationPool( notificationPool )
				{
				}

			explicit AbstractWatcher( void ( *callback )( const NotificationObject* ), void *( *prepareCallback )( void *object ) = NULL ) :
				m_Callback( callback ), m_PrepareCallback( prepareCallback ),  m_IdleCallback( NULL ),
				m_ScanThreadId( 0 ), m_Enabled( false ), m_NotificationType( NotificationObject::TYPE_XMLDOM ),
				m_IdleTimeout( 0 ), m_NotificationPool( NULL )
				{
				}

			virtual void internalScan() = 0;
			
			void ( *m_Callback )( const NotificationObject *notification );
			void *( *m_PrepareCallback )( void *object );
			void ( *m_IdleCallback )( void );

			pthread_t m_ScanThreadId;
			bool m_Enabled;
			NotificationObject::NotificationType m_NotificationType;
			unsigned int m_IdleTimeout;

			NotificationPool* m_NotificationPool;

		public:
			
			virtual ~AbstractWatcher();		

			void setEnableRaisingEvents( bool val );
			void setObjectPrepareMethod( void *( *prepareCallback )( void* ) );

			void setNotificationType( const NotificationObject::NotificationType notifType ) 
			{ 
				m_NotificationType = notifType;
			}

			pthread_t getThreadId() { return m_ScanThreadId; }

			void waitForExit();

		private :

			static void* ScanInNewThread( void *pThis );
	};
}

#endif //ABSTRACTWATCHER_H
