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

#include "FilterChain.h"
#include "XmlUtil.h"
#include "Trace.h"

#include "SSL/SSLFilter.h"
#include "SSL/P7MFilter.h"
//#include "CSV/CSVFilter.h"
#include "Template/TemplateTransformFilter.h"
#include "MQ/MqFilter.h"
#include "Zip/ZipFilter.h"
#if !defined( NO_DB ) && defined( WITH_ORACLE ) && defined( AQ )
#include "MQ/AdvanceQueuing/AqFilter.h"
#endif 
#include "XSLT/XSLTFilter.h"
#include "Base64Filter.h"
#include "AbstractFilter.h"

#include <sstream>
#include <string>
#include <stdexcept>

using namespace std;
using namespace FinTP;

///FilterType implementation
string FilterType::ToString( FilterType::FilterTypeEnum type )
{
	switch( type )
	{
		case FilterType::MQ :
			return "MQ";
		case FilterType::XSLT : 
			return "XSLT";
		case FilterType::XSD : 
			return "XSD";
		case FilterType::BASE64 :
			return "BASE64";
		case FilterType::TEMPLATE :
			return "TEMPLATE";
		case FilterType::SSL :
			return "SSL";
		case FilterType::CHAIN :
			return "CHAIN";
//		case FilterType::CSV :
//			return "CSV";
		case FilterType::P7M :
			return "P7M";
		case FilterType::ZIP :
			return "ZIP";
	}
	throw invalid_argument( "type" );
}

string FilterType::ToString( FilterType::FilterInputDataType type )
{
	switch( type )
	{
		case FilterType::Xml :
			return "Xml";
		case FilterType::Buffer :
			return "Buffer";
	}
	throw invalid_argument( "type" );
}
	
FilterType::FilterTypeEnum FilterType::Parse( const string& type )
{
	if ( type == "MQ" )
		return FilterType::MQ;
	if ( type == "XSLT" )
		return FilterType::XSLT;
	if ( type == "XSD" )
		return FilterType::XSD;
	if ( type == "BASE64" )
		return FilterType::BASE64;
	if ( type == "TEMPLATE" )		
		return FilterType::TEMPLATE;
	if ( type == "SSL" )
		return FilterType::SSL;
	if ( type == "CHAIN" )
		return FilterType::CHAIN;
//	if ( type == "CSV" )
//		return FilterType::CSV;
	if ( type == "P7M" )
		return FilterType::P7M;
	if ( type == "ZIP" )
		return FilterType::ZIP;

	throw invalid_argument( "type" );
}

/// FilterChain implementation
FilterChain::FilterChain() : AbstractFilter( FilterType::CHAIN )
{
	for ( int i=0; i<4; i++ )
	{
		m_CompiledChainsAsClient[ ( AbstractFilter::FilterMethod )i ] = new vector< FilterMethod >;
	}
	
	for ( int j=0; j<4; j++ )
	{
		m_CompiledChainsAsServer[ ( AbstractFilter::FilterMethod )j ] = new vector< FilterMethod >;
	}
}

FilterChain::~FilterChain()
{
	try
	{
		DEBUG( ".dtor" );
	}catch( ... ){}

	try
	{
		vector< AbstractFilter* >::iterator finder;
		for( finder = m_Filters.begin(); finder != m_Filters.end(); finder++ )
		{
			try
			{
				if ( *finder != NULL )
				{
					delete *finder;
					*finder = NULL;
				}
			}catch( ... ){};
		}

		DEBUG( "All filters deleted" );
	}catch( ... ){}

	try
	{
		while( m_Filters.size() > 0 )
			m_Filters.pop_back();
		DEBUG( "All filters erased" );
	}catch( ... ){}

	try
	{
		map< AbstractFilter::FilterMethod, vector< AbstractFilter::FilterMethod >* >::iterator mapFinder;
		for( mapFinder = m_CompiledChainsAsClient.begin(); mapFinder != m_CompiledChainsAsClient.end(); mapFinder++ )
		{
			try
			{
				if ( mapFinder->second != NULL )
				{
					delete mapFinder->second;
					mapFinder->second = NULL;
				}
			}catch( ... ){}
		}
		DEBUG( "All client methods erased" );
	}catch( ... ){}

	try
	{
		map< AbstractFilter::FilterMethod, vector< AbstractFilter::FilterMethod >* >::iterator mapFinder;
		for( mapFinder = m_CompiledChainsAsServer.begin(); mapFinder != m_CompiledChainsAsServer.end(); mapFinder++ )
		{
			if ( mapFinder->second != NULL )
			{
				delete mapFinder->second;
				mapFinder->second = NULL;
			}
		}
		DEBUG( "All server methods erased" );
	}catch( ... ){}
}

int FilterChain::AddFilter( FilterType::FilterTypeEnum type, NameValueCollection* properties )
{
	AbstractFilter *newFilter = NULL;
	
	try
	{
		// create a filter based on type requested
		switch( type )
		{
			case FilterType::MQ :
				newFilter = new MqFilter();
				break;
			case FilterType::XSLT : 
				newFilter = new XSLTFilter();
				break;
			case FilterType::XSD : 
				//newFilter = NULL;
				break;
			case FilterType::BASE64 :
				newFilter = new Base64Filter();
				break;
			case FilterType::TEMPLATE :
				newFilter = new TemplateTransformFilter();
				break;
			case FilterType::SSL :
				newFilter = new SSLFilter();
				break;
			/*case FilterType::CSV :
				newFilter = new CSVFilter();
				break;*/
			case FilterType::P7M :
				newFilter = new P7MFilter();
				break;
			case FilterType::ZIP :
				newFilter = new ZipFilter();
				break;
#if !defined( NO_DB ) && defined( WITH_ORACLE ) && defined( AQ )
			case FilterType::AQ : 
				newFilter = new AqFilter();
				break;
#endif
			default :
				throw invalid_argument( "type" );
		}
	}
	catch( ... ) 
	{
		if ( newFilter != NULL )
			delete newFilter;
		throw;
	}

 	// add it to the array
	if( newFilter != NULL )
	{	
		//add properties
		
		DEBUG( "Adding properties to the new filter ..." );
		
		if( properties != NULL )
		{
			for ( unsigned int i=0; i<properties->getCount(); i++ )
			{
				DEBUG( "Key : ["  << ( *properties )[ i ].first << "] Value : [" << ( *properties )[ i ].second << "]" );
				newFilter->addProperty( ( *properties )[ i ].first, ( *properties )[ i ].second );
			}
		}
		newFilter->Init();
		DEBUG( "Adding the filter to chain .... " );
		m_Filters.push_back( newFilter );
		BuildChains();
	}	
	else
	{
		stringstream errorMessage;
		errorMessage << "Unable to create filter [" << FilterType::ToString( type ) << "]" << endl;
		
		TRACE( errorMessage.str() );
		
		throw runtime_error( errorMessage.str() );
	}
	return ( int )( m_Filters.size() ) - 1;
}

//overrides
AbstractFilter::FilterResult FilterChain::ProcessMessage( XERCES_CPP_NAMESPACE_QUALIFIER DOMDocument* inputOutputData, NameValueCollection& transportHeaders, bool asClient )
{
	XERCES_CPP_NAMESPACE_QUALIFIER DOMDocument* inputDOM = inputOutputData;
	XERCES_CPP_NAMESPACE_QUALIFIER DOMDocument* tempDOM = NULL;

	WorkItem< ManagedBuffer > inputBuffer;
	
	DEBUG( "Processing message in chain... "  );
	if ( !isMethodSupported( AbstractFilter::XmlToXml, asClient ) )
		throw FilterInvalidMethod( AbstractFilter::XmlToXml );
		
	AbstractFilter* crtFilter = NULL;
	
	for( unsigned int i=0; i<m_Filters.size(); i++ )
	{
		crtFilter = m_Filters[ i ];
		if ( crtFilter == NULL )
			throw logic_error( "Fatal error : current filter is NULL" );

		//AbstractFilter::FilterMethod crtMethod = ( *( m_CompiledChainsAsClient[ AbstractFilter::XmlToXml ] ) )[ i ];
			
		AbstractFilter::FilterMethod crtMethod;
		if ( asClient )
		{
			crtMethod = ( *( m_CompiledChainsAsClient[ AbstractFilter::XmlToXml ] ) )[ i ];
			DEBUG( "Client processing" );
		}
		else
		{
			crtMethod = ( *( m_CompiledChainsAsServer[ AbstractFilter::XmlToXml ] ) )[ i ];
			DEBUG( "Server processing" );
		}
		
		DEBUG( "Running filter [" << typeid( *crtFilter ).name() << "] with method [" << AbstractFilter::ToString( crtMethod ) << "]" );
		
		//TODO check all cases
		switch( crtMethod )
		{
			case AbstractFilter::XmlToXml :
				crtFilter->ProcessMessage( inputDOM, transportHeaders, asClient );
				break;
			case AbstractFilter::XmlToBuffer :
				crtFilter->ProcessMessage( inputDOM, inputBuffer, transportHeaders, asClient );
				
				// don't need the input doc anymore -> discard it
				if ( inputDOM != NULL )
				{
					inputDOM->release();
					inputDOM = NULL;
				}
				break;
			case AbstractFilter::BufferToXml :
				crtFilter->ProcessMessage( inputBuffer, tempDOM, transportHeaders, asClient );
				inputDOM = tempDOM;
				break;
			case AbstractFilter::BufferToBuffer :
				{
					WorkItem< ManagedBuffer > tempBuffer( new ManagedBuffer() );
					crtFilter->ProcessMessage( inputBuffer, tempBuffer, transportHeaders, asClient );
					inputBuffer.get()->copyFrom( tempBuffer.get() );
				}
				break;
		}
		DEBUG( "done processing" );
	}
	
	if ( crtFilter == NULL )
		throw logic_error( "Fatal error : current filter is NULL ( no filters ? )" );

	DEBUG( "Done." )
		
	return AbstractFilter::Completed;
}

AbstractFilter::FilterResult FilterChain::ProcessMessage( const XERCES_CPP_NAMESPACE_QUALIFIER DOMDocument* inputData, AbstractFilter::buffer_type outputData, NameValueCollection& transportHeaders, bool asClient )
{
	const XERCES_CPP_NAMESPACE_QUALIFIER DOMDocument* inputDOM = inputData;
	XERCES_CPP_NAMESPACE_QUALIFIER DOMDocument* tempDOM = NULL;
	
	WorkItem< ManagedBuffer > inputBuffer;

	DEBUG( "Processing message in chain... "  );
	if ( !isMethodSupported( AbstractFilter::XmlToBuffer, asClient ) )
		throw FilterInvalidMethod( AbstractFilter::XmlToBuffer );
		
	AbstractFilter* crtFilter = NULL; 
	
	for( unsigned int i=0; i<m_Filters.size(); i++ )
	{
		crtFilter = m_Filters[ i ];
		if ( crtFilter == NULL )
			throw logic_error( "Fatal error : current filter is NULL" );
		//AbstractFilter::FilterMethod crtMethod = ( *( m_CompiledChainsAsClient[ AbstractFilter::XmlToBuffer ] ) )[ i ];

		AbstractFilter::FilterMethod crtMethod;
		if ( asClient )
		{
			crtMethod = ( *( m_CompiledChainsAsClient[ AbstractFilter::XmlToBuffer ] ) )[ i ];
			DEBUG( "Client processing" );
		}
		else
		{
			crtMethod = ( *( m_CompiledChainsAsServer[ AbstractFilter::XmlToBuffer ] ) )[ i ];
			DEBUG( "Server processing" );
		}
		
		DEBUG( "Running filter ["  << typeid( *crtFilter ).name() << "] with method [" << AbstractFilter::ToString( crtMethod ) << "]" );
		
		//TODO check all cases
		switch( crtMethod )
		{
			case AbstractFilter::XmlToXml :
				//TODO : fix this cast
				crtFilter->ProcessMessage( const_cast< XERCES_CPP_NAMESPACE_QUALIFIER DOMDocument* >( inputDOM ), transportHeaders, asClient );
				break;
			case AbstractFilter::XmlToBuffer :
				crtFilter->ProcessMessage( inputDOM, inputBuffer, transportHeaders, asClient );
				
				// don't need the input doc anymore -> discard it
				if ( inputDOM != NULL )
				{
					//TODO : fix this cast
					const_cast< XERCES_CPP_NAMESPACE_QUALIFIER DOMDocument* >( inputDOM )->release();
					inputDOM = NULL;
				}
				break;
			case AbstractFilter::BufferToXml :
				crtFilter->ProcessMessage( inputBuffer, tempDOM, transportHeaders, asClient );
				inputDOM = tempDOM;
				break;
			case AbstractFilter::BufferToBuffer :
				{
					WorkItem< ManagedBuffer > tempBuffer( new ManagedBuffer() );
					crtFilter->ProcessMessage( inputBuffer, tempBuffer, transportHeaders, asClient );
					inputBuffer.get()->copyFrom( tempBuffer.get() );
				}
				break;
		}
		DEBUG( "done processing" );
	}
	
	if ( crtFilter == NULL )
		throw logic_error( "Fatal error : current filter is NULL ( no filters ? )" );

	if( outputData.get() != NULL )
	{
		outputData.get()->copyFrom( inputBuffer.get() );	
		DEBUG( "Done. Output data size : [" << outputData.get()->size() << "]" );
	}

	return AbstractFilter::Completed;
}

AbstractFilter::FilterResult FilterChain::ProcessMessage( WorkItem< ManagedBuffer > inputData, XERCES_CPP_NAMESPACE_QUALIFIER DOMDocument* outputData, NameValueCollection& transportHeaders, bool asClient )
{
	XERCES_CPP_NAMESPACE_QUALIFIER DOMDocument* inputDOM = NULL;
	XERCES_CPP_NAMESPACE_QUALIFIER DOMDocument* tempDOM = outputData;
	
	WorkItem< ManagedBuffer > inputBuffer( new ManagedBuffer() );
	inputBuffer.get()->copyFrom( inputData.get() );
	
	DEBUG( "Processing message in chain... "  );
	if ( !isMethodSupported( AbstractFilter::BufferToXml, asClient ) )
		throw FilterInvalidMethod( AbstractFilter::BufferToXml );
		
	AbstractFilter* crtFilter = NULL;
	
	for( unsigned int i=0; i<m_Filters.size(); i++ )
	{
		crtFilter = m_Filters[ i ];
		if ( crtFilter == NULL )
			throw logic_error( "Fatal error : current filter is NULL" );

		AbstractFilter::FilterMethod crtMethod;
		
		if ( asClient )
		{
			crtMethod = ( *( m_CompiledChainsAsClient[ AbstractFilter::BufferToXml ] ) )[ i ];
			DEBUG( "Client processing" );
		}
		else
		{
			crtMethod = ( *( m_CompiledChainsAsServer[ AbstractFilter::BufferToXml ] ) )[ i ];
			DEBUG( "Server processing" );
		}
		
		DEBUG( "Running filter ["  << typeid( *crtFilter ).name() << "] with method [" << AbstractFilter::ToString( crtMethod ) << "]" );
		
		//TODO check all cases
		switch( crtMethod )
		{
			case AbstractFilter::XmlToXml :
				crtFilter->ProcessMessage( inputDOM, transportHeaders, asClient );
				break;

			case AbstractFilter::XmlToBuffer :
				crtFilter->ProcessMessage( inputDOM, inputBuffer, transportHeaders, asClient );
				
				// don't need the input doc anymore -> discard it
				if ( inputDOM != NULL )
				{
					inputDOM->release();;
					inputDOM = NULL;
				}
				break;

			case AbstractFilter::BufferToXml :
				crtFilter->ProcessMessage( inputBuffer, tempDOM, transportHeaders, asClient );
				inputDOM = tempDOM;
				break;

			case AbstractFilter::BufferToBuffer :
				{
					WorkItem< ManagedBuffer > tempBuffer( new ManagedBuffer() );
					crtFilter->ProcessMessage( inputBuffer, tempBuffer, transportHeaders, asClient );
					inputBuffer.get()->copyFrom( tempBuffer.get() );
				}
				break;
		}
		DEBUG( "done processing" );
	}
	
	if ( crtFilter == NULL )
		throw logic_error( "Fatal error : current filter is NULL ( no filters ? )" );

	DEBUG( "Done" );
	return AbstractFilter::Completed;
}

AbstractFilter::FilterResult FilterChain::ProcessMessage( WorkItem< ManagedBuffer > inputData, WorkItem< ManagedBuffer > outputData, NameValueCollection& transportHeaders, bool asClient )
{
	XERCES_CPP_NAMESPACE_QUALIFIER DOMDocument* inputDOM = NULL;
	
	DOMImplementation* impl = DOMImplementationRegistry::getDOMImplementation( unicodeForm( "LS" ) );
	XERCES_CPP_NAMESPACE_QUALIFIER DOMDocument* tempDOM = impl->createDocument( 0, unicodeForm( "root" ), 0 );

	WorkItem< ManagedBuffer > inputBuffer( new ManagedBuffer() );
	inputBuffer.get()->copyFrom( inputData.get() );

	DEBUG( "Processing message in chain... " );
	if ( !isMethodSupported( AbstractFilter::BufferToBuffer, asClient ) )
		throw FilterInvalidMethod( AbstractFilter::BufferToBuffer );
		
	AbstractFilter* crtFilter = NULL;
	
	for( unsigned int i=0; i<m_Filters.size(); i++ )
	{
		crtFilter = m_Filters[ i ];
		if ( crtFilter == NULL )
			throw logic_error( "Fatal error : current filter is NULL" );

		//AbstractFilter::FilterMethod crtMethod = ( *( m_CompiledChainsAsClient[ AbstractFilter::BufferToBuffer ] ) )[ i ];
		
		AbstractFilter::FilterMethod crtMethod;
		if ( asClient )
		{
			crtMethod = ( *( m_CompiledChainsAsClient[ AbstractFilter::BufferToBuffer ] ) )[ i ];
			DEBUG( "Client processing" );
		}
		else
		{
			crtMethod = ( *( m_CompiledChainsAsServer[ AbstractFilter::BufferToBuffer ] ) )[ i ];
			DEBUG( "Server processing" );
		}
		
		DEBUG( "Running filter ["  << typeid( *crtFilter ).name() << "] with method [" << AbstractFilter::ToString( crtMethod ) << "]" );
		
		//TODO check all cases
		switch( crtMethod )
		{
			case AbstractFilter::XmlToXml :
				crtFilter->ProcessMessage( inputDOM, transportHeaders, asClient );
				break;
			case AbstractFilter::XmlToBuffer :
				crtFilter->ProcessMessage( inputDOM, inputBuffer, transportHeaders, asClient );
				
				// don't need the input doc anymore -> discard it
				if ( inputDOM != NULL )
				{
					inputDOM->release();;
					inputDOM = NULL;
				}
				break;
			case AbstractFilter::BufferToXml :
				crtFilter->ProcessMessage( inputBuffer, tempDOM, transportHeaders, asClient );
				inputDOM = tempDOM;
				break;
			case AbstractFilter::BufferToBuffer :
				{
					WorkItem< ManagedBuffer > tempBuffer( new ManagedBuffer() );
					crtFilter->ProcessMessage( inputBuffer, tempBuffer, transportHeaders, asClient );
					inputBuffer.get()->copyFrom( tempBuffer.get() );
				}
				break;
		}
		DEBUG( "done processing" );
	}
	
	if ( crtFilter == NULL )
		throw logic_error( "Fatal error : current filter is NULL ( no filters ? )" );

	if( outputData.get() != NULL )
	{
		outputData.get()->copyFrom( inputBuffer.get() );	
		DEBUG( "Done. Output data size : [" << outputData.get()->size() << "]" );
	}
	
	return AbstractFilter::Completed;
}

bool FilterChain::isMethodSupported( AbstractFilter::FilterMethod method, bool asClient, bool untilNow )
{
	// compute nuber of filters in chain; if a certeain compiled chain has less methods
	// than this number, it means the chain does not support the filter method
	unsigned int expectedSize = m_Filters.size();
	//DEBUG( "Expected size of chain : "  << expectedSize );

	vector< FilterMethod > *chain = ( asClient ) ? m_CompiledChainsAsClient[ method ] : m_CompiledChainsAsServer[ method ];

	// if the chain was not created, the method is not supported
	if ( chain == NULL )
	{
		DEBUG( "CHAIN NULL" );
		if ( untilNow && ( expectedSize == 1 ) ) 
			return true;
		else
			return false;
	}
		
	DEBUG( "Expected size : " << expectedSize << "; actual : " << chain->size() );
	bool supported;
	
	if( untilNow )
		supported = ( expectedSize == chain->size() + 1 );
	else
		supported = ( expectedSize <= chain->size() );
		
	if( supported ) 
	{
		DEBUG( ( string ( ( asClient ) ? " Client " : " Server " ) ) << " method supported : " << AbstractFilter::ToString( method ) );
	}
	else
	{
		DEBUG( ( string ( ( asClient ) ? " Client " : " Server " ) ) << " method not supported : " << AbstractFilter::ToString( method ) );
	}
		
	return supported;
}

void FilterChain::BuildChains()
{
	for ( int i=0; i<8; i++ )
	{
		// skip unsuported options
		if ( !isMethodSupported( ( FilterMethod )( i%4 ), ( i<4 ), true ) )
		{
			DEBUG( "Skipped unsupported method : ["  << i << "]" );
			continue;
		}
		
		DEBUG ( "Building chain" );
		FilterMethod method = ( FilterMethod )( i%4 );
		
		BuildChain( method, method, ( i<4 ) );
		vector< FilterMethod > *chain = ( i<4 ) ? m_CompiledChainsAsClient[ method ] : m_CompiledChainsAsServer[ method ];
		
		DEBUG( "Done building " << ( string )( ( i<4 ) ? " client " : " server " ) <<
			" chain [" << AbstractFilter::ToString( method ) << 
			"] size of chain is : " << chain->size() );
	}
}

// Decompose the problem : start from the method incomig data type, 
// try to add a method supporting that input and delegate the responsability 
// for creating a following chain to the next filter. Always preffer XML 
bool FilterChain::BuildChain( AbstractFilter::FilterMethod method, AbstractFilter::FilterMethod chainMethod, bool asClient, const unsigned int index )
{
	// recursivity return
	if ( index == m_Filters.size() )
		return true;
	
	vector< FilterMethod > *chain = ( asClient ) ? m_CompiledChainsAsClient[ chainMethod ] : m_CompiledChainsAsServer[ chainMethod ];
	
	// delete old chain and rebuild
	if ( isFirstFilter( index ) ) 
	{
		chain->clear();
	}
	
	unsigned int oldSize = chain->size();
	
	DEBUG( "Index = " << index << " requested method : " << AbstractFilter::ToString( method ) );
	DEBUG( "Building " << ( string )( ( asClient ) ? "client " : "server " ) <<
			" chain; size of chain is : " << chain->size() );
			
	switch( method )
	{
		case AbstractFilter::XmlToXml :
			// we need to input xml and return xml
			if( m_Filters[ index ]->isMethodSupported( AbstractFilter::XmlToXml, asClient ) )
			{
				string supportMessage = "Trying AbstractFilter::XmlToXml ... ";
				
				// add the current filter method
				chain->push_back( AbstractFilter::XmlToXml );
				if ( BuildChain( AbstractFilter::XmlToXml, chainMethod, asClient, index+1 ) )
				{
					DEBUG( supportMessage  << "supported. chain size : " << chain->size() );
					return true;
				}
				// fallback method through buffer
				DEBUG( supportMessage  << "no" );
				// remove myself from the chain
				if( oldSize != chain->size() )
					chain->pop_back();
			}
			if ( !isLastFilter( index ) && m_Filters[ index ]->isMethodSupported( AbstractFilter::XmlToBuffer, asClient ) )
			{
				string supportMessage = "Trying fallback through AbstractFilter::XmlToBuffer ... ";
				
				// add the current filter method
				chain->push_back( AbstractFilter::XmlToBuffer );
				if ( BuildChain( AbstractFilter::BufferToXml, chainMethod, asClient, index+1 ) )
				{
					DEBUG( supportMessage  << "supported. chain size : " << chain->size() );
					return true;
				}
				// no fallback - input as Xml is not supported
			}
			
			DEBUG( "AbstractFilter::XmlToXml no way" );
			// remove myself from the chain
			if( oldSize != chain->size() )
				chain->pop_back();
			return false;
			
		case AbstractFilter::XmlToBuffer :
			// we need to input xml and return buffer
			if( !isLastFilter( index ) && m_Filters[ index ]->isMethodSupported( AbstractFilter::XmlToXml, asClient ) )
			{
				string supportMessage = "Trying AbstractFilter::XmlToXml ... ";
				
				// add the current filter method
				chain->push_back( AbstractFilter::XmlToXml );
				if ( BuildChain( AbstractFilter::XmlToBuffer, chainMethod, asClient, index+1 ) )
				{
					DEBUG( supportMessage  << "supported. chain size : " << chain->size() );
					return true;
				}
				// fallback method through buffer
				DEBUG( supportMessage  << "no" );
				// remove myself from the chain
				if( oldSize != chain->size() )
					chain->pop_back();
			}
			if ( m_Filters[ index ]->isMethodSupported( AbstractFilter::XmlToBuffer, asClient ) )
			{
				string supportMessage = "Trying fallback through AbstractFilter::XmlToBuffer ... ";
				
				// add the current filter method
				chain->push_back( AbstractFilter::XmlToBuffer );
				if ( BuildChain( AbstractFilter::BufferToBuffer, chainMethod, asClient, index+1 ) )
				{
					DEBUG( supportMessage  << "supported. chain size : " << chain->size() );
					return true;
				}
				// no fallback - input as Xml is not supported
			}

			DEBUG( "AbstractFilter::XmlToBuffer no way" );
			// remove myself from the chain
			if( oldSize != chain->size() )
				chain->pop_back();
			return false;
			
		case AbstractFilter::BufferToXml :
			// we need to input buffer and return xml
			if( m_Filters[ index ]->isMethodSupported( AbstractFilter::BufferToXml, asClient ) )
			{
				string supportMessage = "Trying AbstractFilter::BufferToXml ... ";
				
				// add the current filter method
				chain->push_back( AbstractFilter::BufferToXml );
				if ( BuildChain( AbstractFilter::XmlToXml, chainMethod, asClient, index+1 ) )
				{
					DEBUG( supportMessage  << "supported. chain size : " << chain->size() );
					return true;
				}
				// fallback method through buffer
				DEBUG( supportMessage  << "no" );
				// remove myself from the chain
				if( oldSize != chain->size() )
					chain->pop_back();
			}
			if ( !isLastFilter( index ) && m_Filters[ index ]->isMethodSupported( AbstractFilter::BufferToBuffer, asClient ) )
			{
				string supportMessage = "Trying fallback through AbstractFilter::BufferToBuffer ... ";
				
				// add the current filter method
				chain->push_back( AbstractFilter::BufferToBuffer );
				if ( BuildChain( AbstractFilter::BufferToXml, chainMethod, asClient, index+1 ) )
				{
					DEBUG( supportMessage  << "supported. chain size : " << chain->size() );
					return true;
				}
				// no fallback - input as Xml is not supported
			}
			DEBUG( "AbstractFilter::BufferToXml no way" );
						
			// remove myself from the chain
			if( oldSize != chain->size() )
				chain->pop_back();
			return false;
			
		case AbstractFilter::BufferToBuffer :
			// we need to input buffer and return buffer
			if( !isLastFilter( index ) && m_Filters[ index ]->isMethodSupported( AbstractFilter::BufferToXml, asClient ) )
			{
				string supportMessage = "Trying AbstractFilter::BufferToXml ... ";
				
				// add the current filter method
				chain->push_back( AbstractFilter::BufferToXml );
				if ( BuildChain( AbstractFilter::XmlToBuffer, chainMethod, asClient, index+1 ) )
				{
					DEBUG( supportMessage  << "supported. chain size : " << chain->size() );
					return true;
				}
				// fallback method through buffer
				DEBUG( supportMessage  << "no" );
				// remove myself from the chain
				if( oldSize != chain->size() )
					chain->pop_back();
			}
			if ( m_Filters[ index ]->isMethodSupported( AbstractFilter::BufferToBuffer, asClient ) )
			{
				string supportMessage = "Trying fallback through AbstractFilter::BufferToBuffer ... ";
				
				// add the current filter method
				chain->push_back( AbstractFilter::BufferToBuffer );
				if ( BuildChain( AbstractFilter::BufferToBuffer, chainMethod, asClient, index+1 ) )
				{
					DEBUG( supportMessage  << "supported. chain size : " << chain->size() );
					return true;
				}
				// no fallback - input as Xml is not supported
			}

			DEBUG( "AbstractFilter::BufferToBuffer no way" );
			// remove myself from the chain
			if( oldSize != chain->size() )
				chain->pop_back();
			return false;
	}
	throw invalid_argument( "method" );
}

void FilterChain::Rollback()
{
	for ( unsigned int j=0; j<m_Filters.size(); j++ )
		m_Filters[ j ]->Rollback();	
}

void FilterChain::Commit()
{
	for ( unsigned int j=0; j<m_Filters.size(); j++ )
		m_Filters[ j ]->Commit();
}

void FilterChain::Abort()
{
	for ( unsigned int j=0; j<m_Filters.size(); j++ )
		m_Filters[ j ]->Abort();
}

void FilterChain::Report( bool onlySupported, bool displayChain )
{
	DEBUG( "Methods supported : \n\tCLIENT : " );
	for ( int i=0; i<4; i++ )
	{
		stringstream report;
		report << "\t" << AbstractFilter::ToString( ( FilterMethod )i );
		if ( isMethodSupported( ( FilterMethod)i, true ) ) 
		{
			DEBUG( report.str( ) << " : supported" );
			
			if( displayChain )
			{
				stringstream path;
				vector< FilterMethod > *chain = m_CompiledChainsAsClient[ ( FilterMethod )i ];
				
				DEBUG( m_Filters.size( ) << " filters" );
				
				for ( unsigned int j=0; j<m_Filters.size(); j++ )
				{
					path << AbstractFilter::ToString( ( *chain )[ j ] ) << " -> ";
				}
				DEBUG( path.str() );
			}
		}
		else
		{
			if ( onlySupported )
				continue;
			else
			{
				DEBUG( report.str( ) << " : not supported" );
			}
		}
	}
	
	DEBUG( "\n\tSERVER : " );
	for ( int j=0; j<4; j++ )
	{
		stringstream report;
		report << "\t" << AbstractFilter::ToString( ( FilterMethod )j );
		if ( isMethodSupported( ( FilterMethod)j, false ) ) 
		{
			DEBUG( report.str( ) << " : supported" );
			
			if( displayChain )
			{
				stringstream path;
				vector< FilterMethod > *chain = m_CompiledChainsAsServer[ ( FilterMethod )j ];
				
				DEBUG( m_Filters.size( ) << " filters" );
				
				for ( unsigned int k=0; k<m_Filters.size(); k++ )
				{
					path << AbstractFilter::ToString( ( *chain )[ k ] ) << " -> ";
				}
				DEBUG( path.str() );
			}
		}
		else
		{
			if ( onlySupported )
				continue;
			else
			{
				DEBUG( report.str( ) << " : not supported" );
			}
		}
	}
}
