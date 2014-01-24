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

#ifndef XSDFILTER_H
#define XSDFILTER_H

#include "../AbstractFilter.h"

#include <xercesc/util/PlatformUtils.hpp>
#include <xercesc/framework/StdOutFormatTarget.hpp>

namespace FinTP
{
	class ExportedObject XSDFilter : public AbstractFilter
	{
		public:

			static const string XSDFILE ; //XSD FileName get from Properties ( .config file, filter parameter )
			static const string XSDNAMESPACE;
			
			XSDFilter();
			~XSDFilter();
			
			// return true if the filter supports logging payload to a file
			// note for overrides : return true/false if the payload can be read without rewinding ( not a stream )
			bool canLogPayload();
			
			// return true if the filter can execute the requested operation in client/server context
			bool isMethodSupported( FilterMethod method, bool asClient );
			
			// if DOMDocument/buffer is validated returns DOMDocument
			// if not throw an exception
			FilterResult ProcessMessage( XERCES_CPP_NAMESPACE_QUALIFIER DOMDocument* inputOutputData, NameValueCollection& transportHeaders, bool asClient );
			FilterResult ProcessMessage( unsigned char* inputData, XERCES_CPP_NAMESPACE_QUALIFIER DOMDocument* outputData, NameValueCollection& transportHeaders, bool asClient );
			
			FilterResult ProcessMessage( const XERCES_CPP_NAMESPACE_QUALIFIER DOMDocument* inputData, AbstractFilter::buffer_type outputData, NameValueCollection& transportHeaders, bool asClient )
			{
				throw FilterInvalidMethod( AbstractFilter::BufferToXml );
			}

			FilterResult ProcessMessage( const XERCES_CPP_NAMESPACE_QUALIFIER DOMDocument* inputData, unsigned char** outputData, NameValueCollection& transportHeaders, bool asClient )
			{
				throw FilterInvalidMethod( AbstractFilter::BufferToXml );
			}
			
			FilterResult ProcessMessage( AbstractFilter::buffer_type inputData, XERCES_CPP_NAMESPACE_QUALIFIER DOMDocument* outputData, NameValueCollection& transportHeaders, bool asClient )
			{
				throw FilterInvalidMethod( AbstractFilter::BufferToXml );
			}

			FilterResult ProcessMessage( AbstractFilter::buffer_type inputData, AbstractFilter::buffer_type outputData, NameValueCollection& transportHeaders, bool asClient )
			{
				throw FilterInvalidMethod( AbstractFilter::BufferToBuffer );
			}
			
			FilterResult ProcessMessage( AbstractFilter::buffer_type inputData, unsigned char** outputData, NameValueCollection& transportHeaders, bool asClient )
			{
				throw FilterInvalidMethod( AbstractFilter::BufferToBuffer );
			}

		private:

			void ValidateProperties( NameValueCollection& transportHeaders );

			string m_XsdSchemaLocation, m_XsdNamespace;
	};
}


#endif
