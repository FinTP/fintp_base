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
#include "../XmlUtil.h"

#include <xalanc/XalanTransformer/XalanTransformer.hpp>

class ExportedObject BatchFilter : public AbstractFilter
{
		
	public:
	
		static const string BatchFILE;  //Batch File Name, from Properties (.config file, Batch filter parameter )
		static const string BatchPARAM; //Batch Parameters, from TransportHeaders
		static const string BatchUSEEXT;
		static const string BatchOUTPUTFORMAT;
		
		static const string OUTPUT_METHOD_TEXT;
		static const string OUTPUT_METHOD_XML;
		static const string OUTPUT_METHOD_NONE;
		
		BatchFilter();
		~BatchFilter();
		
		// return true if the filter supports logging payload to a file
		// note for overrides : return true/false if the payload can be read without rewinding ( not a stream )
		bool canLogPayload();
		// return true if the filter can execute the requested operation in client/server context
		bool isMethodSupported( FilterMethod method, bool asClient );
		
		
		FilterResult ProcessMessage( XERCES_CPP_NAMESPACE_QUALIFIER DOMDocument* inputOutputData, NameValueCollection& transportHeaders, bool asClient );
		FilterResult ProcessMessage( XERCES_CPP_NAMESPACE_QUALIFIER DOMDocument* inputData, unsigned char* outputData, NameValueCollection& transportHeaders, bool asClient );
		FilterResult ProcessMessage( XERCES_CPP_NAMESPACE_QUALIFIER DOMDocument* inputData, unsigned char** outputData, NameValueCollection& transportHeaders, bool asClient );	
	
		static void Terminate();
			
	private:

		void validateProperties( NameValueCollection& headers );			
		void replyOutputFormat( NameValueCollection& headers, int format );
		
		static XALAN_CPP_NAMESPACE_QUALIFIER XalanTransformer* getTransformer();
		
		XALAN_CPP_NAMESPACE_QUALIFIER XalanCompiledStylesheet* getBatch( const string filename );
		
		static bool m_ExtensionsInstalled;
		static XercesDOMTreeErrorHandler* m_ErrorReporter;
		static XALAN_CPP_NAMESPACE_QUALIFIER XalanTransformer* m_Transformer;
		static map< string, const XALAN_CPP_NAMESPACE_QUALIFIER XalanCompiledStylesheet* > m_BatchCache;
};

#endif
