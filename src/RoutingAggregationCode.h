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

#ifndef ROUTINGCORRELATIONMCODE_H
#define ROUTINGCORRELATIONMCODE_H

#include "DllMain.h"

#include <map>
#include <vector>
#include <string>
using namespace std;

typedef std::map<std::string, std::string> RoutingAggregationFieldArray;
typedef std::map<std::string, std::string> RoutingAggregationConditionArray;

#define DEFAULT_AGGTABLE "FEEDBACKAGG"

class ExportedObject RoutingAggregationCode
{
	public :

		RoutingAggregationCode();
		RoutingAggregationCode( const std::string token, const std::string identifier );
		RoutingAggregationCode( const RoutingAggregationCode& source );
		RoutingAggregationCode& operator=( const RoutingAggregationCode& source );
		
		// value of the correlation field
		void setCorrelId( const string& correlid ) { m_CorrelationId = correlid; }
		string getCorrelId() const { return m_CorrelationId; }
		
		// name of the correlation field
		void setCorrelToken( const string& correltoken ) { m_CorrelationToken = correltoken; }
		string getCorrelToken() const { return m_CorrelationToken; }
		
		void addAggregationField( const string& token, const string& identifier ) { (void)m_FieldArray.insert( pair< string, string >( token, identifier ) ); }
		const RoutingAggregationFieldArray& getFields() const { return m_FieldArray; }

		void addAggregationCondition( const string& token, const string& value ) { (void)m_ConditionArray.insert( pair< string, string >( token, value ) ); }
		const RoutingAggregationConditionArray& getConditions() const { return m_ConditionArray; }

		bool containsAggregationField( const string token ) const;
		string getAggregationField( const string token ) const;
		string getAggregationField( const unsigned int index ) const;
		string getAggregationFieldName( const unsigned int index ) const;

		void setAggregationField( const string& token, const string& identifier ) { m_FieldArray[ token ] = identifier; }
		
		void Dump() const;
		unsigned int Size() const { return m_FieldArray.size(); }
		
		void Clear() { m_FieldArray.clear(); }
				
	private :
		
		string m_CorrelationId;
		string m_CorrelationToken;
		
		RoutingAggregationFieldArray m_FieldArray;
		RoutingAggregationConditionArray m_ConditionArray;
};

#endif // ROUTINGCORRELATIONMCODE_H
