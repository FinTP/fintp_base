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

#include "DbDad.h"
#include "DataParameter.h"
#include "XmlUtil.h"
#include "Trace.h"
#include "StringUtil.h"
#include "Base64.h"

#include <xercesc/dom/DOM.hpp>

using namespace FinTP;

// qPIDbDad implementation
DbDad::DbDad( const string& filename, DatabaseProviderFactory* dbProvider ) : m_TableName( "" )
{
	DEBUG( "The dad file name = [" << filename << "]" );
	
	m_DbProvider = dbProvider;

	XERCES_CPP_NAMESPACE_QUALIFIER DOMDocument *daddoc = NULL;
	try
	{
		daddoc = XmlUtil::DeserializeFromFile( filename );

		// display dad file contents
		string dad = XmlUtil::SerializeToString( daddoc );
		DEBUG( "DAD contents : [" << dad << "]" );

		XERCES_CPP_NAMESPACE_QUALIFIER DOMElement* root = daddoc->getDocumentElement();
		if( root == NULL )
			throw runtime_error( "Invalid document [no root element]" );
	
		//int curIntValue = 0;
		//m_MaxLength =  curIntValue;
		for ( XERCES_CPP_NAMESPACE_QUALIFIER DOMNode* key = root->getFirstChild(); key != 0; key=key->getNextSibling() )
		{
			if ( key->getNodeType() != DOMNode::ELEMENT_NODE )
				continue;
				
			XERCES_CPP_NAMESPACE_QUALIFIER DOMElement* curElem = ( XERCES_CPP_NAMESPACE_QUALIFIER DOMElement* ) key;
			//dynamic_cast< XERCES_CPP_NAMESPACE_QUALIFIER DOMElement* >key;
			string name = localForm(  curElem->getTagName() );
			if( ( name == "element_node" ) || ( name == "attribute_node" ) )
			{
				string elemName = localForm( curElem->getAttribute( unicodeForm( "name" ) ) );
				string origElemTypeStr = localForm( curElem->getAttribute( unicodeForm( "type" ) ) );
				//needed to determine what type is
				string elemTypeStr = StringUtil::ToUpper( origElemTypeStr );
				DataType::DATA_TYPE elemType = DataType::INVALID_TYPE;
				string elemFormat = "";
				string elemColumnName = "";
				int elemLength = -1;

				if (  elemTypeStr.substr( 0, 4 )  == "CHAR" )
				{
					elemLength = StringUtil::ParseInt( StringUtil::FindBetween( elemTypeStr , "(" , ")" ) );
					elemType = DataType::CHAR_TYPE;
				}
				else if ( elemTypeStr.substr( 0, 7 ) == "VARCHAR" )
				{
					elemType = DataType::CHAR_TYPE;
				}
				else if ( elemTypeStr.substr( 0, 7 ) == "NUMERIC" )
				{
					elemLength = StringUtil::ParseInt( StringUtil::FindBetween( elemTypeStr , "(" , "," ) );
					elemType = DataType::NUMBER_TYPE;
				}	
				else if ( elemTypeStr.substr( 0, 4 ) == "CLOB" )
				{
					elemType = DataType::LARGE_CHAR_TYPE;
				}
				else if ( elemTypeStr.substr( 0, 4 ) == "BLOB" )
				{
					elemType = DataType::BINARY;
					elemFormat = origElemTypeStr.substr( 4 );
				}
				else if ( elemTypeStr.substr( 0, 4 ) == "DATE" )
				{
					elemType = DataType::TIMESTAMP_TYPE;
					elemFormat = origElemTypeStr.substr( 4 );
				}
				else
				{
					stringstream errorMessage;
					errorMessage << "Element Type defined in DAD [" << elemTypeStr << "] is not implemented yet";
					throw invalid_argument( errorMessage.str() );
				}

				// look for <text_node><RDB_node><column name=!.../>
				vector< string > columnNames;
				if ( curElem->hasChildNodes() )
				{
					XERCES_CPP_NAMESPACE_QUALIFIER DOMNodeList* text_nodes = curElem->getElementsByTagName( unicodeForm( "text_node" ) );
					if ( ( text_nodes != NULL ) && ( text_nodes->getLength() > 0 ) )
					{
						XERCES_CPP_NAMESPACE_QUALIFIER DOMElement* text_node = dynamic_cast< DOMElement* >( text_nodes->item( 0 ) );
						if ( ( text_node != NULL ) && ( text_node->hasChildNodes() ) )
						{
							XERCES_CPP_NAMESPACE_QUALIFIER DOMNodeList* rdbs_nodes = text_node->getElementsByTagName( unicodeForm( "RDB_node" ) );
							if ( ( rdbs_nodes != NULL ) && ( rdbs_nodes->getLength() > 0 ) )
							{					
								XERCES_CPP_NAMESPACE_QUALIFIER DOMElement* rdb_node = dynamic_cast< DOMElement* >( rdbs_nodes->item( 0 ) );
								if ( ( rdb_node != NULL ) && ( rdb_node->hasChildNodes() ) )
								{
									XERCES_CPP_NAMESPACE_QUALIFIER DOMNodeList* rdb_subnodes = rdb_node->getElementsByTagName( unicodeForm( "column" ) );
									if ( ( rdb_subnodes != NULL ) && ( rdb_subnodes->getLength() > 0 ) )
									{
										for( unsigned int colidx = 0; colidx < rdb_subnodes->getLength(); colidx++ )
										{
											XERCES_CPP_NAMESPACE_QUALIFIER DOMElement* rdb_subnode = dynamic_cast< DOMElement* >( rdb_subnodes->item( colidx ) );
											if ( rdb_subnode != NULL )
											{
												columnNames.push_back( localForm( rdb_subnode->getAttribute( unicodeForm( "name" ) ) ) );
											}
										}
									}
								}
							}
						}
					}
				}

				DbDadElement elem( elemName, elemType, elemLength, elemFormat );
				for( vector< string >::const_iterator nameWalker = columnNames.begin(); nameWalker != columnNames.end(); nameWalker++ )
					elem.addColumnName( *nameWalker );
				
				DEBUG2( "current string value : [" << elemLength << "]" );
				m_Elements.insert( pair< string, DbDadElement >( elemName, elem ) );
			}
			else 
			{
				if( name == "table" )
					m_TableName = localForm( curElem->getAttribute( unicodeForm( "name" ) ) );
			}
		}
		//DEBUG( "MaxLength is : [" << m_MaxLength << "]" );
	}
	catch( const std::exception& ex )
	{
		TRACE( "Error deserializing DAD Document [" << ex.what() << "]" );
		if( daddoc != NULL )
		{
			daddoc->release();
			daddoc = NULL;
		}
		throw;
	}
	catch( ... )
	{
		TRACE( "Error deserializing DAD Document" );
		if( daddoc != NULL )
		{
			daddoc->release();
			daddoc = NULL;
		}
		throw;
	}

	// release dad dom
	if( daddoc != NULL )
	{
		daddoc->release();
		daddoc = NULL;
	}
}

const DbDadElement& DbDad::operator[]( const string& name )
{
	map< string, DbDadElement >::const_iterator elemFinder = m_Elements.find( name );
	if ( elemFinder == m_Elements.end() )
	{
		TRACE( "Element [" << name << "] is not defined in DAD" );
		m_Elements.insert( pair< string, DbDadElement >( name, DbDadElement( name ) ) );
	}
	return m_Elements[ name ];
}

void DbDad::Upload( const string& xmlData, Database *currentDatabase, bool usingParams )
{
	stringstream statementString;
	ParametersVector myParams;
	XERCES_CPP_NAMESPACE_QUALIFIER DOMDocument *doc = NULL;

	try
	{
		doc = XmlUtil::DeserializeFromString( xmlData );
	}
	catch ( ... )
	{
		TRACE( "Could not create dom document from payload" );
		if( doc != NULL )
		{
			doc->release();
			doc = NULL;
		}
		throw;
	}

	try
	{
		//read dad from file if isn't cached otherwise return from cache
		//DbDad& dad = getDbDad( dadFileName );

		//string m_TableName = dad.tableName();
		DEBUG( "Table name is : [" << m_TableName << "]" );

		//get the root element of document "TableName" element in some messages
		XERCES_CPP_NAMESPACE_QUALIFIER DOMElement* root = doc->getDocumentElement();
		if( root == NULL )
			throw runtime_error( "Invalid document [no root element]" );

		//get the first child of root "Record" element in some mesages
		XERCES_CPP_NAMESPACE_QUALIFIER DOMNode* node = root;

		string name = localForm( node->getNodeName() );
		// allow root to be "Record", if not, then it must be the first child
		if( name != "Record" )
		{
			node = root->getFirstChild();
			name = localForm( node->getNodeName() );
			//xmlData may have or not extra CRLF beetwen <TableName><Record>
			//TODO: this ca be removed by parser options
			if( name != "Record" )
			{
				if( ( node == NULL ) || ( node->getNextSibling() == NULL ) )
					throw runtime_error( "Missing record element" );
				node = node->getNextSibling();
				name = localForm( node->getNodeName() );
			}
		}

		if( ( name != "Record" ) || ( node->getNodeType() != DOMNode::ELEMENT_NODE ) )
			throw runtime_error( "Missing record element" );

		root = dynamic_cast< DOMElement* >( node );

		// build insert stmt
		stringstream castString;
		bool first = true;
		statementString << "insert into " << m_TableName << " ( ";

		if( root->hasAttributes() )
        {
			DOMNamedNodeMap *nodeAttr = root->getAttributes();
            for( int i=0; i<nodeAttr->getLength(); i++ )
			{
				string crtValue = "";
				XERCES_CPP_NAMESPACE_QUALIFIER DOMNode* key = nodeAttr->item(i);
				if ( key->getNodeType() == DOMNode::ATTRIBUTE_NODE )
				{
					XERCES_CPP_NAMESPACE_QUALIFIER DOMAttr* curElem = dynamic_cast< DOMAttr* >( key );
					name = localForm( curElem->getName() );
					crtValue = XmlUtil::XMLChtoString( curElem->getValue() );
					DEBUG2( "Element [" << name << "] with value [" << crtValue << "]" );
                }
				else
					continue;

				if ( usingParams )
					CastAndAdd( first, statementString, castString, name, crtValue, myParams );
				else
					CastAndAdd( first, statementString, castString, name, crtValue );
			}
		}

		for ( XERCES_CPP_NAMESPACE_QUALIFIER DOMNode* key = root->getFirstChild(); key != 0; key=key->getNextSibling() )
		{
			string crtValue = "";

			//read from xml every node and if it isn't a ELEMENT_NODE or a ATTRIBUTE_NODE skip
			if ( key->getNodeType() == DOMNode::ELEMENT_NODE )
			{
        		XERCES_CPP_NAMESPACE_QUALIFIER DOMElement* curElem = dynamic_cast< DOMElement* >( key );
				name = localForm( curElem->getTagName() );
				crtValue = XmlUtil::XMLChtoString( curElem->getTextContent() );	
				DEBUG2( "Element [" << name << "] with value [" << crtValue << "]" );
			}
			else if ( key->getNodeType() == DOMNode::ATTRIBUTE_NODE )
			{
				XERCES_CPP_NAMESPACE_QUALIFIER DOMAttr* curElem = dynamic_cast< DOMAttr* >( key );
				name = localForm( curElem->getName() );
				crtValue = XmlUtil::XMLChtoString( curElem->getValue() );
				DEBUG2( "Element [" << name << "] with value [" << crtValue << "]" );
			}
			else
				continue;

			if ( usingParams )
				CastAndAdd( first, statementString, castString, name, crtValue, myParams );
			else
				CastAndAdd( first, statementString, castString, name, crtValue );
		}

		statementString << " ) values (";
		castString << ")";

		statementString << castString.str();

		DEBUG( "Statement is : " << statementString.str() << "]" );
		if ( usingParams )
			currentDatabase->ExecuteNonQueryCached( DataCommand::INLINE, statementString.str(), myParams );
		else
			currentDatabase->ExecuteNonQueryCached( DataCommand::INLINE, statementString.str() );
	}
	catch( const std::exception& error )
	{
		TRACE( "ERROR processing message [" << error.what() << "]" );
		if( doc != NULL )
		{
			doc->release();
			doc = NULL;
		}
		throw;
	}
	catch( ... )
	{
		TRACE( "ERROR processing message [unknown reason]" );
		if( doc != NULL )
		{
			doc->release();
			doc = NULL;
		}
		throw;
	}

	if( doc != NULL )
	{
		doc->release();
		doc = NULL;
	}
}

bool DbDad::CastAndAdd( bool& first, stringstream& statementString, stringstream& castString, const string& parameterName, const string& parameterValue, bool escape )
{
	if ( parameterValue.length() == 0 )
	{
		DEBUG( "Warning : skipping parameter [" << parameterName << "] reason : value empty" );
		return false;
	}
	const DbDadElement& elem = m_Elements[ parameterName ];
	DataType::DATA_TYPE parameterType = elem.type();
	if ( parameterType == DataType::INVALID_TYPE )
	{
		DEBUG( "Warning : skipping parameter [" << parameterName << "] reason : type not defined in DAD" );
		return false;
	}
	DEBUG( "Adding parameter [" << parameterName << "] type [" << DataType::ToString( parameterType ) << "] value [" << parameterValue << "] length [" << parameterValue.length() << "] ..." );

	const vector< string >& columnNames = elem.columnNames();
	for( vector< string >::const_iterator nameWalker = columnNames.begin(); nameWalker != columnNames.end(); nameWalker++ )
	{
		if ( first )
		{
			first = !first;
		}
		else
		{
			statementString << ", ";
			castString << ", ";
		}
		statementString << *nameWalker;

		switch( parameterType )
		{
			case DataType::TIMESTAMP_TYPE:
				if ( escape )
					castString << "TO_DATE( \'" << parameterValue << "\', \'" << elem.format() << "\' )";
				else
					castString << "TO_DATE( " << parameterValue << ", \'" << elem.format() << "\' )";
				break;

			case DataType::CHAR_TYPE:
			case DataType::LARGE_CHAR_TYPE:
				DEBUG( "Replace \' with \'\'" );
				if ( escape )
					castString << "\'" << StringUtil::Replace( parameterValue, "\'", "\'\'") << "\'";
				else
					castString << parameterValue;
				break;

			default:
				castString << parameterValue;
				break;
		}
	}

	return true;
}

bool DbDad::CastAndAdd( bool& first, stringstream& statementString, stringstream& castString, const string& parameterName, const string& parameterValue, ParametersVector& params )
{
	string paramValueReplace = m_DbProvider->getParamPlaceholder( params.size() );
	if ( !CastAndAdd( first, statementString, castString, parameterName, paramValueReplace, false ) )
		return false;

	//create a parameter for every element of xml
	const DbDadElement& elem = m_Elements[ parameterName ];
	int crtLength = elem.length();
	DataType::DATA_TYPE elemType = elem.type();
	DEBUG2( "Current length is : [" << crtLength << "]" );

	DataParameterBase *param = m_DbProvider->createParameter( elemType );
	if ( crtLength == -1 )
		param->setDimension( parameterValue.length() );
	else
		param->setDimension( crtLength );

	if ( ( elemType == DataType::BINARY ) && ( elem.format() == "decodebase64" ) )
	{
		param->setString( Base64::decode( parameterValue ) );
	}
	else
	{
		param->setString( parameterValue );
	}

	param->setName( parameterName );
	params.push_back( param );
	return true;
}

/*
DbDad& DbDad::getDbDad( const string& dadFilename )
{
	if ( m_DadVectorCache.find( dadFilename ) != m_DadVectorCache.end() )
	{
       	DEBUG_GLOBAL( "Returning DAD from cache ..." );
    }
	else
    {
       	m_DadVectorCache.insert( pair< string, DbDad >( dadFilename, DbDad( dadFilename ) ) );
	}
	return m_DadVectorCache[ dadFilename ];
}

const DataType::DATA_TYPE DbDad::elementType( const string& elementName ) const
{
	DataType::DATA_TYPE returnValue = DataType::INVALID_TYPE;
	map< string, DbDadElement >::const_iterator elemFinder = m_Elements.find( elementName );

	if ( elemFinder == m_Elements.end() )
	{
		DEBUG( "Element is not in DAD" );
	}
	else
	{
		returnValue = elemFinder->second.type();
	}

	return returnValue;
}

const int DbDad::elementLength( const string& elementName ) const
{
	int returnValue = -1;
	map< string, DbDadElement >::const_iterator elemFinder = m_Elements.find( elementName );

	if ( elemFinder == m_Elements.end() )
	{
		DEBUG( "Element is not in DAD" );
	}
	else
	{
		returnValue = elemFinder->second.length();
	}

	return returnValue;
}

const string DbDad::elementFormat( const string& elementName ) const
{
	string returnValue = "";
	map< string, DbDadElement >::const_iterator elemFinder = m_Elements.find( elementName );

	if ( elemFinder == m_Elements.end() )
	{
		DEBUG( "Element is not in DAD" );
	}
	else
	{
		returnValue = elemFinder->second.format();
	}

	return returnValue;
}*/

