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

#include <stdexcept>

#include "Trace.h"

using namespace FinTP;

#include "RoutingAggregationCode.h"

// RoutingAggregationCode implementation

RoutingAggregationCode::RoutingAggregationCode() : m_CorrelationId( "" ), m_CorrelationToken( "" )
{
}

RoutingAggregationCode::RoutingAggregationCode( const string token, const string identifier ) :
	m_CorrelationId( identifier ), m_CorrelationToken( token )
{
}

RoutingAggregationCode::RoutingAggregationCode( const RoutingAggregationCode& source ) :
	m_CorrelationId( source.m_CorrelationId ), m_CorrelationToken( source.m_CorrelationToken )
{
	RoutingAggregationFieldArray::const_iterator requestIterator = source.m_FieldArray.begin();
	
	while( requestIterator != source.m_FieldArray.end() )
	{
		m_FieldArray.insert( pair<std::string, std::string>( requestIterator->first, requestIterator->second ) );
		requestIterator++;
	}

	RoutingAggregationConditionArray::const_iterator conditionIterator = source.m_ConditionArray.begin();
	
	while( conditionIterator != source.m_ConditionArray.end() )
	{
		m_ConditionArray.insert( pair<std::string, std::string>( conditionIterator->first, conditionIterator->second ) );
		conditionIterator++;
	}
}

RoutingAggregationCode& RoutingAggregationCode::operator=( const RoutingAggregationCode& source )
{
	m_CorrelationId = source.m_CorrelationId;
	m_CorrelationToken = source.m_CorrelationToken;
	m_FieldArray.clear();
	m_ConditionArray.clear();

	RoutingAggregationFieldArray::const_iterator requestIterator = source.m_FieldArray.begin();
	
	while( requestIterator != source.m_FieldArray.end() )
	{
		m_FieldArray.insert( pair<std::string, std::string>( requestIterator->first, requestIterator->second ) );
		requestIterator++;
	}
	RoutingAggregationConditionArray::const_iterator conditionIterator = source.m_ConditionArray.begin();
	
	while( conditionIterator != source.m_ConditionArray.end() )
	{
		m_ConditionArray.insert( pair<std::string, std::string>( conditionIterator->first, conditionIterator->second ) );
		conditionIterator++;
	}

	return *this;
}

void RoutingAggregationCode::Dump() const
{
	TRACE( "----------- Dump of aggregation code begin ---------- " );
	TRACE( "Correlation token [" << m_CorrelationToken << "] = [" << m_CorrelationId << "]" );

	RoutingAggregationFieldArray::const_iterator requestIterator = m_FieldArray.begin();
	
	while( requestIterator != m_FieldArray.end() )
	{
		TRACE( "\t[" << requestIterator->first << "] = [" << requestIterator->second << "]" );
		requestIterator++;
	}

	TRACE( "--------------------Conditions----------------------- " );
	RoutingAggregationConditionArray::const_iterator conditionIterator = m_ConditionArray.begin();
	
	while( conditionIterator != m_ConditionArray.end() )
	{
		TRACE( "\t[" << conditionIterator->first << "] = [" << conditionIterator->second << "]" );
		conditionIterator++;
	}

	TRACE( "------------ Dump of aggregation code end ----------- " );
}

bool RoutingAggregationCode::containsAggregationField( const string token ) const
{
	RoutingAggregationFieldArray::const_iterator fieldFinder = m_FieldArray.find( token );
	if ( fieldFinder != m_FieldArray.end() )
		return true;
	return false;	
}

string RoutingAggregationCode::getAggregationField( const string token ) const
{
	RoutingAggregationFieldArray::const_iterator fieldFinder = m_FieldArray.find( token );
	if ( fieldFinder != m_FieldArray.end() )
		return fieldFinder->second;
	
	TRACE( "Dumping aggregation fields for [" << m_CorrelationToken << "] = [" << m_CorrelationId << "] ... " );
	for( fieldFinder = m_FieldArray.begin(); fieldFinder != m_FieldArray.end(); fieldFinder++ )
	{
		TRACE( "\t[" << fieldFinder->first << "] = [" << fieldFinder->second << "]" );
	}

	stringstream errorMessage;
	errorMessage << "Argument out of range [token]. Aggregation code doesn't contain a field named [" << token << "]";
	throw invalid_argument( errorMessage.str() ); 
}

string RoutingAggregationCode::getAggregationFieldName( const unsigned int index ) const
{
	if ( index >= m_FieldArray.size() )
	{
		stringstream errorMessage;
		errorMessage << "Argument out of range [index]. Aggregation code doesn't contain a field with index [" << index << "]";
		throw invalid_argument( errorMessage.str() ); 
	}
	RoutingAggregationFieldArray::const_iterator fieldFinder = m_FieldArray.begin();
	for( unsigned int i=0; i<index; i++ )
		fieldFinder++;

	return fieldFinder->first;
}

string RoutingAggregationCode::getAggregationField( const unsigned int index ) const
{
	if ( index >= m_FieldArray.size() )
	{
		stringstream errorMessage;
		errorMessage << "Argument out of range : index. Aggregation code doesn't contain a field with index [" << index << "]";
		throw invalid_argument( errorMessage.str() ); 
	}
	RoutingAggregationFieldArray::const_iterator fieldFinder = m_FieldArray.begin();
	for( unsigned int i=0; i<index; i++ )
		fieldFinder++;

	return fieldFinder->second;
}

