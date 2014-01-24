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

#ifndef DBWATCHER_H
#define DBWATCHER_H

#include <string>
#include <pthread.h>

#include "DllMain.h"
#include "../AbstractWatcher.h"
#include "../InstrumentedObject.h"

#include "DatabaseProvider.h"
#include "ConnectionString.h"

using namespace std;

namespace FinTP
{
	class ExportedObject DbWatcher : public AbstractWatcher, public InstrumentedObject
	{
		public :
		
			enum WatchOptions
			{
				ReturnDataSet = 1,
				ReturnCursorXml = 2
			};

			explicit DbWatcher( void ( *callback )( const NotificationObject* ), const bool fullObjectNotif = false );
			explicit DbWatcher( NotificationPool* notificationPool, const bool fullObjectNotif = false );
			DbWatcher( NotificationPool* notificationPool, const ConnectionString& connectionString, bool fullObjectNotif = false );
			~DbWatcher();
			
			//set connection string for the watch table
			void setConnectionString( const ConnectionString& connectionString );
			void setFullObjectNotification( const bool newValue ) { m_FullObjectNotif = newValue; }
			
			//set/get SP (stored procedure) name associated
			//where SP = execute query to verify if any records available in the watch table
			void setSelectSPName( const string& spName );
			string getSelectSPName() const;

			//void setMaxUncommitedTrns( const int maxUncommit ) { m_MaxUncommitedTrns = maxUncommit; }
			
			void setProvider( const string& provider );
			DatabaseProvider::PROVIDER_TYPE getProvider() const;

			void setWatchOptions( int options ) { m_WatchOptions = options; }
		
		protected :
				
			void internalScan();

		private :
				
			//int m_UncommitedTrns, m_MaxUncommitedTrns;
			int m_WatchOptions;
			bool m_FullObjectNotif;
			string m_SelectSPName; 
			DatabaseProvider::PROVIDER_TYPE m_DatabaseProvider;
			ConnectionString m_ConnectionString;
	};
}

#endif
