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

#ifndef FILESYSTEMWATCHER_H
#define FILESYSTEMWATCHER_H

#include <pthread.h>
#include <vector>
#include <string>

#include "../DllMain.h"
#include "../AbstractWatcher.h"

using namespace std;

namespace FinTP
{
	class ExportedObject FsWatcher : public AbstractWatcher
	{
		public :
			enum WatchOptions
			{
				NotifyUnique = 1,
				SkipEmptyFiles = 2
			};
			
			explicit FsWatcher( void ( *callback )( const NotificationObject* ), const string& path = ".", const string& filter = "" );
			explicit FsWatcher( NotificationPool* notificationPool, const string& path = ".", const string& filter = "" );
			~FsWatcher();

			void setFilter( const string& filter );
			void setPath( const string& path );
		
			void setWatchOptions( int options ) { m_WatchOptions = options; }

		protected :
				
			void internalScan();
			void initScanPaths( const string& paths );

		private :

			int m_WatchOptions;
			string m_Folder;
			string m_Filter;
			vector< string > m_ScanPaths;
	};
}

#endif
