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

/**	
\file Base64Filter.h
\brief Base64 filter functions
*/
#ifndef BASE64FILTER_H
#define BASE64FILTER_H

#include "AbstractFilter.h"
#include "AppSettings.h"
#include <xercesc/util/PlatformUtils.hpp>
#include <xercesc/framework/StdOutFormatTarget.hpp>

namespace FinTP
{
	/**
	\class Base64Filter
	\brief Implement AbstractFilter methods for base64 format
	\author Horia Beschea
	*/
	class ExportedObject Base64Filter : public AbstractFilter
	{
		public:
			
			/// \brief %Path to be used by Xalan to find value in input document
			static const string XPATH;
			/// \brief Keep or no the original text 
			static const string KEEP_ORIGINAL;
			/// \brief Name of the node where encoded data to be inserted
			static const string ENCODED_NODE_NAME;
			
			/// \brief Constructor
			Base64Filter();
			/// \brief Destructor
			~Base64Filter();
		
			/// \brief %Path to be used by Xalan to find value in input documnet
			string m_XPath;
			/// \brief Keep or no the original text
			bool m_KeepOriginal;
			/// \brief Name of the node where encoded data to be inserted
			string m_EncodedNodeName;
		
			/**
			\brief Return \a TRUE if the filter supports logging payload to a file

			Note for overrides : return \a TRUE|FALSE if the payload can be read without rewinding ( not a stream )
			\return \a TRUE if the filter supports logging payload to a file
			*/
			bool canLogPayload();
			/**
			\brief Return \a TRUE if the filter can execute the requested operation in client/server context
			\param[in] method Method to test if supported
			\param[in] asClient \a TRUE if run as client , \a FALSE if run as server
			\return \a TRUE if the filter can execute the requested operation in client/server context
			*/
			bool isMethodSupported( FilterMethod method, bool asClient );
			
			/**
			\brief Process message from XML to XML , encode if as client , decode if as server, using options from transportHeaders
			\param[in,out] inputOutputData Data to be transform in Xerces DOMDocument format
			\param[in] transportHeaders Collection of options and parameters for transform
			\param[in] asClient \a TRUE if run as client , \a FALSE if run as server
			\return AbstractFilter::Completed if successful
			*/
			AbstractFilter::FilterResult ProcessMessage( XERCES_CPP_NAMESPACE_QUALIFIER DOMDocument* inputOutputData, NameValueCollection& transportHeaders, bool asClient );
			/**
			\brief Process message from XML to Buffer , encode if as client , decode if as server, using options from transportHeaders
			\param[in] inputData Data to be transform in Xerces DOMDocument format
			\param[out] outputData Data after transform in a buffer
			\param[in] transportHeaders Collection of options and parameters for transform
			\param[in] asClient \a TRUE if run as client , \a FALSE if run as server
			\return AbstractFilter::Completed if successful
			*/
			AbstractFilter::FilterResult ProcessMessage( const XERCES_CPP_NAMESPACE_QUALIFIER DOMDocument* inputData, AbstractFilter::buffer_type outputData, NameValueCollection& transportHeaders, bool asClient );
			/**
			\brief Process message from Buffer to XML , encode if as client , decode if as server, using options from transportHeaders
			\param[in] inputData Data to be transform in a buffer
			\param[out] outputData Data after transform in Xerces DOMDocument format
			\param[in] transportHeaders Collection of options and parameters for transform
			\param[in] asClient \a TRUE if run as client , \a FALSE if run as server
			\return AbstractFilter::Completed if successful
			*/
			AbstractFilter::FilterResult ProcessMessage( AbstractFilter::buffer_type inputData, XERCES_CPP_NAMESPACE_QUALIFIER DOMDocument* outputData, NameValueCollection& transportHeaders, bool asClient );
			/// \brief	\attention Method Buffer to Buffer not supported by this filter
			AbstractFilter::FilterResult ProcessMessage( AbstractFilter::buffer_type inputData, AbstractFilter::buffer_type outputData, NameValueCollection& transportHeaders, bool asClient )
			{
				throw FilterInvalidMethod( AbstractFilter::BufferToBuffer );
			}

			/// \brief	\attention Method XML to Buffer in char* format not supported by this filter
			AbstractFilter::FilterResult ProcessMessage( const XERCES_CPP_NAMESPACE_QUALIFIER DOMDocument* inputData, unsigned char** outputData, NameValueCollection& transportHeaders, bool asClient )
			{
				throw FilterInvalidMethod( AbstractFilter::XmlToBuffer );
			}

			/// \brief \attention Method Buffer to Buffer in char* format not supported by this filter
			AbstractFilter::FilterResult ProcessMessage( AbstractFilter::buffer_type inputData, unsigned char** outputData, NameValueCollection& transportHeaders, bool asClient )
			{
				throw FilterInvalidMethod( AbstractFilter::BufferToBuffer );
			}
				
			/*FilterResult ProcessMessage( unsigned char* inputData, unsigned char* outputData, 
				NameValueCollection& transportHeaders, bool asClient );*/
			
		private :
			/// \brief Validates required properties
			void ValidateProperties();
	};
}
#endif //BASE64FILTER_H
