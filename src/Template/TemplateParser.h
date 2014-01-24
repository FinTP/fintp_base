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

#ifndef TEMPLATEPARSER_H
#define TEMPLATEPARSER_H

#include <vector>
#include <map>

#ifdef WIN32
	#define __MSXML_LIBRARY_DEFINED__
#endif
#include <xercesc/dom/DOM.hpp>
//#include <xercesc/dom/DOMBuilder.hpp>
#include <xercesc/sax/ErrorHandler.hpp>
//#include <xercesc/parsers/DOMBuilderImpl.hpp>

//#include "../Collections.h"
#include "WorkItemPool.h"
#include "DllMain.h"

XERCES_CPP_NAMESPACE_USE

using namespace std;

namespace FinTP
{
	class TemplateNode;

	//EXPIMP_TEMPLATE template class ExportedObject std::vector< string >;

	class ExportedObject TemplateData
	{
		public :
			
			/// <summary>
			/// Result for node processing function. 
			/// </summary>
	   		enum NodeMatchReturn
			{
				/// <summary>
				/// Create a new node
				/// </summary>
				Add,
				/// <summary>
				/// Don't create the node
				/// </summary>
				Skip,
				/// <summary>
				/// Processing of this node failed
				/// </summary>
				Fail
			};
			
		private :
			
			string m_Value;
			string m_FriendlyName;
			int m_MaxOccurs;
			int m_MinOccurs;
			bool m_Successive;
			bool m_Regex;
			
			int m_MatchEnd;
			
			const TemplateNode *m_Parent;
			vector< string > m_GroupNames;

			// matches at most once a regex node
			NodeMatchReturn MatchSingle( ManagedBuffer buffer, DOMElement* root );
			
		public :
		
			TemplateData();
			explicit TemplateData( const DOMNode* node );
			~TemplateData();
			
			// returns true is the node is a regex, false if it is a reference to an inner template
			bool IsRegex() const { return m_Regex; }
			
			//
			bool MustBeSuccessive() const { return m_Successive; }
			
			int getMinOccurs() const { return m_MinOccurs; }
			int getMaxOccurs() const { return m_MaxOccurs; }
			
			string getValue() const { return m_Value; }
			string getFriendlyName() const { return m_FriendlyName; }
			
			const TemplateNode* getParent() const { return m_Parent; }
			void setParent( const TemplateNode* parent ) { m_Parent = parent; }
			
			int getMatchEnd() const { return m_MatchEnd; }
			
			// returns true if the buffer matches the given regex / innertemplate
			NodeMatchReturn Matches( ManagedBuffer buffer, DOMNode *root );
	};

	//EXPIMP_TEMPLATE template class ExportedObject std::vector< TemplateData >;

	//class TemplateNode;
	//EXPIMP_TEMPLATE template class ExportedObject std::vector< TemplateNode >;

	///<summary>
	/// Node for a template
	///</summary>
	class ExportedObject TemplateNode
	{
		protected :
			string m_Id;
			bool m_Every;
			const TemplateNode *m_Parent;
			int m_MatchEnd;
			
			// regex nodes
			vector< TemplateData > m_Data;
			
			// sub templates
			vector< TemplateNode > m_Children; 

		public :
		
			TemplateNode();
			TemplateNode( const TemplateNode& source );
			virtual ~TemplateNode();
			
			TemplateNode& operator=( const TemplateNode& source );
			
			// if all inner elements must match
			bool MustMatchAllNodes() const { return m_Every; }
			
			// parses a node and constructs its child templates/data
			virtual void Parse( DOMNode* );
			virtual bool MatchTemplate( ManagedBuffer buffer, DOMNode *root );
			//virtual bool AddLineInfoInDom( const string value, DOMNode** node, bool& first );
					
			string getId() const { return m_Id; }
			unsigned int getChildrenCount() const { return m_Children.size(); }
			
			const TemplateNode* getParent() const { return m_Parent; }
			void setParent( const TemplateNode* parent ) { m_Parent = parent; }
			
			const vector< TemplateData >& getData() const { return m_Data; }
			const vector< TemplateNode >& getChildren() const { return m_Children; }
			
			int getMatchEnd() const { return m_MatchEnd; }
			
			const TemplateNode& FindTemplate( const string& templateName ) const;
	};

	///<summary>
	/// The root node for a template
	///</summary>
	class ExportedObject TemplateNodeRoot : public TemplateNode
	{
		private :
			// CR/LF/CRLF mode for handling new line in output files
			string m_CrLfMode;
		
		public :
		
			// .ctor & destructor
			TemplateNodeRoot();
			~TemplateNodeRoot();

			//string getCrLfMode( ) { return m_CrLfMode; };
			
			bool MatchTemplate( ManagedBuffer buffer, DOMNode *root );
			
			void Parse( DOMNode* );
	};

	class ExportedObject TemplateFilter //: public DOMBuilderFilter
	{
		private :
			//unsigned long m_WhatToShow;
			
		public:
		
			TemplateFilter(){}//: DOMBuilderFilter(){};
			~TemplateFilter(){}
			
			// overrides
			//unsigned long getWhatToShow() const { return m_WhatToShow; }
			//unsigned void setWhatToShow(unsigned long toShow) { m_WhatToShow = toShow; }
			short acceptNode( const DOMNode* node ) const;
	};

	///<summary>
	/// Parses a template file and holds a tree with the compiled regexes.
	///</summary>
	class ExportedObject TemplateParser
	{
		private :
			
			TemplateNodeRoot m_Root;
			std::string m_TemplateFile;
			
			void ReleaseParseDocument();

			int m_MatchEnd;
			XERCES_CPP_NAMESPACE_QUALIFIER DOMDocument *m_ParseDocument;
			
		public:
			
			// .ctor and destructor. 
			// reads templateFilename and compiles a template tree
			TemplateParser();
			explicit TemplateParser( const string& templateFilename );
			~TemplateParser();
			//TemplateParser( const TemplateParser& source );
			
			// root accessor
			const TemplateNodeRoot& getRoot() const { return m_Root; }
			// gets the name of the file used as template
			string getTemplateFile() const { return m_TemplateFile; }
			
			// returns true if the buffer matches the given root template node
			bool Matches( ManagedBuffer buffer, XERCES_CPP_NAMESPACE_QUALIFIER DOMNode* root );
			
			//use only for ReconS file, this file is structured by lines
			//bool MatchByLine( ManagedBuffer buffer, XERCES_CPP_NAMESPACE_QUALIFIER DOMNode* root );

			int getMatchEnd() const { return m_MatchEnd; }
			
			static TemplateFilter *Filter;
	};

	typedef pair< std::string, TemplateParser > ParserEntry;
	//EXPIMP_TEMPLATE template class ExportedObject std::map< std::string, TemplateParser >;

	///<summary>
	/// Creates a parser or returns an already created parser for a specific template
	///</summary>
	class ExportedObject TemplateParserFactory
	{
		private : 
			static map< std::string, TemplateParser > Cache;
			
			// private .ctor since the class has only static methods we don't need to instantiate it
			TemplateParserFactory();
			
		public:
		
			// destructor
			~TemplateParserFactory();
			
			// create a new instance of TemplateParser or return an existing parser
			static TemplateParser getParser( const string& filename );
	};
}

#endif // TEMPLATEPARSER_H
