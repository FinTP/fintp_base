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

#include "Trace.h"
#include "StringUtil.h"

#include <sstream>

#include "Currency.h"

#undef S_NORMAL

#define BOOST_REGEX_MATCH_EXTRA
#include <boost/regex.hpp>

Currency::Currency( const string& currencyAsString ) : m_BDPPart( "0" ), m_ADPPart( "0" )
{
	Currency::setAmount( currencyAsString );
}

void Currency::setAmount( const string& currencyAsString )
{
	// capture 1 : before decimal point, capture 2 : decimal point, capture 3 : after decimal point
	boost::regex curMatchRegex( "^(\\d+)(?:[\\.,](\\d*))?$" );
	boost::smatch groupsMatch;

	unsigned int captures = 0;

	try
	{
		bool match = boost::regex_search( currencyAsString, groupsMatch, curMatchRegex, boost::match_single_line );
		if ( !match )
			throw runtime_error( "no match" );

		captures = groupsMatch.size();

		if ( captures != 3 )
			throw runtime_error( "not all fields" );
	}
	catch( const std::exception& ex )
	{
		stringstream errorMessage;
		errorMessage << "Unable to find an amount in [" << currencyAsString << "] [" << ex.what() << "]";
		throw runtime_error( errorMessage.str() );
	}
	catch( ... )
	{
		stringstream errorMessage;
		errorMessage << "Unable to find an amount in [" << currencyAsString << "] [unknown reason]";
		throw runtime_error( errorMessage.str() );
	}

	// if we have a decimal part
	if ( groupsMatch.str( 2 ).length() > 0 )
		m_ADPPart = groupsMatch.str( 2 );
	else
		m_ADPPart = "0";
	
	m_BDPPart = groupsMatch.str( 1 );
}

Currency::Currency() : m_BDPPart( "0" ), m_ADPPart( "0" )
{
}

Currency::~Currency()
{
}
