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

#ifdef WIN32
	#define __MSXML_LIBRARY_DEFINED__
#endif

#include "ZipFilter.h"
#include "Trace.h"
#include "XmlUtil.h"

#include <sstream>
#include <iostream>

#if !defined( __GNUC__ )
	#include <strstream>
#endif

using namespace std;

//#include <boost/iostreams/filtering_streambuf.hpp>
//#include <boost/iostreams/copy.hpp>
//#include <boost/iostreams/filter/zlib.hpp>

XERCES_CPP_NAMESPACE_USE
using namespace FinTP;

//namespace io = boost::iostreams;

//Constructor
ZipFilter::ZipFilter() : AbstractFilter( FilterType::ZIP )
{
}

//Destructor 
ZipFilter::~ZipFilter()
{
}

//
// XML to XML
//
// Xerces-C++ couldn't validate the data contained in a DOMtree directly (see Xerces-C++ FAQ)
// So:
// 	the input DOM Document will be serialized into a memory buffer
// 	and then the buffer data will be processed and validate using filter ProcessMessage (buffer, XML) method
AbstractFilter::FilterResult ZipFilter::ProcessMessage( XERCES_CPP_NAMESPACE_QUALIFIER DOMDocument* inputOutputData, NameValueCollection& transportHeaders, bool asClient )
{	
	//Validate properties to see if contains XSD file
	ValidateProperties( transportHeaders );    
  
   //Serialize the inputOutputData DOM
	string theSerializedDOM = XmlUtil::SerializeToString( inputOutputData );
		
	//Process the buffer containing the serialized DOM tree
	ProcessMessage( ( unsigned char * )theSerializedDOM.data(), inputOutputData, transportHeaders, asClient);

	return AbstractFilter::Completed;	
}

//
//	Buffer to XML
//
AbstractFilter::FilterResult ZipFilter::ProcessMessage( unsigned char* inputData, XERCES_CPP_NAMESPACE_QUALIFIER DOMDocument* outputData, NameValueCollection& transportHeaders, bool asClient )
{
    return AbstractFilter::Completed;
}

AbstractFilter::FilterResult ZipFilter::ProcessMessage( AbstractFilter::buffer_type inputData, AbstractFilter::buffer_type outputData, NameValueCollection& transportHeaders, bool asClient )
{
	// input buffer
	ManagedBuffer* inputBuffer = inputData.get();
	if ( inputBuffer == NULL )
		throw runtime_error( "Input document is empty" );
	
	// input source stream
#if defined ( __GNUC__ )
	stringstream theInputStream( inputBuffer->str() );
#else
	istrstream theInputStream( ( char* )( inputBuffer->buffer() ), inputBuffer->size() );
#endif

   	try
	{
		//io::filtering_streambuf< io::input > inFiltering;
		//inFiltering.push( io::zlib_decompressor() );
		//inFiltering.push( theInputStream );

#if !defined ( __GNUC__ ) 
		// unfreeze buffer( after str ) in order to allow deallocation of the buffer
		theInputStream.rdbuf()->freeze( false );
#endif
	}
	catch( const std::exception& e )
	{
#if !defined ( __GNUC__ )
		// unfreeze buffer( after str ) in order to allow deallocation of the buffer
		theInputStream.rdbuf()->freeze( false );
#endif
		TRACE( typeid( e ).name() << " exception encountered while transforming message : " << e.what() );
		throw;
	}
	catch( ... )
	{
#if !defined ( __GNUC__ )
		// unfreeze buffer( after str ) in order to allow deallocation of the buffer
		theInputStream.rdbuf()->freeze( false );
#endif
		TRACE( "Unknown exception encountered while transforming message" );
		throw;
	}
		
	DEBUG2( "ZIP transform done" );
	return AbstractFilter::Completed;
}


bool ZipFilter::canLogPayload()
{
	return true;
//	throw logic_error( "You must override this function to let callers know if this filter can log the payload without disturbing operations" );
}


bool ZipFilter::isMethodSupported( AbstractFilter::FilterMethod method, bool asClient )
{
	switch( method )
	{
		case AbstractFilter::XmlToXml :
				return true;
		case AbstractFilter::BufferToXml :
				return true;
				
		default:
			return false;
	}
}

/// private methods implementation
void ZipFilter::ValidateProperties( NameValueCollection& transportHeaders )
{
}

