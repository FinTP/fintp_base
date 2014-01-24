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

#include "BatchItemMT950Eval.h"

#include "../../XPathHelper.h"
#include "Trace.h"

#define BOOST_REGEX_MATCH_EXTRA
#include <boost/regex.hpp>

using namespace std;
using namespace FinTP;

BatchItemMT950Eval::BatchItemMT950Eval() : m_Sequence( 0 ), m_LastMessage( true ), m_Statement( "" )
{
}

BatchItemMT950Eval::~BatchItemMT950Eval()
{
}

void BatchItemMT950Eval::setDocument( XALAN_CPP_NAMESPACE_QUALIFIER XalanDocument* document )
{
	m_Document = document;
	string batchStmtSeq = XPathHelper::SerializeToString( XPathHelper::Evaluate( "/smt:MT950/smt:MessageText/smt:tag28C/@tagValue", document ) );
	
	string stmtSeqRegex = "(\\d{1,5})/(\\d{1,5})";
	boost::regex batchStmtSeqRegex( stmtSeqRegex );
	boost::smatch batchStmtSeqMatch;
	
	bool match = false;
	try
	{
		match = boost::regex_search( batchStmtSeq, batchStmtSeqMatch, batchStmtSeqRegex, boost::match_extra );
	}	
	catch( const boost::bad_pattern& ex )
	{
		stringstream errorMessage;
		errorMessage << "Unable to evaluate MT950 statement no/sequence no using regex [" << stmtSeqRegex << "] : Boost regex exception : " << ex.what() ;
		
		TRACE( errorMessage.str() );
		throw runtime_error( errorMessage.str() );
	}
	
	if( !match )
	{
		stringstream errorMessage;
		errorMessage << "Unable to evaluate MT950 statement no/sequence no using regex [" << stmtSeqRegex << "] : no match in [" << batchStmtSeq << "]";
		
		TRACE( errorMessage.str() );

		// it may still be a good message ( no stmt/seq found - consider it 1 page )

		m_Statement = batchStmtSeq;
		m_Sequence = 1;
		m_LastMessage = true;
	}
	else
	{
		DEBUG( "Matched " << batchStmtSeqMatch.size() << " groups... " );

		m_Statement = batchStmtSeqMatch[ 1 ];
		string sequenceString = batchStmtSeqMatch[ 2 ];
	
		string lastMsgEval = XPathHelper::SerializeToString( XPathHelper::Evaluate( "/smt:MT950/smt:MessageText/smt:tag62F/@tagValue", document ) );
		m_Sequence = atol( sequenceString.c_str() );
		m_LastMessage = ( lastMsgEval.length() > 0 );
	}
	
	DEBUG( "Evaluation result : Statement no [" << m_Statement << "], Sequence no [" << m_Sequence << "]" );
}
