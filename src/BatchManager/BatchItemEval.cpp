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

#ifdef WIN32
	#define __MSXML_LIBRARY_DEFINED__
	#include <windows.h>
//	#define sleep(x) Sleep( (x)*1000 )
#else
	#include <unistd.h>
#endif

#include "BatchItemEval.h"

#include <xalanc/PlatformSupport/XSLException.hpp>
#include <xalanc/XPath/NodeRefList.hpp>
#include <xalanc/XalanDOM/XalanDOMString.hpp>

#include "StringUtil.h"
#include "Trace.h"
#include "../XPathHelper.h"

#include "BatchManager.h"

// include all predefined batch evaluators
#include "Evaluators/BatchItemMT940Eval.h"
#include "Evaluators/BatchItemMT950Eval.h"
#include "Evaluators/BatchItemMT104Eval.h"
#include "Evaluators/BatchItemACHEval.h"
#include "Evaluators/BatchItemGSRSEval.h"

using namespace FinTP;

//BatchItemEval* ( *BatchItemEval::m_Callback )( string ) = NULL;
BatchItemEval* BatchItemEval::m_Evaluator = NULL;

BatchItemEval::BatchItemEval() : m_Document( NULL )
{
}

BatchItemEval::~BatchItemEval()
{
}

BatchResolution BatchItemEval::Evaluate( XALAN_CPP_NAMESPACE_QUALIFIER XalanDocument* document, const string& xPath, const string& eyeCatcher )
{
	if( document == NULL )
	{
		TRACE( "Metadata is NULL" );
		BatchResolution resolution;
		resolution.setItemLast();
		return resolution;
	}
	
	BatchItemEval *evaluator = NULL;
	string eyecatcher = eyeCatcher;

	if( m_Evaluator != NULL )
	{
		evaluator = m_Evaluator; //( * BatchItemEval::m_Callback )( "dummy" );
	}
	else if ( eyeCatcher.length() > 0 )
	{
		evaluator = getEvaluator( eyeCatcher );
	}
	else if( xPath.length() > 0 )
	{
		eyecatcher = XPathHelper::SerializeToString( XPathHelper::Evaluate( xPath, document ) );
		evaluator = getEvaluator( eyecatcher );
	}
	
	if( evaluator == NULL )
	{
		DEBUG_GLOBAL( "Evaluator set to NULL" );
		BatchResolution resolution;
		resolution.setItemLast();
		return resolution;
	}
	else
	{
		DEBUG_GLOBAL( "Evaluator set to " << typeid( *evaluator ).name() );
	}
	
	try
	{
		evaluator->setDocument( document );
		BatchResolution resolution( evaluator->getAction(), 
			BatchItem( evaluator->getSequence(), evaluator->getBatchId(), evaluator->getMessageId(), evaluator->isLast(), eyecatcher ) );

		delete evaluator;
		evaluator = NULL;

		return resolution;
	}
	catch( ... )
	{
		if ( evaluator != NULL )
		{
			delete evaluator;
			evaluator = NULL;
		}
		throw;
	}
}

BatchItemEval* BatchItemEval::getEvaluator( const string& messageType )
{
	if ( ( messageType == "MT104" ) || ( messageType == "104" ) )
		return new BatchItemMT104Eval();
	if ( ( messageType == "MT940" ) || ( messageType == "940" ) )
		return new BatchItemMT940Eval();
	if ( ( messageType == "MT950" ) || ( messageType == "950" ) )
		return new BatchItemMT950Eval();
	if ( ( messageType == "FI302" ) || ( messageType == "FI501" ) || ( messageType == "FI502" ) || ( messageType == "FI901" ) )
		return new BatchItemGSRSEval( messageType );
	return NULL;
}

