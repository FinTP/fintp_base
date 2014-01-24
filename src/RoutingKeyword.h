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

#ifndef ROUTINGKEYWORDS_H
#define ROUTINGKEYWORDS_H

#include <map>
#include <vector>
#include <string>

#include "DllMain.h"

class RoutingKeyword;

typedef map< string, RoutingKeyword > RoutingKeywordCollection;
typedef map< string, map< string, string > > RoutingKeywordMappings;
typedef map< string, bool > RoutingIsoMessageTypes;

class ExportedObject RoutingKeyword
{
	public :

		typedef enum
		{
			STRING,
			CURRENCY,
			DATE
		} EVALUATOR_TYPE;
				
					
		RoutingKeyword::EVALUATOR_TYPE parseType( const string& value );

		// ctor
		RoutingKeyword();
		RoutingKeyword( const string& name, const string& comparer, const string& regex, const string& regexIso );
		~RoutingKeyword();
		
		// evaluates a field from a value
		pair< string, EVALUATOR_TYPE > Evaluate( const string& value, const string& field, bool iso = false );
		//EVALUATOR_TYPE operator[]( const string& field );
	
		string getRegex() const { return m_Regex; }
		void setRegex( const string& value ) { m_Regex = value; }

		string getRegexIso() const { return m_RegexIso; }
		void setRegexIso( const string& value ) { m_RegexIso = value; }

		// pretty print
		void Dump();

	private : 
		
		string m_Regex, m_RegexIso;
		string m_Name;		
		vector< pair< string, EVALUATOR_TYPE > > m_Fields;
		vector< pair< string, EVALUATOR_TYPE > > m_FieldsIso;
};

class KeywordMappingNotFound : public logic_error
{
	private : 
		string m_MessageType, m_Keyword;

	public:
		KeywordMappingNotFound( const string& keyword, const string& messageType ):
		  logic_error( "Unable to map keyword to a message type" ), m_MessageType( messageType ), m_Keyword( keyword )
		{}
		~KeywordMappingNotFound() throw() {};

		string getMessageType() const { return m_MessageType; }
		string getKeyword() const { return m_Keyword; }
};

class ExportedObject NotFoundKeywordMappings
{
	private :
		// for each keyword we have a collection of < MT, tag >
		RoutingKeywordMappings m_Keywords;
				
		static void DeleteCollections( void* data );

	public:

		static pthread_once_t NotFoundKeysCreate;
		static pthread_key_t NotFoundKey;
	
		NotFoundKeywordMappings();
		~NotFoundKeywordMappings() {};
		static void CreateKeys();
};
#endif // ROUTINGKEYWORDS_H
