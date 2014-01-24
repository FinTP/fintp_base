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
\file AbstractFilter.h 
\brief Provides a base class for custom filters
*/
#ifndef ABSTRACTFILTER_H
#define ABSTRACTFILTER_H

#include <string>
#include <stdexcept>
#include <sstream>

#include "DllMain.h"
#include "Collections.h"
#include "WorkItemPool.h"

#ifdef WIN32
	#define __MSXML_LIBRARY_DEFINED__
#endif

#include <xercesc/dom/DOM.hpp>

using namespace std;

namespace FinTP
{
	/**
	\class FilterType
	\brief Contains enumeration for filter types
	\author Horia Beschea
	*/
	class ExportedObject FilterType
	{
		public :
			///	\brief Enumeration of filter types 
			enum FilterTypeEnum
			{
				MQ,
				XSLT,
				XSD,
				BASE64,
				TEMPLATE,
				SSL,
				CHAIN,
				P7M,
				ZIP
			};
			
			/// \brief Input data type
			enum FilterInputDataType
			{
				Xml,
				Buffer
			};
			
			/// \brief \return String describing filter type
			static string ToString( FilterType::FilterTypeEnum type );
			/// \brief \return String describing filter input data type
			static string ToString( FilterType::FilterInputDataType type );
			
			/// \brief \return FilterTypeEnum by parsing input parameter type
			static FilterType::FilterTypeEnum Parse( const string& type );
	};


	/**
	\class AbstractFilter
	\brief Base class for filters.
	
	Note for overrides : Always call base constructor in your derived classes
	\author Horia Beschea
	*/
	class ExportedObject AbstractFilter
	{
		public : 

			typedef WorkItem< ManagedBuffer > buffer_type;

			/// \brief Return completed if the filter has successfully processed the message
			enum FilterResult
			{
				Completed = 1
				// other values reserved for future use
			};
			
			/// \brief Methods for filters
			enum FilterMethod
			{
				XmlToXml = 0,
				XmlToBuffer = 1,
				BufferToXml = 2,
				BufferToBuffer = 3
			};	

			/// \brief Destructor
			virtual ~AbstractFilter() {};
				
		protected:

			/** 
			\brief Constructor
			\param type Type of the filter to construct
			*/
			explicit AbstractFilter( FilterType::FilterTypeEnum type ) : m_FilterType( type ) {};
			
			// protected members
			/// \brief Collection of name-value pairs = additional properties for the filter
			/// e.g. XLSTFILE="blabla.xslt" or MQQUEUE="myqueue" ...
			NameValueCollection m_Properties;
			FilterType::FilterTypeEnum m_FilterType;

			// name of the file where the payload will be logged
			string m_LogPayloadFile;
			

		public :
					
			// return true if the filter supports logging payload to a file
			// note for overrides : return true/false if the payload can be read without rewinding ( not a stream )
			virtual bool canLogPayload();
			
			virtual FilterResult ProcessMessage( XERCES_CPP_NAMESPACE_QUALIFIER DOMDocument* inputOutputData, NameValueCollection& transportHeaders, bool asClient ) = 0;
			virtual FilterResult ProcessMessage( const XERCES_CPP_NAMESPACE_QUALIFIER DOMDocument* inputData, AbstractFilter::buffer_type outputData, NameValueCollection& transportHeaders, bool asClient ) = 0;
			virtual FilterResult ProcessMessage( AbstractFilter::buffer_type inputData, XERCES_CPP_NAMESPACE_QUALIFIER DOMDocument* outputData, NameValueCollection& transportHeaders, bool asClient ) = 0;
			virtual FilterResult ProcessMessage( AbstractFilter::buffer_type inputData, AbstractFilter::buffer_type outputData, NameValueCollection& transportHeaders, bool asClient ) = 0;
			
			// take ownership of the buffer
			virtual FilterResult ProcessMessage( const XERCES_CPP_NAMESPACE_QUALIFIER DOMDocument* inputData, unsigned char** outputData, NameValueCollection& transportHeaders, bool asClient ) = 0;	
			virtual FilterResult ProcessMessage( AbstractFilter::buffer_type inputData, unsigned char** outputData, NameValueCollection& transportHeaders, bool asClient ) = 0;

			// return true if the filter can execute the requested operation in client/server context
			virtual bool isMethodSupported( FilterMethod method, bool asClient );
			
			// accessors
			// return the collection of properties for this filter
			const NameValueCollection& getProperties() const;
			
			// sets the name of the file where the payload will be logged
			void setLogFile( const string& filename);
			
			// sets the name of the file where the payload will be logged
			string getLogFile() const;

			// is this a type of filter that receives or sends a message ?
			virtual bool isTransportType() { return false; }
			
			// adds/sets a property
			void addProperty( const string& name, const string& value );
			//void setProperty( string name, string value );
			
			// returns the friendly name for a FilterMethod value
			static string ToString( const FilterMethod type );
			FilterType::FilterTypeEnum getFilterType() const { return m_FilterType; }

			virtual void Init() {}
			
			// transaction support
			virtual void Rollback(){};
			virtual void Commit(){};
			virtual void Abort(){};

			virtual bool isBatch() const 
			{
				stringstream errorMessage;
				errorMessage << "[isBatch] is not implemented by this filter type [" << FilterType::ToString( m_FilterType ) << "]";
				throw logic_error( errorMessage.str() ); 
			}

			virtual string getQueueManagerName() const
			{
				stringstream errorMessage;
				errorMessage << "[getQueueManagerName] is not implemented by this filter type [" << FilterType::ToString( m_FilterType ) << "]";
				throw logic_error( errorMessage.str() ); 
			}

			virtual string getTransportURI() const
			{
				stringstream errorMessage;
				errorMessage << "[getTransportURI] is not implemented by this filter type [" << FilterType::ToString( m_FilterType ) << "]";
				throw logic_error( errorMessage.str() ); 
			}
#ifdef AMQ
			virtual string getBrokerURI() const
			{
				stringstream errorMessage;
				errorMessage << "[getBrokerURI] is not implemented by this filter type [" << FilterType::ToString( m_FilterType ) << "]";
				throw logic_error( errorMessage.str() ); 
			}
#endif			
			virtual string getQueueName() const
			{
				stringstream errorMessage;
				errorMessage << "[getQueueName] is not implemented by this filter type [" << FilterType::ToString( m_FilterType ) << "]";
				throw logic_error( errorMessage.str() ); 
			}
	};

	///<summary>
	/// This class extends the [logic_error] exception class.
	/// Should be used if a Filter does not support a certain processing mode
	/// ex. if the Filter does not support BufferToBuffer, it throws this exception.
	///</summary>
	class ExportedObject FilterInvalidMethod : public logic_error
	{
		public :
		
			// .ctors and destructors
			explicit FilterInvalidMethod( AbstractFilter::FilterMethod method );
			~FilterInvalidMethod() throw() {};
			
			virtual const char *what() const throw()
			{	// return pointer to message string
				try
				{
					return ( m_String.c_str() );
				}
				catch( ... )
				{
					return "unknown error";
				}
			}
			
		private :
		
			string m_String;
	};
}

#endif
