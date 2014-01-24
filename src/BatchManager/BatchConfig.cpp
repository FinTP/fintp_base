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

#include "BatchConfig.h"

//#include <iostream>
#include <sstream>
//#include <fstream>
#include <exception>
#include <stdexcept>
#include <typeinfo>

#include "Trace.h"
#include "XmlUtil.h"

#include <xercesc/dom/DOM.hpp>

#ifdef WIN32
	#define __MSXML_LIBRARY_DEFINED__
	#include <windows.h>
#endif 

using namespace FinTP;

//BatchRequestTransform implementation
BatchRequestTransform::TransformType BatchRequestTransform::Parse( const string& type )
{
	if( type == "single-template" )
		return BatchRequestTransform::SingleTemplate;
	if( type == "batch" )
		return BatchRequestTransform::Batch;
		
	throw invalid_argument( "transformType" );
}

//BatchRequest implementation
BatchRequest::OrderType BatchRequest::Parse( const string& order )
{
	if( order == "None" )
		return BatchRequest::None;
	if( order == "Sequence" )
		return BatchRequest::Sequence;
		
	throw invalid_argument( "Invalid argument : order" );
}

void BatchRequest::AddTransform( const BatchRequestTransform& transform )
{
	m_Transforms.push_back( transform );
}

const BatchRequestTransform& BatchRequest::GetTransform() 
{
	if ( m_Transforms.size() > 0 )
		return m_Transforms[ 0 ];
		
	throw runtime_error( "No transforms defined for this request" );
}

//BatchConfig implementation
BatchConfig::BatchConfig()
{
}

BatchConfig::~BatchConfig()
{
}

void BatchConfig::setConfigFile( const string& configFilename )
{
	XERCES_CPP_NAMESPACE_QUALIFIER DOMDocument* parseDocument = NULL;
	DOMNode* root = NULL;

	DEBUG_GLOBAL( "Reading batch config from file ["  << configFilename << "]" ); 
	if ( configFilename.size() == 0 )
	{
		TRACE_GLOBAL( "Batch config file set to empty string" );
		return;
	}

	try
	{
		parseDocument = XmlUtil::DeserializeFromFile( configFilename );
		root = parseDocument->getDocumentElement();
	}	
    catch( const std::exception& e )
    {
    	stringstream messageBuffer;
   		messageBuffer << typeid( e ).name() << " exception [" << e.what() << "] while parsing config file.";
   		TRACE_GLOBAL( messageBuffer.str() );

		if ( parseDocument != NULL )
		{
			parseDocument->release();
			parseDocument = NULL;
		}

		throw;
    }
    catch( ... )
    {
    	TRACE_GLOBAL( "Unhandled exception while parsing config file" );
 
		if ( parseDocument != NULL )
		{
			parseDocument->release();
			parseDocument = NULL;
		}

		throw;
	}
   		
	try
	{
		if ( ( string )( localForm( root->getNodeName() ) ) != "Request" )
			throw runtime_error( "Expecting [Request] as root element of batch config file" );
				
		DEBUG( "Found [Request] node" );

		for ( DOMNode* appConfigNode = root->getFirstChild(); appConfigNode != 0; appConfigNode=appConfigNode->getNextSibling() )
		{
			DEBUG( "[" << ( localForm( appConfigNode->getNodeName() ) ) << "]" );
			// treat nodes named appConfig
			if ( ( string )( localForm( appConfigNode->getNodeName() ) ) == "RequestType" )
			{
				DEBUG( "Found [RequestType] node" );
				
				DOMAttr* attributeMT, *attributeOrder, *attributeStorage;
				DOMElement* appConfigNodeElem = dynamic_cast< DOMElement* >( appConfigNode );
				if ( appConfigNodeElem == NULL )
					throw runtime_error( "Bad type : Expected [RequestType] to be an element" );

				try
				{
					attributeMT = appConfigNodeElem->getAttributeNode( unicodeForm( "MessageType" ) );
					attributeOrder = appConfigNodeElem->getAttributeNode( unicodeForm( "Order" ) );
					attributeStorage = appConfigNodeElem->getAttributeNode( unicodeForm( "Storage" ) );
					
					if( ( attributeStorage == NULL ) || ( attributeMT == NULL ) )
					{
						throw runtime_error( "Expected attribute [MT/Storage] of node [RequestType] not found" );
					}
				}
				catch( ... )
				{
					throw runtime_error( "Expected attribute [MT/Storage] of node [RequestType] not found" );
				}
				
				BatchRequest request( localForm( attributeMT->getValue() ),
					BatchRequest::Parse( localForm( attributeOrder->getValue() ) ),
					localForm( attributeStorage->getValue() ) );
			
				for ( DOMNode* key = appConfigNode->getFirstChild(); key != 0; key=key->getNextSibling() )
				{
					// skip all nodes not named Transformation
					if ( ( ( string )( localForm( key->getNodeName() ) ) != "Transformation" ) || 
						( key->getNodeType() != DOMNode::ELEMENT_NODE ) )
						continue;
					
					DEBUG( "Found [Transformation] node" );
					
					DOMAttr* attributeType, *attributeName;
					DOMElement* keyElem = dynamic_cast< DOMElement* >( key );
					if ( keyElem == NULL )
						throw runtime_error( "Bad type : Expected [Transformation] to be an element" );
					
					// get attribs
					try
					{
						attributeType = keyElem->getAttributeNode( unicodeForm( "Type" ) );
						attributeName = keyElem->getAttributeNode( unicodeForm( "Name" ) );
						if( ( attributeType == NULL ) || ( attributeName == NULL ) )
						{
							throw runtime_error( "Expected attribute [Type/Name] of node [Transformation] not found" );
						}
					}
					catch( ... )
					{
						throw runtime_error( "Expected attribute [Type/Name] of node [Transformation] not found" );
					}
					
					BatchRequestTransform transform( BatchRequestTransform::Parse( localForm( attributeType->getValue() ) ),
						localForm( attributeName->getValue() ) );
						
					request.AddTransform( transform );
					//DEBUG( "["  << localForm( attributeKey->getValue() ) << "] = [" << localForm( attributeValue->getValue() ) << "]" );
					//m_Settings.Add( localForm( attributeType->getValue() ), localForm( attributeName->getValue() ) );
				}
				
				m_Requests.insert( pair< string, BatchRequest >( request.getStorage(), request ) );
			}
		}
	}
	catch( ... )
	{
		if ( parseDocument != NULL )
		{
			parseDocument->release();
			parseDocument = NULL;
		}
		throw;
	}

	if ( parseDocument != NULL )
	{
		parseDocument->release();
		parseDocument = NULL;
	}
}

const BatchRequest& BatchConfig::GetRequest( const string& storage, const string& messageType )
{
	map< string, BatchRequest >::const_iterator findRequest = m_Requests.begin();
	
	for( ;findRequest != m_Requests.end(); findRequest++ )
	{
		if( ( findRequest->first == storage ) || ( findRequest->first == "*" ) )
		{
			if( messageType == findRequest->second.getMessageType() )
				return findRequest->second;
		}
	}

	// fallback to *
	if ( messageType != "*" )
	{
		return GetRequest( storage, "*" );
	}	
	
	stringstream errorMessage;
	errorMessage << "No request defined for storage [" << storage << "]";
	throw invalid_argument( errorMessage.str() );
}
