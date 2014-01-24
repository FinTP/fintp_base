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

#ifndef XSLTFILTER_H
#define XSLTFILTER_H

#ifdef WIN32
	#define __MSXML_LIBRARY_DEFINED__
#endif

#include "../AbstractFilter.h"
#include "XmlUtil.h"
#include "../XPathHelper.h"

#include <xalanc/XalanTransformer/XercesDOMWrapperParsedSource.hpp>
#include <xalanc/XalanTransformer/XalanTransformer.hpp>

namespace FinTP
{
	class ExportedObject XSLTFilter : public AbstractFilter
	{
	#if defined( TESTDLL_EXPORT ) || defined ( TESTDLL_IMPORT )
		friend class XSLTFilterTest;
	#endif

		public:
		
			static const string XSLTFILE;			// XSLT File Name, from Properties (.config file, XSLT filter parameter )
			static const string XSLTPARAM;			// XSLT Parameters, from TransportHeaders
			static const string XSLTUSEEXT;
			static const string XSLTOUTPUTFORMAT;
			static const string XSLTCRTPARSESOURCE; // Create a parsed source( the document will be reused )
			
			static const string OUTPUT_METHOD_TEXT;
			static const string OUTPUT_METHOD_XML;
			static const string OUTPUT_METHOD_NONE;
			
			XSLTFilter();
			~XSLTFilter();
			
			// return true if the filter supports logging payload to a file
			// note for overrides : return true/false if the payload can be read without rewinding ( not a stream )
			bool canLogPayload();
			// return true if the filter can execute the requested operation in client/server context
			bool isMethodSupported( FilterMethod method, bool asClient );
			
			FilterResult ProcessMessage( XERCES_CPP_NAMESPACE_QUALIFIER DOMDocument* inputOutputData, NameValueCollection& transportHeaders, bool asClient );
			FilterResult ProcessMessage( const XERCES_CPP_NAMESPACE_QUALIFIER DOMDocument* inputData, AbstractFilter::buffer_type outputData, NameValueCollection& transportHeaders, bool asClient );
					
			FilterResult ProcessMessage( AbstractFilter::buffer_type inputData, XERCES_CPP_NAMESPACE_QUALIFIER DOMDocument* outputData, NameValueCollection& transportHeaders, bool asClient )
			{
				throw FilterInvalidMethod( AbstractFilter::BufferToXml );
			}
			FilterResult ProcessMessage( AbstractFilter::buffer_type inputData, AbstractFilter::buffer_type outputData, NameValueCollection& transportHeaders, bool asClient );
			
			FilterResult ProcessMessage( AbstractFilter::buffer_type inputData, unsigned char** outputData, NameValueCollection& transportHeaders, bool asClient )
			{
				throw FilterInvalidMethod( AbstractFilter::BufferToBuffer );
			}

			//custom overloads
			FilterResult ProcessMessage( const XERCES_CPP_NAMESPACE_QUALIFIER DOMDocument* inputData, unsigned char** outputData, NameValueCollection& transportHeaders, bool asClient );	
			FilterResult ProcessMessage( XALAN_CPP_NAMESPACE_QUALIFIER XercesDOMWrapperParsedSource* inputData, AbstractFilter::buffer_type outputData, NameValueCollection& transportHeaders, bool asClient );	

			// custom methods
			static XALAN_CPP_NAMESPACE_QUALIFIER XercesDOMWrapperParsedSource* parseSource( const XERCES_CPP_NAMESPACE_QUALIFIER DOMDocument* inputData );
			static void releaseSource( XALAN_CPP_NAMESPACE_QUALIFIER XercesDOMWrapperParsedSource* source );
				
		private:

			string getTransform( NameValueCollection& headers );
			void replyOutputFormat( NameValueCollection& headers, int format ) const;
			
			// returns a thread-static transformer
			static XALAN_CPP_NAMESPACE_QUALIFIER XalanTransformer* getTransformer();

			// returns a thread-static transformer
			//static XALAN_CPP_NAMESPACE_QUALIFIER XalanTransformer* getErrorReporter();
			
			// returns a thread-static compiled XSLT from the cache, or compiles/inserts in cache/returns the compiled XSLT
			static const XALAN_CPP_NAMESPACE_QUALIFIER XalanCompiledStylesheet* getXSLT( const string& filename );
			
			//static XALAN_CPP_NAMESPACE_QUALIFIER XalanDOMString m_ExtNamespace;

			static pthread_once_t KeysCreate;
			static pthread_key_t TransformerKey;
			static pthread_key_t CompiledXSLTsKey;

			static void CreateKeys();
			static void DeleteTransformers( void* data );
			//static void DeleteCompiledXSLTs( void* data );

			static XALAN_CPP_NAMESPACE_QUALIFIER XercesParserLiaison* m_Liaison;
			static XALAN_CPP_NAMESPACE_QUALIFIER XercesDOMSupport* m_DOMSupport;
	};
}

#endif
