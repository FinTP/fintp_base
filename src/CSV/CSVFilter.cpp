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

#include "CSVFilter.h"
#include "../StringUtil.h"
#include "../XmlUtil.h"
#include "../Log/Trace.h"

#include <xercesc/util/PlatformUtils.hpp>

using namespace std;
XERCES_CPP_NAMESPACE_USE
using namespace FinTP;

CSVFilter::CSVFilter( ) : AbstractFilter( FilterType::CSV )
{
}

CSVFilter::~CSVFilter()
{
}

AbstractFilter::FilterResult CSVFilter::ProcessMessage(FinTP::AbstractFilter::buffer_type inputData, xercesc_2_7::DOMDocument *outputData, NameValueCollection &transportHeaders, bool asClient)
{
	ValidateProperties();

	if( asClient )
	{
		//fetcher 
		if( outputData == NULL )
		{
			TRACE( "Input document is empty" );
			throw runtime_error( "Input document is empty" );
		}

		try
		{
			DOMElement* firstElem = outputData->createElement( unicodeForm( "Mesaj" ) );
			outputData->getDocumentElement()->appendChild( firstElem );
			DEBUG( "Elem. created." );

			ManagedBuffer* inputBuffer = inputData.get();
			
			StringUtil data( inputBuffer->str() );

			data.Split( "\n" );

			do
			{
				StringUtil currentLine( data.NextToken() );
				currentLine.Split( "," );

				int i = 0;
				do
				{
					string nodeValue = currentLine.NextToken();
					if( nodeValue.length() != 0 )
					{
						/*if( i == 5 )
						{
							DOMElement* prodElem = outputData->createElement( unicodeForm( StringUtil::Trim( nodeValue ) ) );
							firstElem->appendChild( prodElem );
							
							nodeValue = currentLine.NextToken();

							DOMText* prodDataVal = outputData->createTextNode( unicodeForm( StringUtil::Trim( nodeValue ) ) );
							prodElem->appendChild( prodDataVal );
						}
						else*/
						{
							stringstream fieldName;
							fieldName << "Field" << i;
							DOMElement* prodElem = outputData->createElement( unicodeForm( fieldName.str() ) );
							firstElem->appendChild( prodElem );
							
							DOMText* prodDataVal = outputData->createTextNode( unicodeForm( StringUtil::Trim( nodeValue ) ) );
							prodElem->appendChild( prodDataVal );
						}
					}
					i++;
				} while( currentLine.MoreTokens() );

			} while ( data.MoreTokens() );
	
			DEBUG( "Message from CSV : " << XmlUtil::SerializeToString( outputData ) );
		}
		catch( const XMLException& e )
		{
			stringstream errorMessage;
			
			errorMessage << "XmlException during csv filter. Type : " << ( int )e.getErrorType() << 
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
	}
	else
	{
		//publisher
	}
	return AbstractFilter::Completed;
}

AbstractFilter::FilterResult CSVFilter::ProcessMessage(FinTP::AbstractFilter::buffer_type inputData, FinTP::AbstractFilter::buffer_type outputData, NameValueCollection &transportHeaders, bool asClient)
{
	ValidateProperties();

	if( asClient )
	{
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
	}
	else
	{
	}

	return AbstractFilter::Completed;
}

void CSVFilter::ValidateProperties()
{
}

bool CSVFilter::isMethodSupported( AbstractFilter::FilterMethod method, bool asClient )
{	
	switch( method )
	{
		case AbstractFilter::BufferToXml :
			return true;
		case AbstractFilter::BufferToBuffer :
			return true;
		default:
			return false;
	}
}
