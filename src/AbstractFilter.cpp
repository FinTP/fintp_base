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

#include "AbstractFilter.h"
#include <sstream>

using namespace std;
using namespace FinTP;

const NameValueCollection& AbstractFilter::getProperties() const
{
	return m_Properties;
}

void AbstractFilter::addProperty( const string& name, const string& value )
{
	if ( m_Properties.ContainsKey( name ) )
		m_Properties.Remove( name );
		
	m_Properties.Add( name, value );

}

/*void AbstractFilter::setProperty( string name, string value )
{
	m_Properties[ name ] = value; 
}*/

/*
 AbstractFilter::FilterResult AbstractFilter::ProcessMessage( XERCES_CPP_NAMESPACE_QUALIFIER DOMDocument* inputOutputData, NameValueCollection& transportHeaders, bool asClient )
{
	throw logic_error( "You must override this function to process the message or throw an InvalidFilterMethodException" );
	// if you don't plan to support a specific method do something like :
	// throw InvalidFilterMethod( FilterMethod.XmlToXml );
}

AbstractFilter::FilterResult AbstractFilter::ProcessMessage( XERCES_CPP_NAMESPACE_QUALIFIER DOMDocument* inputData, AbstractFilter::buffer_type outputData, NameValueCollection& transportHeaders, bool asClient )
{
	throw logic_error( "You must override this function to process the message or throw an InvalidFilterMethodException" );
	// if you don't plan to support a specific method do something like :
	// throw InvalidFilterMethod( FilterMethod.XmlToXml );
	
}

AbstractFilter::FilterResult AbstractFilter::ProcessMessage( AbstractFilter::buffer_type inputData, XERCES_CPP_NAMESPACE_QUALIFIER DOMDocument* outputData, NameValueCollection& transportHeaders, bool asClient )
{
	throw logic_error( "You must override this function to process the message or throw an InvalidFilterMethodException" );
	// if you don't plan to support a specific method do something like :
	// throw InvalidFilterMethod( FilterMethod.XmlToXml );
}

AbstractFilter::FilterResult AbstractFilter::ProcessMessage( AbstractFilter::buffer_type inputData, AbstractFilter::buffer_type outputData, NameValueCollection& transportHeaders, bool asClient )
{
	// if you don't plan to support a specific method do something like :
	// throw InvalidFilterMethod( FilterMethod.XmlToXml );
	throw logic_error( "You must override this function to process the message or throw an InvalidFilterMethodException" );
}

AbstractFilter::FilterResult AbstractFilter::ProcessMessage( XERCES_CPP_NAMESPACE_QUALIFIER DOMDocument* inputData, unsigned char** outputData, NameValueCollection& transportHeaders, bool asClient )
{
	throw logic_error( "Not implemented" );
}

AbstractFilter::FilterResult AbstractFilter::ProcessMessage( AbstractFilter::buffer_type inputData, unsigned char** outputData, NameValueCollection& transportHeaders, bool asClient )
{
	throw logic_error( "Not implemented" );
}
*/

bool AbstractFilter::isMethodSupported( AbstractFilter::FilterMethod method, bool asClient )
{
	throw logic_error( "You must override [isMethodSupported] to let callers know if a method is supported before calling it" );
	// overrides like :
	/*
	switch( method )
	{
		case AbstractFilter::XmlToXml :
			return true;
		case AbstractFilter::XmlToBuffer :
			return false;
		case AbstractFilter::BufferToXml :
			return false;
		case AbstractFilter::BufferToBuffer :
			return false;
	}	
	*/		
}

bool AbstractFilter::canLogPayload()
{
	throw logic_error( "You must override this function to let callers know if this filter can log the payload without disturbing operations" );
}
 
/// sets           the name of the file where the payload will be logged
void AbstractFilter::setLogFile( const string& filename )
{
	m_LogPayloadFile = filename;
}

/// gets the name of the file where the payload will be logged
string AbstractFilter::getLogFile() const
{
	return m_LogPayloadFile;
}

// string serialization
string AbstractFilter::ToString( const FilterMethod type )
{
	switch( type )
	{
		case AbstractFilter::XmlToXml :
			return "XmlToXml";
		case AbstractFilter::XmlToBuffer :
			return "XmlToBuffer";
		case AbstractFilter::BufferToXml :
			return "BufferToXml";
		case AbstractFilter::BufferToBuffer :
			return "BufferToBuffer";
	};
	stringstream errorMessage;
	errorMessage << "Filter method [" << type << "] is unknonwn";
	throw invalid_argument( errorMessage.str() );
}

///FilterInvalidMethod implementation
/// .ctor
FilterInvalidMethod::FilterInvalidMethod( AbstractFilter::FilterMethod method ) : logic_error( "Invalid method" )
{
	switch( method )
	{
		case AbstractFilter::XmlToXml :
			m_String = "Method XmlToXml not supported.";
			break;
		case AbstractFilter::XmlToBuffer :
			m_String = "Method XmlToBuffer not supported.";
			break;
		case AbstractFilter::BufferToXml :
			m_String = "Method BufferToXml not supported.";
			break;
		case AbstractFilter::BufferToBuffer :
			m_String = "Method BufferToBuffer not supported.";
			break;
	}
}
