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

#ifndef MQWATCHER_H
#define MQWATCHER_H

#include <string>
#include <pthread.h>

#include "../DllMain.h"
#include "../AbstractWatcher.h"
#include "../InstrumentedObject.h"
#include "TransportHelper.h"

using namespace std;

namespace FinTP
{
	class ExportedObject MqWatcher : public AbstractWatcher, public InstrumentedObject
	{
		public :

			enum WatchOptions
			{
				NotifyGroups = 1,
				NotifyMessage = 2,
				NotifyUnique = 4
			};
			
			explicit  MqWatcher( NotificationPool* notificationPool, const string& queue = "", const string& queueManager = "", const string& channelDefinition = "", const string& connectionString = "" );
			
			~MqWatcher();
			
			void setHelperType ( const TransportHelper::TRANSPORT_HELPER_TYPE& helperType );
			void setQueue( const string& queue );
			void setQueueManager( const string& queue );
			void setTransportURI( const string& transportURI );
			void setWatchOptions( int options ) { m_WatchOptions = options; }

			void setSSLCypherSpec( const string& cypherSpec ) { m_SSLCypherSpec = cypherSpec; }
			void setSSLPeerName( const string& peerName ) { m_SSLPeerName = peerName; }
			void setSSLKeyRepository( const string& keyRepository ) { m_SSLKeyRepos = keyRepository; }

		protected :
				
			void internalScan();

		private :

			int m_WatchOptions;
			string m_Queue, m_QueueManager, m_TransportURI;
			string m_SSLKeyRepos, m_SSLCypherSpec, m_SSLPeerName;
			TransportHelper::TRANSPORT_HELPER_TYPE m_HelperType;
	};
}

#endif //MQWATCHER_H
