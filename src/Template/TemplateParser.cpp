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

#include "TemplateParser.h"

#include <xercesc/dom/DOM.hpp>
#include "XmlUtil.h"
#include "StringUtil.h"

#include <string>
#include <sstream>

#include "Trace.h"

#undef S_NORMAL

#define BOOST_REGEX_MATCH_EXTRA
#include <boost/version.hpp>
#include <boost/regex.hpp>

#include <xercesc/util/ParseException.hpp>
#include <xercesc/util/OutOfMemoryException.hpp>
#include <xercesc/util/XMLException.hpp>

using namespace std;
using namespace FinTP;

//TemplateData implementation

TemplateData::TemplateData() : m_Value( "" ), m_FriendlyName( "" ), m_MaxOccurs( -1 ), m_MinOccurs( -1 ), m_Successive( false ), 
	m_Regex( false ), m_MatchEnd( 0 ), m_Parent( NULL )
{
}

TemplateData::TemplateData( const DOMNode* node ) : m_FriendlyName( "" ), m_MaxOccurs( -1 ), m_MinOccurs( -1 ), m_Successive( false ),
	m_Regex( false ), m_MatchEnd( 0 ), m_Parent( NULL )
{
	// guard null values
	if ( node == NULL )
		throw invalid_argument( "Parmeter [node] passed to TemplateData is null." );
		
	// guard nodetype ... must be element
	if ( node->getNodeType() != DOMNode::ELEMENT_NODE )
	{
		stringstream errorMessage;
		errorMessage << "Parmeter [node] passed to TemplateData is not an [ELEMENT_NODE], but [" << node->getNodeType() << "]";
		
		throw invalid_argument( errorMessage.str() );
	}
	
	// get all atributes and put them in place
	DOMNamedNodeMap *attributes = node->getAttributes();
	
	//DEBUG( "No. of attributes : "  << attributes->getLength() );
		
	for ( unsigned int i=0; i<attributes->getLength(); i++ )
	{
		DOMAttr *attribute = dynamic_cast< DOMAttr * >( attributes->item( i ) );
		if ( attribute == NULL )
			continue;

		string attributeName = localForm( attribute->getName() );
		string attributeValue = localForm( attribute->getValue() );
		
		//DEBUG_STREAM ( "Attribute : [" ) << attributeName << "] = [" << attributeValue << "]" << endl;
		
		// get id
		if ( attributeName == "key" ) 
		{
			m_Regex = ( attributeValue == "Regex" ) ? true : false;		
		}
		// get cond
		else if ( attributeName == "value" )
		{
			m_Value = attributeValue;
			//DEBUG( m_Value );
		}
		else if ( attributeName == "friendlyName" )
		{
			m_FriendlyName = attributeValue;
		}
		else if ( attributeName == "maxOccurs" ) 
		{
			if ( attributeValue != "unbounded" )
				m_MaxOccurs = atoi( attributeValue.data() );
			else
				m_MaxOccurs = INT_MAX;
		}
		else if ( attributeName == "minOccurs" ) 
		{
			if ( attributeValue != "unbounded" )
				m_MinOccurs = atoi( attributeValue.data() );
			else
				m_MaxOccurs = INT_MAX;
		}
		else if ( attributeName == "succ" ) 
		{
			m_Successive = ( attributeValue == "true" ) ? true : false;
		} 
		// ignore other attributes
	}
	
	if ( m_Regex )
	{
		int matchno=0, length = 0;
		bool match = true;
		string newValue = m_Value;
		string groupRegexStr = "\\((\\?<[a-z0-9]+>)";
	
		// extract group names \(\?<(...)>
		boost::regex groupNamesRegex( groupRegexStr );
		boost::smatch groupsMatch;
	
		while ( match )
		{
			try
			{		
				int matchStartPos = -1, matchEndPos = -1;

				DEBUG2( "Searching.. " );
				match = boost::regex_search( newValue, groupsMatch, groupNamesRegex, boost::match_extra );
				DEBUG2( "Search ... " );
				
				if ( match )
				{
					matchStartPos = groupsMatch.position( 1 );
					matchEndPos = matchStartPos + groupsMatch.length( 1 );
				}
			
				if ( !match || ( matchStartPos < 0 ) ) 
					break;
				
				length = matchEndPos - matchStartPos;
						
				// skip ?< in the returned value, go up to >
				m_GroupNames.push_back( newValue.substr( matchStartPos + 2, length - 3 ) );
				
				newValue = newValue.erase( matchStartPos, length );
				
				DEBUG( "Match #" << matchno++ << " " << m_GroupNames.back() << " in [" << m_Value << "]" );
				
				//DEBUG_STREAM( "New value : [" ) << newValue << endl;
			}
			catch( const std::exception& ex )
			{
				TRACE( "Matching exception : " << ex.what() );
				throw;
			}
		}
		m_Value = newValue;
	}
}

TemplateData::~TemplateData()
{
	//DEBUG( "DESTRUCTOR - "  << this );
}

TemplateData::NodeMatchReturn TemplateData::MatchSingle( ManagedBuffer buffer, DOMElement *root ) 
{
	string bufferEx = "";
	if ( buffer.size() > 10 )
		bufferEx = string( ( char * )buffer.buffer(), 10 ) + string( "..." );
	else
		bufferEx = string( buffer.str() );

	DEBUG( "Trying MatchSingle of [" << m_Value << "] in [" << bufferEx << "]" );
	//XMLCh* transformed = NULL;	
	
	try
	{	
		bool match = false;
		int matchStartPos = -1, matchEndPos = -1;//, captureStartPos = -1;
		unsigned int groups = 0;
		
		// test current value
		boost::regex groupNamesRegex( m_Value );
		//boost::smatch groupsMatch;
		boost::smatch groupsMatch;

		//match = boost::regex_search( buffer.str(), groupsMatch, groupNamesRegex, boost::match_extra );
		string remainingBuffer = string( ( char * )buffer.buffer(), buffer.size() );
		match = boost::regex_search( remainingBuffer, groupsMatch, groupNamesRegex, boost::match_extra );

		// no match or match start position negative
		if ( !match )
		{
			DEBUG( "Trying match single returning FAIL ( regex not matched )" );
			return TemplateData::Fail;
		}
				
		groups = groupsMatch.size();
		/*if ( groups > 0 )
			captureStartPos = groupsMatch.position( 1 );*/

#if ( ( BOOST_VERSION >= 104000 ) || defined ( __GNUC__ ) )
		matchStartPos = groupsMatch.position();
#else
		matchStartPos = groupsMatch.position( 0 );
#endif
		matchEndPos = matchStartPos + groupsMatch.length( 0 );

		//int length = matchEndPos - matchStartPos;
		DEBUG2( "Matched [" << groups << "] groups." );
		DEBUG2( "\t" << ( m_Successive ? "Successive" : "Not fixed" ) << " starting at " << matchStartPos << " and ending at " << matchEndPos );
				
		// if the regex was matched, but no capture was requested, skip input 
		if ( match && ( groups == 1 ) )
		{
			m_MatchEnd = matchEndPos;
			DEBUG( "Trying match single returning SKIP [ between " << matchStartPos << " and " << matchEndPos << "]" );
			return TemplateData::Skip;
		}
		
		// if successive, but start position was not 0, fail
		if ( match && m_Successive && ( matchStartPos != 0 ) )
		{
			DEBUG( "Trying match single returning FAIL ( not starting at index 0 )" );
			return TemplateData::Fail;
		}
		
		// add all nodes to the tree
		for( unsigned int i=1; i<groups; i++ )
		{
			//int groupStartPos = -1;, groupEndPos = -1;

			//groupStartPos = groupsMatch.position( i );
			//groupEndPos = groupStartPos + groupsMatch.length( i );
			string foundValue = groupsMatch.str( i );
			
			if ( i <= m_GroupNames.size() )
			{
				string foundValueToLog = ( foundValue.length() > 100 ) ? foundValue.substr( 0, 100 ) + string( "..." ) : foundValue;
				DEBUG( "Match #" << i << " [" << m_GroupNames[ i - 1 ] << "] = [" << foundValueToLog << "] size : " << foundValue.length() );
				// append an attribute to the node
				if ( root != NULL )
					root->setAttribute( unicodeForm( m_GroupNames[ i - 1 ] ),
						unicodeForm( StringUtil::Trim( foundValue ) ) );
			}
			else
			{
				DEBUG( "Match #" << i << " [<unnamed>] = [" << foundValue << "] size : " << foundValue.length() );
			}
		}
		
		m_MatchEnd += matchEndPos;
		DEBUG2( "Done with groups. Match end : " << m_MatchEnd );
	}
	catch( const OutOfMemoryException& oex )
	{
		stringstream errorMessage;
		errorMessage << "Match single failure : out of memory [" << localForm( oex.getMessage() ) << "]";
		
		TRACE( errorMessage.str() );
		throw runtime_error( errorMessage.str() );
	}
	catch( const ParseException& pex )
	{
		stringstream errorMessage;
		errorMessage << "Match single failure : ParseException type : " << ( int )pex.getErrorType() << 
			" Code : [" << pex.getCode() << "] Message = " << localForm( pex.getMessage() ) <<
			" Line : " << pex.getSrcLine();
			
		TRACE( errorMessage.str() );
		throw runtime_error( errorMessage.str() );
	}
	catch( const boost::bad_pattern& ex )
	{
		stringstream errorMessage;
		errorMessage << "Match single failure on regex [" << m_Value << "] : Boost regex exception : " << ex.what() ;
		
		TRACE( errorMessage.str() );
		throw runtime_error( errorMessage.str() );
	}
	catch( ... )
	{
		TRACE( "Match single failure : unknown reason" );
		throw;
	}
	
	DEBUG2( "Trying match single returning ADD" );
	return TemplateData::Add;
}

TemplateData::NodeMatchReturn TemplateData::Matches( ManagedBuffer buff, DOMNode *root )
{
	if ( m_Regex )	
	{
		//DEBUG( "REGEX" );
		ManagedBuffer buffer = buff.getRef();

		// guard incoming params
		if( root == NULL )
		{
			TRACE( "Match attempted on a null document ( root )" );
			throw invalid_argument( "TemplateData::Matches attempted on a null document ( root )" );
		}
		
		// get owner document from the passes node
		XERCES_CPP_NAMESPACE_QUALIFIER DOMDocument* doc = root->getOwnerDocument();
		if ( doc == NULL )
			throw invalid_argument( "Root node does not belong to a DOMDocument" );

		// save index		
		int capValue = 0, matches = 0;
		if ( m_MaxOccurs == -1 ) // not specified .. match as many time as possible
			capValue = INT_MAX;
		else
			capValue = m_MaxOccurs;
			
		m_MatchEnd = 0;
			
		// match at most capValue nodes
		while( matches < capValue )
		{
			DOMElement* element = NULL;
			try
			{
				bool appendNode = true;
				
				// if the element is just an eyecatcher, not an output element, fake a node
				if ( m_FriendlyName.size() == 0 )
				{
					appendNode = false;
					element = doc->createElement( unicodeForm( "dummy" ) );
				}
				else
					element = doc->createElement( unicodeForm( m_FriendlyName ) );
				
				// don't know many reasons whay this would fail, but...
				if ( element == NULL )
				{
					string errorMessage( "Unable to create node : [" );
					errorMessage += m_FriendlyName;
					errorMessage.append( "]" );

					TRACE( errorMessage );
					throw runtime_error( errorMessage );
				}
				
				// add the node if a match was succesful
				switch( MatchSingle( buffer, element ) )
				{
					case TemplateData::Add :
						DEBUG2( "MatchSingle returned ADD" );
						if ( appendNode ) 
						{
							DEBUG( "REGEX match; node added "  << m_FriendlyName );
							root->appendChild( element );
						}
						else
						{
							DEBUG( "REGEX match; eyecatcher [ADD]" << m_FriendlyName );
						}
						
						if ( matches+1 == capValue )
							return TemplateData::Add;
						buffer = buff + m_MatchEnd;
						break;
						
					case TemplateData::Fail :
						DEBUG2( "MatchSingle returned FAIL" );
						
						if ( element != NULL )
							element->release();
					
						// if minOccurs wasn't specified, it may be ok if no match was found
						if ( ( m_MinOccurs < 0 ) || ( m_MinOccurs <= matches  ) )
						{
							DEBUG( "REGEX not matched; node skipped "  << m_FriendlyName << " minOccurs=" << m_MinOccurs );
							return TemplateData::Add;
						}
						DEBUG( "REGEX not matched; node failed "  << m_FriendlyName );
						return TemplateData::Fail;				
					
					case TemplateData::Skip :
						DEBUG2( "MatchSingle returned SKIP" );

						DEBUG( "REGEX matched; eyecatcher [SKIP] " << m_FriendlyName );
						return TemplateData::Skip;
				}
			}
			catch( ... )
			{
				// release local resources
				try
				{
					if ( element != NULL )
						element->release();
				}
				catch( ... )
				{
					TRACE( "Error releasing element" );
				}
				throw;
			}
			matches++;
		}
	}
	else
	{
		DEBUG( "NOT REGEX"  );
	}
	return TemplateData::Fail;
}

//TemplateNode implementation
TemplateNode::TemplateNode() : m_Id( "Autogenerated" ), m_Every( false ), m_Parent( NULL ), m_MatchEnd( 0 )
{
}

TemplateNode::TemplateNode( const TemplateNode& source )
{
	//DEBUG( "COPY CTOR" );
	m_Every = source.MustMatchAllNodes();
	m_Parent = source.getParent();
	 
	m_Id = source.getId();
	m_Every = source.MustMatchAllNodes();
	
	m_Data = source.getData();
	m_Children = source.getChildren();

	m_MatchEnd = source.getMatchEnd();
	
	// reset parent on contained data
	for ( unsigned int i=0; i<m_Children.size(); i++ )
	{
		m_Children[ i ].setParent( this );
	}
	for ( unsigned int j=0; j<m_Data.size(); j++ )
	{
		m_Data[ j ].setParent( this );
	}
}

TemplateNode& TemplateNode::operator=( const TemplateNode& source )
{
	if ( this == &source )
		return *this;

	//DEBUG( "COPY CTOR" );
	m_Every = source.MustMatchAllNodes();
	m_Parent = source.getParent();
	 
	m_Id = source.getId();
	m_Every = source.MustMatchAllNodes();
	
	m_Data = source.getData();
	m_Children = source.getChildren();
	
	// reset parent on contained data
	for ( unsigned int i=0; i<m_Children.size(); i++ )
	{
		m_Children[ i ].setParent( this );
	}
	for ( unsigned int j=0; j<m_Data.size(); j++ )
	{
		m_Data[ j ].setParent( this );
	}
	
	return *this;
}

TemplateNode::~TemplateNode()
{
	//DEBUG( "DESTRUCTOR - "  << this );
}

void TemplateNode::Parse( DOMNode* node )
{
	//DEBUG( "Parse" )
	
	// guard null values
	if ( node == NULL )
	{
		TRACE( "Node is NULL" );
		throw invalid_argument( "node is null" );
	}
		
	// guard nodetype ... must be element
	if ( node->getNodeType() != DOMNode::ELEMENT_NODE )
	{
		TRACE( "NODE is not ELEMENT_NODE" );
		throw invalid_argument( "node is not an ELEMENT_NODE" );
	}
	
	// get all atributes and put them in place
	DOMNamedNodeMap *attributes = node->getAttributes();
	
	//DEBUG( "No. of attributes : "  << attributes->getLength() );
	for ( unsigned int i=0; i<attributes->getLength(); i++ )
	{
		DOMAttr *attribute = dynamic_cast< DOMAttr * >( attributes->item( i ) );
		if ( attribute == NULL )
			continue;

		string attributeName = localForm( attribute->getName() );
		string attributeValue = localForm( attribute->getValue() );
		
		//DEBUG_STREAM ( "Attribute : " ) << attributeName << " = " << attributeValue << endl;
		
		// get id
		if ( ( attributeName == "idInner" ) || ( attributeName == "idTemplate" ) )
			m_Id = attributeValue;
		// get cond
		else if ( attributeName == "cond" )
		{
			if ( attributeValue == "every" )
				m_Every = true;
			else
				m_Every = false;
		}
		// ignore other attributes
	}
	
	for ( DOMNode* child = node->getFirstChild(); child != 0; child=child->getNextSibling() )
	{
		if ( child->getNodeType() != DOMNode::ELEMENT_NODE )
			continue;
			
		// if the filter rejects the node, skip it and all children
		if( ( TemplateParser::Filter != NULL ) && ( TemplateParser::Filter->acceptNode( child ) == DOMNodeFilter::FILTER_REJECT ) )
			continue;
			
		// if the filter says SKIP, skip the node 
		if( ( TemplateParser::Filter != NULL ) && ( TemplateParser::Filter->acceptNode( child ) == DOMNodeFilter::FILTER_SKIP ) )
			continue;
			
		else
		{
			string nodeName = localForm( child->getNodeName() );
			//DEBUG( "Sub node : "  << nodeName );
			
			if ( nodeName == "InnerTemplate" ) 
			{
				//DEBUG( "Creating InnerTemplate. children sofar = "  << m_Children.size() );
				TemplateNode childNode;// = new TemplateNode();
				childNode.Parse( child );
				childNode.setParent( this );
				
				//DEBUG( "I'm ["  << this << "]" );
				const TemplateNode* parentNode = childNode.getParent();
				if ( parentNode == NULL )
				{
					TRACE( "Set parent of [" << childNode.getId() << "] to [N/A - NULL]" );
				}
				else
				{
					DEBUG( "Set parent of [" << childNode.getId() << "] to [" << parentNode->getId() << " - " << parentNode << "]" );
				}
				m_Children.push_back( childNode );
			}
			else
			{
				//DEBUG( "Creating data. children sofar = "  << m_Data.size() );
				TemplateData childNode( child );// = new TemplateData( child );
				childNode.setParent( this );
				m_Data.push_back( childNode );
			}
		}
	}
	
	//DEBUG( "\tNode ["  << m_Id << "] has " << m_Children.size() << " children and " << m_Data.size() << " regex nodes" );
}

/*bool TemplateNode::AddLineInfoInDom( const string line, DOMNode **node, bool& first )
{
	if ( *node == NULL )
		throw invalid_argument( "Node of the document passed to match is NULL" );
	if ( line.length() == 0 )
		throw invalid_argument( "Line passed to match is empty." );

	XERCES_CPP_NAMESPACE_QUALIFIER DOMDocument* doc = (*node)->getOwnerDocument();
	string foundValue = "";

	for( unsigned int i=0; i<m_Data.size(); i++ )
	{
		DEBUG( "Attempting match of ["  << ( m_Data[ i ].getFriendlyName() ) << "]" << ( ( m_Data[ i ].IsRegex() ) ? " ( regex )" : "( inner template)" ) );

		if( !m_Data[i].IsRegex() ) 
		{
			string startWith = m_Data[i].getValue();
			if( StringUtil::StartsWith( line, startWith ) )
			{
				if( m_Data[i].MustBeSuccessive() )
				{
				//	foundValue = line.substr( 2, line.length()-3 );
				//
				//	DOMElement* element;
				//	if( (*node)->getNodeType() == DOMNode::ELEMENT_NODE )
				//		element = ( DOMElement* )(*node)->getLastChild();
				//	else 
				//		throw exception( "Node is not ElementNode" );
				//
				//	string attributeData( localForm(element->getAttribute( unicodeForm( "data" ) ) ) );
				//	attributeData += foundValue;
				//	
				//	element->setAttribute( unicodeForm( "data" ), unicodeForm( StringUtil::Trim( attributeData ) ) );
					
					if( first )
					{
						first = false;
						DOMElement* element = doc->createElement( unicodeForm( startWith+"0" ) );
						foundValue = line.substr( 1, line.length()-3 );
						element->setAttribute( unicodeForm( "data" ), unicodeForm( StringUtil::Trim( foundValue ) ) );
						(*node)->appendChild( element );
					}
					else
					{
						first = true;
						DOMElement* element = doc->createElement( unicodeForm( startWith+"1" ) );
						foundValue = line.substr( 1, line.length()-3 );
						element->setAttribute( unicodeForm( "data" ), unicodeForm( StringUtil::Trim( foundValue ) ) );
						(*node)->appendChild( element );
					}
				}
				else
				{
					if( !strncmp( localForm( (*node)->getNodeName() ), "SWIFTFORMAT", 11 ) )
					{
						//add node in the tree
						//XERCES_CPP_NAMESPACE_QUALIFIER DOMDocument* doc = (*node)->getOwnerDocument();
						DOMElement* element = doc->createElement( unicodeForm( startWith ) );

						foundValue = line.substr( 2, line.length()-3 );
						element->setAttribute( unicodeForm( "data" ), unicodeForm( StringUtil::Trim( foundValue ) ) );

						(*node)->appendChild( element );
						(*node) = (*node)->getFirstChild();
					}
					else
					{
						(*node) = (*node)->getParentNode();
						//XERCES_CPP_NAMESPACE_QUALIFIER DOMDocument* doc = (*node)->getOwnerDocument();
						DOMElement* element = doc->createElement( unicodeForm( startWith ) );

						foundValue = line.substr( 2, line.length()-3 );
						element->setAttribute( unicodeForm( "data" ), unicodeForm( StringUtil::Trim( foundValue ) ) );
						
						(*node)->appendChild( element );
						(*node) = (*node)->getLastChild();
					}
				}
				return true;
			}
//			else
//			{
//				foundValue = line.substr( 1, line.length()-3 );
//
//				//XERCES_CPP_NAMESPACE_QUALIFIER DOMDocument* doc = (*node)->getOwnerDocument();
//				DOMElement* element = doc->createElement( unicodeForm( line.substr(0,1) ) );
//				element->setAttribute( unicodeForm( "data" ), unicodeForm( StringUtil::Trim( foundValue ) ) );
//			
//				(*node)->appendChild( element );
//				break;
//			}
//			//DEBUG( "xml : [" << XmlUtil::SerializeToString( doc ) << "]" );
		}

//		else
//		{
//			ManagedBuffer buffer( (unsigned char *)line.c_str(), ManagedBuffer::Copy, line.length() );
//			
//			switch ( m_Data[ i ].Matches( buffer, *node ) ) 
//			{
//				// if "every" is true and Fail was returned, no match is available
//				case TemplateData::Fail :
//					DEBUG( "regex failed" );
//					if ( ( ( m_Every ) || ( m_Data[ i ].getMinOccurs() > 0 ) ) && ( i==m_Data.size()-1) )
//						return false;
//					break;
//				case TemplateData::Skip :
//				case TemplateData::Add :
//					//buffer += m_Data[ i ].getMatchEnd();
//					//break;
//					return true;
//			}
//		}
	}
	foundValue = line.substr( 1, line.length()-3 );

	DOMElement* element = doc->createElement( unicodeForm( line.substr(0,1) ) );
	element->setAttribute( unicodeForm( "data" ), unicodeForm( StringUtil::Trim( foundValue ) ) );
			
	(*node)->appendChild( element );
	return true;
}
*/
bool TemplateNode::MatchTemplate( ManagedBuffer buff, DOMNode *root )
{
	//DEBUG( "Node ["  << m_Id << "] has " << m_Children.size() << " children " );
	
	// guard incoming params
	if ( root == NULL )
		throw invalid_argument( "root is NULL" );

	ManagedBuffer buffer = buff.getRef();

	// iterate through the chidren, match all
	for ( unsigned int i=0; i<m_Data.size(); i++ )
	{
		DEBUG( "Attempting match of [" << ( m_Data[ i ].getFriendlyName() ) << "]" << ( ( m_Data[ i ].IsRegex() ) ? " ( regex )" : "( inner template)" ) );
		
		if ( m_Data[ i ].IsRegex() )
		{
			switch ( m_Data[ i ].Matches( buffer, root ) ) 
			{
				// if "every" is true and Fail was returned, no match is available
				case TemplateData::Fail :
					DEBUG( "regex failed" );
					if ( ( m_Every ) || ( m_Data[ i ].getMinOccurs() > 0 ) )
						return false;
					//lint -fallthrough
				case TemplateData::Skip :
					//lint -fallthrough
				case TemplateData::Add :
					buffer += m_Data[ i ].getMatchEnd();
					break;
				default:
					break;
			}
		}	
		else
		{
			XERCES_CPP_NAMESPACE_QUALIFIER DOMDocument* doc = root->getOwnerDocument();
			DOMElement* element = doc->createElement( unicodeForm( m_Data[ i ].getFriendlyName() ) );
			
			try
			{
				// attempt to retrieve the template requested and invoke match on it
				TemplateNode templateNode = FindTemplate( m_Data[ i ].getValue() );

				//TODO create node for innertemplate  
				if ( templateNode.MatchTemplate( buffer, element ) )
				{
					root->appendChild( element );
					buffer += templateNode.getMatchEnd();
				}	
				// if MinOccurs is > 0 we don't care if the node was matched or not...  it must appear in the output	
				if ( m_Data[ i ].getMinOccurs() > 0 )
				{
					root->appendChild( element );
				}
			}
			catch( const std::exception& e  )
			{
				TRACE( e.what() );
				throw;
			}
		}
	}
	m_MatchEnd = buffer - buff;
	return true;	
}

const TemplateNode& TemplateNode::FindTemplate( const string& templateName ) const
{
	DEBUG2( "Node [" << m_Id << "] is trying to find a template named [" << templateName << "]" );
	// check if any of the children is the template requested
	for( unsigned int i=0; i<m_Children.size(); i++ )
	{
		if ( m_Children[ i ].getId() == templateName )
			return m_Children[ i ];
	}
	
	if ( m_Parent == NULL )
	{
		DEBUG( "Parent is [NULL]" );
	}
	else
		DEBUG( "Parent is  [" << m_Parent << "]" );
		
	// not a kid, maybe the parent has info
	if ( m_Parent != NULL )
	{
		DEBUG( "Not a child, trying parent [" << m_Parent->getId() << "]" );
		return m_Parent->FindTemplate( templateName );
	}
	
	stringstream errorBuffer;
	errorBuffer << "Template not found [" << templateName << "]"; 
	throw logic_error( errorBuffer.str() );	
}

//TemplateNodeRoot implementation
TemplateNodeRoot::TemplateNodeRoot() : TemplateNode(), m_CrLfMode( "" )
{
}

TemplateNodeRoot::~TemplateNodeRoot()
{
}

void TemplateNodeRoot::Parse( DOMNode* node )
{
	//DEBUG( "Parse" )

	// invoke base class metod
	TemplateNode::Parse( node );
	
	// we just need to read crlf mode 
	// argument check is performed in base method
	// get all atributes and put them in place
	DOMNamedNodeMap *attributes = node->getAttributes();
	
	for ( unsigned int i=0; i<attributes->getLength(); i++ )
	{
		DOMAttr* attribute = dynamic_cast< DOMAttr* >( attributes->item( i ) );
		if ( attribute == NULL )
			continue;

		string attributeName = localForm( attribute->getName() );
		string attributeValue = localForm( attribute->getValue() );
	
		//DEBUG_STREAM ( "Attribute : " ) << attributeName << " = " << attributeValue << endl;	

		// get id
		if ( attributeName == "crlfMode" )
			m_CrLfMode = attributeValue;
		// ignore other attributes
	}
}

bool TemplateNodeRoot::MatchTemplate( ManagedBuffer buffer, DOMNode *root )
{
	//DEBUG( "Root node ["  << m_Id << " - " << this << "] attempting match" );
	
	// guard incoming values
	if ( root == NULL )
		throw invalid_argument( "Root node of the document passed to match is NULL" );
	if ( buffer.buffer() == NULL )
		throw invalid_argument( "Buffer passed to match is empty." );
		
	string result;
	if( m_CrLfMode == "LF" )
	{
		DEBUG( "Replacing CRLF with LF" );
		result = StringUtil::Replace( buffer.str(), "\r\n", "\n" );
		buffer.copyFrom( result );
	}
	
	XERCES_CPP_NAMESPACE_QUALIFIER DOMDocument* doc = root->getOwnerDocument();
	DOMElement* element = doc->createElement( unicodeForm( m_Id ) );
	root->appendChild( element );
	
	// delegate the task to the first inner template
	bool childMatch = m_Children[ 0 ].MatchTemplate( buffer, element );
	m_MatchEnd = m_Children[ 0 ].getMatchEnd();
	return childMatch;
}

//TemplateParser implementation

TemplateFilter* TemplateParser::Filter = NULL;

void TemplateParser::ReleaseParseDocument( void )
{
	if ( Filter != NULL )
		delete Filter;
	Filter = NULL;

	if ( m_ParseDocument != NULL )
	{
		m_ParseDocument->release();
		m_ParseDocument = NULL;
	}
}

TemplateParser::TemplateParser( const string& templateFilename ) : m_TemplateFile( templateFilename ), m_MatchEnd( 0 ), m_ParseDocument( NULL )
{
	// guard templateFilename - must not be null
	if ( templateFilename.size() == 0 ) 
		throw invalid_argument( "templateFilename empty" );

	if ( m_ParseDocument != NULL )
		m_ParseDocument->release();
	m_ParseDocument = NULL;

	Filter = new TemplateFilter();
	
	// exceptions are handled elsewere
	try
	{
		m_ParseDocument = XmlUtil::DeserializeFromFile( templateFilename );
		if ( m_ParseDocument == NULL )
		{
			TRACE( "Document not parsed. Check if template file exists and is accessible." );
			throw runtime_error( "Document not parsed. Check if template file exists and is accessible." );
		}
	}
	catch( ... )
    {
    	TRACE( "Document not parsed. Check if template file exists and is accessible." );
 
 		ReleaseParseDocument();
    	throw runtime_error( "Parse error" );
    }
			
 	DEBUG( "Document element : " << m_ParseDocument->getDocumentElement() );

	try
	{
		// get the DOM representation
		if( m_ParseDocument->getDocumentElement() == NULL )
			throw runtime_error( "Document element missing. Check if template file is not corrupted." );
		DOMNode* root = ( DOMNode* )( m_ParseDocument->getDocumentElement() );
		
		//m_Root = new TemplateNodeRoot();
		m_Root.Parse( root );
		
		DEBUG( "Root node has " << m_Root.getChildrenCount() << " children " );
	}
	catch( ... )
	{
		ReleaseParseDocument();
		throw;
	}
	ReleaseParseDocument();
}

TemplateParser::TemplateParser() : m_TemplateFile( "" ), m_MatchEnd( 0 ), m_ParseDocument( NULL )
{
	//DEBUG( "CTOR - "  << this );
}

TemplateParser::~TemplateParser()
{
	try
	{
		ReleaseParseDocument();
	}
	catch ( ... )
	{
		try
		{
			TRACE( "An error occured while releasing the parse document" );
		} catch( ... ){}
	}
}

/*bool TemplateParser::MatchByLine( ManagedBuffer buffer, XERCES_CPP_NAMESPACE_QUALIFIER DOMNode *root )
{
	// guard incoming params
	if ( root == NULL )
	{
		throw invalid_argument( "root is NULL" );
	}

	string inputString( buffer.str() );
	//DEBUG( "InputString : [" << inputString << "]" );

	if( m_Root.getCrLfMode() == "LF" )
	{
		DEBUG( "Replacing CRLF with LF" );
		inputString = StringUtil::Replace( inputString, "\r\n", "\n" );
	}

	if( inputString.length() == 0 )
		return true;
	
	bool rootMatch = false;
	bool first = true;
	XERCES_CPP_NAMESPACE_QUALIFIER DOMDocument* doc = root->getOwnerDocument();
	DOMElement* element = doc->createElement( unicodeForm( m_Root.getId() ) );
	root->appendChild( element );
    DOMNode* node = root->getFirstChild();

	while( inputString.length() != 0 )
	{
		string::size_type eol = inputString.find_first_of( "\n" );
		string currentLine = inputString.substr( 0, eol+1 );
		DEBUG( "Current Line = [" << currentLine << "]" );
		//DEBUG( localForm( node->getNodeName() ) );
		
		TemplateNode tmplNode = m_Root.getChildren()[0];

		rootMatch = tmplNode.AddLineInfoInDom( currentLine, &node, first );

		inputString.erase( 0, eol+1 );
	}

	return rootMatch;
}
*/
bool TemplateParser::Matches( ManagedBuffer buffer, XERCES_CPP_NAMESPACE_QUALIFIER DOMNode *root )
{
	// guard incoming params
	if ( root == NULL )
	{
		throw invalid_argument( "root is NULL" );
	}
	bool rootMatch = m_Root.MatchTemplate( buffer, root ); 
	m_MatchEnd = m_Root.getMatchEnd();
	
	return rootMatch;
}

//TemplateFilter implementation

short TemplateFilter::acceptNode( const DOMNode* node ) const
{
	switch ( node->getNodeType() )
	{
		case DOMNode::TEXT_NODE:
			// the WhatToShow will make this no effect
			return DOMNodeFilter::FILTER_SKIP;
		default :
			return DOMNodeFilter::FILTER_ACCEPT;
	}
}

//TemplateParserFactory implementation

map< std::string, TemplateParser, less< std::string > > TemplateParserFactory::Cache;

TemplateParserFactory::TemplateParserFactory()
{
}

TemplateParserFactory::~TemplateParserFactory()
{
}

TemplateParser TemplateParserFactory::getParser( const string& filename )
{ 
	map< std::string, TemplateParser >::const_iterator finder = TemplateParserFactory::Cache.find( filename );
	
	//if no parser is found, create one
	if ( finder == TemplateParserFactory::Cache.end() )
	{
		try
		{
			DEBUG_GLOBAL( "Parser not in cache" );
			// create a parser for this file
			TemplateParser parser( filename );
			
			DEBUG_GLOBAL( "Parser created ( using Boost regex )" );

			// add the created parser to the cache
			TemplateParserFactory::Cache.insert( ParserEntry( filename, parser ) );
		}
		catch( const std::exception& ex )
		{
			// format error code
    		stringstream messageBuffer;
    		messageBuffer << "Unable to create parser for template [";
			messageBuffer << filename << "] : " << ex.what();			

			TRACE_GLOBAL( messageBuffer.str() );
			throw runtime_error( messageBuffer.str() );
		}
		catch( ... )
		{
			// format unknown error
			stringstream messageBuffer;
			messageBuffer << "Unable to create parser for template [";
			messageBuffer << filename << "] : Unspecified error";
			
			TRACE_GLOBAL( messageBuffer.str() )
			throw runtime_error( messageBuffer.str() );
		}
	}
	else
	{
		DEBUG_GLOBAL( "Using parser from cache" );
	}
	
	// return cached parser
	return TemplateParserFactory::Cache[ filename ];
}
