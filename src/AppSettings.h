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

#ifndef APPSETTINGS_H
#define APPSETTINGS_H

#include <string>
#include <map>

#include <xercesc/dom/DOM.hpp>
//#include <xercesc/dom/DOMBuilder.hpp>
//#include <xercesc/parsers/DOMBuilderImpl.hpp>
#include <xercesc/sax/ErrorHandler.hpp>
//#include <xercesc/parsers/DOMBuilderImpl.hpp>

#include "DllMain.h"
#include "Collections.h"

XERCES_CPP_NAMESPACE_USE
using namespace std;

namespace FinTP
{
	typedef pair< std::string, NameValueCollection > SectionEntry;

	//EXPIMP_TEMPLATE template class ExportedObject std::map< std::string, NameValueCollection >;

	class ExportedObject AppSettings
	{
		public :
			
			enum ConfigPrefix
			{
				None
			};

			/** 
			 * Contains config file entries for qPI connectors
			 */
			enum ConfigSettings
			{
				// Common settings
				/**
				 * Config name : <b>LogPrefix</b>
				 * Prefix for log files ( may be relative or absolute ), should be in the form [<folder><path_separator>]<prefix>
				 */
				LOGPREFIX,
				/**
				 * Config name : <b>LogMaxLines</b>
				 * Max log lines. After this limit is reached, the log will be rewriten from the beginning
				 */
				LOGMAXLINES
			};

		private:
		
			//	member variables		
			NameValueCollection m_Settings;
			map< std::string, NameValueCollection > m_Sections;
			map< std::string, void ( * )( DOMElement* ) > m_Readers;
		
		public:
		
			//constructors
			AppSettings( void );
			explicit AppSettings( const string& configFileName, const string& configContent = "" );
		
			//destructor
			~AppSettings();
		
			// attach callback methods for sections
			void setSectionReader( const string& sectionName, void ( *callback )( DOMElement* ) );

			void Dump( void ) const;
			void loadConfig( const string& configFilename, const string& configContent = "" );
			
			const NameValueCollection& getSettings() const { return m_Settings; }
			const map< std::string, NameValueCollection >& getSections() const { return m_Sections; }
					
			bool ContainsSection( const string& section );
			
			NameValueCollection& getSection( const string& sectionName );
			string getSectionAttribute( const string& sectionName, const string& attributeName );
			
			//operators
			string operator[] ( const string& key ) const;

			static string getName( const ConfigPrefix prefix, const ConfigSettings setting );

			void readFiltersForConnector( const DOMNode* filters, const string& connType );
	};
}

#endif
