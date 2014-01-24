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

#ifndef FILTERCHAIN_H
#define FILTERCHAIN_H

#ifdef WIN32
	#define __MSXML_LIBRARY_DEFINED__
#endif

#include <vector>
#include <map>
#include <string>
#include "AbstractFilter.h"

using namespace std;

namespace FinTP
{
	//EXPIMP_TEMPLATE template class ExportedObject std::vector< AbstractFilter* >;
	//EXPIMP_TEMPLATE template class ExportedObject std::vector< AbstractFilter::FilterMethod >;
	//EXPIMP_TEMPLATE template class ExportedObject std::map< AbstractFilter::FilterMethod, vector< AbstractFilter::FilterMethod >* >;

	typedef pair< AbstractFilter::FilterMethod, vector< AbstractFilter::FilterMethod >* > FilterChainPair;

	class ExportedObject FilterChain : public AbstractFilter
	{
		public:
			FilterChain();
			~FilterChain();
			
			// adds a filter and returns its index
			int AddFilter( FilterType::FilterTypeEnum type, NameValueCollection* properties	);

			// access a filter based on its index
			AbstractFilter* operator[]( int i ) { return m_Filters[ i ]; }
			
			//overrides
			bool canLogPayload() { return true; }
			
			AbstractFilter::FilterResult ProcessMessage( XERCES_CPP_NAMESPACE_QUALIFIER DOMDocument* inputOutputData, NameValueCollection& transportHeaders, bool asClient );
			AbstractFilter::FilterResult ProcessMessage( const XERCES_CPP_NAMESPACE_QUALIFIER DOMDocument* inputData, AbstractFilter::buffer_type outputData, NameValueCollection& transportHeaders, bool asClient );
			AbstractFilter::FilterResult ProcessMessage( AbstractFilter::buffer_type inputData, XERCES_CPP_NAMESPACE_QUALIFIER DOMDocument* outputData, NameValueCollection& transportHeaders, bool asClient );
			AbstractFilter::FilterResult ProcessMessage( AbstractFilter::buffer_type inputData, AbstractFilter::buffer_type outputData, NameValueCollection& transportHeaders, bool asClient );
			
			AbstractFilter::FilterResult ProcessMessage( const XERCES_CPP_NAMESPACE_QUALIFIER DOMDocument* inputData, unsigned char** outputData, NameValueCollection& transportHeaders, bool asClient )
			{
				throw FilterInvalidMethod( AbstractFilter::XmlToBuffer );
			}

			AbstractFilter::FilterResult ProcessMessage( AbstractFilter::buffer_type inputData, unsigned char** outputData, NameValueCollection& transportHeaders, bool asClient )
			{
				throw FilterInvalidMethod( AbstractFilter::BufferToBuffer );
			}

			// return true if the filter can execute the requested operation in client/server context
			bool isMethodSupported( FilterMethod method, bool asClient, bool untilNow = false );
			
			void Report( bool onlySupported = false, bool displayChain = false );
			
			void Rollback();
			void Commit();
			void Abort();
			
		private :
		
			vector< AbstractFilter* > m_Filters;
			
			map< AbstractFilter::FilterMethod, vector< AbstractFilter::FilterMethod >* > m_CompiledChainsAsClient;
			map< AbstractFilter::FilterMethod, vector< AbstractFilter::FilterMethod >* > m_CompiledChainsAsServer;
			
			bool BuildChain( AbstractFilter::FilterMethod method, AbstractFilter::FilterMethod chainMethod, bool asClient, const unsigned int index = 0 );
			void BuildChains();
			
			bool isFirstFilter( int index ) const { return index == 0; }
			bool isLastFilter( int index ) const { return index == m_Filters.size() - 1; }
			
			//FilterMethod getFilterMethod( const int filterIndex ) const;
	};
}

#endif // FILTERCHAIN_H
