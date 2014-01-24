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

#include "TemplateTransformFilter.h"
#include <iostream>
#include <vector>
#include <sstream>

#include <xercesc/util/PlatformUtils.hpp>

using namespace std;
XERCES_CPP_NAMESPACE_USE
using namespace FinTP;

#include "XmlUtil.h"
#include "Trace.h"
#include "Base64.h"

const string TemplateTransformFilter::TEMPLATE_FILE = "Template";

/// .ctor - initialize xerces
TemplateTransformFilter::TemplateTransformFilter() : AbstractFilter( FilterType::TEMPLATE )
{
}

/// destructor - release allocated data
TemplateTransformFilter::~TemplateTransformFilter()
{
}

AbstractFilter::FilterResult TemplateTransformFilter::ProcessMessage( AbstractFilter::buffer_type inputData, XERCES_CPP_NAMESPACE_QUALIFIER DOMDocument* outputData, NameValueCollection& transportHeaders, bool asClient )
{
	ValidateProperties();
	
	// only support swift for now
	if ( m_Properties[ TemplateTransformFilter::TEMPLATE_FILE ].find( ".template.xml" ) != string::npos ) 
	{
		TemplateParser parser = TemplateParserFactory::getParser( m_Properties[ TemplateTransformFilter::TEMPLATE_FILE ] );
		
		// discard any data coming through the exit 
		if( outputData == NULL )
		{
			TRACE( "Input document is empty" );
			throw runtime_error( "Input document is empty" );
		}
			
		try
		{
			// append original
			DOMElement* element = outputData->createElement( unicodeForm( "Original" ) );
			DEBUG( "Original elem. created." );
			
			ManagedBuffer* inputBuffer = inputData.get();
			string encodedProdData = Base64::encode( inputBuffer->buffer(), inputBuffer->size() );

			DOMText* prodDataVal = outputData->createTextNode( unicodeForm( encodedProdData ) );
			DEBUG( "TextNode with encoded value created" );
			
			element->appendChild( prodDataVal );
			DEBUG( "TextNode appended" );

			outputData->getDocumentElement()->appendChild( element );
			DEBUG( "Original node appended" );
				
			DOMElement* trelement = outputData->createElement( unicodeForm( "Transformed" ) );
			DEBUG( "Transformed elem. created." );	
			
			outputData->getDocumentElement()->appendChild( trelement );
			DEBUG( "Transformed node appended" );
					
			// attempt match against the template
			/*if( transportHeaders.ContainsKey( "RECONSFORMAT" ) && ( transportHeaders[ "RECONSFORMAT" ] == "true" ) )
			{
				if ( !parser.MatchByLine( inputData.get()->getRef(), trelement ) )
				{
					TRACE( "MATCH FAILED" );
					throw runtime_error( "Template not matched. Document could not be parsed" );
				}
			}
			else 
			{*/
			if ( !parser.Matches( inputData.get()->getRef(), trelement ) )
			{
				TRACE( "MATCH FAILED" );
				throw runtime_error( "Template not matched. Document could not be parsed" );
			}
			//}

			DEBUG( "Xmlized doc : ["  << XmlUtil::SerializeToString( outputData ) << "]" );
			
		}
		catch( const XMLException& e )
		{
			stringstream errorMessage;
			
			errorMessage << "XmlException during maching template. Type : " << ( int )e.getErrorType() << 
				" Code : [" << e.getCode() << "] Message = " << localForm( e.getMessage() ) <<
				" Line : " << e.getSrcLine();

			throw runtime_error( errorMessage.str() );
		}
		catch( const std::exception& e )
		{
			TRACE( "Error parsing / appending original node : "  << e.what() );

			throw;
		}
		catch( ... )
		{
			TRACE( "Error parsing / appending original node : Unknown exception" );

			throw;
		}
		return AbstractFilter::Completed;
	}
	else
	{
		string message( "Invalid template : " );
		message.append( m_Properties[ TemplateTransformFilter::TEMPLATE_FILE ] );
		throw invalid_argument( message );
	}
}

AbstractFilter::FilterResult TemplateTransformFilter::ProcessMessage( AbstractFilter::buffer_type inputData, AbstractFilter::buffer_type outputData, NameValueCollection& transportHeaders, bool asClient )
{
	ValidateProperties();
	
	// only support swift for now
	if ( m_Properties[ TemplateTransformFilter::TEMPLATE_FILE ].find( ".template.xml" ) != string::npos )
	{
		TemplateParser parser = TemplateParserFactory::getParser( m_Properties[ TemplateTransformFilter::TEMPLATE_FILE ] );
		
		XERCES_CPP_NAMESPACE_QUALIFIER DOMDocument *output = NULL;
		try
		{
			DOMImplementation* impl = DOMImplementationRegistry::getDOMImplementation( unicodeForm( "LS" ) );
			output = impl->createDocument( 0, unicodeForm( "root" ), 0 );

			ProcessMessage( inputData, output, transportHeaders, asClient );

			string serializedDoc = XmlUtil::SerializeToString( output );
			outputData.get()->copyFrom( serializedDoc );

			if ( output != NULL )
			{
				output->release();
				output = NULL;
			}
		}
		catch( ... )
		{
			if ( output != NULL )
			{
				output->release();
				output = NULL;
			}
		}

		return AbstractFilter::Completed;
	}
	else
	{
		string message( "Invalid template : " );
		message.append( m_Properties[ TemplateTransformFilter::TEMPLATE_FILE ] );
		throw invalid_argument( message );
	}
}

bool TemplateTransformFilter::canLogPayload()
{
	return true;
}

// the only supported method take input as a buffer and return the XML representation
// of buffered XML implementation
bool TemplateTransformFilter::isMethodSupported( FilterMethod method, bool asClient )
{
	switch( method )
	{
		case AbstractFilter::BufferToXml :
			return true;
		case AbstractFilter::BufferToBuffer :
			return true;
		default : 
			return false;
	}	
}

/// private methods implementation
void TemplateTransformFilter::ValidateProperties()
{
	if ( !m_Properties.ContainsKey( TemplateTransformFilter::TEMPLATE_FILE ) )
		throw invalid_argument( "Required parameter missing : TEMPLATE_FILE" );
}


