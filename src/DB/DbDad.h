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

#ifndef DBDAD_H
#define DBDAD_H

#include <string>
#include <map>
#include "CacheManager.h"

#include "DatabaseProvider.h"
#include "Database.h"
#include "DataParameter.h"

#include "DllMain.h"

using namespace std;

namespace FinTP
{
	class ExportedObject DbDadElement
	{
		private:	

			int m_Length;
			DataType::DATA_TYPE m_Type;
			string m_Format;
			string m_ElementName;
			vector< string > m_ColumnNames;

		public:

			explicit DbDadElement( string name = "UNNAMED", DataType::DATA_TYPE type = DataType::INVALID_TYPE, int length = -1, string format = "" ) : 
				m_Length( length ), m_Type( type ), m_Format( format ), m_ElementName( name )
			{
			}

			int length() const { return m_Length; }
			DataType::DATA_TYPE type() const { return m_Type; }
			string format() const { return m_Format; }
			string name() const { return m_ElementName; }
			const vector< string >& columnNames() const { return m_ColumnNames; }

			void addColumnName( const string& columnName ) { m_ColumnNames.push_back( columnName ); }
	};

	class ExportedObject DbDad 
	{
		public :
			enum DadOptions
			{
				NODAD,
				WITHPARAMS,
				WITHVALUES
			};

		private :

			map< string, DbDadElement > m_Elements;
			string m_TableName;
			DatabaseProviderFactory* m_DbProvider;


			bool CastAndAdd( bool& first, stringstream& statementString, stringstream& castString, const string& parameterName, const string& parameterValue, bool escape = true );
			bool CastAndAdd( bool& first, stringstream& statementString, stringstream& castString, const string& parameterName, const string& parameterValue, ParametersVector& params );

		public :

			DbDad() : m_TableName( "NONE" ) { m_DbProvider = NULL;}
			explicit DbDad( const string& filename, DatabaseProviderFactory* dbProvider );

			/*const DataType::DATA_TYPE elementType( const string& elementName ) const;
			const int elementLength( const string& elementName ) const;
			const string DbDad::elementFormat( const string& elementName ) const;*/
			
			const DbDadElement& operator[]( const string& name );

			const string tableName() const { return m_TableName; }
			void Upload( const string& xmlData, Database* currentDatabase, bool usingParams = false );
	};
}

#endif // DBDAD_H
