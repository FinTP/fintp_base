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

#include <iostream>
#include <sstream>
//#include <fstream>

#include <xercesc/dom/DOM.hpp>
#include "Trace.h"
#include "XmlUtil.h"
#include "AppSettings.h"

#include <exception>
#include <stdexcept>
#include <typeinfo>

#ifdef WIN32
	#define __MSXML_LIBRARY_DEFINED__
	#include <windows.h>
#endif 

XERCES_CPP_NAMESPACE_USE
using namespace std;
using namespace FinTP;

AppSettings::AppSettings()
{
	//DEBUG( "CONSTRUCTOR()" );
}

AppSettings::AppSettings( const string& configFilename, const string& configContent ) 
{
	//DEBUG( "CONSTRUCTOR(string)" );
	try
	{
		loadConfig( configFilename, configContent );
	}
	catch( ... )
	{
		TRACE_GLOBAL( "Error occured during parsing of config file" );
	}
}

AppSettings::~AppSettings()
{
}

string AppSettings::getName( const AppSettings::ConfigPrefix prefix, const AppSettings::ConfigSettings setting )
{
	string settingName = "";
	switch( prefix )
	{
		case None :
			break;
	}
	
	switch( setting )
	{
		// Common settings
		case LOGPREFIX :
			settingName.append( "LogPrefix" );
			break;

		case LOGMAXLINES :
			settingName.append( "LogMaxLines" );
			break;
	}

	return settingName;
}

void AppSettings::setSectionReader( const string& sectionName, void ( *callback )( DOMElement* ) )
{
	if ( callback != NULL )
		m_Readers.insert( pair< std::string, void ( * )( DOMElement* ) >( sectionName, callback ) );
}

void AppSettings::loadConfig( const string& configFilename, const string& configContent )
{
	XERCES_CPP_NAMESPACE_QUALIFIER DOMDocument* parseDocument = NULL;
	DOMNode* root = NULL;

	DEBUG_GLOBAL( "Reading config from file [" << configFilename << "]" ); 
	if ( ( configFilename.length() == 0 ) && ( configContent.length() == 0 ) )
	{
		TRACE( "Config file / contents set to empty string" );
		return;
	}

	try
	{
		if ( configContent.length() > 0 )
			parseDocument = XmlUtil::DeserializeFromString( configContent );
		else
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
	
	//DEBUG( "Found [configuration] node" );

	try
	{
		bool foundAppConfigNode = false;
		for ( DOMNode* appConfigNode = root->getFirstChild(); appConfigNode != 0; appConfigNode=appConfigNode->getNextSibling() )
		{
			//DEBUG( "["  << ( localForm( appConfigNode->getNodeName() ) ) << "]" );
			// treat nodes named appConfig
			if ( ( string )( localForm( appConfigNode->getNodeName() ) ) == "appSettings" )
			{
		
				foundAppConfigNode = true;
				//DEBUG( "Found [appSettings] node" );
				for ( DOMNode* key = appConfigNode->getFirstChild(); key != 0; key=key->getNextSibling() )
				{
					// skip all nodes not named add
					if ( ( ( string )( localForm( key->getNodeName() ) ) != "add" ) || 
						( key->getNodeType() != DOMNode::ELEMENT_NODE ) )
						continue;
					
					DOMElement* elemKey = dynamic_cast< DOMElement* >( key );
					if ( elemKey == NULL )
						throw logic_error( "Bad type : expected [add] to be a DOMElement" );

					// get attribs
					DOMAttr* attributeKey, *attributeValue;
					try
					{
						attributeKey = elemKey->getAttributeNode( unicodeForm( "key" ) );
						attributeValue = elemKey->getAttributeNode( unicodeForm( "value" ) );
						if( ( attributeKey == NULL ) || ( attributeValue == NULL ) )
						{
							throw runtime_error( "Expected attribute [key/value] of node [add] not found" );
						}
					}
					catch( ... )
					{
						throw runtime_error( "Expected attribute [key/value] of node [add] not found" );
					}
					
					//DEBUG( "["  << localForm( attributeKey->getValue() ) << "] = [" << localForm( attributeValue->getValue() ) << "]" );
					m_Settings.Add( localForm( attributeKey->getValue() ), localForm( attributeValue->getValue() ) );	
				}
			}
			// treat nodes named configSections
			if ( ( string )( localForm( appConfigNode->getNodeName() ) ) == "configSections" )
			{
/*#ifdef WBICONN
				for ( DOMNode* filters = appConfigNode->getFirstChild(); filters != 0; filters=filters->getNextSibling() )
				{
					if ( ( string )( localForm( filters->getNodeName() ) ) == "Fetcher" )
					{
						DEBUG( "Read Filters for Fetcher" );
						readFiltersForConnector( filters, "Fetcher" );
					}
					if ( ( string )( localForm( filters->getNodeName() ) ) == "Publisher" )
					{
						DEBUG( "Read Filters for Publisher" );
						readFiltersForConnector( filters,  "Publisher" );
					}
				}
#else*/
				for ( DOMNode* key = appConfigNode->getFirstChild(); key != 0; key=key->getNextSibling() )
				{
					//this is for wbi conn
					if ( ( string )( localForm( key->getNodeName() ) ) == "Fetcher" )
					{
						DEBUG( "Read Filters for Fetcher" );
						readFiltersForConnector( key, "Fetcher" );
						continue;
					}
					if ( ( string )( localForm( key->getNodeName() ) ) == "Publisher" )
					{
						DEBUG( "Read Filters for Publisher" );
						readFiltersForConnector( key,  "Publisher" );
						continue;
					}
					// skip all nodes not named add 
					if ( ( ( string )( localForm( key->getNodeName() ) ) != "section" ) || 
						( key->getNodeType() != DOMNode::ELEMENT_NODE ) )
						continue;
					
					// get attribs
					DOMAttr* attributeKey;//, *attributeValue;
					
					// get all atributes
					DOMNamedNodeMap *attributes = key->getAttributes();
					try
					{
						//DEBUG( "Trying node [" << key << "] named [" << localForm( key->getNodeName() ) << "]" );
						 
						DOMElement* elemKey = dynamic_cast< DOMElement* >( key );
						if ( elemKey == NULL )
							throw logic_error( "Bad type : expected [section] to be a DOMElement" );

						// get name from the attributes collection
						attributeKey = elemKey->getAttributeNode( unicodeForm( "name" ) );
						if( attributeKey == NULL )
						{
							throw runtime_error( "Expected attribute [name] of node [section] not found" );
						}
						
						string sectionName = localForm( attributeKey->getValue() );
						DEBUG_GLOBAL( "Section found : " << sectionName );
						
						// if there is any registered reader for this section, invoke it
						if ( m_Readers.find( sectionName ) != m_Readers.end() )
						{
							try
							{
								m_Readers[ sectionName ]( elemKey );
							}
							catch( ... )
							{
								TRACE( "Error retured by custom section reader [" << sectionName << "]" );
								throw;
							}
						}
						else
						{
							if ( attributes != NULL )
							{
								NameValueCollection sectionAttributes;
								
								// add all attributes to the section collection
								for ( unsigned int i=0; i<attributes->getLength(); i++ )
								{
									DOMAttr *attribute = dynamic_cast< DOMAttr * >( attributes->item( i ) );
									if ( attribute == NULL )
									{
										TRACE( "Bad type : expected [section] attributes to be attributes" );
									}
									else
									{
										sectionAttributes.Add( localForm( attribute->getName() ), localForm( attribute->getValue() ) );
									}
										
									// DEBUG( "Section attribute ["  << localForm( attribute->getName() ) << "] = [" << localForm( attribute->getValue() ) << "]" );
								}
								m_Sections.insert( SectionEntry( sectionName, sectionAttributes ) );
							}
						}
					}
					catch( const std::exception& ex )
					{
						stringstream errorMessage;
						errorMessage << "Invalid [section] declaration in app.config [" << ex.what() << "]";
						throw runtime_error( errorMessage.str() );
					}
					catch( ... )
					{
						throw runtime_error( "Invalid [section] declaration in app.config [unknown error]" );
					}
				}
//#endif
			}
		}
		if ( !foundAppConfigNode )
		{
			throw runtime_error( "Expecting [appSettings] as child element of [configuration]" );	
		}
	}
	catch( ... )
	{
		if ( parseDocument != NULL )
			parseDocument->release();
		throw;
	}

	if ( parseDocument != NULL )
		parseDocument->release();
}

bool AppSettings::ContainsSection( const string& section )
{
	map< std::string, NameValueCollection >::const_iterator finder;
	finder = m_Sections.find( section );
	return ( finder != m_Sections.end() );
}

NameValueCollection& AppSettings::getSection( const string& sectionName )
{
	map< std::string, NameValueCollection >::iterator finder;
	finder = m_Sections.find( sectionName );

	if ( finder != m_Sections.end() )
		return finder->second;

	stringstream errorMessage;
	errorMessage << "Section [" << sectionName << "] does not exist in the config file";
	throw runtime_error( errorMessage.str() );
}

string AppSettings::getSectionAttribute( const string& sectionName, const string& attributeName )
{
	NameValueCollection section = getSection( sectionName );
	return section[ attributeName ];
}

string AppSettings::operator[] ( const string& key ) const
{
	return m_Settings[ key ];
}

void AppSettings::Dump( void ) const
{
	m_Settings.Dump();
}

void AppSettings::readFiltersForConnector( const DOMNode* filters, const string& connType )
{
	DEBUG( "Reading filters for [" << connType << "]" );
					
	try
	{
		NameValueCollection sectionAttributes;
		DOMNamedNodeMap *attributes = filters->getAttributes();
		for ( unsigned int i=0; i<attributes->getLength(); i++ )
		{
			DOMAttr *attribute = dynamic_cast< DOMAttr * >( attributes->item( i ) );
			if ( attribute == NULL )
			{
				TRACE( "Bad type : expected [section] attributes to be attributes" );
			}
			else
			{
				sectionAttributes.Add( localForm( attribute->getName() ), localForm( attribute->getValue() ) );
				DEBUG( "Section attribute ["  << localForm( attribute->getName() ) << "] = [" << localForm( attribute->getValue() ) << "]" );
			}
		}
							
		m_Sections.insert( SectionEntry( connType, sectionAttributes ) );
	}
	catch( ... )
	{
		throw runtime_error( "Invalid [section] declaration in app.config" );
	}
		
	for ( DOMNode* key = filters->getFirstChild(); key != 0; key=key->getNextSibling() )
	{					
		// skip all nodes not named section
		if ( ( ( string )( localForm( key->getNodeName() ) ) != "section" ) || ( key->getNodeType() != DOMNode::ELEMENT_NODE ) )
			continue;
					
		// get attribs
		DOMAttr* attributeKey;//, *attributeValue;
					
		// get all atributes
		DOMNamedNodeMap *keyAttributes = key->getAttributes();
		try
		{
			DOMElement* elemKey = dynamic_cast< DOMElement* >( key );
			if ( elemKey == NULL )
				throw logic_error( "Bad type : expected [section] to be a DOMElement" );

			// get name from the attributes collection
			attributeKey = elemKey->getAttributeNode( unicodeForm( "name" ) );
			if( attributeKey == NULL )
			{
				throw runtime_error( "Expected attribute [name] of node [section] not found" );
			}
						
			DEBUG_GLOBAL( "Section found : " << localForm( attributeKey->getValue() ) );
						
			NameValueCollection sectionAttributes;
						
			// add all attributes to the section collection
			for ( unsigned int i=0; i<keyAttributes->getLength(); i++ )
			{
				DOMAttr *attribute = dynamic_cast< DOMAttr * >( keyAttributes->item( i ) );
				if ( attribute == NULL )
				{
					TRACE( "Bad type : expected [section] attributes to be attributes" );
				}
				else
				{
					sectionAttributes.Add( localForm( attribute->getName() ), localForm( attribute->getValue() ) );
					DEBUG( "Section attribute [" << localForm( attribute->getName() ) << "] = [" << localForm( attribute->getValue() ) << "]" );
				}
			}
						
			m_Sections.insert( SectionEntry( localForm( attributeKey->getValue() ), sectionAttributes ) );
		}
		catch( ... )
		{
			throw runtime_error( "Invalid [section] declaration in app.config" );
		}
	}
}
